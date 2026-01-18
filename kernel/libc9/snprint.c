#include <u.h>
#include "acsl_bounds.h"
#include <libc.h>

/*@
  @ requires len > 0 ==> \valid(buf + (0 .. len-1));
  @ requires valid_string(fmt);
  @ requires \exists integer k; k >= 0 && fmt[k] == '\0';
  @ assigns len > 0 ? buf[0 .. len-1] : \nothing;
  @ ensures 0 <= \result;
  @ ensures len > 0 && \result < len ==> buf[\result] == '\0';
  @*/
int
snprint(char *buf, int len, char *fmt, ...)
{
	int n;
	va_list args;

	va_start(args, fmt);
	n = vsnprint(buf, len, fmt, args);
	va_end(args);
	return n;
}

