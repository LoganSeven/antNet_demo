@echo off
SETLOCAL ENABLEDELAYEDEXPANSION

REM 1) Build C backend with CMake
echo [1/4] Building C backend with CMake...
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build --config Debug

REM 2) Build CFFI bindings
echo [2/4] Building Python CFFI bindings...
IF EXIST "venv\Scripts\activate.bat" (
    call venv\Scripts\activate.bat
)
python src\python\ffi\ffi_build.py

REM 3) Copy compiled module
echo [3/4] Copying compiled module...
IF NOT EXIST "src\python\ffi" mkdir src\python\ffi
copy /Y build\python\backend_cffi*.pyd src\python\ffi\

REM 4) Run tests
echo [4/4] Running tests...
python -m pytest tests\
IF ERRORLEVEL 1 (
    echo ❌ Tests failed!
    EXIT /B 1
)

echo ✅ Build and tests completed successfully.

ENDLOCAL
