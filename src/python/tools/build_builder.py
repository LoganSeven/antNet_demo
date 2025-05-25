# Relative Path: src/python/tools/build_builder.py

import os
import argparse
from pathlib import Path


def walk_for_files(root_dir, extensions, excluded_folders):
    root = Path(root_dir).resolve()
    for dirpath, dirnames, filenames in os.walk(root):
        dirnames[:] = [d for d in dirnames if d not in excluded_folders]
        for f in filenames:
            if Path(f).suffix.lower() in extensions:
                yield str(Path(dirpath, f).resolve())


def generate_flat_include_block(header_root: Path, excluded_folders: set[str]) -> list[str]:
    """
    Returns a list of #include "relative/path/to/header.h" for all headers found recursively.
    Ensures backend.h is included first (even from a subfolder).
    """
    headers = set()

    for header_path in walk_for_files(header_root, extensions={".h"}, excluded_folders=excluded_folders):
        if "pthread.h" in header_path:
            continue
        relative_path = Path(header_path).relative_to(header_root)
        headers.add(str(relative_path).replace("\\", "/"))  # normalize Windows paths

    headers = sorted(headers)

    # Promote backend.h if found anywhere
    backend_entry = next((h for h in headers if Path(h).name == "backend.h"), None)
    if backend_entry:
        headers.remove(backend_entry)
        headers.insert(0, backend_entry)

    return [f'#include "{h}"' for h in headers]


def generate_ffi_build_py(
    c_files, header_includes, src_c_dir, include_dir, third_party_dir, module_name="backend_cffi"
):
    indent = " " * 8
    sources_block = ",\n".join(
        f'{indent}os.path.join(src_c_dir, "{Path(src).relative_to(src_c_dir)}")'
        for src in c_files
    )
    include_block = "\n".join(header_includes)

    return f'''import os
from cffi import FFI
from src.python.ffi.cdef_string import CDEF_SOURCE  # auto-generated structs + declarations

ffi = FFI()
ffi.cdef(CDEF_SOURCE)

# ---------- Build instructions ----------
this_dir    = os.path.dirname(__file__)
src_c_dir   = os.path.abspath(os.path.join(this_dir, "../../c"))
include_dir = os.path.abspath(os.path.join(this_dir, "../../../include"))
ini_c       = os.path.join(this_dir, "../../../third_party/ini.c")

ffi.set_source(
    "{module_name}",
    f"""{include_block}
""",
    sources=[
{sources_block},
        ini_c
    ],
    include_dirs=[
        include_dir,
        os.path.join(this_dir, "../../../third_party")
    ],
    libraries=["EGL", "GLESv2"],
)

if __name__ == "__main__":
    build_dir = os.path.abspath(os.path.join(this_dir, "../../../build/python"))
    os.makedirs(build_dir, exist_ok=True)
    ffi.compile(verbose=True, tmpdir=build_dir)
'''


def main():
    parser = argparse.ArgumentParser(description="Generate ffi_build.py based on scanned project layout.")
    parser.add_argument("--src-c-dir", default="../../c", help="Directory containing .c files")
    parser.add_argument("--include-dir", default="../../../include", help="Directory containing .h files")
    parser.add_argument("--third-party-dir", default="../../../third_party", help="Path to third-party sources")
    parser.add_argument("--exclude", default="", help="Comma-separated list of directory names to exclude")
    parser.add_argument("--output", required=True, help="Path to write generated ffi_build.py")
    parser.add_argument("--module-name", default="backend_cffi", help="CFFI module name")

    args = parser.parse_args()

    this_dir = Path(__file__).parent.resolve()
    src_c_dir = (this_dir / args.src_c_dir).resolve()
    include_dir = (this_dir / args.include_dir).resolve()
    third_party_dir = (this_dir / args.third_party_dir).resolve()
    excluded_folders = {d.strip() for d in args.exclude.split(",") if d.strip()}

    c_files = list(walk_for_files(src_c_dir, extensions={".c"}, excluded_folders=excluded_folders))
    header_includes = generate_flat_include_block(include_dir, excluded_folders)

    output_text = generate_ffi_build_py(
        c_files=c_files,
        header_includes=header_includes,
        src_c_dir=src_c_dir,
        include_dir=include_dir,
        third_party_dir=third_party_dir,
        module_name=args.module_name
    )

    output_path = Path(args.output).resolve()
    with open(output_path, "w", encoding="utf-8") as f:
        f.write(output_text)

    print(f"[✔] ffi_build.py generated at: {output_path}")


if __name__ == "__main__":
    main()
