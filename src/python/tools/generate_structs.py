# Relative Path: src/python/tools/generate_structs.py
import re
import argparse
import os
from collections import OrderedDict

# ------------------------------------------------------------------------ regex patterns
STRUCT_PATTERN = re.compile(
    r"typedef\s+struct(?:\s+\w+)?\s*\{\s*(.*?)\}\s*(\w+)\s*;",
    re.DOTALL
)

FIELD_PATTERN = re.compile(
    r"^\s*([A-Za-z_][A-Za-z0-9_]*)(\s*\*+)?\s+([A-Za-z_][A-Za-z0-9_]*)(\s*\[.*?\])?\s*;",
    re.MULTILINE
)

TYPE_MAP = {
    "int": "int",
    "float": "float",
    "double": "float",
    "_Bool": "bool",
    "bool": "bool",
    "char": "str",  # assume char* = str for now
}

LIST_TYPES = {"int", "float", "double", "char"}

# ------------------------------------------------------------------------ parse logic
def parse_structs_from_file(filepath):
    """
    Parses one header file and returns a list of (struct_name, [(field_name, py_type)], path).
    """
    with open(filepath, "r", encoding="utf-8") as f:
        content = f.read()

    results = []
    for match in STRUCT_PATTERN.finditer(content):
        struct_body = match.group(1)
        struct_name = match.group(2)

        fields = []
        for fmatch in FIELD_PATTERN.finditer(struct_body):
            ctype = fmatch.group(1).strip()
            pointer = fmatch.group(2) or ""
            name = fmatch.group(3)
            array_suffix = fmatch.group(4) or ""

            if pointer.strip() or array_suffix:
                base = TYPE_MAP.get(ctype, "Any")
                pytype = f"List[{base}]" if base in LIST_TYPES else "Any"
            else:
                pytype = TYPE_MAP.get(ctype, "Any")

            fields.append((name, pytype))

        results.append((struct_name, fields, filepath))
    return results


def generate_typed_dict_code(structs_with_paths):
    """
    Given a list of (struct_name, [(field_name, py_type)], filepath),
    emit typed Python code in a string.
    Removes duplicate definitions by struct name.
    """
    lines = []
    lines.append("from typing import TypedDict, List, Any\n")
    lines.append("# This file is auto-generated. Do not edit.\n")

    seen_structs = OrderedDict()  # struct_name → (fields, path)

    for struct_name, fields, path in structs_with_paths:
        if struct_name in seen_structs:
            continue
        seen_structs[struct_name] = (fields, path)

    for struct_name, (fields, path) in seen_structs.items():
        lines.append(f"\n# from {path}")
        lines.append(f"class {struct_name}(TypedDict):")
        if not fields:
            lines.append("    pass")
        else:
            for (fname, ftype) in fields:
                lines.append(f"    {fname}: {ftype}")

    lines.append("")
    return "\n".join(lines)


# ------------------------------------------------------------------------ main
def main():
    parser = argparse.ArgumentParser(description="Generate TypedDict from C structs.")
    parser.add_argument("--headers", nargs="+", required=True, help="List of .h files to parse.")
    parser.add_argument("--output", required=True, help="Output .py file path.")
    args = parser.parse_args()

    all_structs = []
    for header in args.headers:
        if not os.path.exists(header):
            print(f"Warning: header not found: {header}")
            continue
        structs = parse_structs_from_file(header)
        all_structs.extend(structs)

    code = generate_typed_dict_code(all_structs)
    os.makedirs(os.path.dirname(args.output), exist_ok=True)

    with open(args.output, "w", encoding="utf-8") as f:
        f.write(code)

    print(f"✅ Generated {len(all_structs)} struct(s) (deduplicated: {len(set(s[0] for s in all_structs))}) into {args.output}")


if __name__ == "__main__":
    main()
