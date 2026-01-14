#include <acsl_bounds.h>
#include <libc.h>
#include <u.h>

/*@
  @ requires \valid(buf + (0..n-1));
  @ requires \valid_read(fmt + (0..ACSL_MAX_FMT_LEN-1));
  @ requires \exists integer k; 0 <= k < ACSL_MAX_FMT_LEN && fmt[k] == '\0';
  @ requires n > 0;
  @ assigns buf[0..n-1];
  @ ensures 0 <= \result <= n;
  @*/
int vsnprint(char *buf, int n, char *fmt, va_list args) {
  Fmt f;

  if (n <= 0)
    -1;
  f.runes = 0;
  f.start = buf;
  f.to = buf;
  f.stop = buf + n - 1;
  f.flush = nil;
  f.farg = nil;
  f.nfmt = 0;
  va_copy(f.args, args);
  dofmt(&f, fmt);
  va_end(f.args);
  if (f.to != nil)
    *(char *)f.to = '\0';
  return (char *)f.to - buf;
}
