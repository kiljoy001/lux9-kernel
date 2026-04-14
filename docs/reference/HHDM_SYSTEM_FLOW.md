# HHDM System - Complete Memory Flow Diagram

## 1. Boot Phase: Limine Loader to Kernel Entry

```
┌─────────────────────────────────────────────────────────────┐
│ Limine Bootloader                                           │
│ - Maps physical memory at HHDM offset (0xffff800000000000) │
│ - Loads kernel ELF at virtual KZERO (0xffffffff80000000)   │
│ - Provides memory map in E820 format                       │
│ - Provides HHDM offset in response structure                │
└─────────┬───────────────────────────────────────────────────┘
          │
          ▼
┌─────────────────────────────────────────────────────────────┐
│ entry.S (_start)                                            │
│ - Zero BSS section                                          │
│ - Initialize early stack                                   │
│ - Set up UART for debugging                               │
│ - Call bootargsinit()                                     │
└─────────┬───────────────────────────────────────────────────┘
          │
          ▼
┌─────────────────────────────────────────────────────────────┐
│ bootargsinit() [boot.c]                                    │
│ - Extract limine_hhdm_offset from Limine response         │
│ - Save to saved_limine_hhdm_offset (survives CR3 switch)  │
│ - Initialize hhdm_base = limine_hhdm_offset               │
│ - Parse E820 memory map into conf.mem[]                   │
└─────────┬───────────────────────────────────────────────────┘
          │
   HHDM NOW INITIALIZED ✓
          │
          ▼
┌─────────────────────────────────────────────────────────────┐
│ Call to setuppagetables() [mmu.c]                         │
│                                                             │
│ ┌────────────────────────────────────────────────────────┐ │
│ │ 1. Map kernel image to KZERO (0xffffffff80000000)    │ │
│ │    - Maps kernel ELF at proper virtual address       │ │
│ │    - Uses cpu0pml4 from linker script               │ │
│ │                                                      │ │
│ │ 2. Mirror kernel in HHDM region                     │ │
│ │    - Maps same kernel to [HHDM + kernel_phys]      │ │
│ │    - Allows kaddr() to keep working post-switch    │ │
│ │                                                      │ │
│ │ 3. Map conf.mem[] into HHDM                        │ │
│ │    - For each conf.mem[i] entry:                   │ │
│ │    - Maps [phys_addr + HHDM] to [phys_addr]        │ │
│ │    - All physical memory now accessible            │ │
│ │                                                      │ │
│ │ 4. CR3 SWITCH                                       │ │
│ │    - __asm__ volatile("mov %0, %%cr3")            │ │
│ │    - CPU now uses kernel's own page tables        │ │
│ │    - HHDM mappings remain valid                    │ │
│ │                                                      │ │
│ │ 5. Update m->pml4 pointer                          │ │
│ │                                                      │ │
│ └────────────────────────────────────────────────────────┘ │
└─────────┬───────────────────────────────────────────────────┘
          │
   CR3 SWITCH COMPLETE ✓
          │
          ▼
┌─────────────────────────────────────────────────────────────┐
│ Continue main() boot sequence                              │
│ - Initialize architecture (irqinit, etc)                 │
│ - Call xinit() to populate memory holes                   │
└─────────┬───────────────────────────────────────────────────┘
          │
          ▼
```

## 2. Memory Hole Registration Phase

```
┌─────────────────────────────────────────────────────────────┐
│ xinit() [xalloc.c]                                          │
│                                                             │
│ For each memory region in conf.mem[]:                      │
│ ┌───────────────────────────────────────────────────────┐ │
│ │ 1. Calculate kernel allocation size                  │ │
│ │    cm->kbase = KADDR(cm->base)                      │ │
│ │            = cm->base + saved_limine_hhdm_offset    │ │
│ │            = virtual address in HHDM               │ │
│ │                                                     │ │
│ │ 2. Calculate limit                                  │ │
│ │    cm->klimit = cm->kbase + (n_pages * BY2PG)     │ │
│ │                                                     │ │
│ │ 3. Register as hole (for allocations)              │ │
│ │    xhole(cm->base,                                │ │
│ │           cm->klimit - cm->kbase)                 │ │
│ │                                                     │ │
│ │    Input: Physical base address                    │ │
│ │    Input: Size in bytes (kbase and klimit both    │ │
│ │            have same offset so subtraction works) │ │
│ │                                                     │ │
│ └───────────────────────────────────────────────────────┘ │
│                                                             │
│ Result: xlists.table contains holes in HHDM virtual space │
└─────────┬───────────────────────────────────────────────────┘
          │
   KERNEL MEMORY HOLES REGISTERED ✓
          │
          ▼
```

