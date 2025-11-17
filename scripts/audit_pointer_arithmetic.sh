#!/bin/bash
# Pointer arithmetic audit script
# Detects manual pointer arithmetic that should use sizeof expressions

echo "=== Pointer Arithmetic Audit ==="

# Look for problematic patterns
echo "Checking for hardcoded pointer arithmetic..."

# Pattern 1: Manual + 8 patterns
PLUS_8_COUNT=$(grep -r " + 8\b" --include="*.c" --include="*.h" kernel/ | grep -v "sizeof" | wc -l)
if [ $PLUS_8_COUNT -gt 0 ]; then
    echo "⚠️  Found $PLUS_8_COUNT instances of ' + 8' without sizeof:"
    grep -r " + 8\b" --include="*.c" --include="*.h" kernel/ | grep -v "sizeof" | head -5
else
    echo "✅ No problematic ' + 8' patterns found"
fi

# Pattern 2: Manual / 8 patterns
DIV_8_COUNT=$(grep -r " / 8\b" --include="*.c" --include="*.h" kernel/ | grep -v "sizeof" | wc -l)
if [ $DIV_8_COUNT -gt 0 ]; then
    echo "⚠️  Found $DIV_8_COUNT instances of ' / 8' without sizeof:"
    grep -r " / 8\b" --include="*.c" --include="*.h" kernel/ | grep -v "sizeof" | head -5
else
    echo "✅ No problematic ' / 8' patterns found"
fi

# Pattern 3: Manual * 8 patterns
MUL_8_COUNT=$(grep -r " \* 8\b" --include="*.c" --include="*.h" kernel/ | grep -v "sizeof" | wc -l)
if [ $MUL_8_COUNT -gt 0 ]; then
    echo "⚠️  Found $MUL_8_COUNT instances of ' * 8' without sizeof:"
    grep -r " \* 8\b" --include="*.c" --include="*.h" kernel/ | grep -v "sizeof" | head -5
else
    echo "✅ No problematic ' * 8' patterns found"
fi

# Pattern 4: Pointer casting with hardcoded sizes
CAST_COUNT=$(grep -r "(void\*\|char\*\|uchar\*).*+" --include="*.c" --include="*.h" kernel/ | grep -E "[0-9]+" | grep -v "sizeof" | wc -l)
if [ $CAST_COUNT -gt 0 ]; then
    echo "⚠️  Found $CAST_COUNT instances of pointer arithmetic with hardcoded values:"
    grep -r "(void\*\|char\*\|uchar\*).*+" --include="*.c" --include="*.h" kernel/ | grep -E "[0-9]+" | grep -v "sizeof" | head -5
else
    echo "✅ No problematic pointer casting arithmetic found"
fi

echo ""
echo "=== Pointer Arithmetic Audit Complete ==="
echo ""
echo "Recommendation: Replace hardcoded arithmetic with sizeof expressions"
echo "Example: ptr + 8  →  ptr + sizeof(*ptr)"
echo "         ptr / 8  →  ptr / sizeof(*ptr)"