/*
 * strdup - duplicate a string
 */
#include "../include/libc.h"
#include "acsl_bounds.h"
#include "u.h"

/*@
  @ requires s == \null || (valid_string(s) && \exists integer n; n >= 0 && s[n]
  == '\0');
  @ assigns \nothing;
  @ behavior null_input:
  @   assumes s == \null;
  @   ensures \result == \null;
  @ behavior valid_input:
  @   assumes s != \null;
  @   behavior allocation_success:
  @     assumes s != \null;
  @     ensures \result == \null || (
  @       \valid(\result + (0 .. \strlen(s))) &&
  @       \forall integer i; 0 <= i <= \strlen(s) ==> \result[i] == s[i] &&
  @       \strlen(\result) == \strlen(s)
  @     );
  @ complete behaviors;
  @ disjoint behaviors;
  @*/
char *strdup(const char *s) {
  char *new;
  usize len;

  if (s == nil)
    return nil;

  len = strlen(s) + 1;
  new = mallocz(len, 0);
  if (new == nil)
    return nil;

  memmove(new, s, len);
  return new;
}
