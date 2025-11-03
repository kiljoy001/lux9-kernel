# 9Front Memory Management - File Locations Reference

## Core Memory Management Files (MUST STUDY)

### Absolute File Paths

**1. Physical Memory Discovery & Initialization**
- `/home/scott/Repo/lux9-kernel/9/pc/memory.c` (726 lines)
  - rampage() - allocate page for page tables
  - meminit0() - early boot memory setup
  - meminit() - finalize memory map
  - lowraminit() - BIOS region handling
  - e820scan() - E820 BIOS memory map
  - ramscan() - fallback memory test
  - mapkzero() - map discovered regions

**2. Memory Type Tracking**
- `/home/scott/Repo/lux9-kernel/9/port/memmap.c` (272 lines)
  - Mapent structure (type, addr, size)
  - memmapadd() - add region to map
  - memmapalloc() - allocate from map
  - memmapfree() - free allocated region
  - memmapnext() - iterate regions
  - memmapsize() - query region size
  - Memory type constants (MemRAM, MemUMB, etc.)

**3. Early Kernel Allocation (xalloc)**
- `/home/scott/Repo/lux9-kernel/9/port/xalloc.c` (270 lines)
  - xinit() - initialize hole table
  - xalloc() / xallocz() - allocate memory
  - xfree() - free memory
  - xhole() - manage holes
  - xsummary() - debug output
  - Hole structure (tracks free regions)

**4. Page Allocator**
- `/home/scott/Repo/lux9-kernel/9/port/page.c` (300+ lines)
  - pageinit() - initialize page allocator
  - nkpages() - calculate kernel pages
  - newpage() - allocate user page
  - freepages() - return pages to free list
  - pagereclaim() - reclaim cached pages
  - deadpage() - decrement page refcount
  - putpage() - free single page
  - copypage() - copy page content
  - fillpage() - fill page with value
  - Palloc structure (global page allocator state)

**5. Virtual Memory Management**
- `/home/scott/Repo/lux9-kernel/9/pc64/mmu.c` (726 lines)
  - mmuinit() - set up MMU and protection
  - pmap() - map physical to virtual
  - punmap() - unmap virtual addresses
  - mmuwalk() - walk/create page tables
  - mmucreate() - create page table page
  - kmap() / kunmap() - temporary kernel mappings
  - vmap() / vunmap() - device memory mapping
  - preallocpages() - preallocate page structures
  - mmuswitch() - context switch MMU state
  - mmurelease() - clean up MMU state
  - kernelro() - make kernel text read-only

**6. Memory Configuration & Pool Allocation**
- `/home/scott/Repo/lux9-kernel/9/port/alloc.c` (250+ lines)
  - malloc(), mallocz(), smalloc() - kernel memory allocation
  - ralloc() - realloc
  - Pool structures (mainmem, imagmem, secrmem)

**7. Kernel Initialization (Calls Everything)**
- `/home/scott/Repo/lux9-kernel/9/pc64/main.c` (370 lines)
  - main() - entry point with initialization sequence
  - mach0init() - initialize CPU0 structures
  - machinit() - generic CPU initialization
  - confinit() - calculate memory configuration
  - Memory initialization call sequence (lines 189-217)

## Data Structure Definition Files

**Header Files with Key Structures:**

**Memory Layout & Constants**
- `/home/scott/Repo/lux9-kernel/9/pc64/mem.h` (186 lines)
  - KZERO, KTZERO (kernel address space)
  - VMAP, VMAPSIZE (device I/O mapping)
  - KMAP, KMAPSIZE (temporary kernel mappings)
  - BY2PG, PGSHIFT (page size constants)
  - PTE flags (PTEVALID, PTEUSER, PTEWRITE, etc.)
  - Page table macros (PTLX, PGLSZ)
  - Memory address macros (ALIGNED, ROUND, PGROUND)

**Architecture-Specific Data**
- `/home/scott/Repo/lux9-kernel/9/pc64/dat.h` (360 lines)
  - Confmem structure (memory bank definition)
  - MMU structure (page table cache entry)
  - Mach structure (per-CPU state, includes pml4)
  - PMMU structure (per-process MMU state)

**Port-Level Data Structures**
- `/home/scott/Repo/lux9-kernel/9/port/portdat.h` (1000+ lines)
  - Page structure (individual physical page)
  - Palloc structure (page allocator state)
  - Conf structure (global configuration)
  - Swapalloc structure (swap system state)
  - Image structure (page cache/swap image)

## Our Integration Files

