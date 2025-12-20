# Pebble Token Economy - Circular Economy UTXO Model

**Date**: 2025-12-08
**Branch**: secure-ramdisk
**Commit**: a6cd4a74

## Summary

The Pebble system implements a **circular economy** for memory allocation using a UTXO-style (Unspent Transaction Output) token model. All memory allocations go through colored token state transitions, with the **colorless bank** acting as the central recycling pool. This prevents resource leaks and enables precise accounting of system memory.

## Token State Machine

```
                    ┌─────────────┐
                    │  COLORLESS  │ (Bank/Pool)
                    └──────┬──────┘
                           │ pebble_issue_white()
                           ▼
                    ┌─────────────┐
              ┌────▶│    WHITE    │◄────┐ (Speculative Reservation)
              │     └──────┬──────┘     │
              │            │             │ white_free()
              │            │             │
              │            │ pebble_white_verify() + pebble_black_alloc()
              │            ▼             │
              │     ┌─────────────┐     │
              │     │    BLACK    │◄────┼─────┐ (Hard Allocation)
              │     └──────┬──────┘     │     │
              │            │             │     │ pebble_black_free()
              │            │             │     │
              │            └──► block_io_begin()
              │                    │     │     │
              │                    ▼     │     │
              │             ┌─────────────┐   │
              │             │    BLUE     │◄──┼──┐ (Block I/O Working Buffer)
              │             └──────┬──────┘   │  │
              │                    │           │  │ pebble_blue_free()
              │                    │           │  │
              │             pebble_red_copy()  │  │
              │                    │           │  │
              │                    ▼           │  │
              │             ┌─────────────┐   │  │
              │             │     RED     │◄──┼──┼──┐ (Snapshot for Rollback)
              │             └─────────────┘   │  │  │
              │                                │  │  │ pebble_free_red()
              │                                │  │  │
              └────────────────────────────────┴──┴──┴──┘
                     ALL paths return to COLORLESS
```

## Token Colors and Their Roles

### COLORLESS (The Bank)

**What**: Unallocated memory budget available for allocation
**Tracking**: `PebbleState.black_budget` (bytes available)
**Source**: System physical memory pool
**Purpose**: Central recycling pool - all allocations come from here, all frees return here

**Properties**:
- Represents **uncolored** (uncommitted) memory
- Process requests budget from system
- Budget can be denied/policed via policy
- Acts as "UTXO set" in blockchain terminology

### WHITE (Speculative Reservation)

**What**: Promissory note for future allocation
**Tracking**: `PebbleWhite` token structure, `PebbleState.white_pending` (bytes)
**Purpose**: Allow applications to reserve memory ahead of use

**Transitions**:
- `COLORLESS → WHITE`: `pebble_issue_white(ps, data, size)`
- `WHITE → BLACK`: `pebble_white_verify(white, ...)` + `pebble_black_alloc(...)`
- `WHITE → COLORLESS`: Return unused white token (TODO: needs implementation)

**Use Case**:
```c
// App allocates 1MB buffer upfront
white_tokens[256] = request_tokens(1MB);  // Get 256 × 4KB white tokens

// Use tokens gradually as needed
for (i = 0; i < 256; i++) {
    if (need_page()) {
        pebble_black_alloc(4KB, white_tokens[i]);  // Spend white → black
    }
}

// Return unused whites back to colorless bank
return_unused_whites(white_tokens + 200, 56);  // Freed 56 unused tokens
```

**Key Insight**: White tokens are like Bitcoin UTXOs - signed but unbroadcasted transactions. They can be held, transferred, or returned unused.

### BLACK (Hard Allocation)

**What**: Actual physical memory allocation with capability
**Tracking**: `PebbleBlack` structure, `UserCapability`, Blind Ledger entry
**Purpose**: Normal memory allocations for processes

**Transitions**:
- `WHITE → BLACK`: Verify white token + allocate physical memory
- `BLACK → COLORLESS`: `pebble_black_free(cap)` - returns budget
- `BLACK → BLUE`: Begin block device I/O (TODO: needs API)

**Properties**:
- Backed by `xallocz()` physical memory
- Registered in Blind Ledger (cryptographic proof of ownership)
- Tracked by Borrow Checker (ownership enforcement)
- Has associated `UserCapability` (unforgeable token)

