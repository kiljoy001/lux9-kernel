#include "acsl_bounds.h"
#include "portlib.h"
#include "u.h"

/*@
  @ requires valid_string(s);
  @ requires \exists integer n; n >= 0 && s[n] == '\0';
  @ assigns \nothing;
  @ behavior found:
  @   assumes \exists integer i; 0 <= i <= \strlen(s) && s[i] == (char)c;
  @   ensures \result != \null;
  @   ensures s <= \result && \result <= s + \strlen(s);
  @   ensures *\result == (char)c;
  @   ensures \forall integer i; 0 <= i < (\result - s) ==> s[i] != (char)c;
  @ behavior not_found:
  @   assumes \forall integer i; 0 <= i < \strlen(s) ==> s[i] != (char)c;
  @   assumes (char)c != '\0';
  @   ensures \result == \null;
  @ complete behaviors;
  @ disjoint behaviors;
  @*/
char *strchr(const char *s, int c) {
  char r;

  if (c == 0)
    /*@
      @ loop invariant s >= \at(s, Pre);
      @ loop invariant \valid_read(s);
      @ loop assigns s;
      @ loop variant \strlen(s);
      @*/
    while (*s++)
      ;
  else
    /*@
      @ loop invariant s >= \at(s, Pre);
      @ loop invariant \valid_read(s);
      @ loop invariant \forall integer i; 0 <= i < (s - \at(s, Pre)) ==>
      @                  \at(s, Pre)[i] != (char)c && \at(s, Pre)[i] != '\0';
      @ loop assigns s, r;
      @ loop variant \strlen(s);
      @*/
    while ((r = *s++) != c)
      if (r == 0)
        return 0;
  return (char *)s - 1;
}
