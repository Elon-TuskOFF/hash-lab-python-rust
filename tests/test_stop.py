"""Exercise both backends' STOP protocol on Windows, including Python child cleanup."""
import hashlib
import os
import pathlib
import subprocess
import sys
import time

ROOT=pathlib.Path(__file__).resolve().parents[1]
targets=[
    [sys.executable,"-u",str(ROOT/"backend/python/engine.py")],
    [str(ROOT/"backend/rust/target/release/hash_lab_rust.exe")]
]
for cmd in targets:
    name="Python" if "engine.py" in " ".join(cmd) else "Rust"
    args=["--chars","abcdefghijklmnopqrstuvwxyz","--length","6","--algorithm","sha256",
          "--workers","4","--target",hashlib.sha256(b"not_in_charset_123").hexdigest()]
    process=subprocess.Popen(cmd+args,stdin=subprocess.PIPE,stdout=subprocess.PIPE,
                             stderr=subprocess.PIPE,text=True,cwd=ROOT)
    try:
        time.sleep(1.0)
        if process.poll() is not None:
            raise AssertionError(f"{name} terminated before STOP")
        output,errors=process.communicate(input="STOP\n",timeout=12)
        if process.returncode!=0 or not any(line.startswith("DONE\t") and line.endswith("\t1") for line in output.splitlines()):
            raise AssertionError(f"{name} did not acknowledge cancellation: {output[-500:]} {errors[-500:]}")
        print(f"{name} STOP passed")
    finally:
        if process.poll() is None:
            process.kill()
            process.communicate()
