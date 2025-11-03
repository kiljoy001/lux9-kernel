# HHDM System - Issues Summary & Action Items

## Critical Issues (Fix Immediately)

### Issue #1: Memory Reporting Completely Broken
- **File**: `kernel/9front-port/xalloc.c::xsummary()`
- **Problem**: Hole counting shows incorrect totals
- **Impact**: Diagnostic only, doesn't break functionality
- **Root Cause**: Likely mismatch between static and dynamic hole tracking
- **Fix**: Audit hole list traversal, verify counts after each operation

### Issue #2: Dual HHDM Implementation Creates Silent Corruption Risk
- **Files**: 
  - `kernel/include/hhdm.h` (uses `hhdm_base`)
  - `kernel/9front-pc64/mmu.c` (uses `saved_limine_hhdm_offset`)
- **Problem**: Two independent implementations of `hhdm_virt()`
- **Impact**: HIGH - If these diverge, memory corruption is silent
- **Root Cause**: Lack of centralized implementation
- **Fix**: Remove static version from mmu.c, use hhdm.h consistently

### Issue #3: Confusing xhole() API Contract
- **File**: `kernel/9front-port/xalloc.c::xhole()`
- **Problem**: Documentation says "works with VIRTUAL addresses" but code says "converts PHYSICAL to VIRTUAL"
- **Impact**: HIGH - Easy to get wrong during maintenance
- **Callers**:
  - `xinit()`: Passes PHYSICAL (correct)
  - `xspanalloc()`: Converts VIRTUAL to PHYSICAL first (correct)
  - `xfree()`: Converts VIRTUAL to PHYSICAL first (correct)
- **Fix**: Document clearly: "Takes PHYSICAL address, converts to VIRTUAL internally"

---

## High Risk Issues (Fix Soon)

### Issue #4: Uninitialized HHDM Could Cause Early Boot Failure
- **File**: `kernel/9front-pc64/boot.c::bootargsinit()`
- **Problem**: Must be called before any `kaddr()` usage, but not explicitly enforced
- **Impact**: If called too late, all memory access fails
- **Risk**: Medium-High
- **Fix**: Add assertion in `kaddr()` that HHDM is initialized:
```c
void* kaddr(uintptr pa)
{
    if(saved_limine_hhdm_offset == 0)
        panic("kaddr: HHDM not initialized yet!");
    return (void*)hhdm_virt(pa);
}
```

### Issue #5: Page Table Coherency After CR3 Switch
- **File**: `kernel/9front-pc64/mmu.c::setuppagetables()`
- **Problem**: Dual strategy for accessing page tables:
  - Low memory (< 0x400000): Uses `KZERO + phys` 
  - High memory: Uses `hhdm_virt(phys)`
- **Impact**: MEDIUM - Could cause mapping inconsistencies
- **Fix**: Always use HHDM for consistency:
```c
// Instead of:
if(pdp_phys < 0x400000)
    pdp = (u64int*)(pdp_phys + KZERO);
else
    pdp = (u64int*)hhdm_virt(pdp_phys);

// Always use:
pdp = (u64int*)hhdm_virt(pdp_phys);
```

### Issue #6: Kernel Physical Base Fallback is Hardcoded
- **File**: `kernel/9front-pc64/mmu.c::virt2phys()` line 206
- **Problem**: Falls back to `0x7f8fa000` if `limine_kernel_phys_base == 0`
- **Impact**: MEDIUM - Incorrect if kernel loaded elsewhere
- **Fix**: Add assertion instead of fallback:
```c
u64int kernel_phys = limine_kernel_phys_base;
if(kernel_phys == 0)
    panic("virt2phys: kernel physical base not set by Limine");
```

---

## Medium Risk Issues (Fix When Possible)

### Issue #7: Address Space Contamination in virt2phys()
- **File**: `kernel/9front-pc64/mmu.c::virt2phys()` lines 195-211
- **Problem**: Checks HHDM BEFORE KZERO, could misidentify addresses
- **Scenario**: If kernel accidentally loaded in HHDM region, KZERO check won't catch it
- **Risk**: MEDIUM
- **Fix**: Add explicit range checks and assertions