**Files We Created/Modified:**

**Current State:**
- `/home/scott/Repo/lux9-kernel/kernel/9front-pc64/globals.c` (487 lines)
  - HHDM offset and base variables
  - rampage() implementation using xallocz
  - meminit0() and meminit() from 9front
  - Memory type constants
  - Global device pointers and pools

**Memory-Specific Code:**
- `/home/scott/Repo/lux9-kernel/kernel/9front-pc64/memory_9front.c` (585 lines)
  - Complete 9front memory.c with:
  - rampage()
  - meminit0() 
  - meminit()
  - E820 scanning
  - Memory testing
  - BIOS region handling

## Supporting Infrastructure Files

**Bootloader Configuration**
- `/home/scott/Repo/lux9-kernel/kernel/boot/limine.conf`
  - Boot parameters passed to kernel
  - E820 memory map provided here

**Build System**
- `/home/scott/Repo/lux9-kernel/9/pc64/mkfile` (177 lines)
  - Line 54: memory.$O from ../pc/memory.c
  - Line 26: memmap.$O from ../port/memmap.c
  - Line 27: page.$O from ../port/page.c
  - Compilation rules and dependencies

## Function Call Graph (Memory Initialization)

```
main() [pc64/main.c:178]
├─ mach0init() [pc64/main.c:131]
├─ bootargsinit()
├─ trapinit0()
├─ ioinit()
├─ i8250console()
├─ quotefmtinstall()
├─ screeninit()
├─ print("\nPlan 9\n")
├─ cpuidentify()
├─ meminit0() [pc/memory.c:473] ← MEMORY SYSTEM START
│  ├─ lowraminit() [pc/memory.c:104]
│  ├─ e820scan() [pc/memory.c:344]
│  └─ ramscan() [pc/memory.c:398]
├─ archinit()
│  └─ Can call memreserve() to protect BIOS/ACPI
├─ clockinit()
├─ meminit() [pc/memory.c:553] ← FINALIZE MEMORY MAP
├─ ramdiskinit()
├─ confinit() [pc64/main.c:20] ← CALCULATE SPLITS
├─ xinit() [port/xalloc.c:42] ← START EARLY ALLOCATOR
├─ trapinit()
├─ mathinit()
├─ i8237alloc()
├─ pcicfginit()
├─ bootscreeninit()
├─ printinit()
├─ cpuidprint()
├─ mmuinit() [pc64/mmu.c:76] ← SET UP PAGE TABLES
├─ intrinit()
├─ timersinit()
├─ clockenable()
├─ procinit0()
├─ initseg()
├─ links()
├─ chandevreset()
├─ preallocpages() [pc64/mmu.c:671] ← ALLOCATE PALLOC.PAGES
├─ pageinit() [port/page.c:17] ← START PAGE ALLOCATOR
├─ userinit()
└─ schedinit()
```

## What Each Module Needs from Others

### memory.c needs:
- memmap functions (memmapadd, memmapalloc, memmapnext, memmapsize)
- KADDR, PADDR macros (from mmu.c)
- conf.mem[], cankaddr (from main initialization)
- pmap() for memory mapping
- mtrrattr() for MTRR checking

### xalloc.c needs:
- conf.mem[], conf.npage (from confinit)
- KADDR, PADDR for address conversion
- nelem() macro

### page.c needs:
- palloc global variable
- conf.mem[], conf.npage
- newpage sleeps on palloc.pwait (Rendezq)
- kickpager() to wake pager

### mmu.c needs:
- m->pml4, m->mmufree (per-CPU state)
- Page structures from page allocator
- rampage() for allocating page tables
- KZERO, VMAP address definitions
- kaddr(), paddr() conversion

### alloc.c needs:
- xalloc/xfree for underlying allocation
- Pool structure definitions

## Size Summary

Total memory management code:
- Core 9front port: ~2200 lines
- PC64 architecture code: ~1100 lines
- Our integration code: ~1000 lines
- **Total: ~4300 lines**

Key reading order (shortest to longest):
1. mem.h (186 lines) - Understand constants
2. MEMORY_QUICK_REFERENCE.md (200 lines) - Understand concepts
3. xalloc.c (270 lines) - Early allocation
4. memmap.c (272 lines) - Region tracking
5. page.c (300 lines) - Page management
6. mmu.c (726 lines) - Virtual memory
7. memory.c (726 lines) - Physical discovery
8. MEMORY_MANAGEMENT_ANALYSIS.md (478 lines) - Complete picture

