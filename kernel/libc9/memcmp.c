#include <libc.h>
#include <u.h>

int memcmp(const void *a1, const void *a2, usize n) {
  const uchar *s1, *s2;
  uint c1, c2;

  s1 = a1;
  s2 = a2;
  while (n > 0) {
    c1 = *s1++;
    c2 = *s2++;
    if (c1 != c2) {
      if (c1 > c2)
        return 1;
      return -1;
    }
    n--;
  }
  return 0;
}