## 3. xhole() - The Bridge Function

```
┌─────────────────────────────────────────────────────────────┐
│ xhole(uintptr addr, uintptr size)                           │
│                                                             │
│ Input Contract:                                            │
│ - addr: PHYSICAL address of memory start                  │
│ - size: Byte size of region                              │
│                                                             │
│ Internal Processing:                                       │
│ ┌───────────────────────────────────────────────────────┐ │
│ │ 1. Convert physical to virtual                        │ │
│ │    vaddr = addr + limine_hhdm_offset                │ │
│ │    top = vaddr + size                               │ │
│ │                                                     │ │
│ │ 2. Find insertion point in hole list                │ │
│ │    (holes tracked in sorted order)                 │ │
│ │                                                     │ │
│ │ 3. Try to merge with existing holes                │ │
│ │    - If adjacent below: expand existing hole       │ │
│ │    - If adjacent above: merge with following hole  │ │
│ │    - Otherwise: create new hole                    │ │
│ │                                                     │ │
│ │ 4. Update hole list (xlists.table)                 │ │
│ │                                                     │ │
│ └───────────────────────────────────────────────────────┘ │
│                                                             │
│ Storage Format in Hole:                                    │
│ struct Hole {                                              │
│     uintptr addr;  // Virtual address in HHDM             │
│     uintptr top;   // End virtual address (addr + size)   │
│     uintptr size;  // Byte size                           │
│     Hole* link;    // Next hole in list                   │
│ };                                                         │
└─────────────────────────────────────────────────────────────┘
```

## 4. Memory Allocation from Holes

```
┌─────────────────────────────────────────────────────────────┐
│ xallocz(ulong size, int zero) [xalloc.c]                  │
│                                                             │
│ 1. Calculate size with overhead                           │
│    - Add header space (Xhdr)                              │
│    - Round up to vlong boundary                           │
│                                                             │
│ 2. Search hole list                                       │
│    for(h = xlists.table; h; h = h->link) {               │
│        if(h->size >= size) {                              │
│            // Found suitable hole!                        │
│                                                             │
│ 3. Allocate from this hole                               │
│    - p = (Xhdr*)h->addr         // Virtual HHDM address  │
│    - h->addr += size            // Advance hole pointer   │
│    - h->size -= size            // Reduce hole size      │
│    - if(h->size == 0)           // Remove exhausted hole │
│                                                             │
│ 4. Write header & optionally zero                        │
│    - p->magix = Magichole                                │
│    - p->size = size                                      │
│    - if(zero) memset(p->data, 0, orig_size)             │
│                                                             │
│ 5. Return pointer to data                                │
│    return p->data;  // Still HHDM virtual address        │
│    }                                                       │
│                                                             │
│ Output: Virtual address in HHDM region (usable immediately) │
└─────────────────────────────────────────────────────────────┘
```

## 5. Memory Freeing

```
┌─────────────────────────────────────────────────────────────┐
│ xfree(void *p) [xalloc.c]                                 │
│                                                             │
│ 1. Get header                                             │
│    x = (Xhdr*)(p - header_offset)                        │
│    // x is HHDM virtual address                          │
│                                                             │
│ 2. Verify magic                                          │
│    if(x->magix != Magichole) panic(...)                 │
│                                                             │
│ 3. Return to hole system                                 │
│    xhole((uintptr)x - limine_hhdm_offset,  // Convert to PA │
│           x->size)                                        │
│                                                             │
│    Note: Caller must convert VIRTUAL back to PHYSICAL    │
│    for xhole() to work correctly                          │
│                                                             │
│ Result: Memory re-merged into available hole list         │
└─────────────────────────────────────────────────────────────┘
```

## 6. Address Translation System

