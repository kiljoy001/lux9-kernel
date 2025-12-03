#!/bin/bash

echo "=== Final Integration Verification ==="
echo

FAIL=0

# 1. Check Kernel Build
if [ -f lux9.elf ]; then
    echo "✓ Kernel binary exists (lux9.elf)"
    
    # Check for new symbols
    for sym in borrow_broker_transfer borrowinit ringdevtab memory_range_init; do
        if nm lux9.elf | grep -q "$sym"; then
            echo "  ✓ Symbol '$sym' found"
        else
            echo "  ✗ Symbol '$sym' MISSING"
            FAIL=1
        fi
    done
else
    echo "✗ Kernel binary MISSING"
    FAIL=1
fi

echo

# 2. Check Userspace Build
if [ -f userspace/lib/libc.a ] && [ -f userspace/lib/crt0.o ]; then
    echo "✓ Userspace libc built"
else
    echo "✗ Userspace libc MISSING"
    FAIL=1
fi

# Check for syscalls in libc
if nm userspace/lib/libc.a | grep -q "pebble_issue_white"; then
    echo "  ✓ pebble_issue_white syscall found"
else
    echo "  ✗ pebble_issue_white syscall MISSING"
    FAIL=1
fi

echo

# 3. Test Artifacts
if [ -f test_borrow_stress ]; then
    echo "✓ Stress test harness exists"
else
    echo "  (Stress test binary deleted as cleanup, acceptable)"
fi

echo
if [ $FAIL -eq 0 ]; then
    echo "✅ INTEGRATION SUCCESS: System is built and symbols are linked."
    echo "   Ready for boot testing."
else
    echo "❌ INTEGRATION FAILED."
    exit 1
fi
