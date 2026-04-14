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
    mov %rax, %rcx
    add $0xEF8, %rcx
    mov $0x41553950, %edx  # "P9UA"
    mov %edx, (%rcx)
    # Derive argv/argc from the initial stack with validation.
    # Some kernels may not place argc at (%rsp), so validate argv list.
    lea 8(%rsp), %rsi         # argv candidate 1
    mov %rsi, %rcx            # cursor
    xor %rdi, %rdi            # argc = 0
    mov $64, %r8              # max args
1:
    mov (%rcx), %rax
    test %rax, %rax
    je 3f
    cmp $0x1000, %rax
    jb 2f
    mov $0x0000800000000000, %rdx
    cmp %rdx, %rax
    jae 2f
    inc %rdi
    add $8, %rcx
    dec %r8
    jnz 1b
2:
    # Retry with argv candidate 2 (rsp + 16)
    lea 16(%rsp), %rsi
    mov %rsi, %rcx
    xor %rdi, %rdi
    mov $64, %r8
4:
    mov (%rcx), %rax
    test %rax, %rax
    je 3f
    cmp $0x1000, %rax
    jb 5f
    mov $0x0000800000000000, %rdx
    cmp %rdx, %rax
    jae 5f
    inc %rdi
    add $8, %rcx
    dec %r8
    jnz 4b
5:
    xor %rdi, %rdi            # argc = 0
3:

    # Switch to local stack for runtime use
    leaq stack_top(%rip), %rsp

    # Call main(argc, argv)
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
    .skip 131072  # 128KB spare stack
stack_top:
.align 8

.section .data
.align 8
.globl lux_exchange_base
lux_exchange_base:
    .quad 0
