#!/bin/bash
# Script to detect potential misaligned structure definitions

echo "Checking for potential misaligned structure definitions..."

# Check for structures that should probably be aligned but aren't
echo "=== Structures with SIMD fields (should be aligned) ==="
grep -n "xmm\|avx\|sse\|fp_state\|FPsave\|FPssestate" kernel/include/*.h kernel/9front-*/dat.h | grep "struct\|{" 

echo ""
echo "=== Structures with mixed pointer/integer fields ==="
grep -A5 -B5 "uintptr.*;\|ulong.*;\|uint.*;" kernel/include/dat.h | grep -B5 -A5 "struct.*{" 

echo ""
echo "=== Structures containing other structures ==="
grep -A10 "struct.*{" kernel/include/dat.h | grep -A10 -B10 "struct [A-Z]"

echo ""
echo "=== Done ==="