# Bug Report: PC Corruption (0x2000d3 → 0x200009)

## Symptom
- System crashes with "general protection violation pc=0x200009"
- Occurs after SYSCALL[11] completes successfully
- 0x200009 is +2 bytes into a call instruction at 0x200007 (invalid instruction boundary)

## Root Cause: TLB Aliasing Between Parent and Child Processes

### The Problem
1. **init** process loads at virtual address 0x200000
   - Has `call main` instruction at 0x200007

2. **init forks** to create child process
   - Child inherits page tables via `procfork()`
   - Child's 0x200000 initially points to init's code pages

3. **Child execs wasm_test**
   - wasm_test also loads at 0x200000 (same virtual address!)
   - exec replaces segments, but **TLB may still cache old mappings**

4. **Child makes syscall**
   - Syscall returns with correct PC=0x2000d3 (from wasm_test)
   - But CPU's TLB might fetch from WRONG physical page
   - If TLB still has init's mapping for 0x200000, CPU executes init's code!
   - Result: PC appears as 0x200009 (from init's call instruction)

### Evidence
```bash
$ objdump -d boot/init | grep "200007:"
200007:	e8 14 02 00 00       	call   200220 <main>

$ objdump -d boot/wasm_test | grep -A 2 "2000d1:"
2000d1:	0f 05                	syscall
2000d3:	c3                   	ret    <- CORRECT return address
```

Both binaries load at 0x200000, causing virtual address collision!

### Why CLI/Timing Workarounds Didn't Work
- Disabling interrupts (CLI) had no effect
- Adding timing delays had no effect
- **Because it's not an interrupt/timing issue - it's a TLB coherency issue!**

## The Fix

### Option 1: Flush TLB After Exec (Immediate Fix)
In `kernel/9front-port/sysproc.c`, function `sysexec()`, after loading new segments:

```c
/* After exec loads new binary, flush TLB to remove parent's cached mappings */
__asm__ volatile("mov %%cr3, %%rax; mov %%rax, %%cr3" ::: "rax", "memory");
```

This reloads CR3, forcing complete TLB flush.

### Option 2: Use Different Load Addresses (Proper Fix)
Modify linker scripts so binaries load at different addresses:
- init: 0x200000
- wasm_test: 0x400000
- Other processes: 0x600000, 0x800000, etc.

This prevents virtual address aliasing entirely.

### Option 3: ASID/PCID Support (Future Enhancement)
Implement address space identifiers to avoid TLB flushes on context switch.
Requires CR4.PCIDE and proper PCID management.

## Verification
The ACSL analysis in `kernel/9front-pc64/syscall_stack_verify.acsl` confirms:
- Stack layout is CORRECT
- Register save/restore is CORRECT
- The corruption is external to the syscall return path

## Related Commits
- 7a21db2e: "Fix sys_rfork MMU aliasing: invalidate parent exchange page before fork"
  - This fixed exchange page aliasing but didn't address TEXT segment aliasing
- 626d2995: "Fix sys_rfork hang and resurrection server startup"
  - Mentioned IF flag fix but didn't fully address TLB issues

## Recommendation
Implement Option 1 (TLB flush after exec) immediately to unblock testing.
Plan Option 2 (different load addresses) for proper long-term fix.
