#include <u.h>
#include <libc.h>

/*@
  @ requires \valid(buf + (0 .. (e - buf - 1))) || e <= buf;
  @ requires \valid_read(fmt + (0..));
  @ requires \exists integer n; n >= 0 && fmt[n] == '\0';
  @ assigns buf[0 .. (e - buf - 1)];
  @ ensures e <= buf ==> \result == \null;
  @ ensures e > buf ==> \result >= buf && \result < e;
  @ ensures e > buf ==> *\result == '\0';
  @*/
char*
vseprint(char *buf, char *e, char *fmt, va_list args)
{
	Fmt f;

	if(e <= buf)
		return nil;
	f.runes = 0;
	f.start = buf;
	f.to = buf;
	f.stop = e - 1;
	f.flush = nil;
	f.farg = nil;
	f.nfmt = 0;
	va_copy(f.args, args);
	dofmt(&f, fmt);
	*(char*)f.to = '\0';
	return f.to;
}

