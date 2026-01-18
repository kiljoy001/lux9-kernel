.section .text
.global _start
_start:
    # Stack layout on entry (set up by kernel exec):
    #   p9uaddr at (%rsp)
    #   argc at 8(%rsp)
    #   argv[0] at 16(%rsp)
    #   argv[1] at 24(%rsp)
    #   ...

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
    # Get argc into rdi (first arg to main)
    movq 8(%rsp), %rdi
    
    # Get pointer to argv into rsi (second arg to main)
    leaq 16(%rsp), %rsi
    
    # Align stack to 16 bytes (required by ABI)
    andq $-16, %rsp
    
    # Set up a small stack frame
    subq $8, %rsp
    
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
    .skip 8192  # 8KB stack
stack_top:
.align 8
.globl lux_exchange_base
lux_exchange_base:
    .quad 0
