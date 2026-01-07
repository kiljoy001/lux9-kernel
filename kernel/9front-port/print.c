#include "u.h"
#include "portlib.h"
#include "fns.h"

int panicking;

int
_efgfmt(Fmt *f)
{
	/* floating point not supported in kernel yet */
	return fmtprint(f, "NaN");
}

int
print(char *fmt, ...)
{
	va_list arg;
	char buf[1024];
	int n;

	va_start(arg, fmt);
	n = vseprint(buf, buf+sizeof(buf), fmt, arg) - buf;
	va_end(arg);

	if(screenputs)
		screenputs(buf, n);
    else
        uartputs(buf, n);

	return n;
}

int
iprint(char *fmt, ...)
{
    /* Interrupt safe print - for now same as print */
	va_list arg;
	char buf[1024];
	int n;

	va_start(arg, fmt);
	n = vseprint(buf, buf+sizeof(buf), fmt, arg) - buf;
	va_end(arg);

    if(screenputs)
		screenputs(buf, n);
    else
        uartputs(buf, n);

	return n;
}

int
pprint(char *fmt, ...)
{
    /* Process print - for now same as print */
	va_list arg;
	char buf[1024];
	int n;

	va_start(arg, fmt);
	n = vseprint(buf, buf+sizeof(buf), fmt, arg) - buf;
	va_end(arg);

    if(screenputs)
		screenputs(buf, n);
    else
        uartputs(buf, n);

	return n;
}

void
panic(char *fmt, ...)
{
	va_list arg;
	char buf[1024];
	int n;

    splhi();
    panicking = 1;

    uartputs("PANIC: ", 7);
	va_start(arg, fmt);
	n = vseprint(buf, buf+sizeof(buf), fmt, arg) - buf;
	va_end(arg);
    uartputs(buf, n);
    uartputs("\n", 1);

    /* Dump stack or halt */
    for(;;);
}

void
_assert(char *fmt)
{
    panic("assert: %s", fmt);
}

void
abort(void)
{
    panic("abort() called");
}

int
readstr(ulong offset, char *buf, ulong n, char *str)
{
    ulong len = strlen(str);
    if(offset >= len) return 0;
    if(n > len - offset) n = len - offset;
    memmove(buf, str+offset, n);
    return n;
}

int
readnum(ulong offset, char *buf, ulong n, ulong val, int size)
{
    char tmp[64];
    snprint(tmp, sizeof(tmp), "%*lud", size-1, val);
    return readstr(offset, buf, n, tmp);
}

ulong
strnlen(char *s, ulong max)
{
    ulong i;
    for(i=0; i<max && s[i]; i++);
    return i;
}

long
clock(void)
{
    /* TODO: Return actual ticks */
    return 0;
}