### Issue #8: Size Calculation in xinit() is Confusing
- **File**: `kernel/9front-port/xalloc.c::xinit()` lines 96-104
- **Problem**: Comment about "size = virtual - virtual" is misleading
```c
cm->kbase = (uintptr)KADDR(cm->base);           // Virtual address
cm->klimit = (uintptr)cm->kbase + (uintptr)n*BY2PG;  // Virtual address
xhole(cm->base, cm->klimit - cm->kbase);        // Size calculation
```
- **Risk**: MEDIUM - Easy to misunderstand
- **Fix**: Add explicit comment:
```c
// cm->klimit - cm->kbase gives byte size (both have same offset applied)
uintptr size_bytes = cm->klimit - cm->kbase;
xhole(cm->base, size_bytes);
```

### Issue #9: No Validation of Memory Range After setuppagetables()
- **Files**: `kernel/9front-pc64/mmu.c`
- **Problem**: After CR3 switch, nothing verifies HHDM region is accessible
- **Risk**: MEDIUM - Silent page faults possible
- **Fix**: Add validation test after CR3 switch:
```c
// Verify HHDM mapping works
volatile uintptr test_addr = *(volatile uintptr*)hhdm_virt(0);
print("HHDM validation: read from 0x0 = %#p\n", test_addr);
```

---

## Low Risk Issues (Nice to Fix)

### Issue #10: Missing Boot Sequencing Documentation
- **Problem**: Not clear which function should be called in which order
- **Impact**: LOW - Doesn't break anything currently
- **Fix**: Add boot sequence comment in main.c

### Issue #11: Dynamic Hole Allocation Uses malloc()
- **File**: `kernel/9front-port/xalloc.c::xhole()` line 357
- **Problem**: If malloc fails during early boot, panic occurs
- **Risk**: LOW - malloc should be available, but fragile assumption
- **Fix**: Consider pre-allocating larger static hole pool

---

## Testing Checklist

Before considering HHDM "complete", verify:

- [ ] bootargsinit() called before any kaddr() usage
- [ ] HHDM offset from Limine is correctly extracted
- [ ] CR3 switch completes without page faults
- [ ] All conf.mem[] regions accessible via HHDM after switch
- [ ] xalloc works correctly after setuppagetables()
- [ ] xsummary() reports correct memory totals
- [ ] kaddr() works for all physical addresses in conf.mem[]
- [ ] paddr() correctly converts virtual addresses back
- [ ] Page allocator takes over from xalloc without issues
- [ ] No memory corruption detected during stress test

---

## Priority Fix Order

### Phase 1: Critical (Blocks Correct Operation)
1. Issue #2: Unify dual hhdm_virt() implementations
2. Issue #3: Document xhole() API clearly
3. Issue #4: Add HHDM initialization assertion in kaddr()

### Phase 2: High Risk (Prevent Silent Corruption)
4. Issue #5: Fix page table coherency
5. Issue #6: Remove hardcoded kernel phys base fallback
6. Issue #9: Add HHDM range validation

### Phase 3: Medium Risk (Code Quality)
7. Issue #7: Add explicit address space range checks
8. Issue #8: Improve xinit() comments
9. Issue #10: Document boot sequence

### Phase 4: Polish (Nice to Have)
10. Issue #1: Fix memory reporting
11. Issue #11: Pre-allocate hole pool

---

## Questions for Investigation

1. **Why does xsummary() report wrong counts?**
   - Is the issue in the free list traversal?
   - Or the used hole list traversal?
   - Are holes being merged correctly?

2. **When exactly is bootargsinit() called?**
   - Check main.c to verify boot sequence
   - Confirm it's before first HHDM-dependent operation

3. **Does the dual page table access pattern actually cause issues?**
   - Or does it work fine in practice?
   - Should we unify it anyway for safety?

4. **Why fallback to hardcoded kernel base?**
   - When would limine_kernel_phys_base be 0?
   - Should we panic instead?

---

## Files Requiring Changes

Priority order for fixes:

| Priority | File | Lines | Changes |
|----------|------|-------|---------|
| P1 | kernel/9front-pc64/mmu.c | 59-78 | Remove static hhdm_virt() |
| P1 | kernel/9front-port/xalloc.c | 292-299 | Fix documentation |
| P1 | kernel/9front-pc64/mmu.c | 507-517 | Add initialization check |
| P2 | kernel/9front-pc64/mmu.c | 242-259 | Unify page table access |
| P2 | kernel/9front-pc64/mmu.c | 195-211 | Remove hardcoded fallback |
| P2 | kernel/9front-pc64/mmu.c | 426 | Add validation after CR3 |
| P3 | kernel/9front-pc64/mmu.c | 195-211 | Range check improvements |
| P3 | kernel/9front-port/xalloc.c | 96-104 | Improve comments |
| P4 | kernel/9front-port/xalloc.c | 383-400 | Debug hole counting |

