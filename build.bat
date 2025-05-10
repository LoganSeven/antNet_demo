@echo off
SETLOCAL ENABLEDELAYEDEXPANSION

REM -----------------------------------------------------------------------------
REM Resolve root directory
cd /d "%~dp0"

REM -----------------------------------------------------------------------------
echo [1/7] Building C backend with CMake...
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build --config Debug

REM -----------------------------------------------------------------------------
echo [2/7] Generating Python error constants...
IF NOT EXIST src\python\consts\_generated (
    mkdir src\python\consts\_generated
)
python src\python\tools\errors_const_c2python.py ^
    include\error_codes.h ^
    src\python\consts\_generated

REM -----------------------------------------------------------------------------
echo [3/7] Generating Python TypedDicts from C headers...
IF NOT EXIST src\python\structs\_generated (
    mkdir src\python\structs\_generated
)
python src\python\tools\generate_structs.py ^
    --headers ^
        include\antnet_network_types.h ^
        include\antnet_config_types.h ^
        include\antnet_brute_force_types.h ^
        include\backend.h ^
    --output src\python\structs\_generated\auto_structs.py

REM -----------------------------------------------------------------------------
echo [4/7] Generating CFFI CDEF declarations from headers...
python src\python\ffi\generate_cffi_defs.py ^
    --headers ^
        include\antnet_network_types.h ^
        include\antnet_config_types.h ^
        include\antnet_brute_force_types.h ^
        include\backend.h ^
    --output src\python\ffi\cdef_string.py

REM -----------------------------------------------------------------------------
echo [5/7] Building Python CFFI bindings...
IF EXIST "venv\Scripts\activate.bat" (
    call venv\Scripts\activate.bat
)
python src\python\ffi\ffi_build.py

REM -----------------------------------------------------------------------------
echo [6/7] Copying compiled module...
IF NOT EXIST "src\python\ffi" (
    mkdir src\python\ffi
)
copy /Y build\python\backend_cffi*.pyd src\python\ffi\ > NUL

REM -----------------------------------------------------------------------------
echo [7/7] Running tests...
python -m pytest -s tests\
IF ERRORLEVEL 1 (
    echo Tests failed!
    EXIT /B 1
)

echo Build and tests completed successfully.

ENDLOCAL
