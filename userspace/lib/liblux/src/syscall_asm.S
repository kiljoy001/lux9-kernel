.global _syscall
_syscall:
    syscall
    /* RAX now contains kernel-set value (0 for child via forkchild())
     * For child: kernel set ureg->ax = 0, IRETQ restored it to RAX
     * For parent: kernel might set RAX to syscall result
     * Return RAX as C function return value */
    mov %rax, %rax  /* Explicit: RAX is return value */
    ret

.global _syscall_rfork_stack
_syscall_rfork_stack:
    /* Preserve child jump state in registers so the child never needs
     * to read stack_top/func/arg from the shared caller frame after rfork. */
    mov %rdi, %r8
    mov %rsi, %r9
    mov %rdx, %r10
    syscall
    mfence
    test %rax, %rax
    jne 1f
    mov %r8, %rsp
    sub $8, %rsp
    movq $0, (%rsp)
    mov %r10, %rdi
    jmp *%r9
1:
    ret
