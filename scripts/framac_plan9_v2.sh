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
    -D__PLAN9_KERNEL__ \
    -D_PLAN9_SOURCE \
    -DKERNEL \
    -Ikernel/include \
    -Ikernel/9front-pc64 \
    -Ikernel/9front-port \
    -Ikernel/crypto \
    -Ikernel/wasm \
    -Ikernel/wasm/wasm_runtime/wasm3 \
    -I. \
    -E -P -C \
    "$INPUT_FILE" \
> "$GCC_OUTPUT" 2> "$GCC_ERR" || true

cat "$GCC_OUTPUT" | \
# Step 3: Filter out Plan 9-specific constructs that Frama-C can't parse
sed \
    -e 's/µs/us/g' \
    -e '/µs/d' \
    -e 's/@\*\//\*\//g' \
    -e '/#pragma varargck/d' \
    -e '/#pragma lib/d' \
    -e '/#pragma src/d' \
    -e '/#pragma incomplete/d' \
    -e '/#pragma pack/d' \
    -e '/#pragma textflag/d' \
    -e '/#pragma profile/d' \
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

# Step 6: Build minimal header with ONLY truly missing types
cat > "$OUTPUT_FILE" << 'HEADER_START'
/* Frama-C Missing Types - types excluded by #ifndef __FRAMAC__ in Plan 9 headers */
HEADER_START

# Conditionally add uuid_t only if missing.
if [ "$HAS_UUID" -eq 0 ]; then
    cat >> "$OUTPUT_FILE" << 'UUID_DEF'

/* UUID type - not in Plan 9 headers */
typedef unsigned char uuid_t[16];
UUID_DEF
fi

if [ "$HAS_FMT" -eq 0 ]; then
    cat >> "$OUTPUT_FILE" << 'FMT_DEF'

/* Types excluded by #ifndef __FRAMAC__ in portlib.h */
typedef struct Fmt Fmt;
typedef int (*Fmts)(Fmt *);
struct Fmt {
    unsigned char runes;
    void *start;
    void *to;
    void *stop;
    int (*flush)(Fmt *);
    void *farg;
    int nfmt;
    __builtin_va_list args;
    int r;
    int width;
    int prec;
    unsigned long flags;
};
FMT_DEF
fi

if [ "$HAS_QID" -eq 0 ]; then
    cat >> "$OUTPUT_FILE" << 'QID_DEF'

typedef struct Qid Qid;
struct Qid {
    unsigned long long path;
    unsigned long vers;
    unsigned char type;
};
QID_DEF
fi

if [ "$HAS_DIR" -eq 0 ]; then
    cat >> "$OUTPUT_FILE" << 'DIR_DEF'

typedef struct Dir Dir;
struct Dir {
    unsigned short type;
    unsigned int dev;
    Qid qid;
    unsigned long mode;
    unsigned long atime;
    unsigned long mtime;
    long long length;
    char *name;
    char *uid;
    char *gid;
    char *muid;
};
DIR_DEF
fi

if [ "$HAS_WAITMSG" -eq 0 ]; then
    cat >> "$OUTPUT_FILE" << 'WAITMSG_DEF'

typedef struct Waitmsg Waitmsg;
struct Waitmsg {
    int pid;
    unsigned long time[3];
    char msg[128]; /* ERRMAX */
};
WAITMSG_DEF
fi

# Note: Dirtab is NOT excluded by __FRAMAC__ in Plan 9 headers, so do not redefine it

# Append the preprocessed Plan 9 code
cat "$TEMP_OUTPUT" >> "$OUTPUT_FILE"

# Final cleanup: normalize ACSL comment terminators.
sed -i 's/@\*\//\*\//g' "$OUTPUT_FILE"
rm -f "$TEMP_OUTPUT" "$GCC_OUTPUT" "$GCC_ERR"

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
echo "    -no-cpp-frama-c-compliant \\"
echo "    -cpp-command 'cat' \\"
echo "    $OUTPUT_FILE"
