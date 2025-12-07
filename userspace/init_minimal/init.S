.global _start
.section .text

_start:
    /* Infinite loop - dead simple test to verify EXEC success */
    jmp _start
