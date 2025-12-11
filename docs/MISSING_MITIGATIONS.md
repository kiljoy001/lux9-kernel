# Standard Security Mitigations - Missing in Lux9

This document lists industry-standard exploit mitigations that Lux9 currently lacks, with implementation guidance.

---

## Priority 1: CRITICAL (Must Fix for Any Production Use)

### 1. ASLR (Address Space Layout Randomization)

**What it does:** Randomizes memory addresses (kernel base, stack, heap) on each boot
**Why you need it:** Makes ROP/JOP attacks extremely difficult
**Current status:** ❌ Kernel always loads at fixed address

**How to implement:**
```c
// In bootloader/early init:
1. Get random value from TPM/RDRAND (≥28 bits entropy)
2. Relocate kernel to: KERNEL_BASE + (random & PAGE_MASK)
3. Update all absolute addresses in kernel
4. Requires PIE (Position Independent Executable) compilation
```

**Compiler flags:**
```makefile
CFLAGS += -fPIE -fPIC
LDFLAGS += -pie
```

**Test:** `cat /proc/kaslr` should show different addresses per boot

---

### 2. DEP/NX (Data Execution Prevention / No-Execute)

**What it does:** Marks data pages (stack, heap) as non-executable
**Why you need it:** Prevents injected shellcode from running
**Current status:** ❌ Stack is executable

**How to implement:**
```c
// In page table setup (kernel/9front-pc64/mmu.c):
PTE_NX = (1ULL << 63);  // NX bit in page table entry

// For stack pages:
pte |= PTE_NX;

// Only code pages should be executable:
if (is_code_section(addr))
    pte &= ~PTE_NX;  // Allow execute
else
    pte |= PTE_NX;   // Block execute
```

**Linker flags:**
```makefile
LDFLAGS += -z noexecstack   # Mark stack non-executable
LDFLAGS += -z now           # Resolve symbols at load time
```

**Test:** Try executing from stack - should get SIGSEGV

---

### 3. Stack Canaries

**What they do:** Place magic value before return address, check on function exit
**Why you need them:** Detect buffer overflows before attacker overwrites RIP
**Current status:** ❌ No canaries

**How to implement:**
```c
// Enable in compiler:
CFLAGS += -fstack-protector-strong

// Implement in kernel:
void __stack_chk_fail(void) {
    panic("Stack smashing detected!");
}

// Compiler inserts automatically:
void my_function() {
    unsigned long canary = __stack_chk_guard;  // Load at entry
    // ... function body ...
    if (canary != __stack_chk_guard)           // Check at exit
        __stack_chk_fail();
}
```

**Initialization:**
```c
// Early boot (kernel/9front-pc64/main.c):
extern unsigned long __stack_chk_guard;
__stack_chk_guard = rdtsc() ^ rdrand();  // Random per boot
```

**Test:** Intentional overflow should panic kernel

---

### 4. SMEP/SMAP (Supervisor Mode Execution/Access Prevention)

**What they do:**
- SMEP: Kernel can't execute code in user pages
- SMAP: Kernel can't access user pages (without explicit override)

**Why you need them:** Prevent kernel exploits that pivot to user code
**Current status:** ❌ Not enabled

**How to implement:**
```c
// In CPU initialization (kernel/9front-pc64/main.c):
void enable_smep_smap(void) {
    unsigned long cr4;

    // Check CPU support
    if (cpuid_has_smep())
        cr4 |= CR4_SMEP;  // Bit 20
    if (cpuid_has_smap())
        cr4 |= CR4_SMAP;  // Bit 21

    write_cr4(cr4);
}

// When kernel MUST access user pages (copy_from_user):
void *copy_from_user(void *kbuf, const void *ubuf, size_t n) {
    stac();  // Temporarily disable SMAP
    memcpy(kbuf, ubuf, n);
    clac();  // Re-enable SMAP
}
```

**Test:** Kernel jump to user address should triple-fault

---

## Priority 2: HIGH (Production Hardening)

### 5. W^X (Write XOR Execute)

**What it does:** No page can be both writable and executable
**Why you need it:** Prevents runtime code generation attacks
**Current status:** ❌ Not enforced

**Implementation:**
```c
// In mmap/memory allocation:
if (prot & PROT_WRITE)
    pte |= PTE_NX;  // Writable → Not executable

if (prot & PROT_EXEC)
    pte &= ~PTE_W;  // Executable → Not writable
```

---

### 6. Kernel Page Table Isolation (KPTI)

**What it does:** Separate page tables for kernel/user mode
**Why you need it:** Mitigates Meltdown (CVE-2017-5754)
**Current status:** ❌ Not implemented

**Implementation:** Complex, see Linux kernel `CONFIG_PAGE_TABLE_ISOLATION`

---

### 7. Integer Overflow Protection

**What it does:** Trap on signed integer overflow
**Why you need it:** Prevents wraparound bugs
**Current status:** ❌ Not enabled

**Implementation:**
```makefile
CFLAGS += -ftrapv          # Trap on signed overflow
CFLAGS += -fwrapv          # Define wraparound behavior
CFLAGS += -Wconversion     # Warn on implicit conversions
```

---

### 8. Bounds Checking (Fortify Source)

**What it does:** Replace unsafe functions with bounds-checked versions
**Why you need it:** Catch buffer overflows at runtime
**Current status:** ⚠️ Partial (manual checks)

**Implementation:**
```makefile
CFLAGS += -D_FORTIFY_SOURCE=2

# Compiler replaces:
strcpy()   → __strcpy_chk()
sprintf()  → __sprintf_chk()
memcpy()   → __memcpy_chk()
```

---

### 9. Control Flow Integrity (CFI)