**Budget Accounting**:
```c
// Allocation
ps->white_pending -= size;
ps->white_verified--;
ps->black_budget -= size;
ps->black_inuse += size;

// Free
ps->black_budget += size;
ps->black_inuse -= size;
```

### BLUE (Block Device I/O Working Buffer)

**What**: Transactional working buffer for block device operations
**Tracking**: `PebbleBlue` structure, `PebbleState.blue_count`
**Purpose**: Mutable buffer during active block I/O with rollback safety

**Transitions**:
- `BLACK → BLUE`: Begin block I/O transaction (TODO: proper API)
- `BLUE → RED`: `pebble_red_copy(blue, &red)` - create snapshot
- `BLUE → COLORLESS`: Abort I/O, free buffer

**Use Case**:
```c
// Reading from disk with transaction safety
blue = alloc_blue_from_colorless(512KB);     // Working buffer
issue_disk_read(sector, blue->blue_data);    // Read into Blue
if (io_error || checksum_fail) {
    free_blue_to_colorless(blue);            // Abort
} else {
    red = snapshot_blue_to_red(blue);        // Success, snapshot for safety
}
```

**Current Implementation Issue**: Blue is auto-created by `pebble_black_alloc()` and shares the same physical memory as Black (`blue->blue_data = buf`). This needs redesign to make Blue a separate allocation.

### RED (Snapshot for Rollback)

**What**: Immutable snapshot of Blue data for rollback safety
**Tracking**: `PebbleRed` structure, `PebbleState.red_count`
**Purpose**: Safe backup copy in case Blue operation fails

**Transitions**:
- `BLUE → RED`: `pebble_red_copy(blue, &red)` - allocate + copy
- `RED → COLORLESS`: `pebble_free_red(red)` - discard snapshot

**Budget Accounting** (FIXED in commit 7704dfd2):
```c
// pebble_duplicate_blue() - Snapshot creation
lock(&pebble_global_lock);
ps->black_budget -= size;      // Consume from colorless
ps->black_inuse += size;
unlock(&pebble_global_lock);
red->red_data = xallocz(size, 1);  // Backed by budget

// pebble_free_red() - Snapshot discard
xfree(red->red_data);
lock(&pebble_global_lock);
ps->black_budget += size;      // Return to colorless
ps->black_inuse -= size;
unlock(&pebble_global_lock);
```

**Use Case**:
```c
// Block device write with rollback
blue = working_buffer;
red = snapshot(blue);                // Backup original
modify_blue_buffer(blue);            // Modify in-place
if (disk_write_success(blue)) {
    free_red_to_colorless(red);      // Commit: discard backup
} else {
    copy_red_to_blue(red, blue);     // Rollback: restore from backup
    free_red_to_colorless(red);
}
```

## Circular Economy Principles

### 1. Closed System

**Total tokens = Physical memory**

```c
// System initialization
total_memory = detect_physical_ram();
colorless_bank = total_memory;

// Invariant (always true)
colorless_bank + whites_pending + blacks_inuse + blues_inuse + reds_inuse == total_memory
```

### 2. No Fractional Reserve Banking

**Every allocation consumes from colorless bank**

❌ **WRONG** (Money printer):
```c
red->red_data = xallocz(size, 1);  // Creates tokens from thin air!
```

✅ **CORRECT** (Consume from bank):
```c
ps->black_budget -= size;          // Deduct from colorless
red->red_data = xallocz(size, 1);  // Backed by real budget
```

### 3. All Colors Return to Colorless

**Every allocation has a path back to the bank**

- White unused → Colorless
- Black freed → Colorless
- Blue aborted → Colorless
- Red discarded → Colorless

This ensures **zero leaks** and **perfect accounting**.

### 4. UTXO Model (Bitcoin-style)

**Tokens are spent, not copied**

```
Traditional OS:
  malloc(1MB) → ??? (where did memory come from?)
  free(1MB)   → ??? (where did memory go?)

Pebble UTXO:
  Input: COLORLESS (1MB)
  Output: WHITE token (1MB) → BLACK allocation (1MB)
  Free: BLACK (1MB) → COLORLESS (1MB)

  (Provable: Input bytes = Output bytes)
```

## Why This Matters

### 1. Zero Resource Leaks

Every allocation is tracked through state machine. If process crashes, all colored tokens return to colorless during cleanup.

### 2. Prevents Memory Exhaustion Attacks

