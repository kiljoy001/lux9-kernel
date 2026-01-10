#include <libc.h>
#include <u.h>

/*@
  @ requires len > 0 ==> \valid(buf + (0 .. len-1));
  @ requires \valid_read(fmt + (0..));
  @ requires \exists integer n; n >= 0 && fmt[n] == '\0';
  @ assigns buf[0 .. len-1];
  @ ensures \result == -1 || (0 <= \result < len);
  @ ensures \result >= 0 ==> buf[\result] == '\0';
  @ behavior invalid_args:
  @   assumes len <= 0 || buf == \null;
  @   ensures \result == -1;
  @ behavior valid_args:
  @   assumes len > 0 && buf != \null;
  @   ensures 0 <= \result < len;
  @   ensures buf[\result] == '\0';
  @ complete behaviors;
  @ disjoint behaviors;
  @*/
int vsnprint(char *buf, int len, char *fmt, va_list args) {
  Fmt f;

  if (len <= 0 || buf == nil)
    return -1;
  f.runes = 0;
  f.start = buf;
  f.to = buf;
  f.stop = buf + len - 1;
  f.flush = nil;
  f.farg = nil;
  f.nfmt = 0;
  va_copy(f.args, args);
  dofmt(&f, fmt);
  *(char *)f.to = '\0';
  return (char *)f.to - buf;
}
