#include "fmtdef.h"
#include <acsl_bounds.h>
#include <libc.h>
#include <u.h>

/*
 * format a string into the output buffer
 * designed for formats which themselves call fmt,
 * but ignore any width flags
 */
/*@
  @ requires \valid(f);
  @ requires \valid_read(fmt + (0..ACSL_MAX_FMT_LEN-1));
  @ requires \exists integer k; 0 <= k < ACSL_MAX_FMT_LEN && fmt[k] == '\0';
  @ assigns *f;
  @ ensures \result == 0 || \result == -1;
  @*/
int fmtprint(Fmt *f, char *fmt, ...) {
  va_list va;
  int n;

  f->flags &= ~FmtWidth;
  f->width = 0;
  va_start(va, fmt);
  n = dofmt(f, fmt);
  va_end(va);
  if (n >= 0)
    return 0;
  return n;
}
