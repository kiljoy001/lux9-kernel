# Plan 9 ABI Analysis: Stack vs Register

This document analyzes the implications of switching the Lux9 calling convention and syscall ABI from the traditional Plan 9 "Stack-Based" model to a modern "Register-Based" model (e.g., System V AMD64).

## 1. The Current Model (Stack)
Plan 9 (`9c` compiler and `SYSCALL` mechanism) passes all arguments on the stack.
*   **Syscalls**: The kernel retrieves arguments by reading from the user stack pointer (`ureg->sp + offset`).
*   **Functions**: The caller pushes arguments, calls, and then cleans up the stack.
*   **Registers**: Only a few registers are "caller-save". The frame pointer (`BP`) is strictly maintained for backtracing.

### Pros
*   **Simplicity**: Compiler implementation is trivial.
*   **Debugging**: The "Frame Pointer Chain" (`*BP -> *BP -> ...`) allows perfect backtraces without complex metadata (DWARF/eh_frame).
*   **Variadic Functions**: Trivial implementation (just walk the stack).

### Cons
*   **Performance**: High memory traffic. Every argument passing pushes/pops to cache/RAM.
*   **Kernel Safety**: To read syscall arguments, the kernel must access user memory (`ureg->sp`). This requires valid pointer checks (`okaddr`) and can theoretically Page Fault inside the syscall handler if the user stack passes through a boundary.
*   **ToCToU**: Time-of-Check-Time-of-Use attacks are slightly easier if arguments live in shared user memory (though usually cached).

## 2. The Proposed Switch (Register)
Switching to a Register-based ABI (like Linux/System V: `RDI, RSI, RDX, RCX, R8, R9` for args).

### Impact on Kernel (Syscalls)
*   **Mechanism**: The `Ureg` struct in `trap.c` already captures all general-purpose registers (`di`, `si`, `dx`, `r10`, etc.) when the CPU traps.
*   **Performance**: **Zero-Cost Access**. The kernel already has the arguments in the `ureg` struct on the *kernel stack*. No need to read `ureg->sp` (user memory).
*   **Safety**: This eliminates the "read from user stack" step for integer arguments, closing a class of possible faults/attacks.

### Impact on Userspace (Compiler)
*   **Compilers**: `9c` (the Plan 9 C compiler) does not support this. You would need to switch to:
    *   `gcc` / `clang` (Native support for System V ABI).
    *   Modify `pcc` / `9c` (Hard).
*   **Assembly**: All assembly files (`lib/libc/amd64/*.s`) would need rewriting to expecting args in registers, not `0(SP)`, `8(SP)`.
*   **Go/Rust/C#**: These languages already prefer register ABIs. Lux9's goal of running C# (`bflat`) strongly aligns with this switch.

## 3. The "Pure 9P" Consideration
Your `trap.c` mentions: `Phase 6: Pure 9P - TRUE syscall elimination`.
*   If the goal is to move everything to **Message Passing** (Exchange Pages), then the "Stack vs Register" debate for syscalls becomes irrelevant because *there are no syscall arguments*—only a "Doorbell" (the syscall itself).
*   However, for **Userspace Internal Execution** (function A calls function B), switching to Register ABI is still a massive performance win (10-20% overall speedup).

## Recommendation
1.  **For Syscalls**: If aiming for "Pure 9P", **stay with the Doorbell**. Don't over-optimize the legacy syscall path.
2.  **For Userspace Code**: **Switch to Register ABI**. Use `clang`/`gcc` with `-fno-omit-frame-pointer`.
    *   You get the speed of register passing.
    *   You keep the "Plan 9 Style" backtraces (by forcing frame pointers).
    *   You get compatibility with modern languages (C#, Rust, WASM runtimes).
