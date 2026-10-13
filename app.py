"""Hash Lab: Tkinter desktop UI for Python and Rust local hash search engines."""
import hashlib
import os
from pathlib import Path
import queue
import subprocess
import sys
import threading
import time
import tkinter as tk
from tkinter import ttk, messagebox

ROOT = Path(__file__).resolve().parent
LOW = 'abcdefghijklmnopqrstuvwxyz'
UP = 'ABCDEFGHIJKLMNOPQRSTUVWXYZ'
NUM = '0123456789'
SPECIAL = '!@#$%^&*_-'
HASHES = ('md5', 'sha1', 'sha224', 'sha256', 'sha384', 'sha512', 'sha3_224', 'sha3_256', 'sha3_384', 'sha3_512', 'blake2s', 'blake2b')
VARIANT5 = '713417394e2b74aee7fa54d09375b0c78d288a90eba27a560cfc698c5b54886c'


def duration(seconds):
    if seconds < 0 or not (seconds < float('inf')):
        return 'unknown'
    if seconds < 1:
        return f'{seconds:.2f} sec'
    for unit, factor in [('years', 31557600), ('days', 86400), ('hours', 3600), ('minutes', 60)]:
        if seconds >= factor:
            return f'{seconds/factor:,.2f} {unit}'
    return f'{seconds:,.1f} sec'


