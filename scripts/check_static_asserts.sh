#!/bin/bash
# Static assert coverage checker
# Verifies that critical structures have compile-time alignment checks

echo "=== Static Assert Coverage Check ==="

# Define critical structures that should have alignment checks
CRITICAL_STRUCTURES=("FPssestate" "Lock" "Mach" "Proc" "Page" "MMU" "Tss")
CRITICAL_TYPES=("ulong" "uintptr" "usize" "ssize")

# Check for structure alignment assertions in alignment_checks.h
ALIGNMENT_CHECKS_FILE="kernel/include/alignment_checks.h"

echo "Checking structure alignment assertions..."

MISSING_ASSERTS=0
for struct in "${CRITICAL_STRUCTURES[@]}"; do
    if grep -q "sizeof.*struct.*${struct}.*%.*64.*==.*0" "$ALIGNMENT_CHECKS_FILE"; then
        echo "   PASS: ${struct} has size alignment assertion"
    else
        echo "   WARN: ${struct} missing size alignment assertion"
        MISSING_ASSERTS=$((MISSING_ASSERTS + 1))
    fi
done

# Check for type size assertions
echo "Checking type size assertions..."
for type in "${CRITICAL_TYPES[@]}"; do
    if grep -q "sizeof.*${type}.*==.*sizeof.*void" kernel/include/u.h; then
        echo "   PASS: ${type} has pointer size assertion"
    else
        echo "   WARN: ${type} missing pointer size assertion"
    fi
done

# Check for SIMD field alignment in FPssestate
echo "Checking SIMD field alignment..."
if grep -q "offsetof.*struct.*FPssestate.*xmm.*%.*16.*==.*0" "$ALIGNMENT_CHECKS_FILE"; then
    echo "   PASS: FPssestate.xmm has 16-byte alignment assertion"
else
    echo "   WARN: FPssestate.xmm missing 16-byte alignment assertion"
    MISSING_ASSERTS=$((MISSING_ASSERTS + 1))
fi

if grep -q "offsetof.*struct.*FPssestate.*xmm.*%.*32.*==.*0" "$ALIGNMENT_CHECKS_FILE"; then
    echo "   PASS: FPssestate.xmm has 32-byte alignment assertion (AVX)"
else
    echo "   WARN: FPssestate.xmm missing 32-byte alignment assertion (AVX)"
    MISSING_ASSERTS=$((MISSING_ASSERTS + 1))
fi

echo ""
if [ $MISSING_ASSERTS -eq 0 ]; then
    echo "✅ ALL ASSERTION CHECKS PASSED"
else
    echo "⚠️  $MISSING_ASSERTS assertion checks missing - consider adding them"
fi

echo "=== Static Assert Coverage Check Complete ==="