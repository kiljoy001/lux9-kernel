# HHDM MMU Debug Report - User Memory Access Issue

**Date**: 2025-10-06
**Status**: System hangs when calling pmap() to create user page table entries
**Git Commit**: d191266 (Fix HHDM bugs: IRETQ stack corruption and enable user MMU setup)

---

## Executive Summary

The lux9-kernel boots successfully through all kernel initialization and kproc() creation, but **hangs when trying to set up user page tables** in proc0(). The root cause is that the pure HHDM (Higher Half Direct Map) memory model requires explicit page table entries for user memory access, even though the kernel can access all physical memory via HHDM.

### Current Hang Location
- **Function**: `proc0()` in `kernel/9front-port/userinit.c`
- **Line**: 79 - call to `pmap(up->seg[SSEG], USTKTOP - BY2PG, ...)`
- **Debug Marker**: 'P' (appears in boot output, system hangs immediately after)

---

## Boot Progress Timeline

### ✅ Successfully Completed Stages

1. **Limine Bootloader** - Loads kernel at 0xffffffff80000000
2. **Paging Init** - Sets up 2MB pages (1GiB not supported)
3. **Memory Detection** - HHDM top at 0x10000000000
4. **Kernel Initialization** - All subsystems start successfully
5. **KPROC Creation** - Creates 10 kernel processes (I0-I9 markers)
6. **proc0() Entry** - Begins user process setup (J0 marker)
7. **Segment Creation** - Creates SSEG (stack) and TSEG (text) segments
8. **Page Allocation** - Allocates pages for stack and text

### ❌ Current Failure Point

9. **pmap() Call** - System hangs when trying to create MMU page table entries

---

## Technical Analysis

### Problem: User Memory Needs Page Tables

Even in the HHDM model where the kernel can access all physical memory directly, **user virtual addresses still require page table entries**:

```
User Address: 0x00007ffffffff000 (USTKTOP)
             ↓ (needs page table)
Physical:     0x???????? (allocated page)
             ↓ (HHDM mapping)
Kernel View:  0xffff800000?????? (direct access via HHDM)
```

### Why init0() Needs Page Tables

In `kernel/9front-pc64/main.c:271`, init0() tries to access user memory:
```c
sp = (char**)(USTKTOP - sizeof(Tos) - 8 - sizeof(sp[0])*4);  // = 0x7fffffffef80
sp[3] = sp[2] = nil;  // Tries to write to user address
```

Without page tables, this access has nowhere to go, even though the kernel could access the physical page via HHDM if it knew the mapping.

### Discovery Process

1. **Initial symptom**: Boot hung at J0 marker (after kproc creation)
2. **GDB investigation**: Found stuck at `movq $0x0,(%rax)` with rax=0x7fffffffef90
3. **mmuswitch() debugging**: Found proc->mmuhead was NULL
4. **Root cause**: No MMU page table entries were created for user segments
5. **Solution attempt**: Added explicit pmap() calls after segpage()
6. **New hang**: pmap() itself hangs

---

## Code Changes Made

### 1. Fixed IRETQ Stack Corruption (✅ Working)

**File**: `kernel/9front-pc64/l.s:1033`

```asm
# BEFORE (WRONG):
_iretnested:
    ADDQ    $40, SP    # Skips too much
    IRETQ

# AFTER (CORRECT):
_iretnested:
    ADDQ    $32, SP    # Skips 24 (allocated) + 8 (vector)
    IRETQ
```

**Result**: Boot now progresses past kproc creation successfully.

### 2. Enabled User MMU Setup (✅ Working)

**File**: `kernel/9front-pc64/mmu.c:808-812`

Removed early return that was skipping all user page table setup:
```c
# REMOVED:
/* TEMPORARY: Skip ALL MMU switching for now, just do taskswitch */
taskswitch((uintptr)proc);
return;
```

**Result**: mmuswitch() now runs user MMU code, but proc->mmuhead is still NULL.

### 3. Fixed qlock() Calls (✅ Working)

**File**: `kernel/9front-port/segment.c`

Fixed incorrect qlock calls:
```c
# BEFORE:
qlock(s);        // s is Segment*

# AFTER:
qlock(&s->qlock); // Correct
```

### 4. Allocated Initial Stack Page (✅ Working)

