#include <u.h>
#include <libc.h>

/*@
  @ requires n >= 0;
  @ requires \valid_read(s2 + (0..));
  @ requires \exists integer n2; n2 >= 0 && s2[n2] == '\0';
  @ requires \valid(s1 + (0..));
  @ requires \exists integer n1; n1 >= 0 && s1[n1] == '\0';
  @ requires \valid(s1 + (0 .. \strlen(s1) + \min(n, \strlen(s2)) + 1));
  @ assigns s1[\strlen(\old(s1)) .. \strlen(\old(s1)) + \min(n, \strlen(s2)) + 1];
  @ ensures \result == s1;
  @ ensures \forall integer i; 0 <= i < \strlen(\old(s1)) ==> s1[i] == \old(s1[i]);
  @ behavior full_append:
  @   assumes \strlen(s2) < n;
  @   ensures \strlen(s1) == \strlen(\old(s1)) + \strlen(s2);
  @   ensures \forall integer i; 0 <= i <= \strlen(s2) ==>
  @             s1[\strlen(\old(s1)) + i] == s2[i];
  @ behavior truncated_append:
  @   assumes \strlen(s2) >= n;
  @   ensures \strlen(s1) == \strlen(\old(s1)) + n;
  @   ensures \forall integer i; 0 <= i < n ==>
  @             s1[\strlen(\old(s1)) + i] == s2[i];
  @   ensures s1[\strlen(\old(s1)) + n] == '\0';
  @ complete behaviors;
  @ disjoint behaviors;
  @*/
char*
strncat(char *s1, char *s2, long n)
{
	char *os1;

	os1 = s1;
	/*@
	  @ loop invariant s1 >= os1;
	  @ loop invariant \valid(s1);
	  @ loop assigns s1;
	  @ loop variant \strlen(s1);
	  @*/
	while(*s1++)
		;
	s1--;
	/*@
	  @ loop invariant s1 >= os1 + \strlen(\at(os1, Pre));
	  @ loop invariant s2 >= \at(s2, Pre);
	  @ loop invariant n <= \at(n, Pre);
	  @ loop assigns s1, s2, n, os1[\strlen(\at(os1, Pre)) .. \strlen(\at(os1, Pre)) + \at(n, Pre)];
	  @*/
	while(*s1++ = *s2++)
		if(--n < 0) {
			s1[-1] = 0;
			break;
		}
	return os1;
}
