# Lux9 Kernel ABI Compliance Review: Plan 9 ABI Non-Compliance (Code-as-Truth Assessment)

## 1. Introduction

This report provides a strict code-as-truth assessment of why the Lux9 kernel project is not fully compliant with the traditional Plan 9 ABI. Previous analysis may have over-emphasized high-level goals or documentation; this review focuses solely on concrete deviations observed in the C source code, identifying specific instances of non-compliance and their implications for the Plan 9 Application Binary Interface.

## 2. Methodology: Code-as-Truth Examination

The analysis meticulously examined the kernel source code, particularly within the `kernel/9front-port/` directory, for direct evidence of divergence from established Plan 9 ABI conventions. This included:
*   Syscall argument passing mechanisms.
*   Executable binary formats supported.
*   The set of available syscalls.
*   Data structures and their semantics for core OS primitives (e.g., memory management, inter-process communication).
*   Version compatibility for specific syscalls.

## 3. Findings: Concrete Instances of Plan 9 ABI Non-Compliance

The following points detail specific, code-level reasons why the Lux9 kernel's ABI is not fully compliant with a standard Plan 9 ABI.

### 3.1. Low-Level Syscall Calling Convention: Custom `va_list` Handling

*   **Observation:** In `kernel/9front-port/sysproc.c`, the `syscall_vainit` function explicitly manipulates the `va_list` structure used for syscall arguments.
    ```c
    // From kernel/9front-port/sysproc.c
    typedef struct SyscallVaList {
        unsigned int gp_offset;
        unsigned int fp_offset;
        void *overflow_arg_area;
        void *reg_save_area;
    } SyscallVaList;

    static void
    syscall_vainit(va_list vl, uchar *raw)
    {
        SyscallVaList *impl = (SyscallVaList*)vl;
        /* Force everything to be read from the overflow area */
        impl->gp_offset = 6 * sizeof(uintptr);
        impl->fp_offset = 8 * sizeof(double);
        impl->overflow_arg_area = raw;
        impl->reg_save_area = nil;
    }
    ```
*   **Non-Compliance:** This is a **direct and fundamental deviation from standard calling conventions for syscalls**. Typically, initial syscall arguments are passed in registers, with additional arguments "overflowing" onto the stack. By forcing all arguments to be read from a designated `overflow_arg_area`, Lux9's kernel establishes a custom `va_list` ABI.
*   **Impact:** Any userland program or `libc` compiled with standard Plan 9 toolchains (which expect arguments to be passed partially in registers) will likely fail or misinterpret syscall arguments when invoked on Lux9. This custom convention impacts *all* syscalls that extract arguments using `va_arg(list, type)`.

### 3.2. Executable Binary Format ABI: Direct ELF Support in `sysexec`

*   **Observation:** The `sysexec` syscall in `kernel/9front-port/sysproc.c` explicitly includes logic to parse and execute both Plan 9's native a.out format and ELF binaries for `EM_X86_64` architecture.
    ```c
    // From kernel/9front-port/sysproc.c, inside sysexec
    // Check for ELF magic
    if(n >= sizeof(Elf64_Ehdr) &&
       u.buf[0] == ELF_MAGIC_0 && u.buf[1] == ELF_MAGIC_1 &&
       u.buf[2] == ELF_MAGIC_2 && u.buf[3] == ELF_MAGIC_3) {
        // ... parse ELF headers ...
        is_elf = 1; // Flag indicating ELF execution path
        // ...
    }
    ```
*   **Non-Compliance:** A strict Plan 9 ABI for executables expects the native a.out format. The direct support for ELF binaries means the executable format ABI is **not exclusively Plan 9**.
*   **Impact:** While this enables running a broader range of software (e.g., from Linux toolchains), it means a Plan 9 `libc` linked against specific a.out structures might not be directly compatible with generic ELF binaries without modifications.

### 3.3. Syscall Set Extension: New Lux9-Specific Syscalls

*   **Observation:** The Lux9 kernel introduces entirely new syscalls not found in the standard Plan 9 ABI:
    *   **Pebble Capability Syscalls (`syspebble*` in `kernel/9front-port/sysproc.c`):** e.g., `syspebbleblackalloc`, `syspebblewhiteissue`, `syspebbleblackfree`. These expose Lux9's new capability-based memory management system.
    *   **Page Ownership/Borrowing Syscalls (`sysvm*` in `kernel/9front-port/syspageown.c`):** e.g., `sysvmexchange`, `sysvmlend_shared`, `sysvmlend_mut`, `sysvmreturn`, `sysvmowninfo`. These implement Rust-like borrow-checking for zero-copy IPC.
    *   **CLR Compilation Syscall (`sysclrcompile` in `kernel/9front-port/sysproc.c`):** This enables in-kernel compilation of managed code.
*   **Non-Compliance:** These syscalls represent a **significant extension of the kernel's ABI beyond Plan 9's definition**. They introduce new functionalities and interfaces.
*   **Impact:** Existing Plan 9 userland programs cannot access these new features. Lux9-specific applications require a custom `libc` (or direct syscall invocation via the non-standard `va_list` convention) to interact with these unique capabilities.

### 3.4. `stat`/`wstat` Structure ABI Versioning (Backward Incompatibility)

