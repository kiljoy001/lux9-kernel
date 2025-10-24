# 9P Exchange Test Implementation

## Overview

This implementation provides two versions of the exchange device test to verify that we can rely on the 9P library:

1. **Original Test** (`exchange_test`) - Uses direct file operations (read/write) to communicate with the exchange device
2. **9P Library Test** (`exchange_9p_test`) - Uses the `github.com/DeedleFake/p9` library to communicate with the exchange device via proper 9P protocol

## Implementation Details

### Dependencies

- Added `github.com/DeedleFake/p9` as a dependency for the 9P library test
- Updated `go.mod` and `go.sum` files in the exchange directory

### Test Program (`exchange/main.go`)

The test program now runs both tests sequentially:

1. **Original Test**:
   - Opens the exchange device using `os.OpenFile("#X/exchange", os.O_RDWR, 0)`
   - Reads current status using direct `Read()` calls
   - Sends a "prepare 0x10000000" command using direct `Write()` calls
   - Reads status again to see changes

2. **9P Library Test**:
   - Opens the exchange device using `os.OpenFile("#X/exchange", os.O_RDWR, 0)`
   - Wraps the file in a `net.Conn` interface to work with the 9P library
   - Creates a 9P client using `p9.NewClient()`
   - Performs 9P handshake using `client.Handshake(8192)`
   - Attaches to the exchange service using `client.Attach(nil, "", "/")`
   - Reads current status using 9P `Read()` calls
   - Sends a "prepare 0x10000000" command using 9P `Write()` calls
   - Reads status again to see changes

### Build System

- Updated `userspace/go-servers/Makefile` to build both test binaries
- Updated `userspace/Makefile` to include both binaries in the initrd
- Modified `userspace/bin/init.c` to run both tests during boot

## Usage

The tests are automatically run during system boot by the init process. Both tests will output their results to the console, allowing you to compare the behavior of direct file operations versus 9P library operations.

## Verification

By comparing the output of both tests, you can verify:
1. That the 9P library works correctly with our exchange device
2. That both approaches produce the same results
3. That we can rely on the 9P library for future OS userland communication