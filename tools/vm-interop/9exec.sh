#!/bin/bash
# Execute commands in 9front VM via file exchange

if [ $# -eq 0 ]; then
    echo "Usage: $0 <command>"
    echo "Example: $0 'ls /sys/src'"
    exit 1
fi

INTEROP_DIR="/home/scott/Repo/VM-Interop"

# Send command
echo "$1" > "$INTEROP_DIR/command.txt"
echo "Sent: $1"

# Wait for execution
sleep 2

# Check for output
if [ -f "$INTEROP_DIR/output.txt" ]; then
    echo "Output:"
    cat "$INTEROP_DIR/output.txt"
    rm "$INTEROP_DIR/output.txt"
else
    echo "No output received (autorun.rc may not be running)"
fi