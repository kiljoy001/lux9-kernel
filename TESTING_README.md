# Lux9 Kernel 9P Testing Infrastructure

This directory contains comprehensive testing tools for validating the 9P filesystem implementation in the Lux9 kernel.

## Testing Components

### Protocol-Level Testing
- `test-client.go` - Go-based 9P client for protocol validation
- `go-servers/memfs/` - Memory-based 9P server for testing
- `test_memfs.sh` - Automated memfs server testing

### Server Implementation Testing
- `test_ext4fs_simple.sh` - Shell-based ext4fs server validation
- `test_ext4fs.sh` - Extended ext4fs testing with Linux v9fs integration

### Integration Testing
- `test_9p_comprehensive.sh` - Full testing suite covering all layers
- `test_9p_protocol_validation.sh` - Kernel 9P implementation validation

### Documentation
- `9P_TESTING_STRATEGY.md` - Detailed testing approach and recommendations

## Usage

### Quick Validation
```bash
# Run from userspace directory
./test_ext4fs_simple.sh
```

### Comprehensive Testing
```bash
# Run from userspace directory
./test_9p_comprehensive.sh
```

### Protocol Validation
```bash
# Run from root directory
./test_9p_protocol_validation.sh
```

## Testing Layers

1. **Protocol Testing** - Validates 9P2000 message exchange
2. **Server Testing** - Verifies individual server functionality
3. **Integration Testing** - Checks kernel-server interaction
4. **System Testing** - Full boot and operation validation

## Requirements

- Go 1.16+
- gcc with ext2fs support
- dd, mkfs.ext4 utilities
- QEMU for full system testing

## Best Practices

1. Test protocol compliance externally before kernel integration
2. Use C for initrd binaries (smaller footprint)
3. Validate with shell scripts before full integration
4. Use serial console debugging for kernel tracing
5. Follow the ext4fs server pattern as reference

## Expected Output

### Successful Server Test
```
All tests passed!
Server ready for integration with lux9 microkernel!
```

### Successful Protocol Validation
```
The lux9 kernel 9P implementation appears to be correctly structured
```

### Integration Success
Look for these messages in QEMU serial output:
- "init: mounting root at /"
- "root filesystem mounted successfully"