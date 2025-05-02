# BUILD.md

## Overview

This document describes the methods available to build the AntNet Demo project. It covers both manual steps and automated scripts for Linux/macOS and Windows environments.

---

## Prerequisites

Before building the project, ensure that the following tools and packages are installed and configured:

* **Python 3.12+** with a virtual environment
* **pip** for Python package management
* **CMake 3.10+**
* **GCC** (or another compatible C compiler)
* **Make** (if using Unix Makefiles)
* **qtpy** and **PyQt5** Python packages
* **cffi**, **setuptools**, and **pytest** Python packages
* On Linux distributions: `qt5-default`, `libqt5gui5`, `libqt5core5a`, `libqt5widgets5` (for Qt runtime)

---

## Manual Build Steps

Follow these steps from the project root directory (`antnet-demo/`):

1. **Activate the Python virtual environment**

   ```bash
   source venv/bin/activate
   ```

2. **Install Python dependencies**

   ```bash
   pip install -r requirements.txt
   ```

3. **Configure and build the C backend**

   ```bash
   mkdir -p build
   cmake -S . -B build -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Debug
   cmake --build build
   ```

4. **Build the Python CFFI bindings**

   ```bash
   cd src/python/ffi
   python3 ffi_build.py
   cd ../../..
   ```

5. **Copy the compiled CFFI module**

   ```bash
   mkdir -p src/python/ffi
   cp build/python/backend_cffi*.so src/python/ffi/
   ```

6. **Run unit tests**

   ```bash
   python3 -m pytest tests/
   ```

7. **Launch the application**

   ```bash
   python3 main.py
   ```

---

## Automated Build Scripts

The project includes two scripts to perform all build steps automatically.

### Linux/macOS: `build.sh`

1. Ensure the script is executable:

   ```bash
   chmod +x build.sh
   ```

2. Run the script:

   ```bash
   ./build.sh
   ```

This script will:

* Configure and build the C backend with CMake
* Build the Python CFFI bindings
* Copy the compiled `.so` file
* Run the unit tests

### Windows: `build.bat`

1. From Command Prompt or PowerShell, run:

   ```bat
   build.bat
   ```

This batch file will:

* Configure and build the C backend with CMake
* Build the Python CFFI bindings
* Copy the compiled `.pyd` file
* Run the unit tests

---

## Cleaning Up

To remove all generated files and return the project to a clean state, use the Makefile target or delete build directories manually.

**Using Makefile** (if available):

```bash
make clean
```

**Manual cleanup**:

```bash
rm -rf build/
find src/python/ffi -type f \( -name '*.so' -o -name '*.pyd' -o -name '*.o' -o -name '*.c' \) -delete
```

---

## Notes

* Always ensure that the virtual environment is activated when installing Python dependencies or running Python commands.
* Adjust paths if you customize the project layout.
* For a Windows environment, ensure that `venv\Scripts\activate.bat` is called before installing or running Python scripts.

---

Last updated: by Logan7
