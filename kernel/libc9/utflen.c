#include "acsl_bounds.h"
#include <libc.h>
#include <u.h>

/*@
  @ requires \valid_read(s + (0..ACSL_MAXSTR-1));
  @ requires \exists integer k; 0 <= k < ACSL_MAXSTR && s[k] == '\0';
  @ assigns \nothing;
  @ ensures \result >= 0;
  @ ensures \result <= ACSL_MAXSTR;
  @*/
int utflen(char *s) {
  int c;
  long n;
  Rune rune;

  n = 0;
  /*@
    @ loop invariant n >= 0;
    @ loop invariant s >= \at(s, Pre);
    @ loop invariant s <= \at(s, Pre) + ACSL_MAXSTR;
    @ loop invariant \valid_read(s);
    @ loop assigns c, n, s, rune;
    @ loop variant ACSL_MAXSTR - (s - \at(s, Pre));
    @*/
  for (;;) {
    c = *(uchar *)s;
    if (c < Runeself) {
      if (c == 0)
        return n;
      s++;
    } else
      s += chartorune(&rune, s);
    n++;
  }
}
