# Page Ownership and Borrow Checker System

## Overview

The Lux9 kernel implements a Rust-style borrow checker for VM page ownership tracking, providing memory safety for zero-copy IPC operations. This system prevents use-after-free, double-free, and data race bugs by enforcing strict ownership semantics at runtime.

## Architecture

### Core Components

1. **Borrow Checker (`borrowchecker.c`)** - Generic ownership tracking system using hash table
2. **Page Ownership Wrapper (`pageown.c`)** - Plan 9-compatible API layer  
3. **System Calls (`syspageown.c`)** - User interface for page exchange operations

### Ownership States

The system enforces these mutually exclusive states for each tracked resource:

- **FREE** (`BORROW_FREE`) - Unowned, available for acquisition
- **EXCLUSIVE** (`BORROW_EXCLUSIVE`) - Owned exclusively by one process (moved ownership)
- **SHARED_OWNED** (`BORROW_SHARED_OWNED`) - Owner has page, lent as shared references (&)
- **MUT_LENT** (`BORROW_MUT_LENT`) - Owner lent page as mutable reference, blocked (&mut)

## API Reference

### Core Ownership Operations

```c
// Acquire exclusive ownership (like: let x = Page::new())
enum BorrowError borrow_acquire(Proc *p, uintptr key);

// Release ownership (like: drop(x))
enum BorrowError borrow_release(Proc *p, uintptr key);

// Transfer ownership between processes (like: let y = x;)
enum BorrowError borrow_transfer(Proc *from, Proc *to, uintptr key);
```

### Borrow Operations (Rust-style)

```c
// Lend as shared read-only (like: let y = &x;)
enum BorrowError borrow_borrow_shared(Proc *owner, Proc *borrower, uintptr key);

// Lend as exclusive read-write (like: let y = &mut x;)
enum BorrowError borrow_borrow_mut(Proc *owner, Proc *borrower, uintptr key);

// Return shared borrow (like: drop(y); where y was &x)
enum BorrowError borrow_return_shared(Proc *borrower, uintptr key);

// Return mutable borrow (like: drop(y); where y was &mut x)
enum BorrowError borrow_return_mut(Proc *borrower, uintptr key);
```

### Query Operations

```c
// Check if resource is owned
int borrow_is_owned(uintptr key);

// Get current owner
Proc* borrow_get_owner(uintptr key);

// Get ownership state
enum BorrowState borrow_get_state(uintptr key);

// Check if shared borrow is possible
int borrow_can_borrow_shared(uintptr key);

// Check if mutable borrow is possible
int borrow_can_borrow_mut(uintptr key);
```

## Plan 9 Compatibility

To maintain compatibility with existing 9front code while providing modern memory safety, the system provides a thin wrapper layer:

### Legacy API (`pageown.h`)

The original `pageown_*` functions forward directly to their `borrow_*` counterparts:

- `pageown_acquire()` → `borrow_acquire()`
- `pageown_release()` → `borrow_release()`
- `pageown_transfer()` → `borrow_transfer()`
- etc.

### `pa2owner` Adapter

The legacy `pa2owner()` function is implemented as an adapter that:

1. Looks up the `BorrowOwner` entry for a physical address
2. Creates a temporary `PageOwner` struct populated with current state
3. Returns a pointer to per-CPU static buffer (avoiding stack allocation issues)
4. Uses locking to ensure consistent snapshot

This preserves the exact Plan 9 API while the real ownership tracking happens in the generic borrow checker.

## Zero-Copy IPC Implementation

The borrow checker enables safe zero-copy IPC through page exchange system calls:

### `sysvmexchange()` - Move Semantics
Transfers exclusive ownership from one process to another:
- Source process loses ALL access (page unmapped)
- Target process gains exclusive ownership
- Equivalent to Rust move semantics: `let b = a;`

### `sysvmlend_shared()` - Shared Borrowing
Lends page as read-only to target process:
- Source process keeps ownership but becomes read-only
- Multiple processes can hold shared borrows
- Equivalent to Rust shared reference: `fn read(data: &Vec<u8>)`

### `sysvmlend_mut()` - Mutable Borrowing
Lends page as exclusive read-write to target process:
- Source process loses ALL access temporarily
- Target process has exclusive access
- Only one mutable borrow allowed at a time
- Equivalent to Rust mutable reference: `fn modify(data: &mut Vec<u8>)`

### `sysvmreturn()` - Borrow Return
Returns borrowed pages to owner:
- For shared borrows: decrements borrow count
- For mutable borrows: returns exclusive access
- Equivalent to Rust scope end: `}`

## Safety Guarantees

### Compile-Time Equivalents
While C doesn't have compile-time borrow checking, the runtime system enforces the same rules:
- **No data races** - At most one mutable reference XOR multiple shared references
- **No use-after-free** - Resources can only be accessed by their current owner/borrowers
- **No double-free** - Ownership must be released exactly once
- **Memory safety** - All accesses verified through ownership tracking

### Race Prevention
The borrow checker uses a global spinlock to serialize all ownership operations, ensuring:
- Atomic state transitions
- Consistent ownership views
- Prevention of race conditions in ownership tracking

## Performance Considerations

### Hash Table Implementation
- Fixed-size hash table with chaining
- Key-based lookup for O(1) average case
- Per-bucket locking could be added for scalability

### Memory Overhead
- Minimal per-resource tracking overhead
- Static hash table allocation
- Per-CPU buffers for temporary structs

### Locking Strategy
- Single global lock for simplicity and correctness
- Brief hold times for minimal contention
- Future work: fine-grained locking for better scalability

## Integration with Kernel Memory Management

### Page Allocation
New pages automatically acquire ownership for allocating process:
```c
p = newpage(0, 0, color);
pageown_acquire(up, p->pa, hhdm_va);
```

### Page Deallocation
Pages automatically release ownership when freed:
```c
pageown_release(up, p->pa);
putpage(p);
```

### Process Cleanup
When processes exit, all owned resources are automatically cleaned up:
```c
pageown_cleanup_process(up);
```

This ensures no resource leaks even in error conditions.

## Debugging and Monitoring

### Statistics
```c
void borrow_stats(void);  // Print ownership tracking statistics
```

### Resource Inspection
```c
void borrow_dump_resource(uintptr key);  // Debug individual resource
```

### System Call Interface
The `sysvmowninfo()` system call provides ownership information for debugging:
- Owner PID
- Current state
- Shared borrow count
- Mutable borrower PID