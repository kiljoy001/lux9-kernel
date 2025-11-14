#!/bin/bash
# scripts/build.sh - Main Build Script
# Handles all build workflows

set -e

# Configuration
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(dirname "$SCRIPT_DIR")"
LOG_DIR="$ROOT_DIR/logs"

# Logging
LOG_FILE="$LOG_DIR/build-$(date +%Y%m%d-%H%M%S).log"
mkdir -p "$LOG_DIR"

log() {
    echo "[$(date '+%Y-%m-%d %H:%M:%S')] $*" | tee -a "$LOG_FILE"
}

error() {
    echo "ERROR: $*" | tee -a "$LOG_FILE"
    exit 1
}

# Build functions
build_kernel() {
    log "Building kernel..."
    cd "$ROOT_DIR/kernel"
    make clean
    make all
    log "Kernel build complete"
}

build_userspace() {
    log "Building userspace..."
    cd "$ROOT_DIR/userspace"
    make clean
    make all
    log "Userspace build complete"
}

build_os() {
    log "Building complete OS..."
    cd "$ROOT_DIR/os"
    make clean
    make all
    log "OS build complete"
}

# Test functions
test_kernel() {
    log "Testing kernel..."
    cd "$ROOT_DIR/kernel"
    make test
    log "Kernel tests complete"
}

test_userspace() {
    log "Testing userspace..."
    cd "$ROOT_DIR/userspace"
    make test
    log "Userspace tests complete"
}

test_integration() {
    log "Running integration tests..."
    cd "$ROOT_DIR/os"
    make test
    log "Integration tests complete"
}

# Main workflow
main() {
    case "${1:-all}" in
        "kernel")
            build_kernel
            ;;
        "userspace")
            build_userspace
            ;;
        "all")
            log "Starting full build..."
            build_kernel
            build_userspace
            build_os
            log "Full build complete"
            ;;
        "test")
            test_kernel
            test_userspace
            test_integration
            ;;
        "clean")
            log "Cleaning all builds..."
            cd "$ROOT_DIR" && make clean
            log "Clean complete"
            ;;
        *)
            echo "Usage: $0 {kernel|userspace|all|test|clean}"
            echo "  kernel    - Build kernel only"
            echo "  userspace - Build userspace only"
            echo "  all       - Build complete system"
            echo "  test      - Run all tests"
            echo "  clean     - Clean all builds"
            exit 1
            ;;
    esac
}

main "$@"
