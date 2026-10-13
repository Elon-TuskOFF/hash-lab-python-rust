"""Local educational hash search backend. Output protocol: RATE, PROGRESS, DONE, ERROR."""
import argparse
import hashlib
import itertools
import multiprocessing as mp
import os
import queue
import sys
import threading
import time


def make_hash(text, algorithm):
    h = hashlib.new(algorithm)
    h.update(text.encode('utf-8'))
    return h.hexdigest()


def worker(index, count, chars, length, algorithm, target, stop, progress, result):
    n = len(chars)
    checked = 0
    pending = 0
    # Dividing by first character keeps every candidate unique.
    for first in range(index, n, count):
        for tail in itertools.product(chars, repeat=length - 1):
            if stop.is_set():
                progress.put(pending)
                return
            candidate = chars[first] + ''.join(tail)
            checked += 1
            pending += 1
            if make_hash(candidate, algorithm) == target:
                result.put(candidate)
                stop.set()
                progress.put(pending)
                return
            if pending >= 4096:
                progress.put(pending)
                pending = 0
    progress.put(pending)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--chars', required=True)
    ap.add_argument('--length', type=int, required=True)
    ap.add_argument('--algorithm', required=True)
    ap.add_argument('--target', default='')
    ap.add_argument('--workers', type=int, default=1)
    ap.add_argument('--benchmark', action='store_true')
    args = ap.parse_args()
    try:
        if not args.chars or args.length < 1 or args.length > 12 or args.workers < 1:
            raise ValueError('Invalid length, character set, or worker count')
        if args.algorithm not in hashlib.algorithms_available:
            raise ValueError('Unsupported hash algorithm')
        if args.benchmark:
            start = time.perf_counter()
            for i in range(25000):
                make_hash(args.chars[i % len(args.chars)] * args.length, args.algorithm)
            duration = max(time.perf_counter() - start, 0.000001)
            print(f'RATE\t{25000 / duration:.2f}', flush=True)
            return
        expected = hashlib.new(args.algorithm).digest_size * 2
        if len(args.target) != expected or any(c not in '0123456789abcdefABCDEF' for c in args.target):
            raise ValueError(f'Expected {expected} hexadecimal characters for {args.algorithm}')
        target = args.target.lower()
        start = time.perf_counter()
        ctx = mp.get_context('spawn')
        stop = ctx.Event()
        progress = ctx.Queue()
        result = ctx.Queue()
        number = min(args.workers, len(args.chars))
        processes = [ctx.Process(target=worker, args=(i, number, args.chars, args.length, args.algorithm, target, stop, progress, result)) for i in range(number)]
        for p in processes:
            p.start()

        def watch_stdin():
            for line in sys.stdin:
                if line.strip() == 'STOP':
                    stop.set()
                    return
        threading.Thread(target=watch_stdin, daemon=True).start()
        total = 0
        found = None
        last = 0
        while any(p.is_alive() for p in processes):
            try:
                total += progress.get(timeout=0.12)
            except queue.Empty:
                pass
            try:
                found = result.get_nowait()
                stop.set()
            except queue.Empty:
                pass
            if time.monotonic() - last > 0.15:
                print(f'PROGRESS\t{total}\t{time.perf_counter()-start:.3f}', flush=True)
                last = time.monotonic()
        for p in processes:
            p.join()
        while True:
            try:
                total += progress.get_nowait()
            except queue.Empty:
                break
        if found is None:
            try:
                found = result.get_nowait()
            except queue.Empty:
                pass
        print(f'DONE\t{found or ""}\t{total}\t{time.perf_counter()-start:.3f}\t{int(stop.is_set() and not found)}', flush=True)
    except Exception as exc:
        print(f'ERROR\t{exc}', flush=True)
        sys.exit(1)


if __name__ == '__main__':
    mp.freeze_support()
    main()
