/* initcode.S - First userspace code
 * This code executes in the first userspace process created by the kernel.
 * Its job is to exec /boot/bin/init
 */

.section .text
.global _start

_start:
	/* Prepare to exec("/boot/bin/init", NULL) */

	/* Set up argv on stack */
	/* Stack layout:
	 *   argv[1] = NULL
	 *   argv[0] = pointer to "/boot/bin/init"
	 *   path string: "/boot/bin/init\0"
	 */

	/* Push the path string onto stack */
	mov $0x0, %rax
	push %rax              /* null terminator (part of string) */

	mov $0x74696e69, %rax  /* "init" in reverse */
	push %rax

	mov $0x2f6e69622f, %rax /* "/bin/" in reverse */
	push %rax

	mov $0x746f6f622f, %rax /* "/boot" in reverse */
	push %rax

	/* Now rsp points to "/boot/bin/init\0" */
	mov %rsp, %r10         /* Save pointer to path */

	/* Build argv array */
	sub $16, %rsp          /* Make space for 2 pointers (argv[0], argv[1]) */
	mov $0, %rax
	mov %rax, 8(%rsp)      /* argv[1] = NULL */
	mov %r10, 0(%rsp)      /* argv[0] = pointer to "/boot/bin/init" */

	/* Now call exec(path, argv) */
	/* Syscall convention:
	 *   rbp = syscall number (7 = EXEC)
	 *   Stack: return address, arg1, arg2, ...
	 */

	mov $7, %rbp           /* EXEC syscall number */
	push %r10              /* arg1: path */
	push %rsp              /* arg2: argv (pointer to argv array) */
	add $16, (%rsp)        /* Fix argv pointer since we just pushed twice */

	syscall                /* Invoke syscall */

	/* If exec returns, we failed - loop forever */
failed:
	jmp failed
