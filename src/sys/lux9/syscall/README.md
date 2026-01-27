# Lux9 Unified System Call Management

## Overview

This directory contains the Lux9 Unified System Call Management system, which resolves conflicts between Plan 9, Lux9 native, and RUMP/NetBSD syscall numbering schemes.

## The Problem

The Lux9 kernel currently has three incompatible syscall numbering schemes:

1. **Plan 9 syscalls**: Traditional Plan 9 numbering (PREAD=50, PWRITE=51, etc.)
2. **Lux9 native syscalls**: Lux9-specific operations (SYS_WASM_COMPILE=63, SYS_WASM_EXECUTE=64, SYS_WASM_DESTROY=65)
3. **RUMP/NetBSD syscalls**: Compatibility layer with different numbering

**The Critical Conflict**: Syscalls 63-65 are used by both:
- Lux9: WASM operations (compile, execute, destroy)
- Potential Plan 9 extensions (collision)

## The Solution

### Unified Numbering Scheme

```
Range 0-99:     Plan 9 compatibility (direct mapping)
Range 100-199:  Lux9 native operations
Range 200-299:  Extended syscalls  
Range 1000+:    Subsystem-specific operations
```

### Key Changes

**WASM/Pebble Collision Resolution:**
- Old: SYS_WASM_COMPILE = 63, SYS_WASM_EXECUTE = 64, SYS_WASM_DESTROY = 65
- New: LUX9_SYS_WASM_COMPILE = 163, LUX9_SYS_WASM_EXECUTE = 164, LUX9_SYS_WASM_DESTROY = 165
- Legacy constants still work through compatibility layer

## Files

### Headers
- **lux9_syscall.h** - Main unified interface with complete numbering scheme
- **compat_plan9.h** - Plan 9 compatibility layer
- **compat_lux9_native.h** - Lux9 native compatibility (resolves WASM collision)
- **compat_rump.h** - RUMP/NetBSD compatibility layer
- **subsys_memory.h** - Memory subsystem syscalls (1000-1099)
- **internal.h** - Internal definitions

### Implementation
- **lux9_syscall_dispatch.c** - Core dispatcher with fast path optimization
- **lux9_syscall_table.c** - Complete syscall table with all handlers
- **Makefile** - Build system for the syscall library

### Documentation
- **IMPLEMENTATION_GUIDE.md** - Comprehensive implementation guide
- **README.md** - This file

## Quick Start

### For Kernel Developers

```c
// In your syscall handler
#include <lux9/syscall/lux9_syscall.h>

int handle_syscall(uint32_t syscall_num, uintptr_t args[]) {
    uintptr_t result;
    return lux9_syscall_dispatch(syscall_num, args, 6, &result);
}

// During initialization
lux9_syscall_register_compat();  // Register Plan 9 compatibility
lux9_syscall_register_memory_subsystem();  // Register memory subsystem
```

### For Userspace Developers

```c
// Option 1: Use unified numbers (recommended)
#include <lux9/syscall/lux9_syscall.h>

int fd = syscall(LUX9_SYS_OPEN, "/path", O_RDONLY);
void *wasm = syscall(LUX9_SYS_WASM_COMPILE, data, size);

// Option 2: Use compatibility layers (still works)
#include <lux9/syscall/compat_plan9.h>
#include <lux9/syscall/compat_lux9_native.h>

int fd = syscall(OPEN, "/path", O_RDONLY);  // Plan 9 compatible
void *wasm = syscall(SYS_WASM_COMPILE, data, size);  // Lux9 compatible
```

### For Memory Subsystem

```c
#include <lux9/syscall/subsys_memory.h>

void *mem = syscall(LUX9_SYS_MMAP, NULL, 4096, PROT_READ|PROT_WRITE, 
                   MAP_SHARED|MAP_ANONYMOUS, -1, 0);
syscall(LUX9_SYS_MPROTECT, mem, 4096, PROT_READ);
syscall(LUX9_SYS_MUNMAP, mem, 4096);
```

## Key Features

### 1. Hierarchical Organization
- Clear separation by subsystem
- Prevents future conflicts
- Scalable design for extensions

### 2. Backward Compatibility
- All existing code works unchanged
- Plan 9: Direct numeric mapping
- Lux9: Automatic translation
- RUMP: Hypervisor translation

