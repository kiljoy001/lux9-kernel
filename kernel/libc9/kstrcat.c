#include "acsl_bounds.h"
#include "portlib.h"
#include "u.h"

/*@
  @ requires valid_string(s2);
  @ requires \exists integer n2; n2 >= 0 && s2[n2] == '\0';
  @ requires valid_string(s1);
  @ requires \exists integer n1; n1 >= 0 && s1[n1] == '\0';
  @ requires \valid(s1 + (0 .. \strlen(s1) + \strlen(s2)));
  @ assigns s1[\strlen(\old(s1)) .. \strlen(\old(s1)) + \strlen(s2)];
  @ ensures \result == s1;
  @ ensures \strlen(s1) == \strlen(\old(s1)) + \strlen(s2);
  @ ensures \forall integer i; 0 <= i < \strlen(\old(s1)) ==> s1[i] ==
  \old(s1[i]);
  @ ensures \forall integer i; 0 <= i <= \strlen(s2) ==>
  @           s1[\strlen(\old(s1)) + i] == s2[i];
  @*/
char *strcat(char *s1, const char *s2) {
  char *os1;

  os1 = s1;
  /*@
    @ loop invariant s1 >= os1;
    @ loop invariant \valid(s1);
    @ loop assigns s1;
    @ loop variant \strlen(s1);
    @*/
  while (*s1++)
    ;
  s1--;
  /*@
    @ loop invariant s1 >= os1 + \strlen(\at(os1, Pre));
    @ loop invariant s2 >= \at(s2, Pre);
    @ loop invariant s1 - (os1 + \strlen(\at(os1, Pre))) == s2 - \at(s2, Pre);
    @ loop invariant \forall integer i; 0 <= i < (s2 - \at(s2, Pre)) ==>
    @                  (os1 + \strlen(\at(os1, Pre)))[i] == \at(s2, Pre)[i];
    @ loop assigns s1, s2, os1[\strlen(\at(os1, Pre)) .. \strlen(\at(os1, Pre))
    + \strlen(\at(s2, Pre))];
    @ loop variant \strlen(\at(s2, Pre)) - (s2 - \at(s2, Pre));
    @*/
  while ((*s1++ = *s2++))
    ;
  return os1;
}
