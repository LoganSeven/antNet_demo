#!/usr/bin/env bash
set -euo pipefail

# Directory of this script
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

cd "$SCRIPT_DIR"

echo "[1/4] Building C backend with CMake..."
cmake -S . -B build -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Debug
cmake --build build

echo "[2/4] Building Python CFFI bindings..."
if [ -f venv/bin/activate ]; then
  source venv/bin/activate
fi
python3 src/python/ffi/ffi_build.py

echo "[3/4] Copying compiled module..."
mkdir -p src/python/ffi
cp build/python/backend_cffi*.so src/python/ffi/

echo "[4/4] Running tests..."
python3 -m pytest tests/

echo "✅ Build and tests completed successfully."
