#include "acsl_bounds.h"
#include "portlib.h"
#include "u.h"

/*@
  @ requires valid_string(s);
  @ requires \exists integer n; n >= 0 && s[n] == '\0' &&
  @            \forall integer i; 0 <= i < n ==> s[i] != '\0';
  @ assigns \nothing;
  @ ensures \result >= 0;
  @ ensures s[\result] == '\0';
  @ ensures \forall integer i; 0 <= i < \result ==> s[i] != '\0';
  @ ensures \result == \strlen(s);
  @*/
ulong strlen(const char *s) {
  const char *p;
  p = s;
  /*@
    @ loop invariant s <= p;
    @ loop invariant \forall integer i; 0 <= i < (p - s) ==> s[i] != '\0';
    @ loop invariant \valid_read(p);
    @ loop assigns p;
    @ loop variant \strlen(s) - (p - s);
    @*/
  while (*p)
    p++;
  return p - s;
}
