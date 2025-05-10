#!/usr/bin/env python3
# src/python/ffi/preprocess_headers.py
"""
Preprocess C headers for pycparser.

Supports either form:
  --cpp-flag=<flag>    (inline)
  --cpp-flag <flag>    (separate)

One flag per --cpp-flag.  No positional args required.
"""

import argparse
import os
import platform
import subprocess
from pathlib import Path

def main() -> None:
    parser = argparse.ArgumentParser(
        description="Preprocess headers so pycparser can parse them"
    )
    parser.add_argument("--headers", nargs="+", required=True,
                        help="Header files to preprocess")
    parser.add_argument("--outdir", required=True,
                        help="Directory where .i files are written")
    parser.add_argument("--include", action="append", default=[],
                        help="Extra -I paths (repeatable)")
    parser.add_argument("--cpp-flag", dest="cpp_flags",
                        action="append", metavar="FLAG",
                        help="Extra pre-processor flag (repeat once per flag)")

    args = parser.parse_args()
    Path(args.outdir).mkdir(parents=True, exist_ok=True)

    flags: list[str] = []
    for item in args.cpp_flags or []:
        if item is not None:
            flags.append(item)

    system = platform.system()
    for hdr in args.headers:
        if not os.path.isfile(hdr):
            print(f"Warning: header not found: {hdr}")
            continue

        out_i = Path(args.outdir, Path(hdr).with_suffix(".i").name)

        if system == "Windows":
            cmd = ["cl", "/EP", "/DCFFI_BUILD"]
            for inc in args.include:
                cmd.append(f"/I{inc}")
            cmd.extend(flags)
            cmd.append(hdr)
        else:
            cmd = ["cpp", "-DCFFI_BUILD"]
            for inc in args.include:
                cmd.extend(["-I", inc])
            cmd.extend(flags)
            cmd.append(hdr)

        print(f"Preprocessing {hdr} -> {out_i}\n  Command: {' '.join(cmd)}")
        with out_i.open("w", encoding="utf-8") as out_f:
            subprocess.run(cmd, check=True, stdout=out_f)

if __name__ == "__main__":
    main()
