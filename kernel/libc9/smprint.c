#include "../include/libc.h"
#include "../include/u.h"

extern char *vsmprint(char *, va_list);

char *smprint(char *fmt, ...) {
  va_list args;
  char *p;

  va_start(args, fmt);
  p = vsmprint(fmt, args);
  va_end(args);
  return p;
}
