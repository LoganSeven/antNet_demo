# src/build.py
import os, subprocess
from pathlib import Path

def run(tag, cmd): print(f"[{tag}] {' '.join(cmd)}"); subprocess.run(cmd, check=True)
def ensure_dir(p): Path(p).mkdir(parents=True, exist_ok=True)

def main():
    root = Path(__file__).resolve().parent; os.chdir(root)
    run("1/9", ["cmake","-S",".","-B","build","-G","Unix Makefiles","-DCMAKE_BUILD_TYPE=Debug"])
    run("1/9", ["cmake","--build","build"])
    ensure_dir("src/python/consts/_generated")
    run("2/9", ["python3","src/python/tools/errors_const_c2python.py","include/error_codes.h","src/python/consts/_generated"])
    ensure_dir("src/python/structs/_generated")
    run("3/9", ["python3","src/python/tools/generate_structs.py","--headers",
        "include/antnet_network_types.h","include/antnet_config_types.h",
        "include/antnet_brute_force_types.h","include/backend.h",
        "--output","src/python/structs/_generated/auto_structs.py"])
    fake_libc = subprocess.check_output(
        ["python3","-c","import pycparser,os;print(os.path.join(os.path.dirname(pycparser.__file__),'utils','fake_libc_include'))"],
        text=True).strip()
    ensure_dir("build/preprocessed"); [p.unlink() for p in Path("build/preprocessed").glob("*.i")]
    flags = [
        "-nostdinc","-D__attribute__(...)= ","-D__attribute__=","-D__extension__=","-D__inline__=",
        "-D__volatile__=","-D__asm__=","-D__restrict=","-D__restrict__=","-D__builtin_va_list=int","-D__GNUC__=4"
    ]
    run("4/9", ["python3","src/python/ffi/preprocess_headers.py","--headers","include/cffi_entrypoint.h",
        "--outdir","build/preprocessed","--include","./include","--include",fake_libc,
        "--cpp-flag",*flags])
    run("5/9", ["python3","src/python/ffi/generate_cffi_defs.py",
        "--preprocessed","build/preprocessed","--output","src/python/ffi/cdef_string.py"])
    venv_bin = Path("venv/bin");  os.environ["PATH"]=f"{venv_bin}:{os.environ['PATH']}" if venv_bin.exists() else os.environ["PATH"]
    run("6/9", ["python3","-m","src.python.ffi.ffi_build"])
    ensure_dir("src/python/ffi")
    for so in Path("build/python").glob("backend_cffi*.so"): (Path("src/python/ffi")/so.name).write_bytes(so.read_bytes())
    run("8/9", ["python3","-m","pytest","-s","tests/"])
    print("[9/9] Build and tests completed successfully.")
if __name__=="__main__": main()
