#include "acsl_bounds.h"
#include <libc.h>
#include <u.h>

#define UVLONG_MAX (~0ULL)

/*@
  @ requires valid_string(nptr);
  @ requires \exists integer k; k >= 0 && nptr[k] == '\0';
  @ requires endptr == \null || \valid(endptr);
  @ requires -36 <= base <= 36;
  @ assigns endptr == \null ? \nothing : *endptr;
  @ ensures 0 <= \result <= UVLONG_MAX;
  @ ensures endptr == \null || \valid_read(*endptr);
  @ ensures endptr == \null || *endptr >= nptr;
  @ behavior invalid_base:
  @   assumes base < 2 || base > 36;
  @   ensures endptr != \null ==> *endptr == nptr;
  @ behavior valid_base:
  @   assumes base == 0 || (2 <= base <= 36);
  @   ensures endptr != \null ==> *endptr >= nptr;
  @ behavior overflow:
  @   ensures \result == UVLONG_MAX;
  @ complete behaviors;
  @*/
uvlong strtoull(const char *nptr, char **endptr, int base) {
  const char *p;
  uvlong n, nn, m;
  int c, ovfl, v, neg, ndig;

  p = nptr;
  neg = 0;
  n = 0;
  ndig = 0;
  ovfl = 0;

  /*
   * White space
   */
  for (;; p++) {
    switch (*p) {
    case ' ':
    case '\t':
    case '\n':
    case '\f':
    case '\r':
    case '\v':
      continue;
    }
    break;
  }

  /*
   * Sign
   */
  if (*p == '-' || *p == '+')
    if (*p++ == '-')
      neg = 1;

  /*
   * Base
   */
  if (base == 0) {
    base = 10;
    if (*p == '0') {
      base = 8;
      if (p[1] == 'x' || p[1] == 'X') {
        p += 2;
        base = 16;
      }
    }
  } else if (base == 16 && *p == '0') {
    if (p[1] == 'x' || p[1] == 'X')
      p += 2;
  } else if (base < 0 || 36 < base)
    goto Return;

  /*
   * Non-empty sequence of digits
   */
  m = UVLONG_MAX / base;
  for (;; p++, ndig++) {
    c = *p;
    v = base;
    if ('0' <= c && c <= '9')
      v = c - '0';
    else if ('a' <= c && c <= 'z')
      v = c - 'a' + 10;
    else if ('A' <= c && c <= 'Z')
      v = c - 'A' + 10;
    if (v >= base)
      break;
    if (n > m)
      ovfl = 1;
    nn = n * base + v;
    if (nn < n)
      ovfl = 1;
    n = nn;
  }

Return:
  if (ndig == 0)
    p = nptr;
  if (endptr)
    *endptr = (char *)p;
  if (ovfl)
    return UVLONG_MAX;
  if (neg)
    return -n;
  return n;
}
