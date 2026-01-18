#include <u.h>
#include "acsl_bounds.h"
#include <libc.h>

/*@
  @ requires buf <= e;
  @ requires e - buf > 0 ==> \valid(buf + (0 .. (e - buf) - 1));
  @ requires valid_string(fmt);
  @ requires \exists integer k; k >= 0 && fmt[k] == '\0';
  @ assigns e - buf > 0 ? buf[0 .. (e - buf) - 1] : \nothing;
  @ ensures \result >= buf && \result < e;
  @ ensures *\result == '\0';
  @*/
char*
seprint(char *buf, char *e, char *fmt, ...)
{
	char *p;
	va_list args;

	va_start(args, fmt);
	p = vseprint(buf, e, fmt, args);
	va_end(args);
	return p;
}
