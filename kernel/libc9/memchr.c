#include <libc.h>
#include <u.h>

void *memchr(const void *ap, int c, usize n) {
  const uchar *sp;

  sp = ap;
  c &= 0xFF;
  while (n > 0) {
    if (*sp++ == c)
      return (void *)(sp - 1);
    n--;
  }
  return 0;
}
