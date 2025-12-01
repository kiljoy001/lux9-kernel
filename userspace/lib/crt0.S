.text
.globl _start
.extern main
.extern exit

_start:
    /*
     * Stack layout on entry (Plan 9 / Linux style):
     * RSP -> argc (8 bytes)
     *        argv[0]
     *        argv[1]
     *        ...
     *        NULL
     *        envp[0]
     *        ...
     */
    xorq %rbp, %rbp         /* clear frame pointer */
    
    movq (%rsp), %rdi       /* argc */
    leaq 8(%rsp), %rsi      /* argv */
    
    /* Align stack to 16 bytes for SSE/ABI compliance if needed, 
       though _start is usually aligned. 
       Call main(argc, argv) */
    call main

    /* Call exit(ret) with return value from main */
    movq %rax, %rdi
    call exit

    /* Should not reach here */
    hlt
