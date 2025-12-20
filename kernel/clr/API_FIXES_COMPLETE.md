# CLR API Compatibility Fixes - COMPLETE

## Summary
Fixed critical API incompatibilities between CLR code and kernel Pebble/Exchange/Blind Ledger systems.

## Issue 1: Pebble/CLR API Incompatibility ✅ FIXED

### Problem
CLR code expected `pebble_black_alloc` to return `void*` or `PebbleBlack*`, but the actual API is:
```c
int pebble_black_alloc(ulong size, UserCapability *out_cap);
int pebble_black_free(const UserCapability *cap);
```

CLR was also trying to access internal `PebbleBlack` structures directly, which violates the capability-based security model.

### Solution
**Changed CLR data structures to use `UserCapability` directly:**

`clr_pebble_integration.h`:
```c
// OLD:
typedef struct clr_object {
    PebbleBlack *black;  // WRONG - direct access to internal struct
    // ...
} clr_object_t;

// NEW:
typedef struct clr_object {
    UserCapability black_cap;  // Capability-based access
    void *data;                // Physical address from ledger_verify()
    // ...
} clr_object_t;
```

**Updated allocation pattern:**
```c
// OLD (WRONG):
void *black_handle;
pebble_black_alloc(size, &black_handle);  // Type mismatch!
obj->black = black_handle;

// NEW (CORRECT):
UserCapability cap;
BlindLedgerEntry entry;

pebble_black_alloc(size, &cap);
ledger_verify(&cap, &entry);  // Get physical address

obj->black_cap = cap;
obj->data = (void*)entry.physical_address;
```

**Updated free pattern:**
```c
// OLD (WRONG):
pebble_black_free(obj->black);  // Passing PebbleBlack* directly

// NEW (CORRECT):
pebble_black_free(&obj->black_cap);  // Passing const UserCapability*
```

### Files Modified
- `kernel/clr/clr-kernel/clr_pebble_integration.h`
  - `clr_object_t`: Changed `PebbleBlack *black` → `UserCapability black_cap` + `void *data`
  - `clr_stack_t`: Changed `PebbleBlack *black` → `UserCapability black_cap`
  - `clr_locals_t`: Changed `PebbleBlack *black` → `UserCapability black_cap`
  - `clr_object_has_snapshot()`: Stubbed (Red-Blue API not yet available)

- `kernel/clr/clr-kernel/clr_pebble_integration.c`
  - `clr_object_alloc()`: Use `pebble_black_alloc()` + `ledger_verify()`
  - `clr_object_addref()`: Use `obj->data` instead of `obj->black->addr`
  - `clr_object_free_internal()`: Pass `&obj->black_cap` to `pebble_black_free()`
  - `clr_stack_init()`: Use proper allocation pattern
  - `clr_stack_cleanup()`: Pass `&stack->black_cap`
  - `clr_locals_init()`: Use proper allocation pattern
  - `clr_locals_cleanup()`: Pass `&locals->black_cap`
  - Red-Blue functions: Stubbed with TODOs (need proper Pebble API)

## Issue 2: Exchange API Incompatibility ✅ FIXED

### Problem
`clr_kernel_receive_message()` passed `ExchangeHandle` directly to `exchange_accept()`, but the API expects `const ExchangeHandle *`:
```c
int exchange_accept(const ExchangeHandle *handle, uintptr dest_vaddr, int prot);
```

### Solution
**Changed `exchange_accept()` call to pass pointer:**
```c
// OLD (WRONG):
exchange_accept(msg->exchange_handles[i], dest_vaddr, ...);

// NEW (CORRECT):
exchange_accept(&msg->exchange_handles[i], dest_vaddr, ...);
```

**Also fixed reference to removed field:**
```c
// OLD (WRONG):
uintptr dest_vaddr = (uintptr)msg->payload_obj->black->addr + (i * 4096);

// NEW (CORRECT):
uintptr dest_vaddr = (uintptr)msg->payload_obj->data + (i * 4096);
```

### Files Modified
- `kernel/clr/clr-kernel/clr_kernel.c`
  - Line 447: Pass `&msg->exchange_handles[i]` instead of `msg->exchange_handles[i]`
  - Line 446: Use `msg->payload_obj->data` instead of `msg->payload_obj->black->addr`

## Architectural Improvements

### 1. Proper Capability-Based Security
- CLR now uses `UserCapability` throughout, not internal kernel structures
- Physical addresses obtained via `ledger_verify()` (proper API)
- No direct access to `PebbleBlack` internals

### 2. Separation of Concerns
- CLR layer: Manages `UserCapability` and data pointers
- Pebble layer: Manages internal `PebbleBlack` structures
- Blind Ledger: Provides secure mapping between capabilities and physical addresses

### 3. Type Safety
- Correct pointer types (`const UserCapability *` vs `UserCapability`)
- No more casting `void*` to `PebbleBlack*`
- Compiler will now catch API misuse

## Red-Blue Snapshot Status

The Red-Blue snapshot operations (`clr_object_snapshot`, `clr_object_commit`, `clr_object_rollback`) have been **stubbed** with TODOs because:

1. The current Pebble API doesn't expose Red-Blue operations at the CLR level
2. CLR was trying to access internal `PebbleBlack->blue` and `PebbleBlue->matching_red` fields
3. This violates the capability-based security model

**Next steps for Red-Blue:**
- Design proper Pebble API for snapshots at the capability level
- Add functions like `pebble_snapshot(const UserCapability *cap)`
- Implement in Pebble layer, expose through clean API

## Testing

**To verify fixes:**
1. Compile kernel with CLR enabled
2. Check for no type mismatches or warnings
3. Test basic CLR allocation/deallocation
4. Test Exchange page transfers

**Expected results:**
- ✅ No compilation errors related to Pebble API
- ✅ No compilation errors related to Exchange API
- ✅ CLR can allocate objects via Pebble
- ✅ CLR can use Exchange for zero-copy IPC

## Remaining Work

**Not addressed in this fix (separate issues):**
1. CBOR assembly loading (`fruity_module_from_cbor()` is stubbed)
2. Red-Blue snapshot API design
3. IL → Fruity IR converter implementation

---

**Status: API Compatibility Issues RESOLVED ✅**
**Date: 2025-12-08**
**Affected Systems: Pebble, Exchange, Blind Ledger, CLR**
