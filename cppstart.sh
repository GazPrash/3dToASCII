#!/bin/sh
set -eu

CXX="${CXX:-clang++}"
SOURCE_FILES="src/main.cpp src/renderer.cpp src/extractor.cpp"
OUTPUT_FILE="ascii_obj_renderer"

if [ "$#" -eq 0 ]; then
  printf 'Usage: %s <obj-file> [renderer-args...]\n' "$0" >&2
  exit 1
fi

case "$1" in
  -h|--help)
    printf 'Usage: %s <obj-file> [renderer-args...]\n' "$0"
    exit 0
    ;;
esac

OBJ_FILE="$1"
shift

if [ ! -f "$OBJ_FILE" ]; then
  printf 'OBJ file not found: %s\n' "$OBJ_FILE" >&2
  exit 1
fi

"$CXX" -std=c++17 -Wall -Wextra -pedantic $SOURCE_FILES -o "$OUTPUT_FILE"
exec "./$OUTPUT_FILE" "$OBJ_FILE" "$@"
