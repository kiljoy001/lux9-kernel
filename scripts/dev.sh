#!/bin/bash
# scripts/dev.sh - Development Workflow Script
# Streamlined development builds

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(dirname "$SCRIPT_DIR")"

dev_kernel() {
    echo "Building kernel for development..."
    cd "$ROOT_DIR/kernel"
    make clean
    make all
}

dev_userspace() {
    echo "Building userspace for development..."
    cd "$ROOT_DIR/userspace"
    make clean
    make all
}

dev_test() {
    echo "Running development tests..."
    "$SCRIPT_DIR/build.sh" test
}

dev_run() {
    echo "Running OS in QEMU..."
    cd "$ROOT_DIR/os"
    make run
}

case "${1:-help}" in
    "kernel")
        dev_kernel
        ;;
    "userspace")
        dev_userspace
        ;;
    "test")
        dev_test
        ;;
    "run")
        dev_run
        ;;
    "all")
        dev_kernel
        dev_userspace
        dev_test
        echo "Development build ready!"
        ;;
    *)
        echo "Development Workflow:"
        echo "  dev.sh kernel   - Build kernel with debug info"
        echo "  dev.sh userspace - Build userspace"
        echo "  dev.sh test     - Run tests"
        echo "  dev.sh run      - Run in QEMU"
        echo "  dev.sh all      - Full development build"
        ;;
esac
