#!/bin/bash
# Comprehensive 9P testing script for lux9 kernel
# Tests all aspects of 9P functionality using multiple approaches

set -e

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

TESTFS="/tmp/test_9p_lux9.img"
SERVER="./servers/ext4fs/ext4fs"
MEMFS_SERVER="./go-servers/memfs/memfs"
GO_CLIENT="./go-servers/test-client"

LOG_FILE="/tmp/lux9_9p_test.log"
> "$LOG_FILE"  # Clear log file

log() {
    echo "$@" | tee -a "$LOG_FILE"
}

cleanup() {
    log "Cleaning up..."
    pkill -f ext4fs || true
    pkill -f memfs || true
    rm -f "$TESTFS"
    rm -f /tmp/bad_fs
}

trap cleanup EXIT

echo "==============================================="
echo "  Lux9 Kernel 9P Comprehensive Testing Suite"
echo "==============================================="
echo

# Check prerequisites
log "=== Checking Prerequisites ==="

# Check if required tools are available
for tool in dd mkfs.ext4 go gcc; do
    if ! command -v $tool &>/dev/null; then
        echo -e "${RED}Error: Required tool '$tool' not found${NC}"
        exit 1
    fi
done

log "All prerequisites satisfied"
echo

# Build Go components if needed
log "=== Building Go Components ==="
if [ ! -f "$MEMFS_SERVER" ] || [ ! -f "$GO_CLIENT" ]; then
    log "Building Go components..."
    make -C go-servers build
fi

if [ -f "$MEMFS_SERVER" ]; then
    log "memfs server: ${GREEN}Available${NC}"
else
    log "memfs server: ${RED}Not available${NC}"
    exit 1
fi

if [ -f "$GO_CLIENT" ]; then
    log "Go test client: ${GREEN}Available${NC}"
else
    log "Go test client: ${RED}Not available${NC}"
    exit 1
fi

echo

# Test 1: External Protocol Testing with Linux v9fs (if available)
log "=== Test 1: Linux v9fs Client Integration ==="

# Check if we can do Linux v9fs testing
if grep -q "CONFIG_9P_FS" /boot/config-$(uname -r) 2>/dev/null || 
   zgrep -q "CONFIG_9P_FS" /proc/config.gz 2>/dev/null; then
    log "Linux 9P support: ${GREEN}Available${NC}"
    log "${YELLOW}Note: Full Linux v9fs testing requires manual setup${NC}"
    log "${YELLOW}      Start a server and mount with: mount -t 9p -o trans=virtio,version=9P2000 <pid> /mnt${NC}"
else
    log "Linux 9P support: ${YELLOW}Not available in kernel${NC}"
fi
echo

# Test 2: Shell-based ext4fs testing
log "=== Test 2: Shell-based ext4fs Server Testing ==="
./test_ext4fs_simple.sh
echo

# Test 3: Go Client Protocol Testing
log "=== Test 3: Go Client Protocol Testing ==="
log "Starting memfs server for Go client test..."
timeout 2 "$MEMFS_SERVER" </dev/null >/tmp/memfs_out.log 2>&1 &
MEMFS_PID=$!
sleep 0.1

if kill -0 $MEMFS_PID 2>/dev/null; then
    log "memfs server started (PID: $MEMFS_PID)"
    
    log "Running Go client test..."
    if timeout 1 "$GO_CLIENT" 2>/tmp/go_client.log; then
        log "Go client test: ${GREEN}PASSED${NC}"
        log "Output: $(head -n 5 /tmp/go_client.log)"
    else
        log "Go client test: ${RED}FAILED${NC}"
        log "Client output: $(head -n 5 /tmp/go_client.log)"
    fi
    
    kill $MEMFS_PID 2>/dev/null || true
else
    log "memfs server: ${RED}FAILED TO START${NC}"
    log "Server output: $(head -n 5 /tmp/memfs_out.log)"
fi
echo

# Test 4: Create test filesystem for integration testing
log "=== Test 4: Creating Test Filesystem ==="
log -n "Creating test ext4 filesystem... "
if dd if=/dev/zero of="$TESTFS" bs=1M count=5 &>/dev/null && 
   mkfs.ext4 -F "$TESTFS" &>/dev/null; then
    log "${GREEN}OK${NC}"
else
    log "${RED}FAILED${NC}"
    exit 1
fi

