# Lux9 Unified System Call Management - Implementation Guide

## Overview

This document provides a comprehensive implementation guide for the Lux9 Unified System Call Management system, which resolves conflicts between Plan 9, Lux9 native, and RUMP/NetBSD syscall numbering schemes.

## Core Design Principles

### 1. Hierarchical Numbering Scheme

The system uses a hierarchical numbering scheme to prevent conflicts:

```
Range 0-99:     Plan 9 compatibility layer
Range 100-199:  Lux9 native operations
Range 200-299:  Extended syscalls
Range 1000+:    Subsystem-specific operations
```

### 2. Backward Compatibility

All existing code continues to work without modification through compatibility layers:

- Plan 9 code: Uses existing syscall numbers (0-53)
- Lux9 code: Uses mapped syscall numbers (163-165 for WASM, etc.)
- RUMP code: Uses translated syscall numbers (3000+)

### 3. Performance Optimization

- Fast path for common syscalls (< 1µs)
- Direct dispatch for compatibility layer
- Minimized overhead for hot paths
- Efficient syscall table lookup

### 4. Maintainability

- Clear organization by subsystem
- Consistent naming conventions
- Comprehensive validation
- Extensive debugging support

## Architecture Components

### Header Files

1. **lux9_syscall.h** - Main unified interface
2. **compat_plan9.h** - Plan 9 compatibility layer
3. **compat_lux9_native.h** - Lux9 native compatibility
4. **compat_rump.h** - RUMP/NetBSD compatibility
5. **subsys_memory.h** - Memory subsystem syscalls
6. **internal.h** - Internal definitions

### Implementation Files

1. **lux9_syscall_dispatch.c** - Core dispatcher
2. **lux9_syscall_table.c** - Syscall table definition
3. **lux9_syscall_compat.c** - Compatibility layers
4. **lux9_syscall_stats.c** - Statistics and monitoring

## Key Conflict Resolution

### The WASM/Pebble Collision (63-65)

**Problem:**
- Original Lux9 syscalls: SYS_WASM_COMPILE=63, SYS_WASM_EXECUTE=64, SYS_WASM_DESTROY=65
- These conflicted with potential Plan 9 extensions

**Solution:**
- New unified numbers: LUX9_SYS_WASM_COMPILE=163, LUX9_SYS_WASM_EXECUTE=164, LUX9_SYS_WASM_DESTROY=165
- Legacy constants maintained for compatibility
- Migration path provided

### Plan 9 Compatibility

**Approach:**
- Direct mapping of Plan 9 syscall numbers
- Maintain exact numeric compatibility
- Preserve existing semantics

**Example:**
```c
// Old Plan 9 code (still works)
fd = syscall(PREAD, fd, buffer, count, offset);

// New recommended way
fd = syscall(LUX9_SYS_PREAD, fd, buffer, count, offset);

// Both resolve to same unified syscall
```

### RUMP Translation Layer

**Strategy:**
- Separate numbering space (3000+)
- Automatic translation
- Hypervisor-mediated implementation
- Graceful degradation when hypervisor unavailable

## Implementation Strategy

### 1. Dispatch Mechanism

The syscall dispatcher works as follows:

```
User Call → Syscall Number → Validation → Routing → Handler → Result
                ↓
        Fast Path Check
                ↓
        Compatibility Layer
                ↓
        Unified Dispatcher
                ↓
        Handler Execution
```

**Fast Path Optimization:**
- Direct handling for common syscalls (read, write, open, close)
- Bypasses full dispatch for frequently used operations
- Maintains performance for hot paths

### 2. Translation Layer

**Compatibility Translation:**
```c
// Automatic translation of legacy numbers
int legacy_syscall(uint32_t old_num) {
    uint32_t new_num = translate_legacy_number(old_num);
    if (new_num != old_num) {
        log_translation(old_num, new_num);
    }
    return lux9_syscall_dispatch(new_num, ...);
}
```

