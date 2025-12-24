#!/bin/bash
# No-op preprocessor for Frama-C
# Just copies the input file to output, ignoring all preprocessing flags

# Find the input and output files from the arguments
INPUT=""
OUTPUT=""

while [[ $# -gt 0 ]]; do
    case "$1" in
        -o)
            OUTPUT="$2"
            shift 2
            ;;
        -*)
            # Ignore all flags
            shift
            ;;
        *)
            # Assume it's the input file
            INPUT="$1"
            shift
            ;;
    esac
done

# Copy input to output
if [ -n "$INPUT" ] && [ -n "$OUTPUT" ]; then
    cat "$INPUT" > "$OUTPUT"
elif [ -n "$INPUT" ]; then
    cat "$INPUT"
fi
