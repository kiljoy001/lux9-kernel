# 9Front Memory Management Analysis - Document Index

Generated: November 1, 2025
Status: COMPLETE
Scope: Comprehensive 9front kernel memory management analysis

## Quick Start Reading Order

### If you have 15 minutes:
1. Read: MEMORY_QUICK_REFERENCE.md
2. Scan: ANALYSIS_SUMMARY.txt (Executive Summary section)

### If you have 1 hour:
1. Read: MEMORY_QUICK_REFERENCE.md (15 min)
2. Read: FILE_LOCATIONS_REFERENCE.md (20 min)
3. Skim: MEMORY_MANAGEMENT_ANALYSIS.md (25 min)

### If you have 3 hours (Complete Understanding):
1. Read: MEMORY_QUICK_REFERENCE.md (20 min)
2. Read: MEMORY_MANAGEMENT_ANALYSIS.md (60 min)
3. Read: FILE_LOCATIONS_REFERENCE.md (30 min)
4. Study: Source files in order:
   - 9/pc64/mem.h (constants)
   - 9/port/xalloc.c (early allocation)
   - 9/port/memmap.c (region tracking)
   - 9/port/page.c (page allocator)
   - 9/pc/memory.c (discovery)
   - 9/pc64/mmu.c (virtual mapping)

## Document Descriptions

### MEMORY_MANAGEMENT_ANALYSIS.md (14 KB, 478 lines)
**Complete technical reference**
- Sections: 10 major sections covering all aspects
- Depth: Very detailed, suitable for implementation
- Use when: Need to understand complete architecture
- Key sections:
  - Executive summary
  - Key files (7 critical, 10 supporting)
  - Memory initialization sequence (3 phases)
  - Physical address tracking
  - Key data structures
  - Address space layout
  - Allocation paths and flow
  - Virtual memory mapping
  - Critical initialization order
  - Current integration issues

### MEMORY_QUICK_REFERENCE.md (6.7 KB, 200 lines)
**Fast lookup and debugging guide**
- Format: Quick tables, code snippets, checklists
- Depth: Practical and concise
- Use when: Need quick answers, debugging issues
- Key sections:
  - 4 critical files to understand
  - Three memory systems overview
  - Boot sequence diagram
  - Address space map
  - Key data structures
  - Memory discovery process
  - Function allocation guide
  - Critical ordering rules
  - Common bugs to watch
  - Testing procedures

### FILE_LOCATIONS_REFERENCE.md (7.4 KB, 350 lines)
**Map of all source code and structures**
- Format: File paths with line counts, function lists
- Depth: Practical reference
- Use when: Need to find specific code
- Key sections:
  - Absolute file paths (7 core files)
  - Data structure definitions
  - Integration files
  - Support infrastructure
  - Function call graph
  - Module dependencies
  - Recommended reading order
  - Size summary

### ANALYSIS_SUMMARY.txt (13 KB, 300 lines)
**Executive overview and recommendations**
- Format: Structured summary with action items
- Depth: Strategic overview
- Use when: Planning debugging or fixes
- Key sections:
  - Findings summary
  - Three memory systems
  - Critical initialization sequence
  - Memory discovery process
  - Data structures summary
  - Address space layout
  - Current integration status
  - Key insights
  - Recommended next steps
  - Files to examine for bugs

## Key Concepts Quick Map

### The Three Memory Systems
1. **xalloc** - Early allocation (9/port/xalloc.c)
2. **memmap** - Region tracking (9/port/memmap.c)
3. **pageallocator** - Page management (9/port/page.c)

### Most Critical Files (Read in Order)
1. 9/pc64/mem.h - Address constants
2. 9/port/xalloc.c - Early allocation
3. 9/port/memmap.c - Region tracking
4. 9/port/page.c - Page allocator
5. 9/pc/memory.c - Discovery
6. 9/pc64/mmu.c - Virtual memory

### Key Data Structures
- **Confmem** - Memory bank definition
- **Page** - Physical page descriptor
- **Palloc** - Page allocator state
- **Mapent** - Memory region descriptor
- **MMU** - Page table cache entry
- **Hole** - Free region in early allocator

### Boot Phases (10 Key Steps)
1. mach0init() - CPU0 setup
2. meminit0() - Discover memory
3. cpuidentify() - CPU features
4. archinit() - Platform init
5. xinit() - Start xalloc
6. mmuinit() - Page tables
7. confinit() - Memory split
8. preallocpages() - Allocate arrays
9. pageinit() - Page allocator
10. userinit() - Init process

### Common Issues to Check
- conf.mem[] not populated before rampage()
- xalloc used after pageinit()
- preallocpages() called before xalloc ready
- HHDM offset not initialized
- Page table creation failures
- Memory report missing at boot

## Files by Topic

