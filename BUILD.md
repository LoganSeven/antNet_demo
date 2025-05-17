# BUILD.md

## Overview

This document describes how to build and test the **AntNet Demo Project**, a C + Python application using CMake, multithreaded C backends (pthreads), and Python bindings via CFFI. The process is unified via a single script: `build.py`.

---

## Table of Contents

- [Overview](#overview)
- [Prerequisites](#prerequisites)
- [Recommended Workflow](#recommended-workflow)
- [Manual Build Steps](#manual-build-steps)
- [Windows Notes](#windows-notes)
- [Cleaning Up](#cleaning-up)
- [CI Integration](#ci-integration)
- [Notes](#notes)

---

## Prerequisites

Install the following:

| Requirement            | Linux/macOS                    | Windows                              |
|------------------------|--------------------------------|--------------------------------------|
| Python                 | 3.12+                          | 3.12+ via [python.org](https://www.python.org) |
| pip                    | latest                         | included with Python                 |
| C compiler             | gcc, clang                     | MSYS2 with gcc / MinGW-w64           |
| CMake                  | 3.10+                          | [cmake.org](https://cmake.org)       |
| Make (or Ninja)        | included on Linux              | part of MSYS2 or Git Bash            |
| Qt Runtime (optional)  | `qt5-default`, `libqt5widgets5`| Install via pip (`PyQt5`) or [Qt installer](https://www.qt.io/download) |

### Required Python packages

```bash
pip install cffi pycparser pytest qtpy PyQt5

Or:

pip install -r requirements.txt

Recommended Workflow
1. Create and activate virtual environment

python3 -m venv venv
source venv/bin/activate  # On Windows: venv\Scripts\activate.bat

2. Install Python dependencies

pip install -r requirements.txt

3. Run the unified build script

python build.py

This will:

    Compile the C backend with CMake

    Preprocess headers using cpp (with stubbed pthread.h)

    Generate the CDEF bindings using pycparser

    Compile the CFFI Python extension

    Run all unit tests with pytest

Manual Build Steps

You can reproduce the pipeline step by step:

# C backend build
mkdir -p build
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build

# Preprocess headers
python src/python/ffi/preprocess_headers.py \
  --headers include/cffi_entrypoint.h \
  --outdir build/preprocessed \
  --include ./stub_headers --include ./include \
  --cpp-flag=-nostdinc --cpp-flag=-D__attribute__(...)= ...

# Generate CDEF
python src/python/ffi/generate_cffi_defs.py \
  --preprocessed build/preprocessed \
  --output src/python/ffi/cdef_string.py

# Compile FFI
python src/python/ffi/ffi_build.py

# Run tests
python -m pytest tests/

Windows Notes

On Windows, CFFI-based projects that rely on pthreads require a POSIX-compatible environment.
Options:

    MSYS2 (recommended)
    Download from https://www.msys2.org
    After installing, use:

    pacman -S mingw-w64-x86_64-gcc make cmake

    MinGW-w64 standalone
    Available from https://www.mingw-w64.org/
    May require manual setup of environment variables (PATH, CC, etc.)

    Qt
    If you need GUI support and qtpy is used:

        Use pip install PyQt5 to satisfy bindings

        Qt Designer and full runtime may be downloaded from https://www.qt.io/download

CMake and Make are available via MSYS2, Git Bash, or Visual Studio build tools.
Cleaning Up

To remove all build artifacts:

rm -rf build/
find src/python/ffi -type f

