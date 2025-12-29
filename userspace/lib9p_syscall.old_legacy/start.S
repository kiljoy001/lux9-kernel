/* start.S - Entry point for pure 9P init
 *
 * Sets up minimal stack and calls main()
 * After main returns, calls p9_exit
 */

.section .text
.global _start

_start:
    /* Stack is already set up by kernel at sysexec */
    /* Just call main */
    call main
    
    /* If main returns, exit with 0 */
    xor %rdi, %rdi
    call p9_exit
    
    /* Should not reach here */
.Lhang:
    jmp .Lhang
