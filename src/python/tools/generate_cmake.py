#!/usr/bin/env python3
"""
generate_cmake.py

Auto-generates a CMakeLists.txt for the AntNet project by scanning:
- include/ recursively for header paths
- src/c/ and third_party/ recursively for .c sources

Usage:
    python tools/generate_cmake.py --dry-run
    python tools/generate_cmake.py --write
"""

import os
import argparse
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parents[3]
SRC_DIR = REPO_ROOT / "src" / "c"
INCLUDE_DIR = REPO_ROOT / "include"
THIRD_PARTY_DIR = REPO_ROOT / "third_party"
CMAKE_PATH = REPO_ROOT / "CMakeLists.txt"

HEADER_EXCLUDES = {".git", "__pycache__"}
SOURCE_EXT = {".c"}


def collect_include_dirs(base: Path) -> list[str]:
    includes = set()
    for path in base.rglob("*"):
        if path.is_dir() and not any(part in HEADER_EXCLUDES for part in path.parts):
            rel_path = path.relative_to(REPO_ROOT)
            includes.add(str(rel_path))
    return sorted(includes)


def collect_c_sources(*roots: Path) -> list[str]:
    sources = []
    for root in roots:
        for file in root.rglob("*.c"):
            rel_path = file.relative_to(REPO_ROOT)
            sources.append(str(rel_path))
    return sorted(sources)


def generate_cmake(includes: list[str], sources: list[str]) -> str:
    includes_block = "\n".join(f"    ${{CMAKE_SOURCE_DIR}}/{inc}" for inc in includes)
    sources_block = "\n".join(f"    {src}" for src in sources)

    return f"""cmake_minimum_required(VERSION 3.10)

# ------------------ Project setup ---------------------------------
project(AntNetBackend VERSION 0.1.0 LANGUAGES C)
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Build type (Debug or Release)" FORCE)
endif()

message(STATUS "⛏️  Build type: ${{CMAKE_BUILD_TYPE}}")
message(STATUS "💻 Target platform: ${{CMAKE_SYSTEM_NAME}}")

# ------------------ Compiler warnings/options -----------------------------
if(MSVC)
    add_compile_options(/W4)
else()
    add_compile_options(-Wall -Wextra -pedantic)
endif()

# ------------------ Include directories ---------------------------
include_directories(
{includes_block}
)

# ------------------ Source files ----------------------------------
set(SOURCE_FILES
{sources_block}
)

# ------------------ OpenGL ES / EGL -------------------------------
find_library(EGL_LIB EGL REQUIRED)
find_library(GLESv2_LIB GLESv2 REQUIRED)

# ------------------ Build shared library --------------------------
add_library(antnet_backend SHARED ${{SOURCE_FILES}})

# ------------------ Linking ---------------------------------------
if(UNIX AND NOT APPLE)
    find_package(Threads REQUIRED)
    target_link_libraries(antnet_backend
        PRIVATE
            m
            Threads::Threads
            ${{EGL_LIB}}
            ${{GLESv2_LIB}}
    )
endif()

# ------------------ Install targets -------------------------------
install(TARGETS antnet_backend
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        RUNTIME DESTINATION bin)

install(DIRECTORY include/ DESTINATION include)

# ------------------ Final Summary -----------------------------------
message(STATUS "✅ AntNet backend build setup complete.")
message(STATUS "📦 Output: shared library 'antnet_backend' for CFFI loading.")
"""


def main():
    parser = argparse.ArgumentParser(description="Generate a CMakeLists.txt file dynamically.")
    parser.add_argument("--write", action="store_true", help="Write to CMakeLists.txt")
    parser.add_argument("--dry-run", action="store_true", help="Only preview the generated content")
    args = parser.parse_args()

    includes = collect_include_dirs(INCLUDE_DIR)
    sources = collect_c_sources(SRC_DIR, THIRD_PARTY_DIR)
    content = generate_cmake(includes, sources)

    if args.dry_run:
        print(content)
    elif args.write or not (args.dry_run or args.write):
        CMAKE_PATH.write_text(content, encoding="utf-8")
        print(f"[✔] CMakeLists.txt written to: {CMAKE_PATH}")
    else:
        print("[i] Use --dry-run to preview or --write to apply.")


if __name__ == "__main__":
    main()
