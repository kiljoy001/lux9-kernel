#include <libc.h>
#include <u.h>

/*
 * Return pointer to first occurrence of s2 in s1,
 * 0 if none
 */
/* ACSL removed */
char *strstr(const char *s1, const char *s2) {
  char *p, *pa, *pb;
  int c0, c;

  c0 = *s2;
  if (c0 == 0)
    return s1;
  s2++;
  /* ACSL removed */
  for (p = strchr(s1, c0); p; p = strchr(p + 1, c0)) {
    pa = p;
    /* ACSL removed */
    for (pb = s2;; pb++) {
      c = *pb;
      if (c == 0)
        return p;
      if (c != *++pa)
        break;
    }
  }
  return 0;
}
