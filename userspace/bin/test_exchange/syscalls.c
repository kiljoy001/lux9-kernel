/* Syscall implementation for exchange test program */

#include "syscalls.h"
#include <stdint.h>

/* Simple putchar for debugging */
static void
putchar_serial(char c)
{
	/* For now, just ignore output */
	(void)c;
}

/* Simple printf implementation */
int
printf(const char *fmt, ...)
{
	/* Very minimal printf - just output the string */
	const char *p = fmt;
	while(*p) {
		if(*p == '%') {
			p++;
			if(*p == 's') {
				/* Skip for now */
			} else if(*p == 'd') {
				/* Skip for now */
			} else if(*p == 'p') {
				/* Skip for now */
			} else if(*p == 'l') {
				p++; /* Skip 'l' in %ld, %lu, etc. */
			}
		} else {
			putchar_serial(*p);
		}
		p++;
	}
	return 0;
}

/* Assembly syscall function */
extern long __syscall_asm(int num, ...);

/* Syscall wrapper function */
long
syscall(int num, ...)
{
	/* For now, we'll use a simple approach - in a real implementation
	 * this would use inline assembly to make the actual syscall */
	return __syscall_asm(num, ((long*)&num)[1], ((long*)&num)[2], 
	                     ((long*)&num)[3], ((long*)&num)[4],
	                     ((long*)&num)[5], ((long*)&num)[6]);
}