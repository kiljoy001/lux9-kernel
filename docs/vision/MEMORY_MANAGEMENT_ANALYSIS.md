# 9Front Kernel Memory Management System - Comprehensive Analysis

## Executive Summary
The 9front kernel implements a sophisticated two-phase memory management system that separates boot-time from runtime memory management. The system uses a combination of early physical memory allocation, virtual address mapping, and page allocation mechanisms to manage both kernel and user memory.

---

## 1. KEY MEMORY MANAGEMENT FILES

### Architecture-Independent Files (9/port/)
1. **page.c** - Page allocation and management
   - Manages the free page list (palloc structure)
   - Functions: pageinit(), newpage(), freepages(), putpage()
   - Handles page reference counting and lifecycle

2. **memmap.c** - Physical memory map tracking
   - Dynamic memory map data structure
   - Functions: memmapadd(), memmapalloc(), memmapnext(), memmapsize(), memmapfree()
   - Tracks regions of different types (RAM, UMB, UPA, ACPI, Reserved)

3. **alloc.c** - Kernel pool allocation
   - malloc/free implementation using pool allocators
   - Three pools: mainmem, imagmem, secrmem
   - Functions: malloc(), mallocz(), smalloc(), ralloc()

4. **xalloc.c** - Early physical memory allocation
   - "Hole" based allocation system
   - Functions: xalloc(), xallocz(), xfree(), xhole(), xinit()
   - Used before pageinit() is called

### PC64-Specific Files (9/pc64/)
1. **mem.h** - Memory layout constants
   - KZERO, VMAP, KMAP address definitions
   - Page table structure macros (PTLX, PGLSZ)
   - MMU-related constants and flags

2. **mmu.c** - Virtual memory management
   - Page table manipulation: mmuwalk(), mmucreate()
   - TLB management: mmuflushtlb(), invlpg()
   - Process memory: mmuswitch(), mmurelease()
   - Device mapping: kmap(), kunmap(), vmap(), vunmap()
   - Functions: pmap(), punmap(), preallocpages()

3. **main.c** - Initialization sequence
   - confinit() - Configure memory sizes
   - machinit() / mach0init() - Initialize Mach structures
   - meminit0() called early in boot
   - meminit() called after archinit()

### PC-Compatible Files (9/pc/, used by pc64)
1. **memory.c** - Physical memory discovery and initialization
   - rampage() - Allocate single physical page
   - meminit0() - Early memory map setup
   - meminit() - Finalize memory map
   - Low RAM initialization, BIOS region handling
   - E820 BIOS memory map parsing

---

## 2. MEMORY INITIALIZATION SEQUENCE

### Phase 1: Early Boot (before pageinit)
Location: main() -> mach0init(), trapinit0(), cpuidentify(), meminit0()

```
mach0init()
  - Set up Mach structure for CPU0 at CPU0MACH
  - Initialize PML4, GDT pointers
  - Set machp[0] for MP support

meminit0()
  - Initialize memmap system
  - Add kernel memory regions to reserved
  - Add conventional RAM, ROM, UMB regions
  - Discover RAM via E820 BIOS interface or memory test
  - Configure MTRRs (Memory Type Range Registers)
  - Map discovered RAM to KZERO kernel address space
```

**Key Memory Layout at this stage:**
- KZERO (0xffffffff80000000) - Kernel base address
- KTZERO (KZERO + 1MB + 64KB) - Kernel text starts
- Physical memory mapped at KZERO + physical_address

### Phase 2: Allocator Initialization (still early boot)
Location: main() -> archinit(), xinit(), trapinit(), mmuinit()

```
xinit()
  - Initialize xalloc "hole" table
  - Add kernel memory regions to xalloc heap
  - Calculate kpages vs upages split
  - xalloc can now allocate memory for kernel

mmuinit()
  - Set up GDT, TSS, IDT
  - Zero out double-map (PML4 entry 512)
  - Initialize kernel read-only protections
  - Enable syscall extensions
```

