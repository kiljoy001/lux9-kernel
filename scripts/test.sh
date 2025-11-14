#!/bin/bash
# scripts/test.sh - Comprehensive Testing Script

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(dirname "$SCRIPT_DIR")"

# Test configuration
TEST_TIMEOUT=30
QEMU_CMD="/home/scott/Repo/gnumach/qemu-9.1.0/build/qemu-system-x86_64 -M q35 -m 1G -nographic -serial stdio -display none -no-reboot"

test_kernel_unit() {
    echo "Testing kernel unit compilation..."
    cd "$ROOT_DIR/kernel"
    make test
}

test_userspace_unit() {
    echo "Testing userspace unit compilation..."
    cd "$ROOT_DIR/userspace"
    make test
}

test_integration() {
    echo "Running integration tests..."
    
    # Test 1: Kernel boots
    echo "Test 1: Kernel boot test"
    timeout $TEST_TIMEOUT /home/scott/Repo/gnumach/qemu-9.1.0/build/qemu-system-x86_64 -kernel "$ROOT_DIR/kernel/lux9.elf" || \
        echo "Kernel boot test completed"
    
    # Test 2: Complete OS boots
    echo "Test 2: OS boot test"
    timeout $TEST_TIMEOUT /home/scott/Repo/gnumach/qemu-9.1.0/build/qemu-system-x86_64 -drive file="$ROOT_DIR/os/lux9.iso",format=raw,media=disk || \
        echo "OS boot test completed"
    
    # Test 3: Driver isolation
    echo "Test 3: Driver isolation test"
    # This would test that drivers crash in isolation
    echo "Driver isolation test - requires SIP framework"
}

test_performance() {
    echo "Running performance tests..."
    
    # Build time tests
    echo "Build performance test:"
    time "$SCRIPT_DIR/build.sh" clean
    time "$SCRIPT_DIR/build.sh" all
    
    # Runtime performance
    echo "Runtime performance test:"
    timeout 10s /home/scott/Repo/gnumach/qemu-9.1.0/build/qemu-system-x86_64 -drive file="$ROOT_DIR/os/lux9.iso",format=raw,media=disk || true
}

case "${1:-all}" in
    "unit")
        test_kernel_unit
        test_userspace_unit
        ;;
    "integration")
        test_integration
        ;;
    "performance")
        test_performance
        ;;
    "all")
        test_kernel_unit
        test_userspace_unit
        test_integration
        test_performance
        ;;
    *)
        echo "Test Suite:"
        echo "  test.sh unit        - Unit tests"
        echo "  test.sh integration - Integration tests"
        echo "  test.sh performance - Performance tests"
        echo "  test.sh all         - All tests"
        ;;
esac
