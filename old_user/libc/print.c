#include <u.h>
#include <libc.h>

int
vfprint(int fd, char *fmt, va_list args)
{
	Fmt f;
	char buf[256];
	int n;

	fmtfdinit(&f, fd, buf, sizeof(buf));
	va_copy(f.args, args);
	n = dofmt(&f, fmt);
	va_end(f.args);
	if(n > 0 && fmtfdflush(&f) == 0)
		return -1;
	return n;
}

int
print(char *fmt, ...)
{
	va_list v;
	int n;
	va_start(v, fmt);
	n = vfprint(1, fmt, v); /* 1 is stdout */
	va_end(v);
	return n;
}

int
fprint(int fd, char *fmt, ...)
{
	va_list v;
	int n;
	va_start(v, fmt);
	n = vfprint(fd, fmt, v);
	va_end(v);
	return n;
}