**File**: `kernel/9front-port/userinit.c:71-80`

```c
/* Allocate initial stack page and map it */
p = newpage(USTKTOP - BY2PG, nil);
k = kmap(p);
memset((uchar*)VA(k), 0, BY2PG);
kunmap(k);
segpage(up->seg[SSEG], p);
pmap(up->seg[SSEG], USTKTOP - BY2PG, PTEWRITE|PTEUSER, p->pa, BY2PG); // HANGS HERE
```

**Result**: Stack page allocated successfully, but pmap() hangs.

### 5. Added pmap() for Text Segment (Not reached due to hang)

**File**: `kernel/9front-port/userinit.c:96`

```c
segpage(up->seg[TSEG], p);
pmap(up->seg[TSEG], UTZERO, PTEUSER, p->pa, BY2PG);
```

---

## Debug Markers in Boot Output

Boot output ending: `...8FFFFFFFF80228780:[]{T@}sKM1QLKFFFF800000106E68:[]{T@}!2ML345FFFF800000106E68:[]{T@}6P`

### Decoded Marker Sequence:

| Marker | Location | Meaning |
|--------|----------|---------|
| `7` | userinit.c:63 | After cclone() |
| `8` | userinit.c:69 | After SSEG newseg() |
| `sKM1...6` | userinit.c:72-77 | Stack newpage(), kmap(), segpage() |
| `P` | userinit.c:78 | **BEFORE pmap() - HANGS HERE** |
| `S` | userinit.c:80 | (not reached) After pmap() |
| `9` | userinit.c:83 | (not reached) After TSEG newseg() |

### flushmmu() Debugging (Earlier Test):

When we tested without pmap(), flushmmu() output was:
- `F` - flushmmu() entry
- `S` - after splhi()
- `M` - before mmuswitch()
- `&*^KU1%` - mmuswitch() kernel KMAP setup
- `NULL` - **proc->mmuhead was NULL**
- `$` - after empty loop
- `X!` - flushmmu() return

This confirmed no user MMU entries were created.

---

## Memory Layout

### Kernel Address Space
```
0xffffffff80000000  - Kernel code/data (loaded by Limine)
0xffff800000000000  - HHDM base (all physical memory mapped here)
0xffff800000106e68  - Example physical page via HHDM
```

### User Address Space
```
0x0000000000000000  - UTZERO (text segment base)
0x00007fff00000000  - USTKTOP - USTKSIZE (stack base)
0x00007ffffffff000  - USTKTOP (stack top)
```

### Critical Constants
```c
#define USTKTOP     (0x00007ffffffff000ull)
#define USTKSIZE    (16*MiB)
#define UTZERO      (0x0000000000000000ull)
#define BY2PG       (4096)
#define PTEWRITE    (1<<1)
#define PTEUSER     (1<<2)
```

---

## Serial Boot Markers

To make sense of the serial noise, we emit two breadcrumb styles:

- Two-character pairs from `debugchar()` (e.g. `B0`, `77`, `C1`). Treat each consecutive pair as a milestone.
- Human-readable lines prefixed with `BOOT:` (e.g. `BOOT: mmuinit complete - runtime page tables live`).

### Key `debugchar()` pairs

| Marker | Location | Meaning |
| ------ | -------- | ------- |
| `B0`   | `main.c` | Borrow checker initialisation started |
| `B1`   | `main.c` | Borrow checker initialisation finished |
| `69`   | `main.c` | `chandevreset()` finished |
| `70`   | `main.c` | `initrd_register()` about to run |
| `71`   | `main.c` | `initrd_register()` finished |
| `72`   | `main.c` | `preallocpages()` about to run |
| `73`   | `main.c` | `preallocpages()` finished |
| `74`   | `main.c` | `pageinit()` about to run |
| `75`   | `main.c` | `pageinit()` finished |
| `77`   | `main.c` | About to call `userinit()` |
| `78`   | `main.c` | `userinit()` finished |
| `79`   | `main.c` | About to enter the scheduler |
| `C1`   | `proc0()` | Kernel process 0 entered |
| `C2`   | `proc0()` | Returned from `spllo()` |
| `C/`   | `proc0()` | `waserror()` succeeded |
| `Cs`   | `proc0()` | Stack page allocated and mapped |
| `Cp`   | `proc0()` | Init text page mapped |
| `Ch`   | `proc0()` | `flushmmu()` completed |
| `Ci`   | `proc0()` | `poperror()` completed |
| `Cj`   | `proc0()` | Returned from `init0()` (should not happen) |

