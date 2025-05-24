#!/usr/bin/env python3
# build.py — Cross-platform build and test pipeline for the AntNet demo project.

import os
import sys
import subprocess
import shutil
import sysconfig
from pathlib import Path

# -----------------------------------------------------------------------------
# CONFIG
# -----------------------------------------------------------------------------
REPO_ROOT = Path(__file__).resolve().parent
BUILD_DIR = REPO_ROOT / "build"
PYTHON = sys.executable
EXT_SUFFIX = sysconfig.get_config_var("EXT_SUFFIX") or ".so"

VENV_BIN = REPO_ROOT / "venv" / ("Scripts" if os.name == "nt" else "bin")

# -----------------------------------------------------------------------------
# UTILITIES
# -----------------------------------------------------------------------------
def log(icon: str, msg: str) -> None:
    print(f"{icon} {msg}")

def run(tag: str, cmd: list[str]) -> None:
    print(f"[{tag}] {' '.join(cmd)}")
    subprocess.run(cmd, check=True)

def ensure_dir(path: str | Path) -> None:
    Path(path).mkdir(parents=True, exist_ok=True)

def remove_build_dir(path: Path) -> None:
    if path.exists():
        log("🗑️", f"Removing old build directory: {path}")
        shutil.rmtree(path)

# -----------------------------------------------------------------------------
# MAIN BUILD PIPELINE
# -----------------------------------------------------------------------------
def main() -> None:
    os.chdir(REPO_ROOT)

    # Regenerate CMakeFiles.txt taking new .c and header folders in account
    log("🛠️", "Regenerating CMakeLists.txt...")
    run("", [PYTHON, "src/python/tools/generate_cmake.py", "--write"])

    # ---------------------------------------------------------------------
    # Ensure all .c/.h files are AI-annotated with their relative path
    # This helps AI models and developer tools understand file structure,
    # navigate easily, and maintain coherence across headers and sources.
    # ---------------------------------------------------------------------
    log("✨", "annotate files with their relative path, for ai tools...")
    run("", [PYTHON, "src/python/tools/ai_annotate_relative_path.py"])

    # Cleanup
    log("🗑️","removing the old build directory")
    remove_build_dir(BUILD_DIR)


    # Regenerate ffi_build.py from scanned .c/.h layout
    log("🛠️", "Regenerating ffi_build.py from directory structure...")
    run("", [
        PYTHON, "src/python/tools/build_builder.py",
        "--src-c-dir", "../../c",
        "--include-dir", "../../../include",
        "--third-party-dir", "../../../third_party",
        "--exclude", "build,tests,obsolete",
        "--module-name", "backend_cffi",
        "--output", "src/python/ffi/ffi_build.py"
    ])

    # CMake build
    log("🔨", "Configuring project with CMake...")
    run("", ["cmake", "-S", ".", "-B", "build", "-DCMAKE_BUILD_TYPE=Debug"])
    run("", ["cmake", "--build", "build"])

    #  Generate Python error constants
    log("🛠️", "Generating Python constants from error codes... Still useful ❓")
    ensure_dir("src/python/consts/_generated")
    run("2/9", [
        PYTHON, "src/python/tools/errors_const_c2python.py",
        "include/consts/error_codes.h",
        "src/python/consts/_generated"
    ])

    # struct gen
    ensure_dir("src/python/structs/_generated")
    log("🛠️", "Generating Python structs from C headers...")
    run("", [
            PYTHON, "src/python/tools/generate_structs.py",
            "--headers",
            "include/types/antnet_network_types.h",
            "include/types/antnet_config_types.h",
            "include/types/antnet_path_types.h",
            "include/types/antnet_brute_force_types.h",
            "include/types/antnet_aco_v1_types.h",
            "include/types/antnet_sasa_types.h",
            "include/types/antnet_ranking_types.h",
        "--output", "src/python/structs/_generated/auto_structs.py"])

    # Preprocess headers for CFFI
    log("📦", "Preprocessing headers for pycparser...")
    fake_libc = subprocess.check_output([
        PYTHON, "-c",
        "import pycparser, os, pathlib; "
        "print(pathlib.Path(os.path.dirname(pycparser.__file__), 'utils', 'fake_libc_include').resolve())"
    ], text=True).strip()

    ensure_dir("build/preprocessed")
    for f in Path("build/preprocessed").glob("*.i"):
        f.unlink()

    cpp_flags = [
        "-nostdinc", "-D__attribute__(...)=", "-D__attribute__=",
        "-D__extension__=", "-D__inline__=", "-D__volatile__=",
        "-D__asm__=", "-D__restrict=", "-D__restrict__=",
        "-D__builtin_va_list=int", "-D__GNUC__=4",
        "-Dbool=_Bool", "-Dtrue=1", "-Dfalse=0"
    ]
    cpp_tokens = [f"--cpp-flag={flag}" for flag in cpp_flags]

    run("", [
        PYTHON, "src/python/tools/preprocess_headers.py",
        "--headers", "include/cffi_entrypoint.h",
        "--outdir", "build/preprocessed",
        "--include", "./stub_headers",
        "--include", "./include",
        "--include", fake_libc,
        *cpp_tokens
    ])

    # Generate CDEF_SOURCE
    log("🛠️", "Generating CFFI CDEF_SOURCE...")
    run("", [
        PYTHON, "src/python/tools/generate_cffi_defs.py",
        "--preprocessed", "build/preprocessed",
        "--output", "src/python/ffi/cdef_string.py"
    ])

    # Regenerate backend_api.py
    # Appends or refreshes an AUTO-GENERATED block with wrappers for every 
    # antnet_* function found in cdef_string.py that is not already implemented in the base template.
    log("🛠️", "Regenerating backend_api.py from CDEF source...")
    run("",[PYTHON, "src/python/tools/backend_end_api_builder.py"])

    # Build Python bindings
    log("🧱", "Building Python CFFI module...")
    if (VENV_BIN / "activate").exists():
        os.environ["PATH"] = f"{VENV_BIN}:{os.environ['PATH']}"
    run("", [PYTHON, "-m", "src.python.ffi.ffi_build"])

    # Copy generated .so/.pyd to ffi/
    log("📥", f"Copying compiled CFFI module (*.{EXT_SUFFIX}) to src/python/ffi/")
    ensure_dir("src/python/ffi")
    for mod in Path("build/python").glob(f"backend_cffi*{EXT_SUFFIX}"):
        dest = Path("src/python/ffi") / mod.name
        dest.write_bytes(mod.read_bytes())
        log("✅", f"Copied: {mod.name}")

    # Run tests
    log("🧪", "Running unit tests with pytest...")
    run("", [PYTHON, "-m", "pytest", "-s", "tests/"])

    # ✅ Done
    log("🏆", "Build and tests completed successfully.")

if __name__ == "__main__":
    main()
