# Hash Lab — Python + Rust

A small **Windows-friendly Tkinter desktop app** for educational, local brute-force searches of **unsalted test hashes**. The same interface controls either a Python multiprocessing backend or a Rust multithreading backend. No web service, passwords, or hashes are uploaded.

## Features

- Switch between **Python** and **Rust** in one app.
- Choose lowercase, uppercase, digits, and `!@#$%^&*_-` character sets.
- Choose hash: MD5, SHA-1, SHA-224/256/384/512, SHA3-224/256/384/512, BLAKE2s/BLAKE2b.
- Set test-password length (1–12) and CPU worker count (1–128).
- Display full keyspace, sampled hash-speed estimate, live progress, observed speed, elapsed time, remaining-time estimate, and match.
- Stop button and **Variant 5** preset from the provided lab instructions: length **6**, `abcdefghijklmnopqrstuvwxyz!@#$%^&*_-`, **BLAKE2s**, target `713417394e2b74aee7fa54d09375b0c78d288a90eba27a560cfc698c5b54886c`.

## Windows setup

Install **Python 3.10+** and (to use Rust) **Rust via rustup**. Make sure `python` and `cargo` are on PATH. Open PowerShell **inside the repository directory**:

```powershell
python app.py
```

Python mode works immediately and requires **no pip packages**.

For the Rust mode, compile once:

```powershell
cd backend\rust
cargo build --release
cd ..\..
python app.py
```

Use the **Engine** dropdown to choose Python or Rust. For a single CPU worker, set `CPU workers = 1`; for multiple workers, choose your logical CPU count. Each mode uses separate CPU threads/processes (not the GPU).

## Linux

Install `python3-tk` if needed. Run `python3 app.py`. Build the Rust backend with `cd backend/rust && cargo build --release`.

## How to try a quick test

1. Select Python and SHA-256.
2. Choose only lowercase letters, length 3.
3. Click **Generate test hash**, type `cat`, and press **Start search**. The candidate `cat` should be found quickly.
4. Select Rust (after compiling) to compare.

The **full search estimate** is `charset_size ^ length / measured_hashes_per_second`, approximating the worst case. Expected average time for uniformly random passwords is about half the full-search figure. Early estimates are approximate because startup overhead, process contention, OS scheduling, and candidate construction vary. For very large keyspaces the app can run for hours or longer.

## Project layout

```text
app.py                    # Tkinter front end
backend/python/engine.py  # Python multiprocessing backend
backend/rust/Cargo.toml   # Rust dependencies
backend/rust/src/main.rs  # Rust threaded backend
```

## Notes

The CPU count from `os.cpu_count()` represents logical processors and might be larger than physical cores. More workers does not necessarily mean a proportional speed-up. Python uses separate processes to avoid the CPython GIL limiting CPU-bound threads; Rust uses native OS threads.

This educational exercise models **offline raw hashes only** and not modern salted, slow password-storage methods such as Argon2, bcrypt, or scrypt. Only test hashes you create or are authorized to analyze. 
MD5 and SHA-1 are offered for lab compatibility, not recommended for secure password storage.
MD5 and BLAKE2 functionality is from libraries. The backends benchmark the digest function only; real exhaustive scanning also has candidate-generation and coordination costs.
MD5, SHA-1, etc. names in the dropdown denote raw algorithms with UTF-8 input.
