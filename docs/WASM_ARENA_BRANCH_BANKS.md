# Pebble Arena Branch Banks: Per-Container Resource Management

This document captures the design for per-container (per-arena) resource management using the Pebble token economy and colorless branch banking system.

## Current Tokenomics Architecture

The existing Pebble system implements a circular economy:

```
┌────────────────────────────────────────────────────────────────────┐
│                     GLOBAL COLORLESS BANK                          │
│              (pebble_global_colorless_bank)                        │
│           Total: system RAM / 8 bytes per token                    │
│                                                                    │
│     ┌──────────────────────────────────────────────────────────┐   │
│     │  Scarcity-based PoW for budget requests:                 │   │
│     │  diff += (usage% / 10)  → harder to get as pool shrinks  │   │
│     └──────────────────────────────────────────────────────────┘   │
└────────────────────────────────────────────────────────────────────┘
                              │
                    PoW + pebble_increase_budget()
                              ▼
┌────────────────────────────────────────────────────────────────────┐
│               PER-PROCESS COLORLESS BANK                           │
│              (PebbleState.colorless_bank)                          │
│                                                                    │
│  ┌─────────────────────────────────────────────────────────────┐   │
│  │   State Transitions (all 1:1 token ratio):                  │   │
│  │                                                             │   │
│  │   COLORLESS ──issue_white()───► WHITE (pending commitment)  │   │
│  │       │                              │                      │   │
│  │       │                    white_verify()                   │   │
│  │       │                              ▼                      │   │
│  │       ├──black_alloc()────────► BLACK (reified identity)    │   │
│  │       │                              │                      │   │
│  │       ├──blue_alloc()─────────► BLUE (block I/O buffer)     │   │
│  │       │                              │                      │   │
│  │       ├──red_alloc()──────────► RED (snapshot/copy)         │   │
│  │       │                              │                      │   │
│  │       ◄────────────free()────────────┘ (all return 1:1)     │   │
│  └─────────────────────────────────────────────────────────────┘   │
└────────────────────────────────────────────────────────────────────┘
```

## UUID ↔ Pebble Bridge

UUIDs and Pebble tokens encode the same authority in two representations:

```c
/* UUID packing (uuid.h:29) */
void uuid_pack_pebble(uuid_t *u, 
                      unsigned int token,       // 32 bits
                      unsigned int generation,  // 32 bits  
                      unsigned short index);    // 16 bits

/* Packed into UUIDv8 (122 usable bits):
   data_a (48 bits): token << 16 | index
   data_b (12 bits): generation & 0xFFF
   data_c (62 bits): generation >> 12 << 42 */
```

- **UUID**: Portable, passable via 9P/IPC
- **Pebble (token, gen, index)**: Fast internal lookup

## The UUID ↔ Pebble ↔ Capability Triangle

```
                   UUID (Portable)
                   ╱             ╲
         uuid_pack_pebble()   uuid_unpack_pebble()
                 ╱                 ╲
                ▼                   ▼
    PebbleWhite ◄────verify()────► UserCapability
    (token,gen,idx)               (hash[32])
                 ╲                 ╱
                  black_alloc()
                         ╲       ╱  
                          ▼     ▼
                     BlindLedgerEntry
                    (physical resource)
```

---

## Arena Branch Banks Design

Arena branch banks add hierarchical token distribution for lock-free per-container allocation:

