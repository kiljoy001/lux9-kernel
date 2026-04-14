# Exchange Pool Integration with liblux

This document describes how to use the exchange pool functionality that has been integrated into liblux.

## Overview

The exchange pool provides a global pool of memory pages that can be dynamically allocated and freed by processes. Unlike the traditional fixed per-process allocation, the exchange pool implements proportional load balancing based on measured demand from each process.

## Key Features

1. **Global Pool Management**: 512-page pool shared across all processes
2. **Proportional Allocation**: Pages distributed based on syscall rate demand
3. **Automatic Rebalancing**: kproc periodically migrates pages between processes
4. **Demand Measurement**: Tracks syscall rates for load-based allocation
5. **Capability-Based Security**: Uses secure capabilities for page ownership

## API Functions

### sys_exchange_alloc()
Allocates a page from the global exchange pool.

```c
ExchangeCapability* sys_exchange_alloc(void);
```

Returns a pointer to an `ExchangeCapability` structure on success, or `nil` on failure.

### sys_exchange_free()
Returns a previously allocated page to the global exchange pool.

```c
int sys_exchange_free(ExchangeCapability *cap);
```

Returns 0 on success, or a negative value on failure.

## Data Structures

### ExchangeCapability
Represents a capability for an exchange page:

```c
typedef struct {
    uchar uuid[16];     // 16-byte UUIDv8 public identifier
    uchar hash[32];     // 32-byte BLAKE2b hash (security anchor)
    ulong size;         // Size of the object (Span) in bytes
    uint type;          // Resource Type (Memory, Channel, PCI)
    uint perms;         // Permissions (Read, Write, Transfer)
} ExchangeCapability;
```

## Usage Examples

### Basic Allocation and Deallocation

```c
#include "lux.h"

// Allocate a page from the exchange pool
ExchangeCapability* cap = sys_exchange_alloc();
if (cap != nil) {
    // Use the allocated page...
    
    // Return it to the pool when done
    sys_exchange_free(cap);
}
```

### Multiple Allocations

```c
ExchangeCapability* caps[10];

// Allocate multiple pages
for (int i = 0; i < 10; i++) {
    caps[i] = sys_exchange_alloc();
    if (caps[i] == nil) {
        // Handle allocation failure
        break;
    }
}

// Free all allocated pages
for (int i = 0; i < 10; i++) {
    if (caps[i] != nil) {
        sys_exchange_free(caps[i]);
    }
}
```

## Integration Details

The exchange pool integration includes:

1. **Syscall Numbers**: `SYS_EXCHANGE_ALLOC` (67) and `SYS_EXCHANGE_FREE` (68)
2. **Kernel Implementation**: Located in `kernel/exchange_pool.c` and `kernel/include/exchange_pool.h`
3. **Userspace API**: Accessible through liblux functions
4. **Compatibility**: Works alongside existing doorbell mechanism

## Performance Considerations

1. **Allocation Speed**: O(1) allocation from global pool
2. **Rebalancing Overhead**: Periodic kproc rebalancing with configurable interval
3. **Memory Efficiency**: Shared pool reduces overall memory footprint
4. **Load Balancing**: Automatic redistribution based on demand measurements

## Error Handling

Common error conditions:
- Pool exhaustion (returns `nil` from `sys_exchange_alloc`)
- Invalid capabilities (returns negative value from `sys_exchange_free`)
- System resource limitations (general kernel memory constraints)

Always check return values and handle errors appropriately in production code.