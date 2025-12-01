#include <u.h>
#include <libc.h>
#include <sys.h>
#include "syscall.h"
#include "syscall_ops.h"

int
errstr(char *buf, uint nbuf)
{
	return (int)syscall3(ERRSTR, (long)buf, nbuf, 0);
}

void
werrstr(char *fmt, ...)
{
	char buf[ERRMAX];
	va_list arg;

	va_start(arg, fmt);
	vsnprint(buf, sizeof buf, fmt, arg);
	va_end(arg);
	errstr(buf, sizeof buf);
}
