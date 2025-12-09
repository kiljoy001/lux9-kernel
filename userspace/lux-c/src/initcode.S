/* Initial process trampoline */
/* Execs /boot/init using Lux9/Plan9 Syscall ABI */
/* ABI: Syscall Num in RBP. Arguments at SP+8, SP+16... */
/* Requirement: argv must be valid array with argv[0] != NULL */

.code64
.global _start
_start:
    /* 1. Construct string "/boot/init" on stack */
    /* "/boot/init" = 10 bytes */
    
    movq $0, %rax
    pushq %rax                  /* Terminator & padding */
    
    movq $0x0000000000007469, %rax /* "it\0..." */
    pushq %rax
    
    movq $0x6e692f746f6f622f, %rax /* "/boot/in" */
    pushq %rax
    
    /* %rsp points to "/boot/init" string now */
    movq %rsp, %rbx             /* RBX = char *file */
    
    /* 2. Construct argv array on stack */
    /* argv = { file, NULL } */
    
    pushq $0                    /* argv[1] = NULL */
    pushq %rbx                  /* argv[0] = file */
    
    movq %rsp, %rcx             /* RCX = char **argv */
    
    /* 3. Prepare Exec Syscall Arguments on Stack */
    /* Exec(char *file, char **argv) */
    /* Stack layout needed for kernel: */
    /* SP+0:  Dummy (Return Addr slot) */
    /* SP+8:  File Ptr */
    /* SP+16: Argv Ptr */
    /* Note: "file" and "argv" must be separate args. */
    
    pushq %rcx                  /* Arg 1: Argv Ptr */
    pushq %rbx                  /* Arg 0: File Ptr */
    pushq $0                    /* Dummy Return Address Slot */
    
    movq $7, %rbp               /* SYS_EXEC = 7 */
    syscall
    
    /* Should not return */
.loop:
    jmp .loop
