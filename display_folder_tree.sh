#!/bin/bash

# GitHub-compatible folder tree display with vertical bars (│) and proper exclusions.
# Recursively removes __pycache__ and other known transient/irrelevant folders.

EXCLUDE_PATTERNS=(
    ".cache"
    ".pytest_cache"
    ".vscode"
    "sdl_lib_src"
    "SDL"
    "build/.cmake"
    "build/CMakeCache.txt"
    "build/cmake_install.cmake"
    "build/Makefile"
    "build/CMakeFiles"
    "build/python"
    "Documentation"
    "venv"
    ".git"
    "__pycache__"
)

function is_excluded {
    local path="$1"
    for pattern in "${EXCLUDE_PATTERNS[@]}"; do
        if [[ "$path" == *"/$pattern" || "$path" == *"/$pattern/"* || "$(basename "$path")" == "$pattern" ]]; then
            return 0
        fi
    done
    return 1
}

function display_tree {
    local current_dir="$1"
    local prefix="$2"
    local is_last="$3"

    local base=$(basename "$current_dir")
    local connector="├──"
    [[ "$is_last" == "1" ]] && connector="└──"
    echo "${prefix}${connector} $base/"

    local entries=()
    while IFS= read -r -d '' entry; do
        is_excluded "$entry" && continue
        entries+=("$entry")
    done < <(find "$current_dir" -mindepth 1 -maxdepth 1 -print0 | sort -z)

    local total=${#entries[@]}
    for i in "${!entries[@]}"; do
        local entry="${entries[$i]}"
        local name=$(basename "$entry")
        local last=0
        [[ "$i" -eq $((total - 1)) ]] && last=1

        local new_prefix="$prefix"
        [[ "$is_last" == "1" ]] && new_prefix+="    " || new_prefix+="│   "

        if [[ -d "$entry" ]]; then
            display_tree "$entry" "$new_prefix" "$last"
        else
            local file_connector="├──"
            [[ "$last" == "1" ]] && file_connector="└──"
            echo "${new_prefix}${file_connector} $name"
        fi
    done
}

# Entry point
clear
echo '```text'
display_tree "." "" 1
echo '```'
