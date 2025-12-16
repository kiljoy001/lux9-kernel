/* Raw assembly init for Lux9 */
.text
.global _start
_start:
    /* sys_write(fd, buf, len) */
    /* Stack layout expected by trap.c (with my SP+8 fix):
       [SP]   = dummy return address
       [SP+8] = fd
       [SP+16] = buf
       [SP+24] = len
    */

    /* Push args in reverse order */
    
    /* len = 14 */
    pushq $14
    
    /* buf = msg */
    leaq msg(%rip), %rax
    pushq %rax
    
    /* fd = 1 (stdout) */
    pushq $1
    
    /* Dummy return address */
    pushq $0
    
    /* Syscall number 20 (WRITE) into BP */
    movq $20, %rbp
    
    syscall
    
    /* Loop forever */
loop:
    jmp loop

msg:
    .ascii "HELLO FROM ASM\n"
