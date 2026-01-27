#!/bin/bash
# Advanced Plan 9 to Frama-C preprocessor
# Translates Plan 9 C code to standard C that Frama-C can understand

set -euo pipefail

if [ $# -lt 2 ]; then
    echo "Usage: $0 <input.c> <output.c>"
    exit 1
fi

INPUT_FILE="$1"
OUTPUT_FILE="$2"
TEMP_FILE="/tmp/framac_plan9_$$.c"

# Step 1: Create a wrapper that includes our compatibility layer
cat > "$TEMP_FILE" <<'EOF'
// Frama-C Plan 9 preprocessing wrapper
#define __FRAMAC__ 1

// Include compatibility layer first
#include "framac_plan9.h"

// Redefine problematic pragmas before they're encountered
#define pragma_varargck_type(...)
#define pragma_varargck_argpos(...)
#define pragma_varargck_flag(...)

EOF

# Step 2: Add the original file content with pragma filtering
cat "$INPUT_FILE" >> "$TEMP_FILE"

# Step 3: Preprocess with GCC, expanding all includes
gcc -D__FRAMAC__ \
    -Ikernel/include \
    -Ikernel/9front-pc64 \
    -Ikernel/9front-port \
    -I. \
    -E -P -C \
    "$TEMP_FILE" \
| sed \
    -e 's/µs/us/g' \
    -e '/µs/d' \
    -e '/#pragma varargck/d' \
    -e '/#pragma lib/d' \
    -e '/#pragma src/d' \
    -e '/#pragma incomplete/d' \
    -e '/#pragma pack/d' \
    -e '/^$/d' \
> "$OUTPUT_FILE"

# Step 4: Cleanup
rm -f "$TEMP_FILE"

echo "✅ Preprocessed $INPUT_FILE -> $OUTPUT_FILE (Plan 9 → Frama-C compatible)"
echo "   File size: $(wc -l < "$OUTPUT_FILE") lines"
