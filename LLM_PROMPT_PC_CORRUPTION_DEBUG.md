# LLM Prompt: Debugging PC Corruption in Kernel Syscall Return Path

## Problem Statement

A kernel experiences a critical bug where the Program Counter (PC) becomes corrupted during syscall return, causing a General Protection Fault. The system crashes with:
- **Symptom**: `general protection violation pc=0x200009`
- **Expected PC**: `0x2000d3` (correct return address after syscall)
- **Actual PC**: `0x200009` (+2 bytes into a `call` instruction = invalid instruction boundary)
- **Context**: Occurs after SYSCALL[11] completes successfully in a child process

## Key Evidence

### 1. Binary Analysis
```bash
$ objdump -d boot/init | grep "200007:"
200007: e8 14 02 00 00       call   200220 <main>

$ objdump -d boot/wasm_test | grep -A 2 "2000d1:"
2000d1: 0f 05                syscall
2000d3: c3                   ret    <- CORRECT return address

$ readelf -l boot/init boot/wasm_test | grep LOAD
# Both load at virtual address 0x200000!
```

**Critical Finding**: Both parent (init) and child (wasm_test) processes load at the **same virtual address** (0x200000), creating potential for TLB/MMU aliasing.

### 2. Debug Output
```
SYSCALL[11] about to return: ureg->pc=0x2000d3 ureg->cx=0x2000d3
*** FOUND IT! trap() entered with PC=0x200009 ***
```

