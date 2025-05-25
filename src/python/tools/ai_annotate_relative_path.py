# Relative Path: src/python/tools/ai_annotate_relative_path.py
"""
Annotate every *.py file under src/python/** with a first-line comment:

    # Relative Path: path/from/root

Files or folders matched by patterns collected from:
    • .gitignore
    • display_folder_tree.sh   (EXCLUDE_PATTERNS array)

are skipped.

Run from any location inside the repository:
    python tools/ai_annotate_relative_path_python.py
"""

from __future__ import annotations

import fnmatch
import re
import sys
from pathlib import Path
from typing import Iterable, Set

# --------------------------------------------------------------------------- #
# Repository roots
# --------------------------------------------------------------------------- #
THIS_FILE = Path(__file__).resolve()
REPO_ROOT: Path = THIS_FILE.parents[3]          # <repo>/<tools>/ai_annotate...
PY_ROOT:   Path = REPO_ROOT / "src" / "python"

# --------------------------------------------------------------------------- #
# Pattern collection helpers
# --------------------------------------------------------------------------- #
def load_gitignore_patterns(path: Path) -> Set[str]:
    """Collect non-comment, non-empty patterns from .gitignore."""
    patterns: Set[str] = set()
    if path.exists():
        for line in path.read_text(encoding="utf-8").splitlines():
            stripped = line.strip()
            if stripped and not stripped.startswith("#"):
                patterns.add(stripped)
    return patterns


def load_display_tree_patterns(path: Path) -> Set[str]:
    """Extract values from the EXCLUDE_PATTERNS bash array."""
    patterns: Set[str] = set()
    if not path.exists():
        return patterns

    content = path.read_text(encoding="utf-8")
    match = re.search(r"EXCLUDE_PATTERNS=\(([\s\S]*?)\)", content)
    if not match:
        return patterns

    for pat in re.findall(r'"([^"]+)"', match.group(1)):
        patterns.add(pat.strip())
    return patterns


def build_ignore_set() -> Set[str]:
    """Return a unified, deduplicated set of ignore patterns."""
    git_patterns  = load_gitignore_patterns(REPO_ROOT / ".gitignore")
    bash_patterns = load_display_tree_patterns(REPO_ROOT / "display_folder_tree.sh")
    return git_patterns | bash_patterns


IGNORE_PATTERNS: Set[str] = build_ignore_set()

# --------------------------------------------------------------------------- #
# Ignore-matching logic
# --------------------------------------------------------------------------- #
def matches_pattern(rel_posix: str, pattern: str) -> bool:
    """
    Minimal fnmatch-based approximation of .gitignore semantics.

    Rules honoured:
        * Leading '/'   → pattern relative to repo root.
        * Trailing '/'  → directory name match.
        * Wildcards     → fnmatch.
    """
    pat = pattern.lstrip("/").rstrip("/")

    # Exact directory exclude
    if pattern.endswith("/"):
        return pat in rel_posix.split("/")

    return fnmatch.fnmatch(rel_posix, pat)


def should_ignore(path: Path) -> bool:
    rel_posix = path.relative_to(REPO_ROOT).as_posix()
    return any(matches_pattern(rel_posix, pat) for pat in IGNORE_PATTERNS)

# --------------------------------------------------------------------------- #
# Annotation logic
# --------------------------------------------------------------------------- #
def annotate_file(file_path: Path) -> None:
    rel_path_posix = file_path.relative_to(REPO_ROOT).as_posix()
    header = f"# Relative Path: {rel_path_posix}"

    lines = file_path.read_text(encoding="utf-8").splitlines()

    if not lines:
        file_path.write_text(header + "\n", encoding="utf-8")
        print(f"➕ [Inserted] {rel_path_posix}")
        return

    first = lines[0].strip()

    if not first.startswith("# Relative Path:"):
        file_path.write_text("\n".join([header, ""] + lines) + "\n", encoding="utf-8")
        print(f"✨ [Added]    {rel_path_posix}")
    else:
        current = first[len("# Relative Path:"):].strip()
        if current != rel_path_posix:
            lines[0] = header
            file_path.write_text("\n".join(lines) + "\n", encoding="utf-8")
            print(f"♻️  [Updated]  {rel_path_posix}")
        else:
            print(f"✅ [OK]       {rel_path_posix}")

# --------------------------------------------------------------------------- #
# Traversal
# --------------------------------------------------------------------------- #
def collect_python_files(base: Path) -> Iterable[Path]:
    for py_file in base.rglob("*.py"):        # ← *** only *.py files ***
        if not should_ignore(py_file):
            yield py_file

# --------------------------------------------------------------------------- #
# Entry point
# --------------------------------------------------------------------------- #
def main() -> None:
    if not PY_ROOT.exists():
        print(f"src/python not found under {REPO_ROOT}", file=sys.stderr)
        sys.exit(1)

    for py in collect_python_files(PY_ROOT):
        annotate_file(py)


if __name__ == "__main__":
    main()
