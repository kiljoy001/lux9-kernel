.text
.global _start

.set SYS_write, 2
.set SYS_exits, 11

_start:
    /* Write "ASM INIT: Hello from userspace!\n" to stderr (fd 2) */
    movq    $SYS_write, %rax    /* syscall number for write */
    movq    $2, %rdi            /* fd (stderr) */
    leaq    message(%rip), %rsi /* buffer address */
    movq    $message_len, %rdx  /* length */
    syscall                     /* execute syscall */

    /* Exit cleanly */
    movq    $SYS_exits, %rax    /* syscall number for exits */
    leaq    exit_msg(%rip), %rdi /* exit message */
    syscall                     /* execute syscall */

    /* Should not reach here, but just in case */
    ud2                     /* Undefined instruction to halt if exits fails */

.section .rodata
message:
    .ascii "ASM INIT: Hello from userspace!\n"
    message_len = . - message

exit_msg:
    .ascii "init_minimal exits successfully"
    exit_msg_len = . - exit_msg

