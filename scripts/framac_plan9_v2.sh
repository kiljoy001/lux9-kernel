#!/bin/bash
# Frama-C Plan 9 Preprocessor v2
# Strategy: Use Plan 9's own type definitions, only filter pragmas

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"

if [ $# -lt 2 ]; then
    echo "Usage: $0 <input.c> <output.c>"
    exit 1
fi

INPUT_FILE="$1"
OUTPUT_FILE="$2"

echo "📋 Preprocessing $INPUT_FILE for Frama-C..."

# Step 1: Preprocess first without any custom headers to see what Plan 9 provides
TEMP_OUTPUT="/tmp/framac_temp_$$.c"
GCC_OUTPUT="/tmp/framac_gcc_$$.c"
GCC_ERR="/tmp/framac_gcc_$$.err"

# Step 2: Preprocess with GCC, letting Plan 9 headers define everything
gcc -D__FRAMAC__ \
    -D__GCC_PREPROCESS__ \
    -D__PLAN9_KERNEL__ \
    -D_PLAN9_SOURCE \
    -DKERNEL \
    -Ikernel/include \
    -Ikernel/9front-pc64 \
    -Ikernel/9front-port \
    -Ikernel/crypto \
    -Ikernel/wasm \
    -Ikernel/wasm/wasm_runtime/wasm3 \
    -Isrc/9/port \
    -Isrc/9/pc \
    -I. \
    -E -P -C \
    "$INPUT_FILE" \
> "$GCC_OUTPUT" 2> "$GCC_ERR" || true

cat "$GCC_OUTPUT" | \
# Step 3: Filter out Plan 9-specific constructs that Frama-C can't parse
sed \
    -e 's/\\U000000b5/u/g' \
    -e 's/µs/us/g' \
    -e '/µs/d' \
    -e '/#pragma varargck/d' \
    -e '/#pragma lib/d' \
    -e '/#pragma src/d' \
    -e '/#pragma incomplete/d' \
    -e '/#pragma pack/d' \
    -e '/#pragma textflag/d' \
    -e '/#pragma profile/d' \
    -e '/^\*\/\*[[:space:]]\+Local/d' \
    -e '/^\*\/\*[[:space:]].*/d' \
    -e '/"uintptr must match pointer size");/d' \
    -e '/"ulong must match pointer size");/d' \
    -e '/"usize must match pointer size");/d' \
    -e '/"ssize must match pointer size");/d' \
    -e '/_Static_assert/d' \
    -e 's/__builtin_expect(\([^,]*\), [^)]*)/(\1)/g' \
    -e '/\/\*[[:space:]]\+clang-format/d' \
    -e 's/_Float128/long double/g' \
    -e 's/__attribute__(([a-zA-Z0-9_, ]*))//g' \
    -e 's/__attribute__((__noinline__))//g' \
    -e 's/__attribute__((noinline))//g' \
    -e 's/__attribute__((__unused__))//g' \
    -e 's/__attribute__((unused))//g' \
    -e 's/__attribute__((packed))//g' \
    -e '/^# [0-9]/d' \
| \
# Step 4: Remove empty lines for compactness
sed '/^$/d' \
> "$TEMP_OUTPUT"


# Step 5: Check what types are actually missing
HAS_FMT=$(grep -c "struct Fmt {" "$TEMP_OUTPUT" 2>/dev/null || true)
HAS_QID=$(grep -c "struct Qid {" "$TEMP_OUTPUT" 2>/dev/null || true)
HAS_DIRTAB=$(grep -c "struct Dirtab {" "$TEMP_OUTPUT" 2>/dev/null || true)
HAS_DIR=$(grep -c "struct Dir {" "$TEMP_OUTPUT" 2>/dev/null || true)
HAS_WAITMSG=$(grep -c "struct Waitmsg {" "$TEMP_OUTPUT" 2>/dev/null || true)
HAS_UUID=$(grep -E -c "typedef[[:space:]].*uuid_t|} uuid_t;" "$TEMP_OUTPUT" 2>/dev/null || true)

# Step 6: Identify missing types for detection flags
HAS_FMT=$(grep -c "struct Fmt {" "$TEMP_OUTPUT" 2>/dev/null || true)
HAS_QID=$(grep -c "struct Qid {" "$TEMP_OUTPUT" 2>/dev/null || true)
HAS_DIR=$(grep -c "struct Dir {" "$TEMP_OUTPUT" 2>/dev/null || true)
HAS_WAITMSG=$(grep -c "struct Waitmsg {" "$TEMP_OUTPUT" 2>/dev/null || true)
HAS_UUID=$(grep -E -c "typedef[[:space:]].*uuid_t|} uuid_t;" "$TEMP_OUTPUT" 2>/dev/null || true)

# Step 6.4: Add detection flags
DETECTION_FLAGS=""
if [ "$HAS_FMT" -eq 1 ]; then DETECTION_FLAGS="-DHAS_FMT"; fi
if [ "$HAS_QID" -eq 1 ]; then DETECTION_FLAGS="$DETECTION_FLAGS -DHAS_QID"; fi
if [ "$HAS_DIR" -eq 1 ]; then DETECTION_FLAGS="$DETECTION_FLAGS -DHAS_DIR"; fi
if [ "$HAS_WAITMSG" -eq 1 ]; then DETECTION_FLAGS="$DETECTION_FLAGS -DHAS_WAITMSG"; fi
if [ "$HAS_UUID" -eq 1 ]; then DETECTION_FLAGS="$DETECTION_FLAGS -DHAS_UUID"; fi

# Step 7: Final Assembly
cat > "$OUTPUT_FILE" << 'HEADER_START'
/* Frama-C Preprocessed Plan 9 Code */
HEADER_START

# Frama-C stubs are included via kernel headers (u.h) during preprocessing.

# Append the preprocessed Plan 9 code
cat "$TEMP_OUTPUT" >> "$OUTPUT_FILE"

# Final cleanup: normalize ACSL comment terminators. (Modified to preserve @*/ for Frama-C)
# sed -i 's/@\*\//\*\//g' "$OUTPUT_FILE"
# rm -f "$TEMP_OUTPUT" "$GCC_OUTPUT" "$GCC_ERR"

# Step 4: Check for any remaining problematic constructs
PRAGMA_COUNT=$(grep -c "^#pragma" "$OUTPUT_FILE" 2>/dev/null || true)
if [ "$PRAGMA_COUNT" -gt 0 ] 2>/dev/null; then
    echo "⚠️  Warning: $PRAGMA_COUNT pragmas remain in output"
    grep "^#pragma" "$OUTPUT_FILE" | head -5
fi

# Step 5: Report
LINE_COUNT=$(wc -l < "$OUTPUT_FILE")
echo "✅ Preprocessed: $LINE_COUNT lines"
echo "   Output: $OUTPUT_FILE"
echo ""
echo "To verify with Frama-C:"
echo "  frama-c -eva -machdep gcc_x86_64 \\"
echo "    $DETECTION_FLAGS \\"
echo "    -no-cpp-frama-c-compliant \\"
echo "    -cpp-command 'cat' \\"
echo "    $OUTPUT_FILE"
