#!/usr/bin/env bash
# Build + test pipeline for the AntNet demo project.                        # hardening/doc
set -euo pipefail

# -----------------------------------------------------------------------------
# Locate repo root
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

# -----------------------------------------------------------------------------
echo "[1/5] Building C backend with CMake..."
cmake -S . -B build -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Debug
cmake --build build

# -----------------------------------------------------------------------------
echo "[2/5] Generating Python error constants..."
mkdir -p src/python/consts/_generated
python3 src/python/tools/errors_const_c2python.py \
    include/error_codes.h \
    src/python/consts/_generated

# -----------------------------------------------------------------------------
echo "[3/5] Building Python CFFI bindings..."
if [[ -f venv/bin/activate ]]; then
    # shellcheck disable=SC1091
    source venv/bin/activate
fi
python3 src/python/ffi/ffi_build.py

# -----------------------------------------------------------------------------
echo "[4/5] Copying compiled module..."
mkdir -p src/python/ffi
cp build/python/backend_cffi*.so src/python/ffi/

# -----------------------------------------------------------------------------
echo "[5/5] Running tests..."
# Display test-level status lines (✅ / 🛡️ SECURITY ✅ )
python3 -m pytest -s tests/

echo "✅ Build and tests completed successfully."