### 3. Performance Considerations

**Optimization Techniques:**
- Inline validation for common paths
- Direct function calls for compatibility layer
- Optimized syscall table lookup
- Minimal copying of arguments

**Performance Targets:**
- Fast syscalls: < 1µs
- Normal syscalls: < 10µs
- Slow syscalls: < 100µs
- Compatibility overhead: < 5%

### 4. Migration Path

**Phase 1: Dual Support (Current)**
- Maintain all legacy syscall numbers
- Provide new unified constants
- Log usage of deprecated numbers

**Phase 2: Deprecation Warnings**
- Emit warnings for deprecated syscalls
- Suggest migration to unified numbers
- Maintain full compatibility

**Phase 3: Legacy Removal (Future)**
- Remove support for deprecated numbers
- Keep compatibility layer for critical syscalls
- Update documentation and examples

## Header File Usage

### 1. Main Interface (lux9_syscall.h)

```c
#include <lux9/syscall/lux9_syscall.h>

// Use unified syscall numbers
int fd = syscall(LUX9_SYS_OPEN, "/path/to/file", O_RDONLY);
ssize_t n = syscall(LUX9_SYS_READ, fd, buffer, count);
```

### 2. Plan 9 Compatibility (compat_plan9.h)

```c
#include <lux9/syscall/compat_plan9.h>

// Use Plan 9 numbers directly
int fd = syscall(OPEN, "/path/to/file", O_RDONLY);
ssize_t n = syscall(READ, fd, buffer, count);
```

### 3. Lux9 Native Compatibility (compat_lux9_native.h)

```c
#include <lux9/syscall/compat_lux9_native.h>

// Use mapped Lux9 numbers
void *wasm = syscall(SYS_WASM_COMPILE, wasm_data, wasm_size);
```

### 4. Subsystem Headers

```c
#include <lux9/syscall/subsys_memory.h>

// Use memory subsystem syscalls
void *ptr = syscall(LUX9_SYS_MMAP, NULL, 4096, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
```

## Integration Points

### 1. Kernel Integration

**Syscall Entry Point:**
```c
// In kernel syscall handler
int handle_lux9_syscall(uint32_t syscall_number, uintptr_t args[]) {
    uintptr_t result;
    
    // Use unified dispatcher
    int ret = lux9_syscall_dispatch(syscall_number, args, 6, &result);
    
    return ret;
}
```

**Compatibility Layer Registration:**
```c
// During kernel initialization
lux9_syscall_register_compat();
lux9_syscall_register_memory_subsystem();
lux9_syscall_register_network_subsystem();
```

### 2. Userspace Integration

**Library Integration:**
```c
// In libc implementation
int syscall(uintptr_t number, ...) {
    va_list args;
    va_start(args, number);
    
    uintptr_t arg_array[6];
    for (int i = 0; i < 6; i++) {
        arg_array[i] = va_arg(args, uintptr_t);
    }
    
    return lux9_syscall_dispatch(number, arg_array, 6, NULL);
}
```

### 3. Build System Integration

**Adding to Kernel Build:**
```makefile
# In kernel Makefile
SYSCALL_DIR = src/sys/lux9/syscall
SYSCALL_CSRCS = $(SYSCALL_DIR)/lux9_syscall_dispatch.c
SYSCALL_CSRCS += $(SYSCALL_DIR)/lux9_syscall_table.c
SYSCALL_CSRCS += $(SYSCALL_DIR)/lux9_syscall_compat.c

KERNEL_OBJS += $(SYSCALL_CSRCS:.c=.o)

# Include headers
INCLUDES += -I$(SYSCALL_DIR)
```

**Adding to Userspace Library:**
```makefile
# In libc Makefile
SYSCALL_LDFLAGS = -L$(SYSCALL_DIR) -llux9_syscall

# Link with syscall library
$(PROGRAM): $(OBJS) | $(SYSCALL_LDFLAGS)
    $(CC) $(LDFLAGS) -o $@ $^ $(SYSCALL_LDFLAGS)
```

