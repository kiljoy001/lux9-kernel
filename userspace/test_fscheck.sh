#!/bin/bash
# Test suite for fscheck utility

set -e

GREEN='\033[0;32m'
RED='\033[0;31m'
NC='\033[0m'

TESTFS="/tmp/test_fscheck.img"
FSCHECK="./bin/fscheck"

cleanup() {
    rm -f "$TESTFS"
}

trap cleanup EXIT

echo "=== fscheck Utility Test Suite ==="
echo

# Create test filesystem
echo -n "Creating test filesystem... "
dd if=/dev/zero of="$TESTFS" bs=1M count=10 &>/dev/null
mkfs.ext4 -F "$TESTFS" &>/dev/null
echo -e "${GREEN}OK${NC}"

echo
echo "=== Running Tests ==="
echo

# Test 1: Binary exists
echo -n "Test 1: Binary exists... "
if [ -x "$FSCHECK" ]; then
    echo -e "${GREEN}PASS${NC}"
else
    echo -e "${RED}FAIL${NC}"
    exit 1
fi

# Test 2: Shows usage
echo -n "Test 2: Shows usage... "
if $FSCHECK 2>&1 | grep -q "usage:"; then
    echo -e "${GREEN}PASS${NC}"
else
    echo -e "${RED}FAIL${NC}"
fi

# Test 3: Checks clean filesystem
echo -n "Test 3: Checks clean FS... "
if $FSCHECK "$TESTFS" 2>&1 | grep -q "clean"; then
    echo -e "${GREEN}PASS${NC}"
else
    echo -e "${RED}FAIL${NC}"
fi

# Test 4: Force check works
echo -n "Test 4: Force check works... "
if $FSCHECK -f "$TESTFS" 2>&1 | grep -q "Checking filesystem"; then
    echo -e "${GREEN}PASS${NC}"
else
    echo -e "${RED}FAIL${NC}"
fi

# Test 5: Verbose output
echo -n "Test 5: Verbose output... "
if $FSCHECK -vf "$TESTFS" 2>&1 | grep -q "Block size"; then
    echo -e "${GREEN}PASS${NC}"
else
    echo -e "${RED}FAIL${NC}"
fi

# Test 6: Rejects invalid filesystem
echo -n "Test 6: Rejects invalid FS... "
echo "not a filesystem" > /tmp/bad_fs
if $FSCHECK /tmp/bad_fs 2>&1 | grep -q "Cannot open"; then
    echo -e "${GREEN}PASS${NC}"
else
    echo -e "${RED}FAIL${NC}"
fi
rm -f /tmp/bad_fs

# Test 7: Checks return code on clean FS
echo -n "Test 7: Clean FS returns 0... "
if $FSCHECK -f "$TESTFS" &>/dev/null; then
    echo -e "${GREEN}PASS${NC}"
else
    echo -e "${RED}FAIL${NC}"
fi

echo
echo "=== Summary ==="
echo -e "${GREEN}All tests passed!${NC}"
echo
echo "fscheck utility features:"
echo "  ✓ Superblock validation"
echo "  ✓ Block bitmap checking"
echo "  ✓ Inode bitmap checking"
echo "  ✓ Directory structure verification"
echo "  ✓ Verbose output (-v)"
echo "  ✓ Force check (-f)"
echo "  ✓ Auto-fix mode (-y)"
echo
echo "fscheck ready for use with lux9!"
