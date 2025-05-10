#!/usr/bin/env bash
# src/build.sh
# POSIX-compatible build + test pipeline for the AntNet demo project.

set -e

# -----------------------------------------------------------------------------
# [1/9] Locate repo root
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
cd "$SCRIPT_DIR"

# -----------------------------------------------------------------------------
echo "[1/9] Building C backend with CMake..."
cmake -S . -B build -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Debug
cmake --build build

# -----------------------------------------------------------------------------
echo "[2/9] Generating Python error constants..."
mkdir -p src/python/consts/_generated
python3 src/python/tools/errors_const_c2python.py \
    include/error_codes.h \
    src/python/consts/_generated

# -----------------------------------------------------------------------------
echo "[3/9] Generating Python TypedDicts from C headers..."
mkdir -p src/python/structs/_generated
python3 src/python/tools/generate_structs.py \
    --headers \
        include/antnet_network_types.h \
        include/antnet_config_types.h \
        include/antnet_brute_force_types.h \
        include/backend.h \
    --output src/python/structs/_generated/auto_structs.py

# -----------------------------------------------------------------------------
echo "[4/9] Preprocessing aggregator header for pycparser..."
FAKE_LIBC="$(python3 -c 'import pycparser, os; p=os.path.dirname(pycparser.__file__); print(os.path.join(p,"utils","fake_libc_include"))')"

mkdir -p build/preprocessed
rm -f build/preprocessed/*.i || true

python3 src/python/ffi/preprocess_headers.py \
    --headers include/cffi_entrypoint.h \
    --outdir build/preprocessed \
    --include ./include \
    --include "$FAKE_LIBC" \
    --cpp-flag -nostdinc \
    --cpp-flag -D__attribute__(...)= \
    --cpp-flag -D__attribute__= \
    --cpp-flag -D__extension__= \
    --cpp-flag -D__inline__= \
    --cpp-flag -D__volatile__= \
    --cpp-flag -D__asm__= \
    --cpp-flag -D__restrict= \
    --cpp-flag -D__restrict__= \
    --cpp-flag -D__builtin_va_list=int \
    --cpp-flag -D__GNUC__=4

# -----------------------------------------------------------------------------
echo "[5/9] Generating CFFI CDEF_SOURCE with pycparser..."
python3 src/python/ffi/generate_cffi_defs.py \
    --preprocessed build/preprocessed \
    --output src/python/ffi/cdef_string.py

# -----------------------------------------------------------------------------
echo "[6/9] Building Python CFFI bindings..."
if [ -f venv/bin/activate ]; then
    . venv/bin/activate
fi
python3 -m src.python.ffi.ffi_build

# -----------------------------------------------------------------------------
echo "[7/9] Copying compiled module..."
mkdir -p src/python/ffi
cp build/python/backend_cffi*.so src/python/ffi/ || true

# -----------------------------------------------------------------------------
echo "[8/9] Running tests..."
python3 -m pytest -s tests/

# -----------------------------------------------------------------------------
echo "[9/9] Build and tests completed successfully."
