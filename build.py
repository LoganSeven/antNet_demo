#!/usr/bin/env python3
# build.py
# Cross-platform build and test pipeline for the AntNet demo project.

import os
import subprocess
from pathlib import Path

def run(tag: str, cmd: list[str]) -> None:
    """Prints and executes a subprocess command."""
    print(f"[{tag}] {' '.join(cmd)}")
    subprocess.run(cmd, check=True)

def ensure_dir(path: str | Path) -> None:
    """Creates a directory (including parents) if it does not exist."""
    Path(path).mkdir(parents=True, exist_ok=True)

def main() -> None:
    repo_root = Path(__file__).resolve().parent
    os.chdir(repo_root)

    # -------------------------------------------------------------------------
    # 1/9 Build C backend with CMake
    run("1/9", [
        "cmake", "-S", ".", "-B", "build",
        "-G", "Unix Makefiles",
        "-DCMAKE_BUILD_TYPE=Debug"
    ])
    run("1/9", ["cmake", "--build", "build"])

    # -------------------------------------------------------------------------
    # 2/9 Generate Python error constants
    ensure_dir("src/python/consts/_generated")
    run("2/9", [
        "python3", "src/python/tools/errors_const_c2python.py",
        "include/error_codes.h",
        "src/python/consts/_generated"
    ])

    # -------------------------------------------------------------------------
    # 3/9 Generate Python TypedDicts from C headers
    ensure_dir("src/python/structs/_generated")
    run("3/9", [
        "python3", "src/python/tools/generate_structs.py",
        "--headers",
        "include/antnet_network_types.h",
        "include/antnet_config_types.h",
        "include/antnet_brute_force_types.h",
        "include/backend.h",
        "--output", "src/python/structs/_generated/auto_structs.py"
    ])

    # -------------------------------------------------------------------------
    # 4/9 Preprocess aggregator header for pycparser
    fake_libc = subprocess.check_output([
        "python3", "-c",
        "import pycparser, os, pathlib; "
        "print(pathlib.Path(os.path.dirname(pycparser.__file__), "
        "'utils', 'fake_libc_include').resolve())"
    ], text=True).strip()

    ensure_dir("build/preprocessed")
    for file in Path("build/preprocessed").glob("*.i"):
        file.unlink()

    cpp_flags = [
        "-nostdinc",
        "-D__attribute__(...)=",
        "-D__attribute__=",
        "-D__extension__=",
        "-D__inline__=",
        "-D__volatile__=",
        "-D__asm__=",
        "-D__restrict=",
        "-D__restrict__=",
        "-D__builtin_va_list=int",
        "-D__GNUC__=4",
        "-Dbool=_Bool",
        "-Dtrue=1",
        "-Dfalse=0"
    ]
    cpp_flag_tokens = [f"--cpp-flag={flag}" for flag in cpp_flags]

    run("4/9", [
        "python3", "src/python/ffi/preprocess_headers.py",
        "--headers", "include/cffi_entrypoint.h",
        "--outdir", "build/preprocessed",
        "--include", "./stub_headers",      # fake stdbool.h lives here
        "--include", "./include",
        "--include", fake_libc,
        *cpp_flag_tokens
    ])

    # -------------------------------------------------------------------------
    # 5/9 Generate CFFI CDEF_SOURCE with pycparser
    run("5/9", [
        "python3", "src/python/ffi/generate_cffi_defs.py",
        "--preprocessed", "build/preprocessed",
        "--output", "src/python/ffi/cdef_string.py"
    ])

    # -------------------------------------------------------------------------
    # 6/9 Build Python CFFI bindings
    if Path("venv/bin/activate").exists():
        os.environ["PATH"] = f"{repo_root/'venv'/'bin'}:{os.environ['PATH']}"
    run("6/9", ["python3", "-m", "src.python.ffi.ffi_build"])

    # -------------------------------------------------------------------------
    # 7/9 Copy compiled module to package directory
    ensure_dir("src/python/ffi")
    for so in Path("build/python").glob("backend_cffi*.so"):
        destination = Path("src/python/ffi") / so.name
        destination.write_bytes(so.read_bytes())

    # -------------------------------------------------------------------------
    # 8/9 Run tests with pytest
    run("8/9", ["python3", "-m", "pytest", "-s", "tests/"])

    # -------------------------------------------------------------------------
    # 9/9 Done
    print("[9/9] Build and tests completed successfully.")

if __name__ == "__main__":
    main()
