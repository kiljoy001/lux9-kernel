# Lux9 Kernel ABI Compliance: Go Compatibility Review (Code-as-Truth Assessment)

## 1. Introduction

This report specifically addresses the challenge of enabling Go to compile directly to the Lux9 kernel, identified as the most salient reason for ABI compatibility. This assessment relies strictly on code-level analysis of both the Lux9 kernel's ABI and the known conventions of the Go runtime, particularly for x86-64 architectures and Plan 9-like systems.

## 2. Methodology

The analysis involved:
*   Reviewing Lux9 kernel syscall implementation details in `kernel/9front-port/sysproc.c` and `kernel/9front-port/syspageown.c`, focusing on argument passing (`syscall_vainit`) and the syscall set.
*   Consulting external knowledge of Go's runtime syscall conventions for x86-64 Unix and Plan 9 targets (specifically how `golang.org/x/sys` operates).
*   Identifying critical mismatches at the ABI level.

## 3. Findings: Go's Syscall Expectations vs. Lux9's ABI

### 3.1. Critical Mismatch: Syscall Argument Passing Convention

This is the most significant and immediate barrier to Go compatibility.

*   **Go's Syscall Expectation (x86-64 Unix/Plan 9):**
    *   Syscall number is typically placed in the `%rax` register.
    *   Syscall arguments are passed in a specific sequence of general-purpose registers: `%rdi`, `%rsi`, `%rdx`, `%rcx`, `%r8`, `%r9` (up to 6 arguments).
    *   The `syscall` instruction is then executed.
*   **Lux9 Kernel's Syscall Expectation (`syscall_vainit`):**
    *   In `kernel/9front-port/sysproc.c`, the `dosyscall` function uses a custom `syscall_vainit` to prepare `va_list` arguments. This function explicitly *forces all arguments to be read from a dedicated memory region* (`overflow_arg_area` within the `SyscallVaList` structure).
    ```c
    // From kernel/9front-port/sysproc.c
    typedef struct SyscallVaList {
        unsigned int gp_offset;
        unsigned int fp_offset;
        void *overflow_arg_area; // <-- This is where Lux9 expects args to be
        void *reg_save_area;
    } SyscallVaList;

    static void
    syscall_vainit(va_list vl, uchar *raw) // `raw` is up->s.args
    {
        SyscallVaList *impl = (SyscallVaList*)vl;
        /* Force everything to be read from the overflow area */
        impl->gp_offset = 6 * sizeof(uintptr); // effectively skipping register reads
        impl->fp_offset = 8 * sizeof(double); // effectively skipping register reads
        impl->overflow_arg_area = raw; // pointer to the actual argument buffer
        impl->reg_save_area = nil;
    }
    ```
*   **Consequence:** Go's runtime generates assembly code that places syscall arguments in registers. Lux9's kernel, however, *ignores those registers* and attempts to read arguments from a specific memory buffer (`up->s.args` which `overflow_arg_area` points to) that Go's runtime does not populate in this manner for syscalls. This is a **direct and severe low-level ABI incompatibility** that prevents Go programs from correctly invoking *any* syscall.

### 3.2. Syscall Set Divergence

*   **Go's Expectation:** Go's standard library expects a set of POSIX-like or Plan 9-like syscalls for basic OS interaction (file I/O, process management, networking, memory allocation).
*   **Lux9's Provided Syscalls:**
    *   **New Syscalls:** Lux9 introduces many new, non-standard syscalls (`syspebble*`, `sysvm*`, `sysclrcompile`) for its unique features. Go's runtime would not know how to use these without explicit bindings.
    *   **Missing Plan 9 Syscalls:** While many Plan 9 syscalls are present, it's unclear if the *entire* set expected by Go's Plan 9 port (`golang.org/x/sys/plan9`) is available with identical semantics.
    *   **Modified Semantics:** Lux9's capability system for memory management fundamentally changes the semantics of resource handling compared to traditional Plan 9. While Plan 9 `fd`s are passed around, the underlying behavior for memory regions is tied to Lux9's capabilities.

### 3.3. Memory Management Interface

*   **Go's Expectation:** Go's garbage collector and memory allocator (`mheap`, `sysmon`) typically interact with the OS using `mmap`/`munmap` or `sbrk` equivalents to acquire and release large blocks of memory.
*   **Lux9's Interface:** Lux9 uses `syssegbrk`, `syssegattach`, `syssegfree` for segment-based memory. More critically, Lux9's secure memory model is managed by Pebble capabilities (`syspebbleblackalloc`, `syspebbleblackfree`).
*   **Consequence:** Go's runtime would need a significant rewrite of its `mheap` implementation to interface with Lux9's segment management and, more importantly, to manage memory through Pebble's capability-based allocation/deallocation mechanisms.

### 3.4. Process and Concurrency Model

