.section .text
.global _start
_start:
    # Capture p9uaddr from stack (preferred), then rax/rdi (fallbacks)
    mov %rax, %r8
    mov %rdi, %r9
    mov (%rsp), %rax
    cmp $0x1000, %rax
    jb check_stack_minus8
    mov $0x0000800000000000, %rdx
    cmp %rdx, %rax
    jb p9uaddr_set
check_stack_minus8:
    mov -8(%rsp), %rax
    cmp $0x1000, %rax
    jb check_rax
    mov $0x0000800000000000, %rdx
    cmp %rdx, %rax
    jb p9uaddr_set
check_rax:
    mov %r8, %rax
    cmp $0x1000, %rax
    jb check_rdi
    mov $0x0000800000000000, %rdx
    cmp %rdx, %rax
    jb p9uaddr_set
check_rdi:
    mov %r9, %rax
    cmp $0x1000, %rax
    jb fallback_p9uaddr
    mov $0x0000800000000000, %rdx
    cmp %rdx, %rax
    jb p9uaddr_set
fallback_p9uaddr:
    mov $0x7FFFFEEFF000, %rax
p9uaddr_set:
    mov %rax, lux_exchange_base(%rip)
    # Set up stack
    movq $stack_top, %rsp

    # Call main
    call main

    # Exit with status from main's return value (in %rax)
    # _exit is defined in exit.c and sends proper Tsysexit message
    mov %rax, %rdi          # Pass return value as exit status
    call _exit

    # Should never reach here
1:
    hlt
    jmp 1b

.section .bss
.align 16
stack_bottom:
    .skip 131072  # 128KB stack for rump
stack_top:
.align 8
.globl lux_exchange_base
lux_exchange_base:
    .quad 0
