.global _start
.global _write
.global _read
.global _open
.global _close
.global _exits

.text

_start:
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    call __managed__Main
    movq $0, %rdi
    movq $8, %rax # EXITS
    syscall
    ret


# _write(int fd, void *buf, long n)
_write:
    subq $32, %rsp
    movq %rdi, 8(%rsp)
    movq %rsi, 16(%rsp)
    movq %rdx, 24(%rsp)
    movq $20, %rax       # WRITE
    syscall
    addq $32, %rsp
    ret

# _read(int fd, void *buf, long n)
_read:
    subq $32, %rsp
    movq %rdi, 8(%rsp)
    movq %rsi, 16(%rsp)
    movq $15, %rax       # READ
    syscall
    addq $32, %rsp
    ret

# _open(char *path, int mode)
_open:
    subq $24, %rsp
    movq %rdi, 8(%rsp)
    movq %rsi, 16(%rsp)
    movq $14, %rax
    syscall
    addq $24, %rsp
    ret

# _close(int fd)
_close:
    subq $16, %rsp
    movq %rdi, 8(%rsp)
    movq $4, %rax
    syscall
    addq $16, %rsp
    ret

# _exits(char *msg)
_exits:
    subq $16, %rsp
    movq %rdi, 8(%rsp)
    movq $8, %rax
    syscall
    addq $16, %rsp
    ret