class App(tk.Tk):
    def __init__(self):
        super().__init__()
        self.title('Hash Lab — Python / Rust')
        self.geometry('760x710')
        self.minsize(670, 650)
        self.configure(bg='#f5f7fb')
        self.events = queue.Queue()
        self.process = None
        self.active = False
        self.cancelled = False
        self.total = 0
        self.checked = 0
        self.started = 0
        self.workers = tk.IntVar(value=max(1, os.cpu_count() or 1))
        self.engine = tk.StringVar(value='Python')
        self.length = tk.IntVar(value=6)
        self.algorithm = tk.StringVar(value='blake2s')
        self.target = tk.StringVar(value=VARIANT5)
        self.use_low = tk.BooleanVar(value=True)
        self.use_up = tk.BooleanVar(value=False)
        self.use_num = tk.BooleanVar(value=False)
        self.use_special = tk.BooleanVar(value=True)
        self.rate = 0.0
        self.last_config = None
        self._ui()
        self.after(100, self.drain)

    def _ui(self):
        style = ttk.Style(self)
        if 'clam' in style.theme_names(): style.theme_use('clam')
        style.configure('TButton', padding=8)
        style.configure('TCombobox', padding=5)
        style.configure('TProgressbar', thickness=19)
        base = tk.Frame(self, bg='#f5f7fb', padx=24, pady=16)
        base.pack(fill='both', expand=True)
        tk.Label(base, text='HASH LAB', fg='#1e3a8a', bg='#f5f7fb', font=('Segoe UI', 22, 'bold')).pack(anchor='w')
        tk.Label(base, text='Local educational hash search · Python or Rust · CPU parallelism', bg='#f5f7fb', fg='#64748b', font=('Segoe UI', 10)).pack(anchor='w', pady=(0, 14))
        pane = tk.LabelFrame(base, text=' Search configuration ', bg='white', fg='#334155', padx=13, pady=12, font=('Segoe UI', 10, 'bold'))
        pane.pack(fill='x')
        row = tk.Frame(pane, bg='white'); row.pack(fill='x', pady=4)
        self.field(row, 'Engine', ttk.Combobox(row, values=('Python', 'Rust'), state='readonly', textvariable=self.engine, width=14), 0)
        self.field(row, 'Hash function', ttk.Combobox(row, values=HASHES, state='readonly', textvariable=self.algorithm, width=16), 1)
        row2 = tk.Frame(pane, bg='white'); row2.pack(fill='x', pady=4)
        self.field(row2, 'Password length', tk.Spinbox(row2, from_=1, to=12, textvariable=self.length, width=12), 0)
        self.field(row2, 'CPU workers', tk.Spinbox(row2, from_=1, to=max(128, os.cpu_count() or 1), textvariable=self.workers, width=12), 1)
        tk.Label(pane, text='Characters', bg='white', fg='#334155').pack(anchor='w', pady=(10, 4))
        cs = tk.Frame(pane, bg='white'); cs.pack(fill='x')
        for label, var in [('a–z', self.use_low), ('A–Z', self.use_up), ('0–9', self.use_num), ('!@#$%^&*_-', self.use_special)]:
            ttk.Checkbutton(cs, text=label, variable=var).pack(side='left', padx=(0, 14))
        tk.Label(pane, text='Target hexadecimal hash (of your own test password)', bg='white', fg='#334155').pack(anchor='w', pady=(14, 4))
        ttk.Entry(pane, textvariable=self.target).pack(fill='x')
        buttons = tk.Frame(pane, bg='white'); buttons.pack(fill='x', pady=(12, 0))
        ttk.Button(buttons, text='Load lab Variant 5', command=self.preset).pack(side='left')
        ttk.Button(buttons, text='Generate test hash', command=self.test_hash).pack(side='left', padx=9)
        self.run_button = ttk.Button(buttons, text='Start search', command=self.start)
        self.run_button.pack(side='right')
        self.stop_button = ttk.Button(buttons, text='Stop', command=self.stop, state='disabled')
        self.stop_button.pack(side='right', padx=8)
        stats = tk.LabelFrame(base, text=' Progress & results ', bg='white', fg='#334155', padx=13, pady=12, font=('Segoe UI', 10, 'bold'))
        stats.pack(fill='both', expand=True, pady=(14, 0))
        self.pbar = ttk.Progressbar(stats, maximum=100, value=0)
        self.pbar.pack(fill='x', pady=(2, 9))
        self.status = tk.StringVar(value='Ready. Select a backend and click Start search.')
        tk.Label(stats, textvariable=self.status, bg='white', fg='#1e40af', anchor='w', wraplength=650).pack(fill='x')
        self.fields = {}
        for name in ('Candidates', 'Checked', 'Speed', 'Estimated full search', 'Estimated remaining', 'Elapsed', 'Result'):
            r = tk.Frame(stats, bg='white'); r.pack(fill='x', pady=4)
            tk.Label(r, text=name, bg='white', fg='#64748b', width=23, anchor='w').pack(side='left')
            v = tk.StringVar(value='—')
            tk.Label(r, textvariable=v, bg='white', fg='#0f172a', anchor='w', font=('Consolas', 10), wraplength=420).pack(side='left', fill='x', expand=True)
            self.fields[name] = v
        tk.Label(base, text='Estimates assume uniform search. Results vary with candidate order, CPU load, and hash type.', bg='#f5f7fb', fg='#64748b', font=('Segoe UI', 9), wraplength=700).pack(anchor='w', pady=(12, 0))

    def field(self, parent, title, widget, col):
        f = tk.Frame(parent, bg='white'); f.pack(side='left', fill='x', expand=True)
        tk.Label(f, text=title, bg='white', fg='#334155').pack(anchor='w')
        widget.pack(anchor='w', pady=(4, 0))

    def preset(self):
        self.use_low.set(True); self.use_up.set(False); self.use_num.set(False); self.use_special.set(True)
        self.length.set(6); self.algorithm.set('blake2s'); self.target.set(VARIANT5)
        self.status.set('Variant 5 loaded: 6 chars, lowercase + special, BLAKE2s.')

    def test_hash(self):
        from tkinter import simpledialog
        p = simpledialog.askstring('Test password', 'Enter a TEST password to hash locally:', parent=self)
        if p is not None:
            self.target.set(hashlib.new(self.algorithm.get(), p.encode('utf-8')).hexdigest())
            self.length.set(len(p))
            self.status.set('Test hash created locally. Choose character sets containing all password characters.')

    def config(self):
        chars = ''.join(s for enabled, s in ((self.use_low.get(), LOW), (self.use_up.get(), UP), (self.use_num.get(), NUM), (self.use_special.get(), SPECIAL)) if enabled)
        length = self.length.get(); workers = self.workers.get(); alg = self.algorithm.get()
        if not chars: raise ValueError('Select at least one character set.')
        if not 1 <= length <= 12: raise ValueError('Length must be between 1 and 12.')
        if not 1 <= workers <= 128: raise ValueError('Worker count must be 1–128.')
        digest = self.target.get().strip().lower()
        if len(digest) != hashlib.new(alg).digest_size * 2 or any(c not in '0123456789abcdef' for c in digest):
            raise ValueError('Target hash does not match the selected algorithm.')
        exe = ROOT / 'backend' / 'rust' / 'target' / 'release' / ('hash_lab_rust.exe' if os.name == 'nt' else 'hash_lab_rust')
        if self.engine.get() == 'Rust':
            if not exe.exists(): raise ValueError('Rust executable missing. Build it first: cd backend/rust && cargo build --release')
            cmd = [str(exe)]
        else:
            cmd = [sys.executable, '-u', str(ROOT / 'backend' / 'python' / 'engine.py')]
        return cmd, chars, length, workers, alg, digest

    def start(self):
        if self.active: return
        try: cfg = self.config()
        except (ValueError, tk.TclError) as exc:
            messagebox.showerror('Configuration', str(exc)); return
        cmd, chars, length, workers, alg, digest = cfg
        self.total = len(chars) ** length; self.checked = 0; self.rate = 0
        self.fields['Candidates'].set(f'{self.total:,}')
        for name in ('Checked', 'Speed', 'Estimated full search', 'Estimated remaining', 'Elapsed', 'Result'): self.fields[name].set('—')
        self.pbar['value'] = 0
        self.active = True; self.cancelled = False
        self.run_button.config(state='disabled'); self.stop_button.config(state='normal')
        self.status.set('Benchmarking backend...')
        threading.Thread(target=self.launch, args=(cfg,), daemon=True).start()

    def launch(self, cfg):
        cmd, chars, length, workers, alg, digest = cfg
        flags = ['--chars', chars, '--length', str(length), '--algorithm', alg, '--workers', str(workers)]
        try:
            proc = subprocess.run(cmd + flags + ['--benchmark'], capture_output=True, text=True, timeout=30)
            if proc.returncode != 0: raise RuntimeError(proc.stdout.strip() or proc.stderr.strip() or 'Benchmark failed')
            rate = float(next(l.split('\t')[1] for l in proc.stdout.splitlines() if l.startswith('RATE\t')))
            self.events.put(('rate', rate * workers))
            if self.cancelled:
                self.events.put(('done_ui',)); return
            self.events.put(('running',))
            self.process = subprocess.Popen(cmd + flags + ['--target', digest], stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True, bufsize=1)
            for line in self.process.stdout:
                self.events.put(('line', line.rstrip('\r\n')))
            code = self.process.wait()
            if code and not self.cancelled:
                self.events.put(('error', self.process.stderr.read().strip() or f'Backend exited with code {code}'))
            self.events.put(('done_ui',))
        except Exception as exc:
            self.events.put(('error', str(exc))); self.events.put(('done_ui',))

    def stop(self):
        self.cancelled = True
        self.status.set('Stopping workers...')
        proc = self.process
        if proc and proc.poll() is None:
            try:
                proc.stdin.write('STOP\n'); proc.stdin.flush()
            except (OSError, ValueError): pass

    def drain(self):
        try:
            while True:
                event = self.events.get_nowait()
                kind = event[0]
                if kind == 'rate':
                    self.rate = event[1]
                    self.fields['Speed'].set(f'~{self.rate:,.0f} hashes/s (benchmark estimate)')
                    self.fields['Estimated full search'].set(duration(self.total / max(1, self.rate)))
                    self.fields['Estimated remaining'].set(duration(self.total / max(1, self.rate)))
                elif kind == 'running':
                    self.started = time.perf_counter(); self.status.set('Searching...')
                elif kind == 'line': self.parse_line(event[1])
                elif kind == 'error': self.status.set('Error: ' + event[1])
                elif kind == 'done_ui':
                    self.active = False; self.process = None
                    self.run_button.config(state='normal'); self.stop_button.config(state='disabled')
                    if self.cancelled and not self.status.get().startswith('Error'): self.status.set('Stopped.')
        except queue.Empty: pass
        self.after(100, self.drain)

    def parse_line(self, line):
        parts = line.split('\t')
        if not parts: return
        if parts[0] == 'PROGRESS' and len(parts) >= 3:
            count = int(parts[1]); elapsed = float(parts[2]); self.display_progress(count, elapsed)
        elif parts[0] == 'DONE' and len(parts) >= 5:
            password, count, elapsed, cancelled = parts[1], int(parts[2]), float(parts[3]), parts[4] == '1'
            self.display_progress(count, elapsed)
            self.fields['Result'].set(password if password else ('Cancelled' if cancelled else 'Not found'))
            self.status.set('Found matching password!' if password else ('Stopped.' if cancelled else 'Search finished; no match.'))
        elif parts[0] == 'ERROR': self.status.set('Error: ' + '\t'.join(parts[1:]))

    def display_progress(self, count, elapsed):
        self.checked = count
        self.fields['Checked'].set(f'{count:,} / {self.total:,} ({count / self.total * 100:.3f}%)')
        self.fields['Elapsed'].set(duration(elapsed))
        self.pbar['value'] = min(100, count / self.total * 100)
        if elapsed > .3 and count > 0:
            actual = count / elapsed
            self.fields['Speed'].set(f'{actual:,.0f} hashes/s (observed)')
            self.fields['Estimated remaining'].set(duration((self.total-count)/actual))


if __name__ == '__main__':
    App().mainloop()
