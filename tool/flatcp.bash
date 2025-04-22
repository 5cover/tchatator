#!/usr/bin/env bash

set -euo pipefail

if (($# < 2)); then
    echo "Usage: $0 <file_or_dir>... <output_dir>" >&2
    exit 1
fi

output_dir="${!#}"  # Last argument
inputs=("${@:1:$#-1}") # All but the last argument

mkdir -p "$output_dir"

for input in "${inputs[@]}"; do
    if [[ -f "$input" ]]; then
        rel_path=$(basename "$input")
        flat_name="${input//\//\\}"
        cp "$input" "$output_dir/$flat_name"
    elif [[ -d "$input" ]]; then
        find "$input" -type f | while read -r file; do
            rel_path="${file#"$input"/}"
            flat_name="${input//\//\\}\\${rel_path//\//\\}"
            cp "$file" "$output_dir/$flat_name"
        done
    else
        echo "Warning: $input is not a valid file or directory, skipping." >&2
    fi
done
