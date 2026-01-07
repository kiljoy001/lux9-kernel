#!/bin/bash
# Edit Plan 9 files from Linux using your favorite editor
# The files are shared via 9P mount

INTEROP_DIR="/home/scott/Repo/VM-Interop"
EDITOR="${EDITOR:-nano}"  # Use nano by default, or vim if you prefer

if [ $# -eq 0 ]; then
    echo "Remote Edit for Plan 9"
    echo "====================="
    echo "Edit files here, access them in Plan 9 at /n/interop/"
    echo ""
    echo "Usage: $0 <filename>"
    echo "Example: $0 test.rc"
    echo ""
    echo "Current files:"
    ls -la "$INTEROP_DIR" | grep -v "^total"
    exit 0
fi

FILE="$INTEROP_DIR/$1"

# Create file if it doesn't exist
if [ ! -f "$FILE" ]; then
    echo "Creating new file: $1"
    touch "$FILE"
fi

# Edit with your preferred editor
$EDITOR "$FILE"

echo "File saved. In Plan 9, access it at: /n/interop/$1"
echo "To run it: rc /n/interop/$1"