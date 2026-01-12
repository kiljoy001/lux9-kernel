.section .text
.global _start
_start:
    # Set up stack
    leaq stack_top(%rip), %rsp

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
    .skip 16384  # 16KB stack
stack_top:
