# Native C++ interface (Qt 6)

The project now includes a dark-themed native desktop interface in `src/main.cpp`. It uses Qt Widgets and Qt's `QProcess` to control the existing Python and Rust search engines.

## Windows requirements

- Qt 6 with Qt Widgets and a desktop compiler kit (e.g. MSVC 2022 x64)
- CMake 3.21+
- Python 3 on PATH for Python mode
- Rust/Cargo for Rust mode

## Build in Qt Creator

1. Open `CMakeLists.txt` in Qt Creator.
2. Select a Qt 6 Desktop kit.
3. Build and run.
4. Set the **working directory** to the repository root in Projects → Run if needed.

Or use the Qt-configured command prompt:

```powershell
cmake -S . -B build
cmake --build build --config Release
```

For the Rust backend:

```powershell
cd backend\rust
cargo build --release
cd ..\..
```

Launch the C++ application from the repository root so it can locate `backend/python/engine.py`. The old Python UI is still available with `python app.py`.

The C++ interface includes engine selection, hash algorithm, character sets, worker count, Variant 5 preset, progress, measured rate, time estimate, and result.

**Note:** The C++ version has not yet been compile-tested on Windows. The estimate is approximate, and long searches may take many hours.
