#include "../include/lux.h"

void *memmove(void *dest, const void *src, ulong n) {
  char *d = dest;
  const char *s = src;
  if (d < s) {
    while (n--)
      *d++ = *s++;
  } else {
    d += n;
    s += n;
    while (n--)
      *--d = *--s;
  }
  return dest;
}
