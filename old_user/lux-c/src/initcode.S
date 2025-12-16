/* Initial process trampoline */
/* Loads /boot/Init.dll and writes to /dev/clr/execute to run .NET init */
/* Uses Lux9/Plan9 Syscall ABI: Syscall Num in RBP, args on stack */

.code64
.global _start
_start:
    /* Just exec /boot/init (the C binary) which will handle CLR init */
    /* This is simpler - let C code handle the complex CLR loading */

    /* 1. Construct string "/boot/init" on stack */
    movq $0, %rax
    pushq %rax                  /* Terminator & padding */

    movq $0x0000000000007469, %rax /* "it\0..." */
    pushq %rax

    movq $0x6e692f746f6f622f, %rax /* "/boot/in" */
    pushq %rax

    movq %rsp, %rbx             /* RBX = char *file */

    /* 2. Construct argv array */
    pushq $0                    /* argv[1] = NULL */
    pushq %rbx                  /* argv[0] = file */
    movq %rsp, %rcx             /* RCX = char **argv */

    /* 3. Prepare Exec Syscall Arguments */
    pushq %rcx                  /* Arg 1: Argv Ptr */
    pushq %rbx                  /* Arg 0: File Ptr */
    pushq $0                    /* Dummy Return Address */

    movq $7, %rbp               /* SYS_EXEC = 7 */
    syscall

    /* Should not return */
.loop:
    jmp .loop