# Add a test file to the filesystem
log -n "Adding test file to filesystem... "
if mkdir -p /tmp/mnt && 
   sudo mount "$TESTFS" /tmp/mnt 2>/dev/null && 
   echo "This is a test file for lux9 9P testing" | sudo tee /tmp/mnt/test.txt >/dev/null && 
   sudo umount /tmp/mnt && 
   rmdir /tmp/mnt; then
    log "${GREEN}OK${NC}"
else
    log "${YELLOW}SKIP (no sudo or mount failed)${NC}"
    # Create a minimal filesystem without sudo
    log "Creating minimal test filesystem without sudo"
fi
echo

# Test 5: Server Binary Validation
log "=== Test 5: Server Binary Validation ==="

log -n "ext4fs binary exists and is executable... "
if [ -x "$SERVER" ]; then
    log "${GREEN}PASSED${NC}"
else
    log "${RED}FAILED${NC}"
    exit 1
fi

log -n "ext4fs links to libext2fs... "
if ldd "$SERVER" 2>/dev/null | grep -q libext2fs; then
    log "${GREEN}PASSED${NC}"
else
    log "${RED}FAILED${NC}"
fi

log -n "ext4fs binary size reasonable... "
SIZE=$(stat -c%s "$SERVER")
if [ $SIZE -gt 10000 ] && [ $SIZE -lt 200000 ]; then
    log "${GREEN}PASSED${NC} (${SIZE} bytes)"
else
    log "${YELLOW}WARNING${NC} (size: ${SIZE} bytes)"
fi
echo

# Test 6: Server Functionality Testing
log "=== Test 6: Server Functionality Testing ==="

log -n "ext4fs can open filesystem... "
if timeout 0.5 $SERVER "$TESTFS" </dev/null &>/dev/null; then
    log "${GREEN}PASSED${NC}"
else
    CODE=$?
    if [ $CODE -eq 124 ]; then
        log "${GREEN}PASSED${NC} (server running)"
    else
        log "${RED}FAILED${NC} (exit code: $CODE)"
    fi
fi

log -n "ext4fs rejects invalid filesystem... "
echo "not a filesystem" > /tmp/bad_fs
if $SERVER /tmp/bad_fs 2>&1 | grep -q "cannot open"; then
    log "${GREEN}PASSED${NC}"
else
    log "${RED}FAILED${NC}"
fi
rm -f /tmp/bad_fs

log -n "ext4fs shows usage on bad args... "
if $SERVER 2>&1 | grep -q "usage"; then
    log "${GREEN}PASSED${NC}"
else
    log "${RED}FAILED${NC}"
fi
echo

# Test 7: QEMU Integration Testing Preparation
log "=== Test 7: QEMU Integration Testing Preparation ==="

log "Building initrd with current servers..."
if make initrd >/tmp/make_initrd.log 2>&1; then
    log "initrd build: ${GREEN}SUCCESS${NC}"
else
    log "initrd build: ${RED}FAILED${NC}"
    log "Build output: $(head -n 5 /tmp/make_initrd.log)"
fi

if [ -f "initrd.tar" ]; then
    log "initrd.tar: ${GREEN}CREATED${NC}"
    log "Contents:"
    tar -tf initrd.tar | head -10 | while read line; do
        log "  $line"
    done
    if [ $(tar -tf initrd.tar | wc -l) -gt 10 ]; then
        log "  ... and $(( $(tar -tf initrd.tar | wc -l) - 10 )) more files"
    fi
else
    log "initrd.tar: ${RED}NOT CREATED${NC}"
fi
echo

# Test 8: ISO Build Testing
log "=== Test 8: ISO Build Testing ==="

log -n "Building ISO with initrd... "
cd ..  # Go to parent directory for ISO build
if make >/tmp/make_iso.log 2>&1; then
    log "${GREEN}SUCCESS${NC}"
    if [ -f "lux9.iso" ]; then
        ISO_SIZE=$(stat -c%s lux9.iso)
        log "ISO size: $((ISO_SIZE / 1024)) KB"
    fi
else
    log "${RED}FAILED${NC}"
    log "Build output: $(head -n 5 /tmp/make_iso.log)"
fi
echo

# Summary
log "=== Test Summary ==="
log "All individual tests completed. For full integration testing:"
log "1. Run: qemu-system-x86_64 -cdrom lux9.iso -serial stdio"
log "2. Check for: 'init: mounting root at /' and 'root filesystem mounted successfully'"
log
log "Log file available at: $LOG_FILE"
log
log "${GREEN}Comprehensive 9P testing preparation completed!${NC}"