#include <u.h>
#include <libc.h>
#include <acsl_bounds.h>

/*@
  @ requires \valid(buf + (0..65535));
  @ requires \valid_read(fmt + (0..ACSL_MAX_FMT_LEN-1));
  @ requires \exists integer k; 0 <= k < ACSL_MAX_FMT_LEN && fmt[k] == '\0';
  @ assigns buf[0..65535];
  @ ensures 0 <= \result <= 65536;
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