```
┌──────────────────────────────────────────────────────────────────┐
│                    GLOBAL COLORLESS BANK                         │
│              (pebble_global_colorless_bank)                      │
│                                                                  │
│   ┌─────────────────────────────────────────────────────────┐    │
│   │  Epoch reconciliation with arena branches:              │    │
│   │  - Branches report usage statistics                     │    │
│   │  - Branches request refills when low                    │    │
│   │  - Branches return excess when high                     │    │
│   └─────────────────────────────────────────────────────────┘    │
└────────────────────────────────────────────────────────────────┬─┘
                                                                 │
                          provision/reconcile                    │
    ┌────────────────────────────────────────────────────────────┼──┐
    ▼                        ▼                        ▼          │  │
┌────────┐              ┌────────┐              ┌────────┐       │  │
│Arena 0 │              │Arena 1 │              │Arena 2 │       │  │
│Branch  │              │Branch  │              │Branch  │       │  │
├────────┤              ├────────┤              ├────────┤       │  │
│○○○○○○○ │              │○○○○○○○ │              │○○○○○○○ │       │  │
│local   │              │local   │              │local   │       │  │
│tokens  │              │tokens  │              │tokens  │       │  │
└────┬───┘              └────┬───┘              └────┬───┘       │  │
     │                       │                       │           │  │
     ▼                       ▼                       ▼           │  │
┌──────────────────────────────────────────────────────────────┐ │  │
│              PER-PROCESS PEBBLE STATE                        │ │  │
│  (PebbleState.colorless_bank subsumes arena branch)          │ │  │
│  ┌──────────────────────────────────────────────────────────────┐│
│  │  Arena-local allocations: lock-free from branch      │    │ │  │
│  │  Cross-arena: fall back to process colorless bank    │    │ │  │
│  └──────────────────────────────────────────────────────────┘    │
└──────────────────────────────────────────────────────────────┘
```

### Key Design Points

**1. All color transitions remain 1:1:**
```c
ps->colorless_bank -= size;  // Consume
ps->blue_inuse += size;      // Allocate BLUE
// ...
ps->colorless_bank += size;  // Free
ps->blue_inuse -= size;      // Release BLUE
```

**2. Branch bank structure:**
```c
typedef struct arena_branch {
  ulong local_colorless;     // Tokens available locally
  ulong borrowed_from_proc;  // Accounting: how much from process bank
  ulong low_water;           // Trigger refill below this
  ulong high_water;          // Return excess above this
} arena_branch_t;
```

**3. Reconciliation is 1:1 (no exchange rates):**
```c
void branch_refill(arena_branch_t *branch, PebbleState *ps) {
  ulong refill = branch->high_water - branch->local_colorless;
  if (ps->colorless_bank >= refill) {
    ps->colorless_bank -= refill;
    branch->local_colorless += refill;
    branch->borrowed_from_proc += refill;
  }
}
```

### What Arena Branches Add

| Feature | Benefit |
|---------|---------|
| Per-arena lock | No global `pebble_global_lock` contention |
| Hot-path local | Lock-free allocations within arena |
| Cold-path batched | Amortized reconciliation cost |
| WASM integration | Each WASM module arena has its own branch |
| 1:1 conservation | Tokens remain fungible across all levels |

---

## WASM Container Integration

WASM containers use arena branches for resource isolation:

```
┌─────────────────────────────────────────────────────────────┐
│                    WASM Container                           │
│  ┌───────────────────────────────────────────────────────┐  │
│  │  arena_branch_t branch;  // Container's token budget  │  │
│  │  IM3Runtime runtime;     // wasm3 runtime             │  │
│  │  void *linear_memory;    // Maps to Pebble BLACK      │  │
│  │  UserCapability mem_cap; // Capability for memory     │  │
│  └───────────────────────────────────────────────────────┘  │
│                                                             │
│  Allocation: branch_alloc() → BLACK (capability-backed)     │
│  Access:     LINEAR MEMORY via capability verification      │
│  Cleanup:    branch_drain() → tokens return to process      │
└─────────────────────────────────────────────────────────────┘
```

### WASM Memory Allocation Flow

1. **Container creation**: Provision arena branch from process budget
2. **Linear memory**: `pebble_black_alloc()` from branch → `UserCapability`
3. **Memory access**: All access via capability-verified pointers
4. **Container exit**: `branch_drain()` returns all tokens to process

This ensures WASM containers:
- Cannot access kernel memory directly
- Have bounded resource usage (branch budget)
- Properly clean up on exit (tokens return 1:1)
- Integrate with existing Pebble/Capability/Ledger systems

---

## Implementation Roadmap

1. **Define `arena_branch_t`** in `pebble.h`
2. **Add branch allocation helpers** in `pebble.c`
3. **Integrate with Proc** - each process can have multiple arena branches
4. **WASM container** - create branch on container init, drain on exit
5. **Cleanup in pexit()** - drain all process branches back to global bank

---

## References

- `kernel/pebble.c` - Core Pebble implementation
- `kernel/include/pebble.h` - Pebble types and API
- `kernel/include/blind_ledger.h` - Capability/Ledger system
- `kernel/include/uuid.h` - UUID ↔ Pebble packing
