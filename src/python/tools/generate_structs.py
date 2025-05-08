"""
generate_structs.py
Auto-generates Python TypedDict definitions from designated C headers.

Usage:
  python generate_structs.py --headers <header1> <header2> ... --output <outfile>

Example:
  python generate_structs.py \
    --headers ../../../include/antnet_network_types.h ../../../include/antnet_config_types.h \
    --output src/python/structs/_generated/auto_structs.py

This script searches for patterns like:
    typedef struct {
        <type> <fieldname>;
        ...
    } <StructName>;

It then creates a Python TypedDict for each struct.
"""

import re
import argparse
import os

# ------------------------------------------------------------------------ regex patterns
STRUCT_PATTERN = re.compile(
    r"typedef\s+struct\s*\{\s*(.*?)\}\s*(\w+)\s*;",  # group(1) = struct body, group(2) = struct name
    re.DOTALL
)

FIELD_PATTERN = re.compile(
    r"^\s*([A-Za-z_][A-Za-z0-9_]*)(\s*\*+)?\s+([A-Za-z_][A-Za-z0-9_]*)\s*\[*.*\]*\s*;",  # "<type> ... <name>;"
    re.MULTILINE
)

TYPE_MAP = {
    "int": "int",
    "float": "float",
    "double": "float",
    "_Bool": "bool",
    "bool": "bool",
}

# ------------------------------------------------------------------------ parse logic
def parse_structs_from_file(filepath):
    """
    Reads the given .h file, finds all 'typedef struct {...} Name;'
    Returns a list of (struct_name, [ (field_type, field_name), ... ]).
    """
    with open(filepath, "r", encoding="utf-8") as f:
        content = f.read()

    results = []
    for match in STRUCT_PATTERN.finditer(content):
        struct_body = match.group(1)
        struct_name = match.group(2)

        fields = []
        for fmatch in FIELD_PATTERN.finditer(struct_body):
            ctype = fmatch.group(1)
            pointer_part = fmatch.group(2)
            field_name = fmatch.group(3)

            ctype = ctype.strip()

            # If pointer or array, treat the field as 'Any'.
            # In practice you might choose a more specific Python type.
            if pointer_part or "[]" in field_name:
                pytype = "Any"
            else:
                pytype = TYPE_MAP.get(ctype, "Any")

            fields.append((field_name, pytype))

        results.append((struct_name, fields))

    return results


def generate_typed_dict_code(structs):
    """
    Given a list of (struct_name, [(field_name, py_type), ...]),
    emit typed Python code in a string.
    """
    lines = []
    lines.append('from typing import TypedDict, Any\n')
    lines.append('# This file is auto-generated. Do not edit.\n')

    for struct_name, fields in structs:
        lines.append(f"\nclass {struct_name}(TypedDict):")
        if not fields:
            lines.append("    pass")
        else:
            for (fname, ftype) in fields:
                lines.append(f"    {fname}: {ftype}")

    lines.append("")  # final newline
    return "\n".join(lines)

# ------------------------------------------------------------------------ main
def main():
    parser = argparse.ArgumentParser(description="Generate TypedDict from C structs.")
    parser.add_argument("--headers", nargs="+", required=True,
                        help="Path(s) to the .h files to parse.")
    parser.add_argument("--output", required=True,
                        help="Output .py file path.")
    args = parser.parse_args()

    all_structs = []
    for header in args.headers:
        if not os.path.exists(header):
            print(f"Warning: header file not found: {header}")
            continue
        structs = parse_structs_from_file(header)
        all_structs.extend(structs)

    code = generate_typed_dict_code(all_structs)

    out_path = args.output
    os.makedirs(os.path.dirname(out_path), exist_ok=True)

    with open(out_path, "w", encoding="utf-8") as out_f:
        out_f.write(code)

    print(f"Generated {len(all_structs)} struct TypedDict(s) into {out_path}")


if __name__ == "__main__":
    main()