## Testing and Validation

### 1. Unit Tests

**Syscall Number Validation:**
```c
void test_syscall_numbers(void) {
    // Test valid numbers
    assert(LUX9_SYSCALL_IS_VALID(LUX9_SYS_READ));
    assert(LUX9_SYSCALL_IS_VALID(LUX9_SYS_WASM_COMPILE));
    assert(LUX9_SYSCALL_IS_VALID(LUX9_SYS_MMAP));
    
    // Test invalid numbers
    assert(!LUX9_SYSCALL_IS_VALID(5000));
    assert(!LUX9_SYSCALL_IS_VALID((uint32_t)-1));
}
```

**Compatibility Layer:**
```c
void test_compatibility_layer(void) {
    // Test Plan 9 compatibility
    assert(LUX9_SYS_READ == PREAD);
    assert(LUX9_SYS_WRITE == PWRITE);
    
    // Test Lux9 native compatibility
    assert(LUX9_SYS_WASM_COMPILE == SYS_WASM_COMPILE);
    
    // Test RUMP translation
    assert(rump_translate(RUMP_SYS_READ) == LUX9_RUMP_SYS_READ);
}
```

### 2. Integration Tests

**End-to-End Syscall Testing:**
```c
void test_syscall_integration(void) {
    // Test Plan 9 compatibility
    int fd = syscall(OPEN, "/tmp/test", O_CREAT|O_WRONLY, 0644);
    assert(fd >= 0);
    
    const char *msg = "Hello, Lux9!\n";
    syscall(WRITE, fd, msg, strlen(msg));
    syscall(CLOSE, fd);
    
    // Test Lux9 native
    void *ptr = syscall(SYS_WASM_COMPILE, wasm_data, wasm_size);
    assert(ptr != NULL);
    
    // Test memory subsystem
    void *mem = syscall(LUX9_SYS_MMAP, NULL, 4096, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
    assert(mem != MAP_FAILED);
}
```

### 3. Performance Tests

**Latency Measurement:**
```c
void test_syscall_latency(void) {
    struct timespec start, end;
    
    clock_gettime(CLOCK_MONOTONIC, &start);
    
    for (int i = 0; i < 1000000; i++) {
        syscall(LUX9_SYS_GETPID);
    }
    
    clock_gettime(CLOCK_MONOTONIC, &end);
    
    uint64_t total_ns = (end.tv_sec - start.tv_sec) * 1000000000 + 
                       (end.tv_nsec - start.tv_nsec);
    uint64_t avg_ns = total_ns / 1000000;
    
    // Should be < 1µs per call
    assert(avg_ns < 1000);
}
```

## Migration Guide

### For Existing Plan 9 Code

**No Changes Required:**
- All existing Plan 9 syscalls continue to work
- PREAD, PWRITE, OPEN, CLOSE, etc. are unchanged
- Performance is maintained through fast path

**Optional Optimization:**
```c
// Optional: migrate to unified numbers
// Before:
fd = syscall(PREAD, fd, buf, count, offset);

// After:
fd = syscall(LUX9_SYS_PREAD, fd, buf, count, offset);
```

### For Existing Lux9 Code

**Critical Update Required:**
```c
// Before (legacy numbers):
void *wasm = syscall(SYS_WASM_COMPILE, data, size);
syscall(SYS_EXCHANGE_PUBLISH, topic, msg, len);

// After (unified numbers):
void *wasm = syscall(LUX9_SYS_WASM_COMPILE, data, size);
syscall(LUX9_SYS_EXCHANGE_PUBLISH, topic, msg, len);

// Or using compatibility layer (still works):
void *wasm = syscall(SYS_WASM_COMPILE, data, size);  // Maps to 163
syscall(SYS_EXCHANGE_PUBLISH, topic, msg, len);      // Maps to 173
```