**What it does:** Validates indirect call targets
**Why you need it:** Prevents control-flow hijacking
**Current status:** ❌ Not implemented

**Implementation:**
```makefile
# Clang/LLVM only:
CFLAGS += -fsanitize=cfi
LDFLAGS += -fsanitize=cfi
```

---

### 10. Secure Boot

**What it does:** Verify bootloader and kernel signatures
**Why you need it:** Prevent malicious code at boot
**Current status:** ❌ Not implemented

**Implementation:** Use UEFI Secure Boot + sign kernel with private key

---

## Priority 3: MEDIUM (Defense in Depth)

### 11. Kernel Address Sanitizer (KASAN)

**What it does:** Detects use-after-free, out-of-bounds, etc.
**Why you need it:** Find bugs during development
**Current status:** ❌ Not available

**Implementation:** Port from Linux (complex)

---

### 12. Constant-Time Crypto

**What it does:** Prevent timing side-channels
**Why you need it:** Protect BlindLedger verification
**Current status:** ⚠️ Not verified

**Implementation:**
```c
// BAD: Timing leak
if (memcmp(hash1, hash2, 32) == 0) { ... }

// GOOD: Constant-time
if (timingsafe_bcmp(hash1, hash2, 32) == 0) { ... }
```

---

### 13. Shadow Call Stack

**What it does:** Separate stack for return addresses only
**Why you need it:** Protect against ROP even if canary bypassed
**Current status:** ❌ Not implemented

---

### 14. Audit Logging

**What it does:** Log security events (capability use, auth failures)
**Why you need it:** Forensics and intrusion detection
**Current status:** ❌ None

**Implementation:**
```c
void audit_log(const char *event, Proc *p, const char *details) {
    print("AUDIT [%lld] pid=%d event=%s: %s\n",
          nsec(), p->pid, event, details);
    // Also write to /dev/auditlog or syslog
}

// Usage:
if (!check_permission(p, PEBBLE_PERM_WRITE)) {
    audit_log("DENIED_WRITE", p, path);
    return -1;
}
```

---

## How Other OSes Implement These

### Linux

```bash
# Check mitigations on Linux:
cat /proc/sys/kernel/randomize_va_space   # ASLR: should be 2
dmesg | grep "NX protection"              # DEP/NX
cat /proc/cpuinfo | grep smep             # SMEP
cat /proc/cpuinfo | grep smap             # SMAP
checksec --kernel                         # All mitigations
```

**Linux kernel config:**
```
CONFIG_RANDOMIZE_BASE=y         # KASLR
CONFIG_STACKPROTECTOR_STRONG=y  # Stack canaries
CONFIG_PAGE_TABLE_ISOLATION=y   # KPTI
CONFIG_HARDENED_USERCOPY=y      # Bounds checking
CONFIG_FORTIFY_SOURCE=y         # Fortify
```

---

### OpenBSD

OpenBSD is the gold standard for security mitigations:
- ✅ ASLR since 2003
- ✅ W^X enforcement everywhere
- ✅ Stack canaries (`propolice`)
- ✅ Guard pages
- ✅ Pledge/unveil (syscall restriction)

Study their implementation: https://www.openbsd.org/innovations.html

---

## Quick Wins for Lux9

These can be implemented QUICKLY (1-2 weeks each):

1. **Enable DEP/NX** (linker flags)
   ```makefile
   LDFLAGS += -z noexecstack
   ```

2. **Add Stack Canaries** (compiler flags)
   ```makefile
   CFLAGS += -fstack-protector-strong
   ```

3. **Enable Warnings** (catch bugs early)
   ```makefile
   CFLAGS += -Wall -Wextra -Werror
   CFLAGS += -Wformat-security
   CFLAGS += -Wconversion
   ```

4. **Fortify Source** (bounds checking)
   ```makefile
   CFLAGS += -D_FORTIFY_SOURCE=2
   ```

5. **Relro** (GOT hardening)
   ```makefile
   LDFLAGS += -Wl,-z,relro,-z,now
   ```

---

## Testing Mitigations

### Automated Tools

```bash
# Check binary hardening:
checksec --file=lux9.elf

# Expected output after implementing:
RELRO:    Full RELRO
Stack:    Canary found
NX:       NX enabled
PIE:      PIE enabled
FORTIFY:  Enabled
```

### Manual Tests

1. **Stack canary:** Overflow buffer, check for panic
2. **DEP/NX:** Try executing shellcode on stack
3. **SMEP:** Kernel jump to user code should fault
4. **ASLR:** Check `/proc/modules` shows random addresses

---

## Recommended Reading

1. **"The Tangled Web"** by Michal Zalewski - Web security but principles apply
2. **"A Guide to Kernel Exploitation"** by Enrico Perla - Attack techniques
3. **"Linux Kernel Development"** by Robert Love - How Linux does it
4. **OpenBSD source code** - Best practices for secure OS
5. **grsecurity patches** - Advanced hardening for Linux

---

## Summary: Implementation Priority

**Week 1:**
- ✅ Enable DEP/NX
- ✅ Add stack canaries
- ✅ Enable compiler warnings

**Month 1:**
- ✅ Implement ASLR (kernel + userspace)
- ✅ Enable SMEP/SMAP
- ✅ W^X enforcement

**Month 2-3:**
- ✅ Comprehensive fuzzing (AFL++, libFuzzer)
- ✅ Bounds checking audit
- ✅ Integer overflow protection

**Month 4-6:**
- ✅ External security audit
- ✅ KPTI for Meltdown
- ✅ Audit logging framework

**Month 6+:**
- ✅ CFI
- ✅ Formal verification of critical paths
- ✅ Security certifications (Common Criteria?)

---

*Document Version: 1.0*
*Last Updated: December 2025*
