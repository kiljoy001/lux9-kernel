.text
.globl _start
.globl _syscall

# Entry point for userspace programs
# Stack layout at entry (from kernel's touser):
#   USTKTOP - sizeof(Tos) - 8 - sizeof(sp[0])*4
#   sp[0] = nil (argv[0])
#   sp[1] = "boot" (argv[1])  
#   sp[2] = nil
#   sp[3] = nil

_start:
    # argc is computed from argv (scan for NULL)
    # For now, assume argc = 1 (boot program)
    movq $1, %rdi        # argc
    leaq 8(%rsp), %rsi   # argv (skip past return address if any)
    
    # Call main(argc, argv)
    call main
    
    # If main returns, exit
    movq %rax, %rdi      # Use return value as exits string (0 = NULL = success)
    call exits
    
    # Should never reach here
    ud2

# Syscall wrapper
# long _syscall(long num, ...)
# Arguments: rdi = syscall number, rsi, rdx, rcx, r8, r9 = syscall args
# Kernel expects: rbp = syscall number, rsp = pointer to argument block
_syscall:
    pushq %rbp
    pushq %rbx
    pushq %r12
    pushq %r13
    
    # Allocate space for argument block
    subq $48, %rsp       # 6 args * 8 bytes
    movq %rsp, %r12
    
    # Copy arguments to block
    movq %rsi, 0(%r12)
    movq %rdx, 8(%r12)
    movq %rcx, 16(%r12)
    movq %r8,  24(%r12)
    movq %r9,  32(%r12)
    # slot at 40 reserved
    
    # Setup kernel syscall convention
    movq %rdi, %rbp      # syscall number in rbp
    movq %r12, %rsp      # rsp points to argument block
    
    syscall
    
    # Restore stack
    movq %rsp, %r13
    addq $48, %r13
    movq %r13, %rsp
    
    popq %r13
    popq %r12
    popq %rbx
    popq %rbp
    ret
