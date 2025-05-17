#!/usr/bin/env python3
"""
ai_annotate_relative_path.py

AI-friendly utility to annotate .c/.h files with their relative path.
This helps coders and AI tools align context, structure, and logic.

Scans:
- src/c/**/*.c
- include/**/*.h

Adds or updates the first line of each file to:
    /* Relative Path: path/to/file */

Usage:
    python tools/ai_annotate_relative_path.py
"""

from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parents[3]
C_ROOT = REPO_ROOT / "src" / "c"
INCLUDE_ROOT = REPO_ROOT / "include"

def get_all_target_files():
    return list(C_ROOT.rglob("*.c")) + list(INCLUDE_ROOT.rglob("*.h"))

def update_file(filepath: Path):
    rel_path = filepath.relative_to(REPO_ROOT)
    rel_str = f"/* Relative Path: {rel_path.as_posix()} */"

    lines = filepath.read_text(encoding="utf-8").splitlines()

    if not lines:
        filepath.write_text(rel_str + "\n", encoding="utf-8")
        print(f"➕ [Inserted] {rel_path}")
        return

    first_line = lines[0].strip()

    if not first_line.startswith("/* Relative Path:"):
        updated = [rel_str] + [""] + lines
        filepath.write_text("\n".join(updated) + "\n", encoding="utf-8")
        print(f"✨ [Added]    {rel_path}")
    else:
        current_path = first_line[len("/* Relative Path:"):].strip(" */")
        if current_path != rel_path.as_posix():
            lines[0] = rel_str
            filepath.write_text("\n".join(lines) + "\n", encoding="utf-8")
            print(f"♻️  [Updated]  {rel_path}")
        else:
            print(f"✅ [OK]       {rel_path}")

def main():
    files = get_all_target_files()
    for f in files:
        update_file(f)

if __name__ == "__main__":
    main()