Attacker can't create unbounded allocations - limited by colorless bank budget.

### 3. Enables Formal Verification

State machine with invariants → provable properties:
- Conservation of tokens: `Σ(colors) = total_memory`
- No double-spending: Each token has exactly one color
- Eventual return: All colored tokens reach colorless

### 4. Block Device Transaction Safety

Red snapshots enable atomic commit/rollback for disk I/O without traditional journaling overhead.

## Implementation Status

### ✅ Completed

1. **Colorless Bank**: `PebbleState.black_budget` tracking
2. **White Tokens**: Issue/verify mechanism
3. **Black Allocations**: Full lifecycle with Blind Ledger + Borrow Checker
4. **Red Budget Fix**: Red now consumes/returns budget (commit 7704dfd2)

### ⚠️ Needs Work

1. **Blue Allocation**: Currently auto-created by `pebble_black_alloc()` as shadow
   - Should be separate allocation with own budget consumption
   - Needs `pebble_blue_alloc()` and `pebble_blue_free()` API

2. **Blue/Red Decoupling**: `blue->matching_red` pointer model is shadow-based
   - Should track Blue and Red independently
   - Current model violates "separate colors" principle

3. **White Return**: No API to return unused white tokens to colorless
   - Needs `pebble_white_return(white)` function

4. **CLR Integration**: Reverted in commit a6cd4a74
   - Needs redesign for proper token economy
   - Should use Blue/Red for block device ops only, NOT for general CLR objects

## Block Device I/O vs IPC

### Block Device Operations (Use Red/Blue)

**Why**: Data must be copied between memory ↔ disk

```c
// Can't "transfer ownership" of disk sector!
blue = alloc_working_buffer();
red = snapshot(blue);
disk_read(sector_42, blue);
if (checksum_ok) {
    commit(blue, red);  // Keep blue, discard red
} else {
    rollback(blue, red); // Restore from red
}
```

### IPC (Use Exchange Pages)

**Why**: Zero-copy capability transfer

```c
// No copying needed - just transfer ownership
capability = process_A_memory;
exchange_prepare(capability);
exchange_transfer(process_A, process_B);
// Process B now owns the same physical pages
```

**Key Difference**: IPC = ownership transfer (UTXO-style), Block I/O = data copy (transaction-style)

## Future Work

### 1. Token Accounting Visualization

Add `/dev/pebble/economy` to show:
```
Colorless Bank: 245 MB
White Reserved: 5 MB (12 tokens)
Black In-Use:   240 MB (4821 allocations)
Blue Active:    8 MB (16 transactions)
Red Snapshots:  2 MB (4 backups)
Total:          500 MB (matches physical RAM)
```

### 2. Policy Enforcement

Per-process budget limits:
```c
pebble_set_budget(proc, 100*MB);  // Max 100MB per process
pebble_set_white_limit(proc, 50); // Max 50 white tokens
```

### 3. Token Garbage Collection

Automatic cleanup on process exit:
```c
pebble_cleanup(proc) {
    // Free all whites → colorless
    // Free all blacks → colorless
    // Free all blues → colorless
    // Free all reds → colorless
}
```

### 4. Formal Verification

Coq proof of token conservation:
```coq
Theorem token_conservation:
  forall (state : PebbleState),
    colorless + whites + blacks + blues + reds = total_memory.
```

## Conclusion

The Pebble token economy is a **circular economy** where:
- All allocations come from the **colorless bank**
- Tokens transition through **colored states** (White/Black/Blue/Red)
- All tokens eventually **return to colorless**
- No fractional reserve banking - **strict UTXO model**

This enables:
- ✅ Zero resource leaks
- ✅ Perfect memory accounting
- ✅ Transaction safety for block I/O
- ✅ Formal verification potential

**The budget system is a blockchain UTXO model without the blockchain.**

---

**Related Documents**:
- `docs/SIPHASH_IMPLEMENTATION.md` - Hash table security
- `docs/RDRAND_IMPLEMENTATION.md` - Hardware RNG
- `kernel/include/pebble.h` - Data structures
- `kernel/pebble.c` - Implementation

**Commits**:
- `7704dfd2` - Fix Red allocation token economy violation
- `a6cd4a74` - Revert incorrect CLR Red-Blue implementation
- `1a2de635` - Harden deallocation error handling
