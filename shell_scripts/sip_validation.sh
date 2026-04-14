#!/bin/bash

echo "=== SIP (Software Isolated Process) Infrastructure Validation ==="
echo

RET=0

echo "1. Checking Kernel Components..."

# Check for source files
for f in kernel/9front-port/devsip.c kernel/9front-port/devpebble.c kernel/9front-port/devexchange.c kernel/borrowchecker.c; do
    if [ -f "$f" ]; then
        echo "  ✓ $f present"
    else
        echo "  ✗ $f MISSING"
        RET=1
    fi
done

# Check for symbols in the ELF
if [ -f lux9.elf ]; then
    echo "  Checking lux9.elf symbols:"
    for sym in sipdevtab pebbledevtab exchdevtab borrow_acquire exchange_prepare; do
        if nm lux9.elf | grep -q "$sym"; then
            echo "    ✓ Symbol '$sym' found"
        else
            echo "    ✗ Symbol '$sym' MISSING in kernel"
            RET=1
        fi
    done
else
    echo "  ✗ lux9.elf not found (build failed?)"
    RET=1
fi

echo
echo "2. Checking Userspace Framework..."

# Check Go files
for f in userspace/go-servers/sip/sip.go userspace/go-servers/sip/kernel.go userspace/go-servers/sip/pebble.go; do
    if [ -f "$f" ]; then
        echo "  ✓ $f present"
    else
        echo "  ✗ $f MISSING"
        RET=1
    fi
done

# Run Go tests
echo "  Running Go tests..."
cd userspace/go-servers/sip
go test ./... > /dev/null 2>&1
if [ $? -eq 0 ]; then
    echo "    ✓ Go tests passed"
else
    echo "    ✗ Go tests FAILED"
    RET=1
fi
cd - > /dev/null

echo
if [ $RET -eq 0 ]; then
    echo "✅ VALIDATION SUCCESS: SIP Infrastructure is correct and built."
else
    echo "❌ VALIDATION FAILED: Some components are missing or broken."
fi

exit $RET