These pairs appear concatenated in the log; reading them in twos reveals where boot paused.

### `BOOT:` text breadcrumbs

- `BOOT: printinit complete - serial console ready`
- `BOOT: borrow checker initialised`
- `BOOT: mmuinit complete - runtime page tables live`
- `BOOT: procinit0 complete - process table ready`
- `BOOT: device reset sequence finished`
- `BOOT: userinit scheduled *init* kernel process`
- `BOOT: entering scheduler - expecting proc0 hand-off`
- `BOOT[init0]: chandevinit complete`
- `BOOT[init0]: environment configured`
- `BOOT[init0]: entering user mode with initcode`
- `BOOT[proc0]: about to call init0 - switching to userspace`
- `BOOT[proc0]: init0 returned - this should never happen!`
- `BOOT[userinit]: spawned proc0 kernel process`

Use these markers to correlate serial output with boot progress without treating it as random noise.

---

## Why pmap() Hangs - Investigation Needed

### Possible Causes:

1. **Locking Issue**
   - segpage() may hold segment qlock
   - pmap() may try to acquire the same lock
   - Need to check if pmap() expects lock to be held or not

2. **HHDM Model Bug in pmap()**
   - pmap() may have assumptions that break in HHDM model
   - May try to access page tables before they exist
   - May have infinite loop or recursion

3. **Memory Allocation Issue**
   - pmap() needs to allocate page table pages
   - Page allocation might fail or loop
   - May run out of memory or hit allocator bug

4. **Missing Initialization**
   - proc->mmuhead infrastructure may need setup
   - MMU structures may be uninitialized

### Next Debug Steps:

1. **Add debug markers inside pmap()**
   ```c
   // At start of pmap()
   __asm__ volatile("outb %0, %1" : : "a"((char)'['), "Nd"((unsigned short)0x3F8));

   // At key points
   __asm__ volatile("outb %0, %1" : : "a"((char)'1'), "Nd"((unsigned short)0x3F8));

   // At return
   __asm__ volatile("outb %0, %1" : : "a"((char)']'), "Nd"((unsigned short)0x3F8));
   ```

2. **Check pmap() signature and implementation**
   ```bash
   grep -A 30 "^pmap" kernel/9front-pc64/mmu.c
   ```

3. **Verify locking requirements**
   - Does pmap() expect segment qlock to be held?
   - Should we unlock before calling pmap()?

4. **Consider alternative approaches**
   - Manually create page table entries instead of using pmap()
   - Implement HHDM-specific page table setup
   - Defer page table creation until page fault

---

## Alternative Solution: Page Fault Handler

In original Plan 9, user memory access triggers page faults which:
1. Check if address is in a segment
2. Call segpage() to get the page
3. Call pmap() to install page table entry
4. Resume execution

The HHDM model may have broken this flow. We could:
- Enable page faults for user addresses
- Fix page fault handler to work with HHDM
- Let page tables be created on-demand

---

## Files Modified

### Primary Changes:
1. `kernel/9front-pc64/l.s` - IRETQ fix
2. `kernel/9front-pc64/mmu.c` - Removed MMU early return, added debug
3. `kernel/9front-port/userinit.c` - Added stack page allocation and pmap() calls
4. `kernel/9front-port/segment.c` - Fixed qlock calls

### Debug Markers Added To:
- `kernel/9front-pc64/mmu.c` - flushmmu(), mmuswitch()
- `kernel/9front-port/userinit.c` - proc0()
- `kernel/9front-pc64/main.c` - init0()
- `kernel/9front-pc64/trap.c` - Page fault handler
- `kernel/9front-port/qlock.c` - Lock operations
- `kernel/9front-port/taslock.c` - Lock operations
- `kernel/9front-pc64/boot.c` - Early boot
- `kernel/9front-pc64/globals.c` - Global init

---

## Build Commands

