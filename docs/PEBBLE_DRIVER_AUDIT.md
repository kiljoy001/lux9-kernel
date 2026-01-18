# Pebble Tracking Audit for Driver Allocations

## Issue
Driver allocations in `kernel/family/` are using `xalloc()` and `malloc()` which do NOT track pebble budgets. This violates the economic security model where all memory allocations should be accounted.

## Current State

### Proper Pebble-Tracked Allocation
`kernel/9front-port/alloc.c` provides:
- `pebble_arena_alloc()` - Properly tracks allocations via WHITE→BLACK token conversion
- `pebble_meta_alloc()` - For internal pebble metadata (uses metamem pool)

### Untracked Driver Allocations (19 instances found)

#### kernel/family/pci_resource_pool.c
- Line 206: `pool = xalloc(sizeof(struct PCIResourcePool))`
- Line 219: `pool->bar_resources = xalloc(...)`
- Line 220: `pool->irq_resources = xalloc(...)`
- Line 221: `pool->dma_resources = xalloc(...)`

#### kernel/family/pci_family.c
- Line 279: `malloc(sizeof(struct PCIDeviceDescriptor))`
- Line 372: `global_pci_ctx = xalloc(sizeof(struct PCIFamilyContext))`

#### kernel/family/pci_channel.c
- Line 264: `channel = xalloc(sizeof(struct PCIChannel))`
- Line 555: `mgr = xalloc(sizeof(struct PCIEChannelManager))`
- Line 566: `mgr->channels = xalloc(...)`
- Line 567: `mgr->channel_ids = xalloc(...)`
- Line 677: `bar_res = xalloc(...)`

#### kernel/family/pci_9p.c
- Line 151: `p = smalloc(4096)`
- Line 183: `p = smalloc(512)`

#### kernel/family/pci_9p.h
- Line 208: `ctx = xalloc(sizeof(*driver))`

#### kernel/family/family.c
- Line 114: `family = xalloc(sizeof(struct FamilyExchangePage))`

#### kernel/family/secure_element_family.c
- Line 105: `ctx = malloc(sizeof(TPMFamilyContext))`
- Line 125: `ctx->tpm_ctx = malloc(sizeof(TPMContext))`
- Line 206: `tpm_channel = malloc(sizeof(TPMChannel))`

#### kernel/family/pci_transactions.c
- Line 117: `chain = xalloc(sizeof(struct MultiDevicePebbleChain))`

## Recommended Solutions

### Option 1: Make xalloc pebble-aware
Add pebble budget checking to `xalloc_internal()` in `kernel/9front-port/xalloc.c`:
- Check if current process has pebble budget
- Deduct from colorless bank on allocation
- Return to bank on free

**Pros:** Automatic coverage for all existing code
**Cons:** May track kernel-internal allocations that shouldn't be user-charged

### Option 2: Replace driver xalloc with pebble_arena_alloc
Change all driver allocations to use `pebble_arena_alloc()`.

**Pros:** Explicit control, clear separation of tracked vs untracked
**Cons:** Requires touching 19+ call sites, error-prone

### Option 3: Create driver-specific pebble allocator
Add `driver_pebble_alloc()` that wraps pebble tracking with driver-appropriate defaults.

**Pros:** Clean API, easy to audit driver resource usage
**Cons:** Yet another allocator function

## Recommendation
**Option 1** with a flag to distinguish kernel-internal from driver allocations. Add `xalloc_driver()` that routes to pebble-tracked path.

## Next Steps
1. Decide on approach
2. Implement chosen solution
3. Update all driver allocation sites
4. Add tests to verify pebble accounting
5. Update TODO_MASTER_LIST.md when complete
