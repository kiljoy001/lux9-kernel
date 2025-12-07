#!/bin/bash
# Build C init program

set -e

echo "Building C init program..."

# Rebuild libc first to include dup()
cd libc
make clean
make
cd ..

# Build init
gcc -Wall -Wextra -O2 -g \
    -nostdlib -static -fno-stack-protector \
    -I./include \
    -Iinclude \
    -o build/init \
    bin/init.c \
    lib/crt0.o \
    -Llib -lc

echo "Built: build/init"
ls -lh build/init