*   **Go's Expectation:** Go's scheduler manages goroutines and uses OS threads (typically via `clone`/`rfork` equivalents) to execute them. It expects standard process/thread creation, scheduling hints, and signal handling.
*   **Lux9's Interface:** Lux9 provides `sysrfork`, `sysexec` (with ELF support), and its own process management. While `sysrfork` is Plan 9-compatible, subtleties in thread scheduling, process signaling, and resource inheritance (especially with Lux9's capabilities) might pose challenges for Go's runtime.

### 3.5. Binary Format (Partial Compatibility)

*   **Go's Expectation:** Go compiles to ELF binaries for Unix-like systems.
*   **Lux9's Interface:** Lux9's `sysexec` (in `sysproc.c`) already supports loading and executing ELF binaries.
*   **Compatibility:** This is the only direct point of compatibility observed. However, a working binary still requires correct syscall invocation and memory management.

## 4. The "Why": Code-Driven Reasons for Go Incompatibility

The primary reason Go cannot compile directly to Lux9 stems from fundamental architectural choices made in Lux9 to achieve its specific goals, which directly conflict with Go's low-level runtime expectations:

1.  **Custom Syscall Argument Handling:** The `syscall_vainit` mechanism is a deliberate, low-level tweak to argument passing, likely driven by the underlying GNU Mach microkernel's context switching requirements or specific kernel design choices. Go's runtime assembly is hardcoded to a different convention.
2.  **Novel Security & Resource Model:** Lux9's core innovations (Pebble capabilities, Rust-like borrow checker for memory) require entirely new syscalls and a different paradigm for interacting with OS resources, which are unknown to Go's standard runtime.
3.  **Plan 9 vs. Lux9 Philosophy:** While Plan 9-inspired, Lux9 has prioritized its unique security and performance model over strict ABI compatibility with external ecosystems (including Plan 9's own Go port or a generic Unix Go build).

## 5. What to do about it: Code-Centric Recommendations for Go Compatibility

To enable Go to compile directly to Lux9, significant, low-level modifications to the Go toolchain and runtime are required. This is a substantial undertaking.

1.  **Modify Go's Runtime Syscall Invocation Assembly (High Priority):**
    *   **Action:** A new `GOOS` target (e.g., `lux9`) must be created for Go. Within this target, the platform-specific assembly code responsible for making syscalls (typically in `runtime/sys_GOOS_GOARCH.s` or similar files within `golang.org/x/sys`) must be rewritten.
    *   **Mechanism:** This assembly must:
        1.  Take the syscall arguments (passed by Go's calling convention in registers/stack) and *repackage them* into a memory buffer (e.g., a `Sargs` struct) that matches the `overflow_arg_area` format expected by Lux9's `syscall_vainit`.
        2.  Call the kernel's syscall entry point, ensuring the `va_list` pointer correctly points to this repackaged buffer.
    *   **Rationale:** This directly resolves the most critical low-level ABI mismatch. Without this, no syscalls will function correctly.

2.  **Adapt Go's Memory Allocator to Lux9's Pebble (High Priority):**
    *   **Action:** The Go runtime's `mheap` (heap management) and `sysmon` (OS memory interaction) components need to be extensively modified.
    *   **Mechanism:** Replace calls to OS memory allocation primitives (`mmap`/`sbrk` equivalents) with calls to Lux9's `syspebbleblackalloc` and `syspebbleblackfree` (via the newly established Go-compatible syscall mechanism). Go's GC needs to understand and interact with Pebble's capability lifecycle.
    *   **Rationale:** Go's runtime needs to acquire and manage memory. Integrating with Pebble is crucial for secure and efficient memory management on Lux9.

3.  **Implement Lux9-Specific `golang.org/x/sys` Bindings:**
    *   **Action:** Create and maintain a `golang.org/x/sys/lux9` package.
    *   **Mechanism:** This package would provide:
        *   Correct syscall numbers for Lux9.
        *   Go functions that correctly prepare and invoke syscalls using the custom Lux9 ABI.
        *   Bindings for Lux9-specific syscalls (`syspebble*`, `sysvm*`, `sysclrcompile`).
    *   **Rationale:** This provides the idiomatic Go way for user-level Go code to interact with Lux9's unique features.

4.  **Map Go's Concurrency Primitives to Lux9's Process Model:**
    *   **Action:** Ensure that Go's runtime's creation and management of OS threads/processes (e.g., for goroutine execution) correctly uses Lux9's `sysrfork` and related process management primitives.
    *   **Rationale:** Proper integration of Go's scheduler with Lux9's process model is essential for stable and performant Go applications.

5.  **Address Standard Library Dependencies:**
    *   **Action:** Review and adapt Go's `net`, `os`, and other standard library packages to correctly interact with Lux9's 9P-based filesystem and networking stack.
    *   **Rationale:** These libraries make numerous OS calls that must be correctly implemented for the Lux9 environment.

This endeavor requires a dedicated effort to port the Go toolchain and runtime to Lux9, involving deep changes to Go's low-level platform-specific code. It is more than just providing a `libc` wrapper; it's about making Go aware of a new OS and its unique ABI at the runtime level.
