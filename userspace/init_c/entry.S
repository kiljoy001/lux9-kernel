.global _start
.global p9_write
.global p9_exits

.text

_start:
    call main
    
    # Exit if main returns
    mov $0, %rdi
    call p9_exits
    jmp .

# void p9_write(int fd, void *buf, long n)
# SysV ABI: rdi=fd, rsi=buf, rdx=n
p9_write:
    push %rdx
    push %rsi
    push %rdi
    push $0         # dummy for return address slot kernel skips
    
    mov $20, %rbp   # sys_write
    syscall
    
    add $32, %rsp
    ret

# void p9_exits(char *msg)
# SysV ABI: rdi=msg
p9_exits:
    push %rdi
    push $0         # dummy
    
    mov $8, %rbp    # sys_exits
    syscall
    
    add $16, %rsp
    ret