```bash
# Clean rebuild
make clean
make

# Build ISO
make iso

# Test boot
qemu-system-x86_64 -M q35 -m 2G -cdrom lux9.iso -serial stdio \
  -display none -no-reboot -no-shutdown

# Test with GDB
qemu-system-x86_64 -M q35 -m 2G -cdrom lux9.iso -serial stdio \
  -display none -no-reboot -no-shutdown -s -S

# In another terminal:
gdb lux9.elf
(gdb) target remote localhost:1234
(gdb) continue
```

---

## GDB Investigation Results

### Previous GDB Session (Before pmap() attempt):

```
RIP: 0xffffffff802a1560 <init0+562>
Instruction: movq $0x0,(%rax)
RAX: 0x7fffffffef90

Disassembly around hang:
0xffffffff802a1557:  lea    -0x1070(%rbx),%rax  # Calculate sp address
0xffffffff802a155e:  movq   $0x0,0x8(%rax)     # sp[2] = nil (succeeds)
0xffffffff802a1560:  movq   $0x0,(%rax)         # sp[3] = nil (HANGS)
```

The address 0x7fffffffef90 is in user space with no page table entry.

---

## Key Insights

1. **HHDM is not a silver bullet** - Even though kernel can access all physical memory directly, user processes still need proper page tables for virtual address translation.

2. **Page tables are mandatory** - x86-64 hardware requires valid page table entries for user addresses, regardless of HHDM.

3. **segpage() != pmap()** - `segpage()` adds a page to the segment's internal map, but does NOT create MMU page table entries. That's pmap()'s job.

4. **proc->mmuhead chain** - This linked list is populated by pmap() and used by mmuswitch() to install user page tables into the kernel's PML4.

5. **pmap() is critical** - Without working pmap(), user processes cannot run.

---

## Success Criteria

Boot should proceed past the 'P' marker and show:
```
...6PS...  (S = after successful pmap() for stack)
...dpe...  (p = before pmap() for text, e = after)
...fgFSM...  (flushmmu() with user entries)
```

Then proc->mmuhead should contain entries, mmuswitch() should install them, and init0() should successfully access user memory.

---

## References

### Key Source Files:
- `kernel/9front-pc64/mmu.c` - MMU management, pmap() implementation
- `kernel/9front-port/userinit.c` - proc0(), user process setup
- `kernel/9front-pc64/main.c` - init0(), first user code
- `kernel/9front-port/segment.c` - Segment management
- `kernel/9front-port/fault.c` - Page fault handler
- `kernel/include/mem.h` - Memory layout constants

### Useful Functions:
- `pmap(seg, va, prot, pa, size)` - Create page table mapping
- `segpage(seg, page)` - Add page to segment
- `newpage(va, seg)` - Allocate new page
- `mmuswitch(proc)` - Install process page tables
- `flushmmu()` - Flush TLB and update MMU

---

## Boot Output Samples

### Latest Boot (Hangs at pmap):
```
...7ML[FFFF800000106E60]Zz8FFFFFFFF80228780:[]{T@}sKM1QLKFFFF800000106E68:[]{T@}!2ML345FFFF800000106E68:[]{T@}6P
```

### Previous Boot (Without pmap, NULL mmuhead):
```
...efgFSM&*^KU1%NULL$=+-_F~;X!hiI0T0...
```

The 'NULL' marker confirmed proc->mmuhead was NULL when mmuswitch() ran.

---

## Recommendations for Next Session

1. **Instrument pmap()** - Add debug markers to understand where it hangs
2. **Check locking** - Verify if qlock needs to be held/released
3. **Examine HHDM changes** - Review what changes were made to pmap() for HHDM
4. **Consider workaround** - Manually build page table entries if pmap() is too broken
5. **Test page faults** - Enable page fault handling as alternative approach

---

**End of Report**
- `PB?[…]` — `pageinit()` reporting a memory bank (`?` is `A`+index, contents show base and page count in hex).
- `PF…U…I…` — `pageinit()` summary (`PF` = free pages, `U` = unmapped skips, `I` = poisoned skips).
- `CL[…]` — `cankaddr()` reporting the physical-address limit it enforces (currently 4 GiB).
- `CN[…]` — milestone addresses emitted roughly every 512 MiB while `cankaddr()` walks the map.
- `C0[…]` — `cankaddr()` rejecting an address above its limit.
- `KA[…]` — first successful `kaddr()` translation (`{}` contains the saved HHDM offset).
