.text
.globl __syscall
.globl __syscall_asm

#define ARG_BYTES 48

__syscall:
__syscall_asm:
    pushq %rbp
    pushq %rbx
    pushq %r12
    pushq %r13

    subq $ARG_BYTES, %rsp
    movq %rsp, %r12

    movq %rsi, 0(%r12)
    movq %rdx, 8(%r12)
    movq %rcx, 16(%r12)
    movq %r8,  24(%r12)
    movq %r9,  32(%r12)
    /* slot at 40(%r12) reserved/alignment */

    movq %rdi, %rbx            /* syscall number */
    movq %rsp, %r13            /* pointer to argument block */

    movq %rbx, %rbp            /* kernel expects syscall number in %rbp */
    movq %r13, %rsp            /* user stack points at argument block */
    syscall

    /* restore original stack */
    movq %rsp, %r13
    addq $ARG_BYTES, %r13
    movq %r13, %rsp

    popq %r13
    popq %r12
    popq %rbx
    popq %rbp
    ret
