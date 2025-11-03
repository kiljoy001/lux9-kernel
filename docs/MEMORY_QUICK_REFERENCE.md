# 9Front Memory Management - Quick Reference

## Critical Files to Know

### Must Understand These 4 Files:
1. **9/pc64/mem.h** - Address space layout (KZERO, VMAP, KMAP values)
2. **9/port/page.c** - User page allocation and management
3. **9/port/xalloc.c** - Early kernel memory allocation
4. **9/pc/memory.c** - Physical memory discovery and initialization

### Important Supporting Files:
- **9/port/memmap.c** - Memory map tracking (type/address/size regions)
- **9/port/alloc.c** - Pool-based malloc (uses xalloc underneath)
- **9/pc64/mmu.c** - Virtual memory and page table management

## Three Memory Systems (Each with Different Purpose)

### 1. Early Memory (xalloc) - Used first
**File:** 9/port/xalloc.c
**When:** Before pageinit()
**Data:** Hole table tracks free regions
**Functions:** xinit(), xalloc(), xfree(), xhole()
**Purpose:** Allocate kernel structures

### 2. Memory Map - Tracks what memory exists
**File:** 9/port/memmap.c
**Data:** Array of Mapent (type, address, size)
**Types:** MemRAM, MemUMB, MemUPA, MemACPI, MemReserved
**Functions:** memmapadd(), memmapalloc(), memmapnext()
**Purpose:** Inventory and allocate physical regions

### 3. Page Allocator - Final system
**File:** 9/port/page.c
**Data:** palloc.pages[] array + free list
**Functions:** pageinit(), newpage(), freepages()
**Purpose:** Allocate individual pages for user processes

## Boot Sequence (Simplified)

```
main()
├─ mach0init()         ← Set up CPU0 structures
├─ meminit0()          ← Discover physical memory
├─ cpuidentify()       ← Detect CPU capabilities  
├─ archinit()          ← Platform-specific init (can reserve memory)
├─ xinit()             ← Start xalloc system
├─ mmuinit()           ← Set up paging + protections
├─ confinit()          ← Calculate memory splits
├─ preallocpages()     ← Allocate palloc.pages[] in VMAP
├─ pageinit()          ← Start page allocator
└─ userinit()          ← Create init process
```

## Address Space Quick Map

```
User Code:    0x00000000-0x00007fff ffffffff  (128TB, canonical)
Kernel Code:  0xffffffff 80000000+           (Higher-half)
VMAP:         0xfffffe80 00000000-0xffffffff (1GB device I/O)
KMAP:         0xfffffe00 00000000-0xffffffff (2MB temp pages)
```

## Key Data Structure: Confmem
```c
conf.mem[0].base      // Physical address of bank 0
conf.mem[0].npage     // Number of pages in bank
conf.mem[0].kbase     // Kernel virtual address (base + KZERO)
conf.mem[0].klimit    // Kernel virtual limit
```
Populated by meminit() from discovered memory.

## Key Data Structure: Page
```c
Page.pa               // Physical address
Page.va               // Virtual address (user)
Page.next             // Free list chain
Page.ref              // Reference count (>0 = in use)
Page.image            // Associated file/swap
```

## Memory Discovery Process

### Option 1: E820 BIOS Map (Preferred)
```
Bootloader provides via getconf("*e820")
Format: "base top type base top type ..." (hex)
Types: 1=RAM, 2=Reserved, 3=ACPI, 4+=Device
→ Call e820scan()
```

### Option 2: Memory Test (Fallback)
```
If E820 unavailable: ramscan()
Test in 4MB chunks with pattern writes
Verify patterns survive read-back
Stop on first failure
```

## Function: How to Allocate Memory

### Early Boot (while xalloc is active, before pageinit):
```c
void *p = xalloc(size);           // Returns KADDR virtual
p = KADDR(physical_address);       // Convert PA to KZERO virtual
uintptr pa = PADDR(kernel_ptr);    // Convert KZERO virtual to PA
```

### Runtime (after pageinit):
```c
void *p = malloc(size);            // Safe, but can return nil
void *p = smalloc(size);           // Waits until memory available
void *p = xalloc(size);            // WRONG after pageinit - uses old pool
```

### Page Allocation:
```c
Page *p = newpage(virtual_addr, nil);  // Get free page
putpage(p);                             // Return to free list
```

## Critical Ordering Rules

1. **rampage() depends on conf.mem[]** being populated
   - So meminit0() must come before rampage() is called
   
2. **xalloc() must start before preallocpages()**
   - preallocpages() calls xspanalloc()
   
3. **preallocpages() must come before pageinit()**
   - pageinit() uses palloc.pages that was allocated
   
4. **mmuinit() must come before newpage()**
   - newpage() returns pages with PTEUSER flag set
   
5. **pageinit() must come before userinit()**
   - userinit() creates processes that need pages

## Memory Reporting

**Boot output example:**
```
memory: 2048M kernel data, 6144M user, 2048M swap
```

**Calculated by:**
- Kernel = all kalloc'd memory + kernel image + pools + structures
- User = upages × BY2PG ÷ 1048576
- Swap = conf.nswap × BY2PG ÷ 1048576

## Debugging Tips

### Check What Memory System is Active
```c
if(conf.mem[0].npage == 0)
    return xallocz(...);           // Still in early boot
else
    return xspanalloc(...);        // After pageinit
```

### Physical Address Validity
```c
if(cankaddr(pa) != 0)              // Can be accessed via KZERO?
    return (KMap*)KADDR(pa);       // Use direct KZERO mapping
else
    return kmap(page);              // Use KMAP rotating window
```

### Page Table Issues
```c
pte = mmuwalk(m->pml4, va, 0, 0);  // Walk without creating
if(pte == nil) print("page table missing\n");
if(!(*pte & PTEVALID)) print("page not mapped\n");
```

## Common Bugs to Watch

1. **Using xalloc after pageinit()** - Returns from old pool
   - Fix: Use malloc or smalloc instead

2. **rampage() returns pointer in VMAP, not KZERO**
   - Access with +VMAP offset if converting to PA

3. **conf.mem not populated until after meminit()**
   - Calling code that needs it before is broken

4. **Page tables allocated with rampage() during bootstrap**
   - These come from early memmap, not xalloc

5. **KMAP is only 2MB - not for large mappings**
   - Use vmap() for device memory instead

## Testing Memory System

**Check early allocations:**
```bash
Print from meminit0 - should show memory regions added
```

**Check page allocator:**
```bash
Print from pageinit - shows memory report
Check palloc.freecount - should be positive
```

**Check xalloc:**
```bash
Call xsummary() - shows hole table usage
```

**Verify page mapping:**
```c
newpage(va, nil);      // Allocate page
pte = mmuwalk(...);    // Check PTE was created
*pte should have PTEVALID | PTEUSER set
```

## Remember

- **Three separate memory systems** - they work in sequence
- **Address spaces matter** - KZERO vs VMAP vs physical addresses
- **Order is critical** - initialization must happen in exact sequence  
- **Transitional point is pageinit()** - everything changes after it
- **HHDM is optional** - 9front doesn't require it, but we're integrating it
