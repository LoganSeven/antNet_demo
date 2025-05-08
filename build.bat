@echo off
SETLOCAL ENABLEDELAYEDEXPANSION

REM -----------------------------------------------------------------------------
REM Resolve root directory
cd /d "%~dp0"

REM -----------------------------------------------------------------------------
echo [1/5] Building C backend with CMake...
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build --config Debug

REM -----------------------------------------------------------------------------
echo [2/5] Generating Python error constants...
IF NOT EXIST src\python\consts\_generated (
    mkdir src\python\consts\_generated
)
python src\python\tools\errors_const_c2python.py ^
    include\error_codes.h ^
    src\python\consts\_generated

REM -----------------------------------------------------------------------------
echo [3/5] Building Python CFFI bindings...
IF EXIST "venv\Scripts\activate.bat" (
    call venv\Scripts\activate.bat
)
python src\python\ffi\ffi_build.py

REM -----------------------------------------------------------------------------
echo [4/5] Copying compiled module...
IF NOT EXIST "src\python\ffi" (
    mkdir src\python\ffi
)
copy /Y build\python\backend_cffi*.pyd src\python\ffi\ > NUL

REM -----------------------------------------------------------------------------
echo [5/5] Running tests...
python -m pytest -s tests\
IF ERRORLEVEL 1 (
    echo ❌ Tests failed!
    EXIT /B 1
)

echo ✅ Build and tests completed successfully.

ENDLOCAL
