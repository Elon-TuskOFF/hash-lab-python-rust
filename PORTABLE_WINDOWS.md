# Portable Windows edition

You do **not** need to install Python, Rust, Qt, Visual Studio, or CMake to run the packaged Windows app.

## Download and run

1. Open [GitHub Actions](https://github.com/Elon-TuskOFF/hash-lab-python-rust/actions/workflows/windows-portable.yml).
2. Open the newest successful **Build portable Windows app** run.
3. Download the artifact named **HashLab-Windows-Portable** at the bottom of the run page (GitHub may require you to sign in).
4. Extract the downloaded archive, then extract `HashLab-Windows-Portable.zip` if it is nested.
5. Double-click `HashLab.exe`.

**Keep the extracted folders next to the EXE.** The portable package contains all required Qt libraries and two engines, including its own Python interpreter and Rust executable. No installer and no command line are required for ordinary use.

Windows 10/11 x64 is the target. The app is not currently a single standalone EXE: it is a portable folder inside a ZIP. Running from inside the archive without extracting will not work.

The Python/Rust selection is inside the app. Initial loading and hash speed depend on your CPU. Only test hashes you own or have permission to analyze.

## Build from source

Developers can see `QT_SETUP.md`. GitHub Actions builds the portable ZIP from source using the workflow under `.github/workflows/windows-portable.yml`.

**Build verification:** The availability of the download depends on the latest GitHub Actions workflow finishing successfully.
