/* initcode.S - First userspace code
 * This code executes in the first userspace process created by the kernel.
 * Its job is to exec /boot/init
 */

.section .text
.global _start

_start:
	/* Build the string "/bin/init\0" on the stack */
	mov $0, %rax
	push %rax              /* null terminator */

	mov $0x0000007469, %rax  /* "it\0\0" */
	push %rax

	mov $0x6e692f6e69622f, %rax  /* "/bin/in" */
	push %rax

	/* rsp now points to the start of the string */
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
