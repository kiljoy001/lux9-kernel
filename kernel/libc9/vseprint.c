#include <acsl_bounds.h>
#include <libc.h>
#include <u.h>

/*@
  @ requires \valid(buf + (0..65535));
  @ requires \valid_read(fmt + (0..ACSL_MAX_FMT_LEN-1));
  @ requires \exists integer k; 0 <= k < ACSL_MAX_FMT_LEN && fmt[k] == '\0';
  @ assigns buf[0..65535];
  @ ensures 0 <= \result <= 65536;
  @*/
char *vseprint(char *buf, char *e, const char *fmt, va_list args) {
  Fmt f;

  if (e <= buf)
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
  va_end(f.args);
  *(char *)f.to = '\0';
  return (char *)f.to;
}
