#!/bin/bash
set -e
export GOOS=plan9
export GOARCH=amd64

echo "Building init (asm)..."
# -E main._start overrides entry point.
# -H plan9 explicitly tells the linker to produce Plan 9 a.out
go build -ldflags "-s -w -E main._start -T 0x200000 -H plan9" -o init .

echo "Built init."
file init