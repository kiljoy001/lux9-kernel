#!/bin/bash
# Validation script for Phase 1 changes

echo "=== Phase 1 Validation ==="

# Check 1: Verify bootstrap allocator alignment changes
echo "1. Checking bootstrap allocator function signature..."
if grep -q "bootstrap_alloc_aligned" kernel/9front-port/xalloc.c; then
    echo "   PASS: bootstrap_alloc_aligned function found"
else
    echo "   FAIL: bootstrap_alloc_aligned function not found"
fi

# Check 2: Verify alignment in critical structures
echo "2. Checking structure alignment attributes..."
if grep -A20 "struct FPssestate" kernel/include/dat.h | grep -q "__attribute__((aligned"; then
    echo "   PASS: FPssestate has alignment attribute"
else
    echo "   FAIL: FPssestate missing alignment attribute"
fi

if grep -A15 "struct Lock" kernel/include/dat.h | grep -q "__attribute__((aligned"; then
    echo "   PASS: Lock has alignment attribute"
else
    echo "   FAIL: Lock missing alignment attribute"
fi

if grep -A50 "struct Mach" kernel/include/dat.h | grep -q "__attribute__((aligned"; then
    echo "   PASS: Mach has alignment attribute"
else
    echo "   FAIL: Mach missing alignment attribute"
fi

if grep "Tss.*__attribute__((aligned" kernel/include/dat.h; then
    echo "   PASS: Tss has alignment attribute"
else
    echo "   FAIL: Tss missing alignment attribute"
fi

# Check 3: Verify memory definition updates
echo "3. Checking memory definition updates..."
if grep -q "BLOCKALIGN.*64" kernel/include/mem.h; then
    echo "   PASS: BLOCKALIGN updated to 64"
else
    echo "   FAIL: BLOCKALIGN not updated"
fi

if grep -q "BY2WD.*sizeof" kernel/include/mem.h; then
    echo "   PASS: BY2WD uses sizeof expressions"
else
    echo "   FAIL: BY2WD still hardcoded"
fi

# Check 4: Verify static_assert support
echo "4. Checking static_assert support..."
if grep -q "static_assert.*_Static_assert" kernel/include/u.h; then
    echo "   PASS: static_assert support added"
else
    echo "   FAIL: static_assert support missing"
fi

# Check 5: Verify type size assertions
echo "5. Checking type size assertions..."
if grep -q "sizeof.*ulong.*==.*sizeof.*void" kernel/include/u.h; then
    echo "   PASS: Type size assertions present"
else
    echo "   FAIL: Type size assertions missing"
fi

echo "=== Phase 1 Validation Complete ==="