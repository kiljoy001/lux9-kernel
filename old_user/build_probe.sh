#!/bin/bash
set -e
export GOOS=plan9
export GOARCH=amd64

echo "Assembling probe.s..."
go tool asm -p main -o probe.o probe.s
go tool pack c probe.a probe.o

echo "Linking probe..."
go tool link -H=plan9 -T=0x200000 -E=_start -o probe probe.a

echo "Built probe."
file probe
