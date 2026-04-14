#!/bin/bash
# Script to re-enable disabled ACSL invariants in WASM3 runtime
# This removes the "/* DISABLED: " and "*/ */" wrappers from ACSL comments

set -e

FILE="$1"

if [ -z "$FILE" ]; then
    echo "Usage: $0 <file>"
    exit 1
fi

if [ ! -f "$FILE" ]; then
    echo "Error: File $FILE does not exist"
    exit 1
fi

# Create backup
cp "$FILE" "$FILE.bak"

# Use sed to remove the DISABLED wrapper
# Pattern: /* DISABLED: /*@ ... @*/ */
# Result:  /*@ ... @*/

# This handles multi-line ACSL annotations
# We need to:
# 1. Replace "/* DISABLED: /*@" with "/*@"
# 2. Replace "@*/ */" with "@*/"

sed -i 's|/\* DISABLED: /\*@|/*@|g' "$FILE"
sed -i 's|@\*/ \*/|@*/|g' "$FILE"

echo "Re-enabled ACSL invariants in $FILE"
echo "Backup saved to $FILE.bak"

# Count how many were changed
CHANGES=$(diff "$FILE.bak" "$FILE" | grep -c "^<" || true)
echo "Changed $CHANGES lines"
