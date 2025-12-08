.global _start
.text
_start:
    # Breakpoint to prove execution
    int $3

    # Setup stack
    subq $64, %rsp
    
    # Args for PWRITE (fd, buf, len, off)
    movq $1, 8(%rsp)    # fd
    leaq msg(%rip), %rax
    movq %rax, 16(%rsp) # buf
    movq $14, 24(%rsp)  # len
    movq $-1, 32(%rsp)  # off

loop:
    movq $51, %rbp      # PWRITE
    syscall
    jmp loop

.data
msg:
    .ascii "Alive!\n\0"