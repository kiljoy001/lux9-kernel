/* initcode.S - First userspace code
 * This code executes in the first userspace process created by the kernel.
 * Its job is to exec /boot/init
 */

.section .text
.global _start

_start:
	/* Build the string "/boot/boot\0" on the stack (11 bytes, padded to 16) */
	/* Stack grows down, so push high bytes first, low bytes last */
	/* rsp will point to the start of the string after pushes */
	
	/* Push "ot\0..." (high 8 bytes - at higher address) */
	movabs $0x000000000000746f, %rax  /* "ot\0\0\0\0\0\0" in little-endian */
	push %rax
	
	/* Push "/boot/bo" (low 8 bytes - at lower address = string start) */
	movabs $0x6f622f746f6f622f, %rax  /* "/boot/bo" in little-endian */
	push %rax

	/* rsp now points to the start of "/boot/boot\0" */
	mov %rsp, %rbx         /* rbx = pointer */

	/* Build argv array: [pointer_to_string, NULL] */
	sub $16, %rsp          /* Space for 2 pointers */
	mov $0, %rax
	mov %rax, 8(%rsp)      /* argv[1] = NULL */
	mov %rbx, 0(%rsp)      /* argv[0] = pointer to path */

	/* rsp now points to start of argv array */
	mov %rsp, %rcx         /* rcx = pointer to argv array */

	/* Set up syscall: exec(path, argv) */
	mov $7, %rbp           /* syscall number = 7 (EXEC) */

	/* Push arguments onto stack in reverse order */
	push %rcx              /* arg2: argv */
	push %rbx              /* arg1: path */

	syscall                /* invoke EXEC */

	/* If we get here, exec failed - loop forever */
fail:
	jmp fail