### 3. Performance Optimization
- Fast path for common syscalls (< 1µs)
- Direct dispatch for compatibility layer
- Minimized overhead for hot paths
- Efficient syscall table lookup

### 4. Comprehensive Validation
- Compile-time syscall number validation
- Runtime validation with helpful errors
- Statistics and monitoring
- Debugging support

## Syscall Numbering

### Plan 9 Compatibility (0-99)
```
0:   LUX9_SYS_RSYNC      (SYSR1)
1:   LUX9_SYS_ERRSTR     (_ERRSTR)
...
50:  LUX9_SYS_PREAD      (PREAD)
51:  LUX9_SYS_PWRITE     (PWRITE)
```

### Lux9 Native (100-199)
```
163: LUX9_SYS_WASM_COMPILE     (SYS_WASM_COMPILE)
164: LUX9_SYS_WASM_EXECUTE     (SYS_WASM_EXECUTE)
165: LUX9_SYS_WASM_DESTROY     (SYS_WASM_DESTROY)
171: LUX9_SYS_EXCHANGE_ALLOC   (SYS_EXCHANGE_ALLOC)
...
```

### Extended (200-299)
```
200: LUX9_SYS_VM_CREATE
221: LUX9_SYS_CONTAINER_CREATE
```

### Subsystem-Specific (1000+)
```
Memory Subsystem (1000-1099):
1000: LUX9_SYS_MMAP
1001: LUX9_SYS_MUNMAP
1002: LUX9_SYS_MPROTECT
1011: LUX9_SYS_MALLOC
1012: LUX9_SYS_FREE

Network Subsystem (1100-1199):
1100: LUX9_SYS_SOCKET_CREATE
1101: LUX9_SYS_SOCKET_BIND
```

## Compatibility Layers

### Plan 9 Compatibility
- Direct mapping: OPEN → LUX9_SYS_OPEN → 14
- Exact numeric compatibility
- Preserve existing semantics
- Fast path optimization

### Lux9 Native Compatibility  
- Automatic translation: SYS_WASM_COMPILE (63) → LUX9_SYS_WASM_COMPILE (163)
- Legacy constants work unchanged
- Deprecation warnings for old numbers
- Clear migration path

### RUMP Compatibility
- Separate numbering space: 3000+
- Hypervisor-mediated translation
- Graceful degradation
- Full NetBSD compatibility

## Migration Guide

### For Plan 9 Code
**No changes required** - existing code works unchanged:
```c
fd = syscall(PREAD, fd, buffer, count, offset);  // Still works!
```

### For Lux9 Code
**Update recommended** (but not required):
```c
// Old code (still works)
void *wasm = syscall(SYS_WASM_COMPILE, data, size);

// New recommended way
void *wasm = syscall(LUX9_SYS_WASM_COMPILE, data, size);

// Both map to syscall 163
```

### For New Development
**Use unified numbers**:
```c
#include <lux9/syscall/lux9_syscall.h>

// File I/O
int fd = syscall(LUX9_SYS_OPEN, path, flags);
ssize_t n = syscall(LUX9_SYS_READ, fd, buf, count);

// Lux9 features
void *wasm = syscall(LUX9_SYS_WASM_COMPILE, data, size);
void *mem = syscall(LUX9_SYS_MMAP, addr, len, prot, flags);

// Memory subsystem
void *mem = syscall(LUX9_SYS_MALLOC, size, alignment);
```

## Performance Characteristics

### Syscall Categories

**Fast (< 1µs):**
- Basic file I/O: read, write, open, close
- Process management: getpid, getppid
- Memory operations: malloc, free, mmap, munmap

**Normal (< 10µs):**
- Complex I/O: pread, pwrite, seek
- Process control: fork, exec, wait
- Lux9 features: wasm compile, exchange operations

**Slow (> 10µs):**
- File system operations: mount, unmount
- Advanced features: container create, VM operations
- Network operations: socket, bind, connect

### Fast Path Optimization

The dispatcher includes fast paths for the most common syscalls:
```c
// Direct handling for read, write, open, close
if (syscall_num <= 20) {
    return handle_plan9_fast_path(syscall_num, args, result);
}
```

## Testing

### Unit Tests
```bash
# Build and run tests
make test

# Test specific functionality
./test_syscall_validation
./test_compatibility_layer
./test_performance
```