### For RUMP Code

**Automatic Translation:**
```c
// RUMP code works unchanged
int fd = rump_syscall(RUMP_SYS_OPEN, "file", O_RDONLY);
// Automatically translated to unified number
```

## Error Handling

### Error Code Mapping

```c
// Unified error codes
typedef enum {
    LUX9_E_SUCCESS = 0,
    LUX9_E_INVAL,
    LUX9_E_NOENT,
    LUX9_E_ACCES,
    LUX9_E_NOMEM,
    LUX9_E_FAULT,
    LUX9_E_BUSY,
    LUX9_E_NOTSUP,
} lux9_error_t;

// Convert to platform error codes
int lux9_error_to_errno(lux9_error_t err) {
    switch (err) {
        case LUX9_E_SUCCESS: return 0;
        case LUX9_E_INVAL: return EINVAL;
        case LUX9_E_NOENT: return ENOENT;
        case LUX9_E_ACCES: return EACCES;
        case LUX9_E_NOMEM: return ENOMEM;
        case LUX9_E_FAULT: return EFAULT;
        case LUX9_E_BUSY: return EBUSY;
        case LUX9_E_NOTSUP: return ENOSYS;
        default: return EINVAL;
    }
}
```

### Error Recovery

```c
int robust_syscall(uint32_t num, uintptr_t args[]) {
    uintptr_t result;
    
    int ret = lux9_syscall_dispatch(num, args, 6, &result);
    
    if (ret < 0) {
        // Log error for debugging
        log_syscall_error(num, ret);
        
        // Attempt recovery if possible
        if (ret == LUX9_E_NOMEM) {
            // Try to free cache and retry
            lux9_syscall_gc();
            ret = lux9_syscall_dispatch(num, args, 6, &result);
        }
    }
    
    return ret;
}
```

## Debugging and Monitoring

### Syscall Tracing

```c
// Enable tracing
lux9_syscall_trace_enable();

// Trace specific syscall
lux9_syscall_trace_syscall(LUX9_SYS_READ, 1);

// Trace all syscalls in category
lux9_syscall_trace_category(LUX9_SYSCALL_NATIVE, 1);

// Print trace
lux9_syscall_trace_print();
```

### Statistics Collection

```c
// Get syscall statistics
struct lux9_syscall_stats stats;
lux9_syscall_get_stats(LUX9_SYS_READ, &stats);

printf("Syscall READ statistics:\n");
printf("  Calls: %lu\n", stats.call_count);
printf("  Errors: %lu\n", stats.error_count);
printf("  Avg time: %u ns\n", stats.avg_time_ns);

// Get global statistics
lux9_syscall_get_stats(0, &stats);
```

### Debug Output

```c
// Enable debug mode
lux9_syscall_debug_enable();

// Print syscall table
lux9_syscall_print_table();

// Validate syscall table
if (lux9_syscall_table_validate() != 0) {
    printf("Syscall table validation failed!\n");
}
```

## Best Practices

### 1. Syscall Selection

**Use Unified Numbers:**
```c
// Good: Use LUX9_SYS_* constants
syscall(LUX9_SYS_READ, fd, buf, count);

// Avoid: Use compatibility constants
syscall(READ, fd, buf, count);  // Still works but less clear
```

**Use Subsystem Headers:**
```c
// Good: Include subsystem-specific headers
#include <lux9/syscall/subsys_memory.h>
syscall(LUX9_SYS_MMAP, ...);

// Avoid: Hardcode subsystem numbers
syscall(1000, ...);  // Unclear and fragile
```

### 2. Error Handling

**Always Check Return Values:**
```c
int fd = syscall(LUX9_SYS_OPEN, path, flags, mode);
if (fd < 0) {
    // Handle error appropriately
    return -1;
}
```

**Use Unified Error Handling:**
```c
int ret = syscall(LUX9_SYS_WASM_COMPILE, data, size);
if (ret == LUX9_E_NOTSUP) {
    // WASM not supported, fallback to interpretation
    return interpret_wasm(data, size);
}
```

