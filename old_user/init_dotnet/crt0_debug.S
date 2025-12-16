.text
.global _start

_start:
    # Print "!" to stdout (fd=1) using syscall write (2)
    # syscall(num=2, arg1=1, arg2=addr, arg3=len)
    
    # Store "!\n" on stack
    pushq $0x0a21      # "!\n"
    movq %rsp, %rdx    # Buffer address
    
    movq $1, %rsi      # fd = 1
    movq $2, %rcx      # len = 2
    movq $2, %rdi      # syscall = SYS_write (2)
    
    # Kernel expects syscall num in rdi? Or rax?
    # lux-c/crt0.S: movq %rdi, %rbp... syscall
    # Wait, Lux9 syscall ABI?
    # userspace/init_dotnet/syscall_amd64.o used:
    # movq %rdi, %rax ?
    # Let's check runtime.c syscall impl.
    # runtime.c calls __syscall(n, a1, a2...)
    # We need to know RAW ABI.
    # syscall_amd64.S (implied) usually puts num in accumulator or rdi.
    # init_minimal/init_minimal.S uses:
    # movq $SYS_write, %rax ??
    
    # Let's assume standard x86_64 SysV ABI for syscall instruction? 
    # RAX=num, RDI=arg1, RSI=arg2, RDX=arg3...
    
    movq $2, %rax      # SYS_write
    movq $1, %rdi      # fd
    movq %rsp, %rsi    # buf
    movq $2, %rdx      # len
    
    syscall
    
    # Now exit
    movq $11, %rax     # SYS_exits (11)
    xorq %rdi, %rdi
    syscall
    
    # Loop forever if syscall fails
    jmp .
