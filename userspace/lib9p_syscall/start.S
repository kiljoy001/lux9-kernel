.section .text
.global _start
_start:
    # Set up stack
    movq $stack_top, %rsp
    
    # Call main
    call main
    
    # Exit (infinite loop for now)
1:
    hlt
    jmp 1b

.section .bss
.align 16
stack_bottom:
    .skip 16384  # 16KB stack
stack_top:
