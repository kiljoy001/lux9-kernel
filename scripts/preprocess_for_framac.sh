#!/bin/bash
# Preprocess Plan 9 C files for Frama-C
# Removes Plan 9-specific pragmas that Frama-C can't parse

set -euo pipefail

INPUT_FILE="$1"
OUTPUT_FILE="$2"

# Preprocess with gcc to expand includes and macros
gcc -D__FRAMAC__ -E -C -P \
    -Ikernel/include \
    -Ikernel/9front-pc64 \
    -Ikernel/9front-port \
    -I. \
    "$INPUT_FILE" \
| sed -e 's/µs/us/g' \
      -e '/µs/d' \
      -e '/#pragma varargck/d' \
      -e '/#pragma lib/d' \
      -e '/#pragma src/d' \
      -e '/#pragma pack/d' \
      -e '/#pragma incomplete/d' \
      -e '/^$/d' \
> "$OUTPUT_FILE"

echo "Preprocessed $INPUT_FILE -> $OUTPUT_FILE"
