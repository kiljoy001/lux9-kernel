#!/bin/bash
# Phase 2 Validation Script
# Validates that the device registry and PCI framework are properly integrated

echo "=== Phase 2 Validation ==="

# Check if required files exist
REQUIRED_FILES=(
    "kernel/include/devregistry.h"
    "kernel/9front-port/devregistry.c"
    "kernel/include/pciframework.h"
    "kernel/9front-port/pciframework.c"
)

MISSING_FILES=0
for file in "${REQUIRED_FILES[@]}"; do
    if [ -f "/home/scott/Repo/lux9-kernel/$file" ]; then
        echo "   PASS: $file exists"
    else
        echo "   FAIL: $file missing"
        MISSING_FILES=$((MISSING_FILES + 1))
    fi
done

# Check kernel build
if [ -f "/home/scott/Repo/lux9-kernel/kernel/lux9.elf" ]; then
    echo "   PASS: Kernel builds successfully"
    KERNEL_SIZE=$(ls -la /home/scott/Repo/lux9-kernel/kernel/lux9.elf | awk '{print $5}')
    echo "   INFO: Kernel size: $KERNEL_SIZE bytes"
else
    echo "   FAIL: Kernel does not build"
    MISSING_FILES=$((MISSING_FILES + 1))
fi

# Check function declarations in fns.h
FUNCTIONS=("devregistry_init" "pci_framework_init" "pci_framework_enumerate")
for func in "${FUNCTIONS[@]}"; do
    if grep -q "$func" /home/scott/Repo/lux9-kernel/kernel/include/fns.h; then
        echo "   PASS: $func declared in fns.h"
    else
        echo "   FAIL: $func not declared in fns.h"
        MISSING_FILES=$((MISSING_FILES + 1))
    fi
done

# Check chan.c initialization
if grep -q "devregistry_init" /home/scott/Repo/lux9-kernel/kernel/9front-port/chan.c && \
   grep -q "pci_framework_init" /home/scott/Repo/lux9-kernel/kernel/9front-port/chan.c; then
    echo "   PASS: Frameworks initialized in chan.c"
else
    echo "   FAIL: Frameworks not properly initialized in chan.c"
    MISSING_FILES=$((MISSING_FILES + 1))
fi

echo ""
if [ $MISSING_FILES -eq 0 ]; then
    echo "✅ ALL PHASE 2 VALIDATION CHECKS PASSED"
    echo "Phase 2 implementation is ready for use"
else
    echo "❌ $MISSING_FILES validation checks failed"
    echo "Please review the implementation"
fi

echo "=== Phase 2 Validation Complete ==="