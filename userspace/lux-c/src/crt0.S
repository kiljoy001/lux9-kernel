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
    # Plan 9 ABI: kernel sets up stack with nargs at (%rsp)
    # Stack layout from kernel:
    #   (%rsp) = nargs
    # We need to convert this to C ABI: main(int argc, char **argv)

    # For now, just use fixed argc=0, argv=NULL since kernel
    # doesn't properly set up argv array yet
    xorq %rdi, %rdi      # argc = 0
    xorq %rsi, %rsi      # argv = NULL

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
# Kernel expects: rbp = syscall number, arguments on stack after return address
_syscall:
    # Build argument block on stack for kernel
    # Kernel reads args from (rsp+8), (rsp+16), etc
    pushq %r9       # arg 5
    pushq %r8       # arg 4
    pushq %rcx      # arg 3
    pushq %rdx      # arg 2
    pushq %rsi      # arg 1
    pushq $0        # arg 0 (return address slot)

    # Setup syscall number
    movq %rdi, %rbp

    # Execute syscall (RSP stays normal!)
    syscall

    # Clean up stack
    addq $48, %rsp  # Remove 6 args
    ret
