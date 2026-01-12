.section .text
.global _start
_start:
    # Stack layout on entry (set up by kernel exec):
    #   argc at (%rsp)
    #   argv[0] at 8(%rsp)
    #   argv[1] at 16(%rsp)
    #   ...
    
    # Get argc into rdi (first arg to main)
    movq (%rsp), %rdi
    
    # Get pointer to argv into rsi (second arg to main)
    leaq 8(%rsp), %rsi
    
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