### Phase 3: Page Allocator Setup
Location: main() -> confinit(), preallocpages(), pageinit()

```
confinit()
  - Calculate total memory pages (conf.npage)
  - Split into kernel vs user pages
  - Configure swap parameters
  - Set maximum allocation sizes for pools

preallocpages()
  - Steal memory for palloc.pages array from high memory
  - Pre-allocate MMU pool structures
  - Map preallocated pages to VMAP range
  - This happens BEFORE pageinit() to ensure success

pageinit()
  - Initialize page allocator
  - Walk conf.mem[] banks
  - Create Page structures for all user pages
  - Link pages into free list by color
  - Initialize swap parameters
```

**Key Output:**
```
Memory: XXXX M kernel data, YYYY M user, ZZZZ M swap
```

---

## 3. MEMORY DISCOVERY: PHYSICAL ADDRESS TRACKING

### The Memory Map System (memmap.c)

**Mapent Structure** - tracks memory regions:
```c
struct Mapent {
    ulong type;      // MemRAM, MemUMB, MemUPA, MemACPI, MemReserved
    uvlong addr;     // Physical address
    uvlong size;     // Region size in bytes
};
```

**Memory Type Constants** (from pc/memory.c):
```c
MemUPA      = 0    // Unbacked physical address (device I/O)
MemUMB      = 1    // Upper memory blocks (0-16MB, ISA devices)
MemRAM      = 2    // Actual usable RAM
MemACPI     = 3    // ACPI tables (reserved)
MemReserved = 4    // Don't allocate (BIOS, ROM, etc)
```

### Memory Discovery Process

**E820 Interface** (getconf("*e820")):
- Bootloader provides E820 memory map from BIOS
- Format: "base top type base top type ..." (hex addresses)
- Types: 1=RAM, 3=ACPI, others=Reserved
- Entry function: e820scan() in memory.c

**Fallback: Memory Test** (ramscan()):
- If E820 unavailable, test memory with pattern writes
- Test in 4MB chunks
- Verify pattern survives read-back
- Stop on first failed chunk (end of RAM)

**BIOS Region Handling** (lowraminit()):
- Conventional RAM: 0 to 640KB
- VGA frame buffer: 0xA0000-0xC0000 (reserve)
- VGA ROM: 0xC0000-0xD0000 (reserve)
- Option ROM scan: 0xD0000-0xF0000 (mark ROMs as reserved)
- BIOS ROM: 0xF0000-0x100000 (reserve)

---

## 4. KEY DATA STRUCTURES

### Confmem Structure (portdat.h)
```c
struct Confmem {
    uintptr base;      // Physical base address
    ulong npage;       // Total pages in bank
    uintptr kbase;     // Kernel virtual address base
    uintptr klimit;    // Kernel virtual address limit
};
```
- Array: conf.mem[64] - up to 64 memory banks
- Populated by meminit() from discovered RAM

### Page Structure (portdat.h)
```c
struct Page {
    Ref;               // Reference count
    Page *next;        // Free list chain
    uintptr pa;        // Physical address
    uintptr va;        // Virtual address (for user)
    uintptr daddr;     // Disk address (swap)
    Image *image;      // Associated text/swap image
    ushort refage;     // Swap reference age
    char modref;       // Simulated M/R bits
    char color;        // Cache coloring
    ulong txtflush[];  // Icache bitmap
};
```
- One Page struct per physical page
- Array: palloc.pages[] indexed by page number

### Palloc Structure (portdat.h)
```c
struct Palloc {
    Lock;              // Spinlock
    Page *head;        // Free list head
    ulong freecount;   // Pages on free list
    Page *pages;       // Array of all pages
    ulong user;        // User pages count
    Rendezq pwait[2];  // Wait queue for memory pressure
};
```
- Global: palloc
- Points to page array allocated in high VMAP region

### MMU Structure (pc64/dat.h)
```c
struct MMU {
    MMU *next;         // Link in free list
    uintptr *page;     // Pointer to page table page
    int index;         // PML4 entry index
    int level;         // Hierarchy level (2=PDPE, 1=PDE, 0=PTE)
};
```
- Per-CPU: mmupool.free
- Cached per-process: proc->mmuhead/mmutail

### Conf Structure (portdat.h)
```c
struct Conf {
    ulong nmach;       // Number of CPUs
    ulong nproc;       // Max processes
    ulong npage;       // Total memory pages
    ulong upages;      // User-available pages
    ulong nimage;      // Page cache images
    ulong nswap;       // Swap pages
    Confmem mem[64];   // Memory banks
};
```
- Global: conf
- Initialized by meminit0(), finalized by meminit()

---

## 5. ADDRESS SPACE LAYOUT

### Kernel Virtual Address Space (x86-64 pc64)
```
Address Range            Purpose
============            =======
0x00000000000000000     User space (canonical)
  to
0x00007fffffffffff      User space limit

0xffffffff80000000      KZERO (kernel starts)
0xffffffff80010000      KTZERO (kernel text)
0xffffffff...           End of kernel (etext)

0xfffffe0000000000      KMAP (2MB - small page temps)
0xfffffe8000000000      VMAP (1GB - device I/O mapping)
```

### Physical Memory Addressing
- 0 to MemMin: Boot kernel structures
- MemMin: Start of discovered RAM
- CPU0END: End of first CPU's workspace

### Key Kernel Memory Regions
```
CONFADDR (KZERO+0x1200)     Configuration info from bootloader
IDTADDR (KZERO+0x10000)     Interrupt descriptor table
CPU0PML4 (KZERO+0x13000)    Level-4 page table for CPU0
CPU0PDP (KZERO+0x14000)     Level-3 page table (PDPE)
CPU0PD0/PD1 (KZERO+0x15000) Level-2 page tables (PDE)
CPU0GDT (KZERO+0x17000)     Global descriptor table
CPU0MACH (KZERO+0x18000)    Mach structure (2*KSTACK)
```

---

## 6. ALLOCATION PATHS AND FLOW

### Early Boot: xalloc (before pageinit)
```
xinit()
  Initialize hole table from Confmem regions
  
KADDR(pa) [mmu.c]
  Convert physical to KZERO virtual (pa + KZERO)
  
xalloc(size)
  xallocz(size, 1) - allocate and zero
  Search hole table for big-enough hole
  Split hole if larger than needed
  Return KADDR(pa) virtual address
  
PADDR(va) [mmu.c]
  Convert KZERO virtual to physical
  Return va - KZERO
```

### Runtime: Pool Allocation (malloc/free)
```
malloc(size)
  poolalloc(mainmem, size)
  Returns zeroed allocation or nil
  
smalloc(size)
  Like malloc but waits for memory if needed
  Panics if memory never available
  
poolalloc
  Calls xalloc() for underlying memory
  xalloc finds hole from Confmem regions
```

### Page Allocation (user memory)
```
newpage(va, QLock *locked)
  Lock palloc
  While no free pages: sleep on palloc.pwait
  Find page matching color(va)
  If no color match, take first free page
  Increment ref count
  Return Page structure

freepages(head, tail, np)
  Link pages into free list
  Wake any waiting processes
  Update freecount
```

### Physical Page for Page Tables
```
rampage()
  If conf.mem[0].npage != 0:
    xspanalloc(BY2PG, BY2PG, 0) - use xalloc pool
  Else (during early boot before pageinit):
    memmapalloc(-1, BY2PG, BY2PG, MemRAM)
    Return KADDR(pa)
```

---

## 7. VIRTUAL MEMORY MAPPING

### Page Table Walk and Creation
```c
mmuwalk(uintptr *table, uintptr va, int level, int create)
  table = PML4 base
  level = 0 (PTE), 1 (PDE), 2 (PDPE), 3 (PML4E)
  
  Walk down page hierarchy:
    For i = 2 down to level:
      pte = table[PTLX(va, 3)]
      if (pte & PTEVALID):
        Follow to next level
      else if (create):
        mmucreate() - allocate new page table
      else:
        return nil
  
  Return address of final PTE
```