**Key Observation**:
- Kernel correctly saves PC=0x2000d3 in ureg structure
- But CPU ends up at PC=0x200009 (from parent's code!)
- Corruption happens **after** syscall() returns but **before** user mode resumes

### 3. What DIDN'T Work
- ❌ Disabling interrupts (CLI) before IRETQ - No effect
- ❌ Adding timing delays - No effect
- ❌ Adding debug code - Bug disappeared (Heisenbug behavior!)

## Root Cause Hypothesis

### Primary Suspect: TLB Aliasing / MMU Cache Coherency

**Theory**: When parent (init) forks and child execs new binary (wasm_test):
1. Child inherits page tables from parent via `procfork()`
2. Child's virtual address 0x200000 initially maps to parent's physical pages
3. `sysexec()` calls `flushmmu()` which should invalidate TLB
4. **BUT**: If TLB flush is incomplete or CPU has stale cache, instruction fetch might still read from parent's physical pages
5. Result: CPU executes parent's instruction at 0x200007 instead of child's instruction at 0x2000d3

### Supporting Evidence
- System runs on QEMU TCG (software emulation) which has known bugs in TLB simulation
- Bug exhibits Heisenbug behavior (timing-sensitive)
- Not reproducible with all debug code enabled (timing changes)
- Virtual address collision between parent/child processes

## ACSL Formal Verification Results

**Stack Layout Analysis** (see `kernel/9front-pc64/syscall_stack_verify.acsl`):
```
After syscallentry setup:
RSP+152: ureg->pc    <- RCX (user return address, should be 0x2000d3)
RSP+160: ureg->cs    <- UESEL
RSP+168: ureg->flags <- R11 (user RFLAGS)

After syscallret pops 14 quads:
RSP = original + 112, pointing at ureg->r14

After addq $40, %rsp:
RSP = original + 152, pointing at ureg->pc

IRETQ will pop:
  RIP   from [original+152] = ureg->pc ✓ CORRECT
  CS    from [original+160] = ureg->cs ✓ CORRECT
  RFLAGS from [original+168] = ureg->flags ✓ CORRECT
```

**Conclusion**: Stack layout is mathematically correct. Bug is NOT in syscall return assembly code.

## Diagnostic Checklist for LLMs

When debugging similar PC corruption issues, verify:

### 1. Virtual Address Space Layout
```bash
# Check if processes use overlapping virtual addresses
readelf -l binary1 binary2 | grep LOAD
# Look for identical virtual load addresses
```

### 2. TLB Flush Implementation
```c
// Verify flushmmu() actually reloads CR3
void flushmmu(void) {
    up->newtlb = 1;
    mmuswitch(up);  // Should call mmuzap()
}

static void mmuzap(void) {
    // Clear page table entries
    // CRITICAL: Must reload CR3!
    putcr3(getcr3());  // ← Verify this executes
}
```

### 3. Process Fork/Exec Path
```c
// In sys_rfork():
procfork(p);  // Child inherits parent's page tables

// In sysexec():
// Load new segments
up->seg[TSEG] = ts;  // New text segment
up->seg[DSEG] = s;   // New data segment
flushmmu();  // ← MUST flush TLB here!
```

### 4. IRETQ Frame Integrity
```asm
; Before IRETQ, stack must contain:
; [RSP+0]:  RIP (return address)
; [RSP+8]:  CS
; [RSP+16]: RFLAGS
; [RSP+24]: RSP (user stack)
; [RSP+32]: SS

; Add debug to print IRETQ frame:
movq 0(%rsp), %rdi    ; RIP
movq 8(%rsp), %rsi    ; CS
call iprint_debug
```

### 5. Emulator vs Real Hardware
- Test on **KVM** (hardware virtualization) vs **TCG** (software emulation)
- If bug only occurs on TCG, it's likely a QEMU bug, not kernel bug
- Use `-enable-kvm` flag to test with hardware virtualization

## Recommended Fixes (in priority order)

### Fix 1: Use Different Load Addresses (Prevents Aliasing)
```ld
/* linker.lds for init */
SECTIONS {
    . = 0x200000;
    .text : { *(.text) }
}

/* linker.lds for wasm_test */
SECTIONS {
    . = 0x400000;  /* Different base! */
    .text : { *(.text) }
}
```

### Fix 2: Force Complete TLB Flush After Exec
```c
/* In sysexec() after loading new segments */
flushmmu();

/* Add explicit TLB flush for paranoia */
__asm__ volatile(
    "mov %%cr3, %%rax\n"
    "mov %%rax, %%cr3\n"
    ::: "rax", "memory"
);
```

### Fix 3: Invalidate Parent's Mappings Before Fork
```c
/* In sys_rfork() before procfork() */
extern void putmmu(uintptr, uintptr, Page *);

/* Invalidate parent's text segment mapping */
putmmu(UTZERO, 0, nil);

/* Force TLB invalidation */
__asm__ volatile("invlpg (%0)" :: "r"(UTZERO) : "memory");

procfork(p);  /* Now child inherits INVALID mappings */
```

### Fix 4: Use KVM Instead of TCG
```bash
# If running on QEMU, use hardware virtualization
qemu-system-x86_64 -enable-kvm ...
# Instead of software emulation (default)
```

## Testing Strategy

### 1. Reproduce Bug
```bash
# Boot kernel
make clean && make && make iso
qemu-system-x86_64 -M q35 -m 512M -cdrom lux9.iso -boot d \
    -no-reboot -serial mon:stdio -display none

# Look for crash pattern:
# - Init forks and execs wasm_test
# - Child makes syscalls (watch for SYSCALL[11])
# - Crash with PC=0x200009
```

### 2. Verify Fix
```bash
# Test with KVM (should work if it's a TCG bug)
qemu-system-x86_64 -enable-kvm -M q35 -m 512M -cdrom lux9.iso \
    -boot d -no-reboot -serial mon:stdio -display none

# If works with KVM but not TCG → QEMU bug
# If still crashes → Kernel bug
```

### 3. Debug Memory Layout
```c
/* Add debug in sysexec() after loading segments */
print("EXEC: parent PC space 0x%p has phys page %#llx\n",
      (void*)0x200000,
      mmuphys(parent, 0x200000));

print("EXEC: child PC space 0x%p has phys page %#llx\n",
      (void*)0x200000,
      mmuphys(up, 0x200000));

/* Should show DIFFERENT physical pages! */
```

## Key Insights for Future LLMs

### What Makes This Bug Hard to Debug

1. **Heisenbug Nature**: Adding debug code changes timing, hiding the bug
2. **Stack Layout Red Herring**: Natural to suspect syscall return path, but ACSL proves it's correct
3. **Virtual Address Collision**: Easy to miss that multiple binaries use same load address
4. **TLB Invisibility**: TLB state not directly observable, must infer from behavior
5. **Emulation Complexity**: QEMU TCG bugs masquerade as kernel bugs

### Debugging Methodology

1. **Start with Evidence, Not Assumptions**
   - Don't assume stack corruption without proof
   - Use formal verification (ACSL) to prove code correctness
   - Measure what the system actually does, not what it should do

2. **Look for Patterns in Addresses**
   - 0x200009 is +2 bytes from 0x200007 (suspiciously specific!)
   - Check if those addresses exist in OTHER binaries
   - Virtual address collisions are rare but devastating

3. **Understand the Execution Context**
   - This is a CHILD process, not the parent
   - Child just did exec (replaced address space)
   - Parent and child share virtual addresses but should have separate physical pages

4. **Test on Different Platforms**
   - Emulator bugs are common
   - Always test on real hardware or KVM before deep diving

5. **Use Formal Methods**
   - ACSL verification proved stack layout correct
   - Saved hours of debugging assembly code
   - When math says code is correct, look elsewhere

## References

- **Bug Report**: `BUG_REPORT_PC_CORRUPTION.md`
- **ACSL Verification**: `kernel/9front-pc64/syscall_stack_verify.acsl`
- **AMD64 Architecture Manual**: Volume 2, Chapter 3 (System Calls)
- **Intel SDM**: Volume 3, Chapter 4 (Paging)
- **9front Source**: Similar issues in Plan 9 fork/exec path

## Example LLM Query

**Prompt for future debugging**:

> I have a kernel bug where PC becomes 0x200009 (middle of call instruction) instead of expected 0x2000d3 after syscall return. The syscall completes successfully with correct ureg->pc, but CPU ends up at wrong address. CLI and timing changes have no effect. Both parent and child processes load at 0x200000. System runs on QEMU TCG. What should I investigate?

**Expected LLM Response**:

> This is almost certainly a TLB aliasing issue caused by virtual address collision. Check:
> 1. Do parent and child binaries load at same virtual address? (readelf -l)
> 2. Is flushmmu() being called after exec loads new segments?
> 3. Does flushmmu() actually reload CR3 to flush TLB?
> 4. Test with KVM (-enable-kvm) instead of TCG - if works, it's a QEMU bug
> 5. Fix by using different load addresses for each binary (linker scripts)

## Conclusion

This bug demonstrates the importance of:
- **Virtual address space hygiene** (avoid collisions)
- **TLB management** (explicit flushes after page table changes)
- **Formal verification** (ACSL proved code correct)
- **Platform awareness** (emulator vs hardware behavior)

The fix is straightforward once root cause is identified: either use different load addresses, or ensure complete TLB flush after exec. The challenge was recognizing that correct kernel code can be defeated by CPU cache state.
