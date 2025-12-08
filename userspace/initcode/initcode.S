.code64
.text
.globl _start
_start:
    # Prepare argv array on stack
    # We need argv = ["/boot/init", NULL]
    
    pushq $0                    # argv[1] = NULL
    leaq bootinit(%rip), %rax   # Load address of string
    pushq %rax                  # argv[0] = "/boot/init"
    movq %rsp, %rbx             # rbx = pointer to argv array
    
    # Prepare syscall arguments
    # Expected by kernel trap.c: SP+8=Path, SP+16=Argv
    
    pushq %rbx                  # Arg2: argv pointer
    pushq %rax                  # Arg1: path pointer
    pushq $0                    # Arg0: Fake Return Address (gap)
    
    movq $7, %rbp               # Exec
    syscall
    
loop:
    jmp loop

bootinit:
    .ascii "/boot/init\0"
