#!/usr/bin/env python3
# src/python/ffi/generate_cffi_defs.py
"""
Generates "src/python/ffi/cdef_string.py" from pre-processed ".i" files.

Strategy
--------
• If a typedef embeds the full body of a struct/union/enum, emit:
    typedef struct { … } Alias;
  and mark the tag as handled.
• Otherwise (if no such typedef), emit a normal:
    struct tag { … };
  line.
• Ignore forward declarations.
• Ensure uniqueness of function prototypes.
• Exclude any function that does not begin with "pub_".
"""

from __future__ import annotations
import argparse
from pathlib import Path
from typing import List, Set

from pycparser import c_ast, c_generator, parse_file


class CDefExtractor(c_ast.NodeVisitor):
    """
    Visits AST nodes to extract:
      - typedef declarations
      - struct bodies
      - public function prototypes (pub_* only)
    """
    def __init__(self) -> None:
        super().__init__()
        self.gen = c_generator.CGenerator()
        self.emitted_tags: Set[str] = set()
        self.lines: List[str] = []
        self.funcs: List[str] = []

    # -------------------------------------------------------- typedef handler
    def visit_Typedef(self, node: c_ast.Typedef) -> None:
        """
        Checks if a typedef embeds a struct/union/enum with a body.
        Emits an anonymous-body typedef if present,
        or a simple typedef if body is elsewhere.
        """
        if isinstance(node.type, c_ast.TypeDecl):
            base = node.type.type
            if isinstance(base, (c_ast.Struct, c_ast.Union, c_ast.Enum)):
                tag = base.name or ""
                if base.decls:  # a body is present
                    if tag in self.emitted_tags:
                        return
                    body = "\n".join(
                        f"    {self.gen.visit(d)};" for d in base.decls
                    )
                    self.lines.append(
                        f"typedef struct {{\n{body}\n}} {node.name};"
                    )
                    if tag:
                        self.emitted_tags.add(tag)
                    # Do not descend further
                    return
                else:
                    # typedef to a tag only (no body)
                    if tag and tag not in self.emitted_tags:
                        typedef_line = self.gen.visit(node) + ";"
                        self.lines.append(typedef_line)
                    return
        self.generic_visit(node)

    # ----------------------------------------------------------- struct body
    def visit_Struct(self, node: c_ast.Struct) -> None:
        """
        Emits a struct with its body unless it has already been emitted
        via a typedef. Skips forward declarations.
        """
        tag = node.name or ""
        if not node.decls:  # forward declaration -> ignore
            return
        if tag in self.emitted_tags:
            return
        body = "\n".join(
            f"    {self.gen.visit(d)};" for d in node.decls
        )
        self.lines.append(f"struct {tag} {{\n{body}\n}};")
        if tag:
            self.emitted_tags.add(tag)
        # Visit nested declarations
        for decl in node.decls:
            self.visit(decl)

    # ------------------------------------------------------------ functions
    def visit_Decl(self, node: c_ast.Decl) -> None:
        """
        Extracts function prototypes. Appends them if they begin with "pub_"
        and have not been added before.
        """
        if isinstance(node.type, c_ast.FuncDecl):
            # Check for "pub_" prefix before emitting
            if node.name and node.name.startswith("pub_"):
                proto = self.gen.visit(node) + ";"
                if proto not in self.funcs:
                    self.funcs.append(proto)
        self.generic_visit(node)


def main() -> None:
    parser = argparse.ArgumentParser(description="Generates CFFI definitions from preprocessed .i files.")
    parser.add_argument("--preprocessed", required=True, help="Path to folder containing preprocessed .i files.")
    parser.add_argument("--output", required=True, help="Path to output the generated cdef_string.py file.")
    ns = parser.parse_args()

    extractor = CDefExtractor()

    cpp_args = [
        "-D__attribute__(...)=", "-D__attribute__=",
        "-D__extension__=", "-D__inline__=",
        "-D__volatile__=", "-D__asm__=",
        "-D__restrict=", "-D__restrict__=",
        "-D__builtin_va_list=int", "-D__GNUC__=4",
        "-Dbool=_Bool", "-Dtrue=1", "-Dfalse=0",
    ]

    # Parse all *.i files in the preprocessed directory
    for i_file in sorted(Path(ns.preprocessed).glob("*.i")):
        ast = parse_file(str(i_file), use_cpp=True, cpp_args=cpp_args)
        extractor.visit(ast)

    # Combine typedefs/structs (lines) and function prototypes (funcs)
    chunks: List[str] = []
    if extractor.lines:
        chunks.append("\n".join(extractor.lines))
    if extractor.funcs:
        if chunks:
            chunks.append("")  # separate lines from funcs
        chunks.append("\n".join(extractor.funcs))

    # Ensure output directory exists
    (Path(ns.output).parent).mkdir(parents=True, exist_ok=True)

    # Write the combined result
    Path(ns.output).write_text(
        "# Auto-generated by generate_cffi_defs.py – DO NOT EDIT\n"
        "CDEF_SOURCE = '''\\\n"
        + "\n\n".join(chunks).rstrip()
        + "\n'''",
        encoding="utf-8",
    )

    print(
        f"Wrote {len(extractor.lines)} struct/typedef lines and "
        f"{len(extractor.funcs)} function(s) to {ns.output}"
    )


if __name__ == "__main__":
    main()
