#!/bin/bash
# Frama-C Plan 9 Preprocessor v2
# Strategy: Use Plan 9's own type definitions, only filter pragmas

set -euo pipefail

if [ $# -lt 2 ]; then
    echo "Usage: $0 <input.c> <output.c>"
    exit 1
fi

INPUT_FILE="$1"
OUTPUT_FILE="$2"

echo "📋 Preprocessing $INPUT_FILE for Frama-C..."

# Step 1: Preprocess first without any custom headers to see what Plan 9 provides
TEMP_OUTPUT="/tmp/framac_temp_$$.c"

# Step 2: Preprocess with GCC, letting Plan 9 headers define everything
gcc -D__FRAMAC__ \
    -Ikernel/include \
    -Ikernel/9front-pc64 \
    -Ikernel/9front-port \
    -I. \
    -E -P -C \
    "$INPUT_FILE" \
2>&1 | \
# Step 3: Filter out Plan 9-specific constructs that Frama-C can't parse
sed \
    -e 's/µs/us/g' \
    -e '/µs/d' \
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
HAS_FMT=$(grep -c "struct Fmt {" "$TEMP_OUTPUT" 2>/dev/null || echo 0)
HAS_QID=$(grep -c "struct Qid {" "$TEMP_OUTPUT" 2>/dev/null || echo 0)
HAS_DIRTAB=$(grep -c "struct Dirtab {" "$TEMP_OUTPUT" 2>/dev/null || echo 0)
HAS_WAITMSG=$(grep -c "struct Waitmsg {" "$TEMP_OUTPUT" 2>/dev/null || echo 0)
HAS_UUID=$(grep -E "typedef.*uuid_t" "$TEMP_OUTPUT" 2>/dev/null | wc -l)

# Step 6: Build minimal header with ONLY truly missing types
cat > "$OUTPUT_FILE" << 'HEADER_START'
/* Frama-C Missing Types - types excluded by #ifndef __FRAMAC__ in Plan 9 headers */
HEADER_START

# Conditionally add uuid_t if not already defined
# Note: We check for typedef in preprocessed output and add it at the top
# to avoid forward reference issues (e.g., when pebble.h uses it before uuid.h defines it)
if [ "$HAS_UUID" -gt 0 ]; then
    # uuid_t is defined in the file, add matching struct definition at top to avoid forward refs
    cat >> "$OUTPUT_FILE" << 'UUID_DEF'

/* UUID type - defined early to avoid forward reference issues */
typedef struct {
  unsigned char data[16];
} uuid_t;
UUID_DEF
else
    # uuid_t not defined anywhere, add array typedef for compatibility
    cat >> "$OUTPUT_FILE" << 'UUID_DEF'

/* UUID type - not in Plan 9 headers */
typedef unsigned char uuid_t[16];
UUID_DEF
fi

cat >> "$OUTPUT_FILE" << 'TYPES_DEF'

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
    void *args;
    int r;
    int width;
    int prec;
    unsigned long flags;
};

typedef struct Qid Qid;
struct Qid {
    unsigned long long path;
    unsigned long vers;
    unsigned char type;
};

typedef struct Waitmsg Waitmsg;
struct Waitmsg {
    int pid;
    unsigned long time[3];
    char msg[128]; /* ERRMAX */
};

/* Note: Dirtab is NOT excluded by __FRAMAC__ in Plan 9 headers, so don't redefine it */

TYPES_DEF

# Append the preprocessed Plan 9 code
# If we added uuid_t at the top, remove it from the middle to avoid redefinition
if [ "$HAS_UUID" -gt 0 ]; then
    # Remove the uuid_t typedef from preprocessed output to avoid redefinition
    # This pattern matches: typedef struct { ... } uuid_t;
    awk '
        /^typedef struct \{$/ { in_uuid=1; buffer=$0; next }
        in_uuid {
            buffer=buffer "\n" $0
            if (/^} uuid_t;$/) {
                in_uuid=0
                next
            }
            next
        }
        { print }
    ' "$TEMP_OUTPUT" >> "$OUTPUT_FILE"
else
    cat "$TEMP_OUTPUT" >> "$OUTPUT_FILE"
fi
rm -f "$TEMP_OUTPUT"

# Step 4: Check for any remaining problematic constructs
PRAGMA_COUNT=$(grep -c "^#pragma" "$OUTPUT_FILE" 2>/dev/null || echo "0")
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
