#include <u.h>
#include <libc.h>

/*@
  @ requires \valid(buf + (0 .. 65535));
  @ requires \valid_read(fmt + (0..));
  @ requires \exists integer k; k >= 0 && fmt[k] == '\0';
  @ assigns buf[0 .. 65535];
  @ ensures 0 <= \result <= 65536;
  @ ensures \result < 65536 ==> buf[\result] == '\0';
  @*/
int
sprint(char *buf, char *fmt, ...)
{
	int n;
	va_list args;

	va_start(args, fmt);
	n = vsnprint(buf, 65536, fmt, args);
	va_end(args);
	return n;
}
