/*
 * strdup - duplicate a string
 */
#include "u.h"
#include "../include/libc.h"

/*@
  @ requires s == \null || (\valid_read(s + (0..)) && \exists integer n; n >= 0 && s[n] == '\0');
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
char*
strdup(char *s)
{
	char *new;
	usize len;

	if(s == nil)
		return nil;

	len = strlen(s) + 1;
	new = mallocz(len, 0);
	if(new == nil)
		return nil;

	memmove(new, s, len);
	return new;
}
