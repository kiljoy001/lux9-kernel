/* Minimal syscall stubs for init */
#include <stddef.h>

/* Placeholder syscalls - will be replaced with real implementations */

int
fork(void)
{
	/* TODO: real syscall */
	return -1;
}

int
exec(const char *path, char *const argv[])
{
	(void)path;
	(void)argv;
	/* TODO: real syscall */
	return -1;
}

int
mount(const char *path, int server_pid, const char *proto)
{
	(void)path;
	(void)server_pid;
	(void)proto;
	/* TODO: real syscall */
	return -1;
}

int
open(const char *path, int flags)
{
	(void)path;
	(void)flags;
	/* TODO: real syscall */
	return -1;
}

void
exit(int status)
{
	(void)status;
	/* TODO: real syscall */
	for(;;);
}

int
wait(int *status)
{
	(void)status;
	/* TODO: real syscall */
	return -1;
}

void
sleep_ms(int ms)
{
	/* Busy wait for now */
	for(volatile int i = 0; i < ms * 100000; i++);
}

/* Minimal printf implementation */
static void
putchar_serial(char c)
{
	/* TODO: serial port output */
	(void)c;
}

int
putchar(int c)
{
	putchar_serial((char)c);
	return c;
}

int
puts(const char *s)
{
	while(*s) {
		putchar_serial(*s++);
	}
	putchar_serial('\n');
	return 1;
}

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
			}
		} else {
			putchar_serial(*p);
		}
		p++;
	}
	return 0;
}

int
__printf_chk(int flag, const char *fmt, ...)
{
	(void)flag;
	return printf(fmt);
}

/* String functions */
int
strcmp(const char *s1, const char *s2)
{
	while(*s1 && (*s1 == *s2)) {
		s1++;
		s2++;
	}
	return *(const unsigned char*)s1 - *(const unsigned char*)s2;
}

char*
strcpy(char *dest, const char *src)
{
	char *ret = dest;
	while((*dest++ = *src++));
	return ret;
}
