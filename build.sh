#!/usr/bin/env bash
# Build + test pipeline for the AntNet demo project.
set -euo pipefail

# -----------------------------------------------------------------------------
# Locate repo root
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

# -----------------------------------------------------------------------------
echo "[1/6] Building C backend with CMake..."
cmake -S . -B build -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Debug
cmake --build build

# -----------------------------------------------------------------------------
echo "[2/6] Generating Python error constants..."
mkdir -p src/python/consts/_generated
python3 src/python/tools/errors_const_c2python.py \
    include/error_codes.h \
    src/python/consts/_generated

# -----------------------------------------------------------------------------
echo "[3/6] Generating Python TypedDicts from C headers..."
mkdir -p src/python/structs/_generated
python3 src/python/tools/generate_structs.py \
    --headers include/antnet_network_types.h include/antnet_config_types.h include/antnet_brute_force_types.h \
    --output src/python/structs/_generated/auto_structs.py

# -----------------------------------------------------------------------------
echo "[4/6] Building Python CFFI bindings..."
if [[ -f venv/bin/activate ]]; then
    # shellcheck disable=SC1091
    source venv/bin/activate
fi
python3 src/python/ffi/ffi_build.py

# -----------------------------------------------------------------------------
echo "[5/6] Copying compiled module..."
mkdir -p src/python/ffi
cp build/python/backend_cffi*.so src/python/ffi/

# -----------------------------------------------------------------------------
echo "[6/6] Running tests..."
# Display test-level status lines (✅ / 🛡️ SECURITY ✅ )
python3 -m pytest -s tests/

echo "✅ Build and tests completed successfully."
