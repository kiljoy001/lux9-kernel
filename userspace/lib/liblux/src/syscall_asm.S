.global _syscall
_syscall:
    syscall
    /* RAX now contains kernel-set value (0 for child via forkchild())
     * For child: kernel set ureg->ax = 0, IRETQ restored it to RAX
     * For parent: kernel might set RAX to syscall result
     * Return RAX as C function return value */
    mov %rax, %rax  /* Explicit: RAX is return value */
    ret
