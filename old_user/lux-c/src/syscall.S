/*
 * syscall.S - AMD64 Syscall shim for Lux9
 * Bridges GCC System V ABI (registers) to Plan 9 Syscall ABI (stack)
 * 
 * GCC ABI:    RDI, RSI, RDX, RCX, R8,  R9
 * Plan 9 ABI: BP (num), SP+8 (arg1), SP+16 (arg2)...
 */

.code64
.text
.global _syscall

_syscall:
    /* RDI = Syscall Number */
    /* RSI = Arg 1 */
    /* RDX = Arg 2 */
    /* RCX = Arg 3 */
    /* R8  = Arg 4 */
    /* R9  = Arg 5 */

    /* Save RBP only if we need to preserve it (System V says RBP is callee-saved) */
    pushq %rbp

    /* Setup Syscall Number */
    movq %rdi, %rbp
    
    /* Setup Arguments on Stack */
    /* We need SP+8 -> Arg 1 (RSI) */
    /* SP+16 -> Arg 2 (RDX) */
    /* ... */
    
    /* We push args in reverse order. */
    /* We need to ensure final SP+8 is Arg 1. */
    /* Current SP points to saved RBP. */

    pushq %r9         /* Arg 5 */
    pushq %r8         /* Arg 4 */
    pushq %rcx        /* Arg 3 */
    pushq %rdx        /* Arg 2 */
    pushq %rsi        /* Arg 1 */
    pushq $0          /* Dummy Slot (corresponds to Return Address in kernel view) */
    
    /* Now:
       SP -> Dummy (0)
       SP+8 -> RSI (Arg 1)
       SP+16 -> RDX (Arg 2)
       ...
       Kernel reads arguments from SP+8. Perfect.
    */

    syscall
    
    /* Restore stack */
    addq $48, %rsp   /* 6 pushes * 8 = 48 bytes */
    
    /* Restore RBP */
    popq %rbp
    ret
