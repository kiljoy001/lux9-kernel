#include "../include/libc.h"
#include "acsl_bounds.h"
#include "../include/u.h"

extern char *vsmprint(char *, va_list);

/*@
  @ requires valid_string(fmt);
  @ requires \exists integer k; k >= 0 && fmt[k] == '\0';
  @ assigns \nothing;
  @ ensures \result == \null || \valid(\result + (0 .. \strlen(\result)));
  @*/
char *smprint(char *fmt, ...) {
  va_list args;
  char *p;

  va_start(args, fmt);
  p = vsmprint(fmt, args);
  va_end(args);
  return p;
}