### Physical Memory Discovery
- `/home/scott/Repo/lux9-kernel/9/pc/memory.c` - Main discovery
- `/home/scott/Repo/lux9-kernel/9/port/memmap.c` - Region tracking
- Functions: meminit0(), e820scan(), ramscan(), lowraminit()

### Early Allocation
- `/home/scott/Repo/lux9-kernel/9/port/xalloc.c` - Hole-based allocator
- Functions: xinit(), xalloc(), xfree(), xhole()

### Page Management
- `/home/scott/Repo/lux9-kernel/9/port/page.c` - Page allocator
- Functions: pageinit(), newpage(), freepages()

### Virtual Memory
- `/home/scott/Repo/lux9-kernel/9/pc64/mmu.c` - Page tables
- `/home/scott/Repo/lux9-kernel/9/pc64/mem.h` - Constants
- Functions: pmap(), mmuwalk(), kmap(), vmap()

### Kernel Initialization
- `/home/scott/Repo/lux9-kernel/9/pc64/main.c` - Startup sequence
- Functions: main(), mach0init(), confinit()

### Our Integration
- `/home/scott/Repo/lux9-kernel/kernel/9front-pc64/globals.c` - Stubs
- `/home/scott/Repo/lux9-kernel/kernel/9front-pc64/memory_9front.c` - Copy of memory.c

## Debugging Checklist

When memory system fails:
- [ ] Is conf.mem[] populated after meminit()?
- [ ] Is pageinit() printing memory report?
- [ ] Is palloc.freecount > 0 after pageinit()?
- [ ] Are we calling xalloc after pageinit()?
- [ ] Is rampage() being called at correct time?
- [ ] Do page table walks complete successfully?
- [ ] Are address conversions (KADDR/PADDR) working?
- [ ] Is HHDM offset initialized?
- [ ] Are free lists properly linked?
- [ ] Is memory type tracking correct (MemRAM, MemUMB)?

## Search Keywords

Use these when looking for specific functionality:
- **"physical memory"** → memory.c, memmap.c
- **"allocat"** → xalloc.c, page.c, alloc.c
- **"page table"** → mmu.c, mem.h
- **"E820"** → memory.c (e820scan)
- **"BIOS"** → memory.c (lowraminit)
- **"KZERO"** → mem.h, mmu.c
- **"VMAP"** → mem.h, mmu.c, preallocpages
- **"MMU"** → mmu.c, dat.h
- **"Confmem"** → portdat.h, main.c
- **"palloc"** → page.c, portdat.h

## Useful Command Snippets

Count lines of code:
```bash
wc -l 9/pc64/mem.h 9/port/{xalloc,memmap,page,alloc}.c 9/pc64/mmu.c 9/pc/memory.c
```

Find all memory functions:
```bash
grep -n "^void.*\|^.*\*$" 9/port/memmap.c | head -20
```

Find all structs:
```bash
grep "^struct " 9/port/portdat.h | head -10
```

Trace initialization:
```bash
grep -n "meminit\|pageinit\|xinit" 9/pc64/main.c
```

## Document Statistics

| Document | Size | Lines | Use Case |
|----------|------|-------|----------|
| MEMORY_MANAGEMENT_ANALYSIS.md | 14 KB | 478 | Complete reference |
| MEMORY_QUICK_REFERENCE.md | 6.7 KB | 200 | Quick lookup |
| FILE_LOCATIONS_REFERENCE.md | 7.4 KB | 350 | Source navigation |
| ANALYSIS_SUMMARY.txt | 13 KB | 300 | Executive summary |
| This index | 6 KB | 280 | Navigation guide |
| **Total** | **47 KB** | **1600** | **Complete docs** |

## Last Updated

Created: November 1, 2025
Analyzed: 639 9front source files
Core code examined: ~4300 lines
Time to create: ~2 hours
Status: Ready for debugging and integration

## Next Steps

1. **For Debugging Memory Issues:**
   - Start with MEMORY_QUICK_REFERENCE.md
   - Use ANALYSIS_SUMMARY.txt "Recommended Next Steps" section
   - Check specific files using FILE_LOCATIONS_REFERENCE.md

2. **For Understanding Architecture:**
   - Read MEMORY_MANAGEMENT_ANALYSIS.md section by section
   - Reference FILE_LOCATIONS_REFERENCE.md for file paths
   - Study source files in recommended reading order

3. **For Fixing Specific Bugs:**
   - Find bug type in ANALYSIS_SUMMARY.txt "Files to Examine When Fixing Bugs"
   - Look up function in FILE_LOCATIONS_REFERENCE.md
   - Read implementation in source, reference MEMORY_MANAGEMENT_ANALYSIS.md

---

**All documents located in:** `/home/scott/Repo/lux9-kernel/docs/`

**Questions?** Check MEMORY_QUICK_REFERENCE.md for debugging tips.