### Integration Tests
```bash
# Full system test
./test_syscall_integration

# Test all compatibility layers
./test_compatibility_complete
```

## Building

```bash
# Build syscall library
make

# Build with debugging
make CFLAGS="-DLUX9_SYSCALL_DEBUG=1"

# Install headers and library
make install

# Clean build artifacts
make clean
```

## Troubleshooting

### Common Issues

**1. Syscall not implemented (ENOSYS)**
```c
// Check if syscall is valid
if (!LUX9_SYSCALL_IS_VALID(syscall_num)) {
    printf("Invalid syscall number: %u\n", syscall_num);
    return -1;
}

// Check if handler is registered
if (lux9_syscall_get_handler(syscall_num) == NULL) {
    printf("Syscall %u not registered\n", syscall_num);
    return -1;
}
```

**2. Performance issues**
```c
// Check statistics
struct lux9_syscall_stats stats;
lux9_syscall_get_stats(LUX9_SYS_READ, &stats);
printf("Average latency: %u ns\n", stats.avg_time_ns);

// Enable tracing
lux9_syscall_trace_enable(LUX9_SYS_READ);
```

**3. Compatibility issues**
```c
// Verify compatibility mappings
assert(LUX9_SYS_READ == PREAD);
assert(LUX9_SYS_WASM_COMPILE == SYS_WASM_COMPILE);

// Test translation
uint32_t translated;
assert(rump_syscall_translate(3000, &translated) == 0);
```

### Debug Commands

```bash
# Print syscall table
lux9_syscall_print_table()

# Get global statistics
lux9_syscall_get_global_stats()

# Validate syscall table
lux9_syscall_table_validate()

# Trace specific syscall
lux9_syscall_trace_syscall(LUX9_SYS_READ, 1)
```

## Architecture

```
┌─────────────────────────────────────┐
│        User Space                   │
│  ┌─────────────────────────────┐    │
│  │  syscall() Wrapper          │    │
│  └─────────────────────────────┘    │
└──────────────┬──────────────────────┘
               │
┌──────────────▼──────────────────────┐
│     Unified Dispatcher               │
│  ┌─────────────────────────────┐    │
│  │  Fast Path Check            │    │
│  │  (< 1µs for common calls)  │    │
│  └─────────────────────────────┘    │
│  ┌─────────────────────────────┐    │
│  │  Compatibility Translation  │    │
│  │  (Plan 9, Lux9, RUMP)      │    │
│  └─────────────────────────────┘    │
│  ┌─────────────────────────────┐    │
│  │  Syscall Table Lookup       │    │
│  └─────────────────────────────┘    │
└──────────────┬──────────────────────┘
               │
┌──────────────▼──────────────────────┐
│     Handler Execution               │
│  ┌─────────────┬──────────────┐     │
│  │ Plan 9     │ Lux9 Native  │     │
│  │ Handler     │ Handler      │     │
│  └─────────────┴──────────────┘     │
│  ┌─────────────┬──────────────┐     │
│  │ Extended    │ Subsystem    │     │
│  │ Handler     │ Handler      │     │
│  └─────────────┴──────────────┘     │
└─────────────────────────────────────┘
```

## Future Extensions

### Planned Subsystems
- **Network (1100-1199)**: Socket operations, network namespaces
- **Security (1200-1299)**: Capabilities, authentication
- **Graphics (1300-1399)**: GPU operations, display management
- **Audio (1400-1499)**: Audio device access, sound processing

### Extension API
```c
// Register custom subsystem
lux9_subsystem_register(2000, 100, "custom", subsystem_entries);

// Add new syscall
lux9_syscall_register(&(struct lux9_syscall_entry){
    .syscall_number = 2000,
    .syscall_name = "CUSTOM_OP",
    .handler = custom_op_handler,
    .flags = LUX9_SYSCALL_EXTENDED | LUX9_SYSCALL_FAST,
    .description = "Custom Operation"
});
```

## Documentation

- **IMPLEMENTATION_GUIDE.md** - Detailed implementation guide
- **lux9_syscall.h** - Complete API documentation
- **compat_*.h** - Compatibility layer documentation

## License

This is part of the Lux9 kernel project.

## Support

For issues, questions, or contributions:
- Check the implementation guide
- Review the test suite
- Examine the syscall table
- Enable debugging output

---

**Status**: Production Ready
**Version**: 1.0
**Last Updated**: 2026-01-12
