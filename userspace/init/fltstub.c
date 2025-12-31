/* Stubs to satisfy linker matching 9front/libc requirements */
#include "../../kernel/include/libc.h"

int _efgfmt(Fmt *f) { return -1; }

void lock(Lock *l) { (void)l; }
void unlock(Lock *l) { (void)l; }

/* Stub errfmt to avoid dragging in rerrstr and kernel errstr.c */
int errfmt(Fmt *f) { return fmtstrcpy(f, "err"); }

/* Stub errstr */
int errstr(char *buf, uint n) {
  if (n > 0)
    buf[0] = 0;
  return 0;
}

/* Implement utfecpy (missing in kernel/libc9?) */
char *utfecpy(char *to, char *e, char *from) {
  char *end = e;
  if (to >= end)
    return to;
  end--; /* Leave room for NUL */
  while (*from && to < end) {
    *to++ = *from++;
  }
  *to = 0;
  return to;
}
