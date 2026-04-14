#include "acsl_bounds.h"
#include "portlib.h"
#include "u.h"

/*@
  @ requires valid_string(s2);
  @ requires \exists integer n; n >= 0 && s2[n] == '\0';
  @ requires \valid(s1 + (0 .. \strlen(s2)));
  @ requires \separated(s1 + (0 .. \strlen(s2)), s2 + (0 .. \strlen(s2)));
  @ assigns s1[0 .. \strlen(s2)];
  @ ensures \result == s1;
  @ ensures \strlen(s1) == \strlen(s2);
  @ ensures \forall integer i; 0 <= i <= \strlen(s2) ==> s1[i] == s2[i];
  @*/
char *strcpy(char *s1, const char *s2) {
  char *os1;

  os1 = s1;
  /*@
    @ loop invariant s1 >= os1;
    @ loop invariant s2 >= \at(s2, Pre);
    @ loop invariant s1 - os1 == s2 - \at(s2, Pre);
    @ loop invariant \forall integer i; 0 <= i < (s1 - os1) ==>
    @                  os1[i] == \at(s2, Pre)[i];
    @ loop invariant \valid_read(s2);
    @ loop invariant \valid(s1);
    @ loop assigns s1, s2, os1[0 .. \strlen(\at(s2, Pre))];
    @ loop variant \strlen(\at(s2, Pre)) - (s2 - \at(s2, Pre));
    @*/
  while ((*s1++ = *s2++))
    ;
  return os1;
}