*   **Observation:** In `kernel/9front-port/sysfile.c`, functions like `sys_stat` and `sys_fstat` explicitly handle an "old stat buffer" format (`validaddr((uintptr)s, 116, 1);`). More critically, `sys_wstat` and `sys_fwstat` *explicitly error out* with `"old wstat system call - recompile"`.
*   **Non-Compliance:** This demonstrates a **direct break in backward binary compatibility for `stat` and `wstat` syscalls**. Lux9 (or the 9front port it's based on) has evolved its `stat` structure or serialization format, rendering older binaries incompatible without recompilation.
*   **Impact:** Legacy Plan 9 binaries relying on the older `_stat` family of syscalls will either fail to function correctly (due to incorrect buffer sizes/formats) or explicitly be rejected by the kernel.

### 3.5. Fundamental Resource Handling Semantics: Capability-Based `Chan` and `Qid` Abstraction

*   **Observation:** While standard Plan 9 `Chan` and `Qid` structures are used, the Lux9 kernel implicitly alters their underlying semantics for memory resources. Previous code reviews (e.g., of `pebble.h` and `blind_ledger.h`) showed that `ExchangeHandle` is `typedef UserCapability`, and `syspebble*` syscalls operate on `PebbleWhite*` or `void*` handles. These capabilities fundamentally change how memory ownership and access are managed.
*   **Non-Compliance:** Plan 9 typically relies on `fd`s associated with `Qid`s for uniform resource access. Lux9's capability system introduces an parallel or superseding mechanism for secure memory management that requires a different programming model than simple `read`/`write` on file descriptors.
*   **Impact:** A Plan 9 application expecting to manage memory or perform IPC solely through traditional `fd`/`Qid` mechanisms will find the Lux9 capability system incompatible for direct resource control.

## 4. The "Why": Code-Driven Reasons for Non-Compliance

The non-compliance with the pure Plan 9 ABI is a direct consequence of specific, deliberate engineering choices within the Lux9 kernel, as evidenced by the code:

1.  **Custom Syscall Dispatch:** The `syscall_vainit`'s manipulation of `va_list` is a low-level adaptation to the argument passing mechanism. This could be due to architectural requirements of the GNU Mach microkernel, compiler/toolchain choices, or a deliberate simplification of kernel-side argument parsing that deviates from standard Plan 9 `va_list` expectations.
2.  **Expanded Binary Ecosystem:** The inclusion of ELF support in `sysexec` (`kernel/9front-port/sysproc.c`) is a clear choice to broaden the types of executables the kernel can run, moving beyond Plan 9's native a.out format.
3.  **Lux9's Unique Features:** The introduction of Pebble capabilities, Rust-like page ownership/borrowing, and the CLR runtime are core innovations of the Lux9 project. These advanced features require new kernel interfaces (syscalls) to expose them to userland, inherently extending and modifying the standard Plan 9 ABI.
4.  **ABI Evolution Management:** The explicit handling of "old stat buffer" formats and the rejection of old `wstat` syscalls indicate an internal ABI evolution choice, prioritizing a newer `stat` structure over backward binary compatibility for that specific interface.

These are not accidental incompatibilities but rather direct results of design decisions to implement specific functionalities and architectural models in Lux9 that diverge from, and build upon, Plan 9's foundation.

## 5. What to do about it: Code-Centric Recommendations

Addressing these non-compliances requires embracing the reality of Lux9's unique ABI and providing the necessary infrastructure for developers targeting it.

1.  **Formalize and Publish the Lux9 Syscall ABI Specification:**
    *   **Action:** Create a definitive technical specification detailing the exact syscall numbers, argument types, return values, and calling conventions for *all* Lux9 syscalls, including:
        *   The precise `va_list` structure and argument passing convention (as dictated by `syscall_vainit`).
        *   The signatures and semantics of all new `syspebble*`, `sysvm*`, `sysclrcompile` syscalls.
        *   The current `stat`/`wstat` formats and their expected usage.
    *   **Rationale:** This document will be the "ground truth" for developing compatible userland software and toolchains.

2.  **Develop/Adapt a Lux9-Specific Standard C Library (`libc`):**
    *   **Action:** A custom `libc` tailored for Lux9 is critical. This `libc` must:
        *   **Strictly conform to the `syscall_vainit` ABI** for generating syscall argument lists.
        *   Include declarations and wrapper functions for all Lux9-specific syscalls.
        *   Provide `exec` wrappers that transparently handle both a.out and supported ELF executable formats.
        *   Implement `stat`/`wstat` functions compatible with the current Lux9 kernel ABI, potentially providing optional backward compatibility layers for source code (but not binary) if deemed necessary.
    *   **Rationale:** This `libc` is the primary interface for applications, abstracting kernel details and enabling seamless development for the Lux9 platform.

3.  **Define and Support the Lux9 Executable Binary Interface:**
    *   **Action:** Provide clear specifications on which versions and features of a.out and ELF binaries are supported by `sysexec`.
    *   **Rationale:** This guides toolchain developers (compilers, assemblers, linkers) in generating compatible binaries.

4.  **Provide Migration/Adaptation Guidance:**
    *   **Action:** For developers migrating Plan 9 applications, offer clear guides on how to adapt their code for the Lux9 ABI, particularly regarding memory management, IPC, and if they need to leverage new capabilities.
    *   **Rationale:** Eases the transition and fosters a developer ecosystem.

In conclusion, the Lux9 kernel's ABI is not fully Plan 9 compliant because it has deliberately evolved to integrate unique security, memory management, and runtime features, along with low-level architectural adaptations and expanded binary support. Addressing this involves clearly defining the new "Lux9 ABI" and building the necessary toolchain and developer support around it.