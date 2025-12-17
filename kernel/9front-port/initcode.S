/* initcode.S - First userspace code using 9P doorbell
 * This code executes in the first userspace process created by the kernel.
 * It uses the exchange page doorbell mechanism to execute /boot/init
 */

.section .text
.global _start

/* Exchange page constants */
.equ EXCHANGE_PAGE_ADDR, 0x7FFFFEEFF000
.equ P9_REQUEST_OFFSET, 0x000
.equ P9_CONTROL_OFFSET, 0xF00

/* 9P message constants */
.equ Texec, 128

_start:
	/* Build Texec message in exchange page request buffer
	 * Format: size[4] type[1] tag[2] count[4] pathlen[2] path[...]
	 * String is "/boot/init" (10 bytes)
	 */
	
	movabs $EXCHANGE_PAGE_ADDR, %rdi
	
	/* Write message header */
	movl $23, 0(%rdi)		/* size = 4+1+2+4+2+10 = 23 */
	movb $Texec, 4(%rdi)		/* type = Texec (128) */
	movw $0, 5(%rdi)		/* tag = 0 */
	movl $12, 7(%rdi)		/* count = 2+10 = 12 (pathlen + path) */
	movw $10, 11(%rdi)		/* pathlen = 10 */
	
	/* Write path string "/boot/init" */
	movabs $0x6e692f746f6f622f, %rax	/* "/boot/in" */
	movq %rax, 13(%rdi)
	movw $0x7469, 21(%rdi)			/* "it" */
	/* 23 bytes total message size */

	/* Update req_tail to 23 (Message Size) 
	 * Control block is at offset 0xF00. req_tail is at 0xF0C (offset 12)
	 */
	movl $23, 0xF0C(%rdi)

	/* Ring the doorbell - write 1 to doorbell field (offset 0xF00) */
	movl $1, 0xF00(%rdi)
	
	/* Issue syscall - kernel will check doorbell and process the message */
	syscall
	
	/* If we get here, exec failed - loop forever */
fail:
	jmp fail