```
┌─────────────────────────────────────────────────────────────┐
│ kaddr(uintptr pa) [mmu.c]                                  │
│                                                             │
│ Purpose: Convert PHYSICAL to VIRTUAL (HHDM)              │
│                                                             │
│ Input:  pa - Physical address                            │
│ Output: va - Virtual address in HHDM                     │
│                                                             │
│ Implementation:                                          │
│ return (void*)hhdm_virt(pa);                            │
│     = (void*)(hhdm_base + pa);                           │
│     = (void*)(saved_limine_hhdm_offset + pa);           │
│     = (void*)(0xffff800000000000 + pa);                │
│                                                             │
│ Example:                                                  │
│ kaddr(0x1000) = 0xffff800000001000                       │
│                                                             │
└─────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────┐
│ paddr(void *v) [mmu.c]                                    │
│                                                             │
│ Purpose: Convert VIRTUAL addresses back to PHYSICAL      │
│ Handles three address spaces:                            │
│                                                             │
│ 1. HHDM Virtual Addresses (0xffff800000000000+)         │
│    if(is_hhdm_va(va))                                    │
│        return va - saved_limine_hhdm_offset             │
│                                                             │
│ 2. KZERO Addresses (0xffffffff80000000+)               │
│    if(va >= KZERO)                                      │
│        return va - KZERO + limine_kernel_phys_base     │
│                                                             │
│ 3. Already Physical (0x0 - 0x1000000, etc.)            │
│    return va;                                            │
│                                                             │
│ Example:                                                  │
│ paddr(0xffff800000001000) = 0x1000                       │
│                                                             │
└─────────────────────────────────────────────────────────────┘
```

## 7. Memory State After Boot

```
Virtual Address Space (on CPU 0):
┌─────────────────────────────────────────────────────────────┐
│ Kernel Code/Data                                            │
│ 0xffffffff80000000 - 0xffffffff80??????               (KZERO) │
│ (Mapped to physical kernel base)                           │
└─────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────┐
│ (User/Kernel Heap Space)                                    │
│ 0x0000000000000000 - 0x00007fffffffffff            (USERSPACE) │
│ (User processes and kernel heap)                           │
└─────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────┐
│ HHDM - All Physical Memory                                  │
│ 0xffff800000000000 - 0xffff80ffffffffff           (HHDM REGION) │
│ - Kernel image mirror                                      │
│ - All conf.mem[] regions                                  │
│ - Accessible via kaddr() for xalloc operations           │
└─────────────────────────────────────────────────────────────┘

Physical Address Space:
┌─────────────────────────────────────────────────────────────┐
│ Kernel Image                                                │
│ 0x00??????? (from limine_kernel_phys_base)                  │
└─────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────┐
│ System RAM (from E820 / conf.mem[])                        │
│ 0x00100000 - 0x???????? (accessible via HHDM)            │
│ - Kernel heap (via xalloc)                               │
│ - User memory (via page allocator)                        │
└─────────────────────────────────────────────────────────────┘
```

## 8. Critical Data Structures

### Saved Boot Configuration

```c
// globals.c
uintptr saved_limine_hhdm_offset;  // HHDM offset (survives CR3)
uintptr hhdm_base;                 // Alias for use by hhdm.h

// boot.c
uintptr limine_hhdm_offset;        // Extracted from Limine response
u64int limine_kernel_phys_base;    // Kernel physical load address
```

### Memory Configuration

```c
// dat.h (in 9front structures)
struct Confmem {
    uintptr base;      // Physical address
    uintptr npage;     // Number of pages
    uintptr kbase;     // Virtual address (set by xinit)
    uintptr klimit;    // Virtual limit (set by xinit)
};

Conf conf;
conf.mem[nelem];       // Array of memory regions from E820
```

### Hole Tracking

```c
// xalloc.c
struct Hole {
    uintptr addr;      // Virtual address in HHDM
    uintptr top;       // End of hole (addr + size)
    uintptr size;      // Size in bytes
    Hole* link;        // Linked list pointer
};

struct Xalloc {
    Lock lk;
    Hole hole[128];    // Static hole descriptors
    Hole* flist;       // Free list of descriptors
    Hole* table;       // Allocated holes
};

static Xalloc xlists;  // Global allocator state
```

## 9. Key Invariants

1. **HHDM Offset Constant**
   - saved_limine_hhdm_offset set once at boot, never changes
   - Physical address PA maps to VA = PA + saved_limine_hhdm_offset

2. **Kernel Page Tables**
   - cpu0pml4 is kernel's own page tables from linker
   - Mapped both at KZERO and in HHDM region
   - CR3 points to physical address of cpu0pml4

3. **Hole Tracking**
   - All holes store VIRTUAL addresses in HHDM region
   - Holes are merged when adjacent
   - Free list provides hole descriptors

4. **Memory Allocation Chain**
   - xalloc provides kernel early memory
   - After mmuinit, page allocator takes over
   - Both see same physical memory (via conf.mem)

5. **Address Translation**
   - HHDM addresses: VA = PA + offset
   - KZERO addresses: VA = PA + (KZERO - kernel_phys_base)
   - Physical addresses: Returned as-is