### 3. Performance

**Batch Operations When Possible:**
```c
// Good: Use scatter-gather I/O
struct iovec iov[2] = {{buf1, len1}, {buf2, len2}};
syscall(LUX9_SYS_IOVEC_READ, fd, iov, 2);

// Avoid: Multiple small reads
for (i = 0; i < count; i++) {
    syscall(LUX9_SYS_READ, fd, &buf[i], 1);
}
```

**Use Fast Paths When Available:**
```c
// Good: Use fast syscall when possible
syscall(LUX9_SYS_GETPID);  // Fast path optimized

// Avoid: Unnecessary overhead
syscall(LUX9_SYS_GETPID2); // More features, slower
```

## Troubleshooting

### Common Issues

**Issue 1: Syscall Not Implemented**
```
Error: ENOSYS (Function not implemented)
```
**Solution:**
- Check if syscall number is valid
- Verify compatibility layer is loaded
- Ensure subsystem is registered

**Issue 2: Invalid Syscall Number**
```
Error: EINVAL (Invalid argument)
```
**Solution:**
- Validate syscall number with LUX9_SYSCALL_IS_VALID()
- Check for typos in syscall names
- Verify header files are included correctly

**Issue 3: Performance Degradation**
```
Syscalls slower than expected
```
**Solution:**
- Check if using fast paths
- Verify compatibility layer overhead
- Monitor with lux9_syscall_get_stats()

### Debug Commands

```bash
# Print syscall table
lux9_syscall_print_table

# Enable tracing
echo "trace on" > /proc/lux9_syscall

# Get statistics
cat /proc/lux9_syscall/stats

# Validate table
lux9_syscall_table_validate
```

## Future Extensions

### Planned Features

1. **Additional Subsystems:**
   - Graphics subsystem (1300-1399)
   - Audio subsystem (1400-1499)
   - Network subsystem (1100-1199)
   - Security subsystem (1200-1299)

2. **Performance Enhancements:**
   - JIT compilation for hot syscalls
   - Lock-free syscall fast paths
   - Adaptive syscall caching

3. **Debugging Improvements:**
   - Real-time syscall profiling
   - Advanced tracing with filters
   - Integration with systemtap/dtrace

4. **Compatibility:**
   - Linux syscall compatibility layer
   - Windows subsystem translation
   - BSD compatibility

### Extension API

```c
// Register custom syscall subsystem
int lux9_subsystem_register(uint32_t base, uint32_t count, 
                          const char *name, 
                          struct lux9_syscall_entry *entries);

// Create new syscall
#define LUX9_SYS_CUSTOM_OP  250

int custom_op_handler(uintptr_t args[], size_t count, uintptr_t *result) {
    // Implementation
    return LUX9_E_SUCCESS;
}

// Register it
lux9_syscall_register(&(struct lux9_syscall_entry){
    .syscall_number = LUX9_SYS_CUSTOM_OP,
    .syscall_name = "CUSTOM_OP",
    .handler = custom_op_handler,
    .flags = LUX9_SYSCALL_EXTENDED | LUX9_SYSCALL_FAST,
    .description = "Custom Operation"
});
```

## Conclusion

The Lux9 Unified System Call Management system provides a comprehensive solution to syscall numbering conflicts while maintaining backward compatibility and optimizing performance. The hierarchical design ensures scalability for future extensions, while the compatibility layers allow seamless migration from existing code.

Key benefits:
- **No Breaking Changes**: All existing code continues to work
- **Clear Organization**: Hierarchical numbering prevents future conflicts
- **Performance**: Optimized fast paths for common operations
- **Maintainability**: Well-structured, documented, and tested
- **Extensibility**: Easy to add new subsystems and features

The system is production-ready and provides a solid foundation for the Lux9 kernel's system call infrastructure.
