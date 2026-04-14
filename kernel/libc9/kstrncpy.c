#include "acsl_bounds.h"
#include "portlib.h"
#include "u.h"

/*@
  @ requires n >= 0;
  @ requires \valid(s1 + (0 .. n-1));
  @ requires valid_string(s2);
  @ requires \exists integer m; m >= 0 && s2[m] == '\0';
  @ assigns s1[0 .. n-1];
  @ ensures \result == s1;
  @ behavior complete_copy:
  @   assumes \strlen(s2) < n;
  @   ensures \strlen(s1) == \strlen(s2);
  @   ensures \forall integer i; 0 <= i <= \strlen(s2) ==> s1[i] == s2[i];
  @   ensures \forall integer i; \strlen(s2) < i < n ==> s1[i] == '\0';
  @ behavior truncated:
  @   assumes \strlen(s2) >= n;
  @   ensures \forall integer i; 0 <= i < n ==> s1[i] == s2[i];
  @ complete behaviors;
  @ disjoint behaviors;
  @*/
char *strncpy(char *s1, const char *s2, ulong n) {
  int i;
  char *os1;

  os1 = s1;
  /*@
    @ loop invariant 0 <= i <= n;
    @ loop invariant s1 == os1 + i;
    @ loop invariant s2 == \at(s2, Pre) + i;
    @ loop invariant \forall integer j; 0 <= j < i ==> os1[j] == \at(s2,
    Pre)[j];
    @ loop assigns i, s1, s2, os1[0 .. n-1];
    @ loop variant n - i;
    @*/
  for (i = 0; i < n; i++)
    if ((*s1++ = *s2++) == 0) {
      /*@
        @ loop invariant i <= i < n;
        @ loop invariant s1 == os1 + i;
        @ loop invariant \forall integer j; 0 <= j < i ==> os1[j] == '\0' ||
        os1[j] == \at(s2, Pre)[j];
        @ loop assigns i, s1, os1[0 .. n-1];
        @ loop variant n - i;
        @*/
      while (++i < n)
        *s1++ = 0;
      return os1;
    }
  return os1;
}