### Memory Mapping to Kernel Space
```c
pmap(pa, va, size)
  Map physical address pa to virtual va
  Size can span multiple page table levels
  Set PTEACCESSED, PTEDIRTY, PTEGLOBAL
  Handle 2MB large pages when aligned
  
vmap(pa, size)
  Map device memory to VMAP region
  Use uncached PTE flags
  Return virtual address in VMAP
  
kmap(page)
  Map page to temporary KMAP location
  If within KZERO-mappable range: use KADDR()
  Else: use rotating KMAP slots
  Returns KMap pointer (same as void*)
```

### Device Mapping
```c
VMAP (0xfffffe8000000000) - 1GB device I/O mapping
  vmap() adds device memory here uncached
  Same VMAP/KZERO PDPs shared by all CPUs
  No synchronization needed (global mapping)
```

---

## 8. CRITICAL INITIALIZATION ORDER

**Must happen in this sequence:**
1. mach0init() - Set up CPU0
2. meminit0() - Initialize memory map, add regions
3. ioinit() - Initialize I/O before archinit
4. cpuidentify() - Identify CPU before meminit0
5. archinit() - CPU/chipset-specific init (can call memreserve)
6. xinit() - Initialize xalloc after archinit
7. mmuinit() - Set up protection (before any user pages)
8. confinit() - Calculate memory splits
9. preallocpages() - Allocate palloc.pages array in VMAP
10. pageinit() - Initialize page allocator
11. userinit() - Create init process

**Why this order matters:**
- rampage() needs conf.mem[] populated by meminit0()
- preallocpages() needs xalloc working (from xinit())
- mmuinit() must happen before allocating user memory (for protection)
- pageinit() must happen after preallocpages() sets up palloc.pages

---

## 9. CURRENT INTEGRATION ISSUES

Based on kernel/9front-pc64/globals.c:

**Working:**
- Memory type constants (MemUPA, MemUMB, etc.)
- meminit0() and meminit() implementations
- rampage() using xallocz
- mapkzero() for mapping discovered regions
- E820 scan and memory test code

**Potential Issues:**
- HHDM (Higher Half Direct Mapping) support only partially integrated
- Some stubs for functions that may need real implementations
- vmxshutdown(), zeroprivatepages() are stubs
- poolreset(secrmem) in exit() needs real pool implementation

---

## 10. MEMORY REPORTING

During boot, pageinit() prints:
```
XXXM memory: AAAM kernel data, BBBM user, CCCM swap
```

Calculation:
- A = kernel image + data + structures
- B = user memory = (upages * BY2PG) / MiB
- C = conf.nswap * BY2PG / MiB

---

## SUMMARY TABLE: Key Functions and Their Roles

| Function | File | Purpose |
|----------|------|---------|
| meminit0() | pc/memory.c | Early memory map setup, BIOS discovery |
| meminit() | pc/memory.c | Finalize memory map, populate conf.mem[] |
| xinit() | port/xalloc.c | Initialize early allocation ("hole" system) |
| xalloc() | port/xalloc.c | Allocate kernel memory early |
| pageinit() | port/page.c | Initialize page allocator |
| newpage() | port/page.c | Allocate one user page |
| memmapadd() | port/memmap.c | Add memory region to map |
| memmapalloc() | port/memmap.c | Allocate from memory map |
| mmuinit() | pc64/mmu.c | Set up MMU protection |
| pmap() | pc64/mmu.c | Map physical to virtual |
| mmuwalk() | pc64/mmu.c | Walk/create page tables |
| kmap() | pc64/mmu.c | Map page to kernel space |
| vmap() | pc64/mmu.c | Map device to VMAP space |
| rampage() | pc/memory.c | Allocate page for page tables |

