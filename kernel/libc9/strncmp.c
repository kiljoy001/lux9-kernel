#include <u.h>
#include <libc.h>

/*@
  @ requires n >= 0;
  @ requires \valid_read(s1 + (0 .. n-1)) || (\exists integer k; 0 <= k < n && s1[k] == '\0');
  @ requires \valid_read(s2 + (0 .. n-1)) || (\exists integer k; 0 <= k < n && s2[k] == '\0');
  @ assigns \nothing;
  @ ensures \result == -1 || \result == 0 || \result == 1;
  @ behavior equal:
  @   assumes \forall integer i; 0 <= i < n && s1[i] != '\0' ==> s1[i] == s2[i];
  @   ensures \result == 0;
  @ behavior less:
  @   assumes \exists integer i; 0 <= i < n &&
  @           (\forall integer j; 0 <= j < i ==> s1[j] == s2[j]) &&
  @           (unsigned char)s1[i] < (unsigned char)s2[i];
  @   ensures \result == -1;
  @ behavior greater:
  @   assumes \exists integer i; 0 <= i < n &&
  @           (\forall integer j; 0 <= j < i ==> s1[j] == s2[j]) &&
  @           (unsigned char)s1[i] > (unsigned char)s2[i];
  @   ensures \result == 1;
  @ complete behaviors;
  @ disjoint behaviors;
  @*/
int
strncmp(char *s1, char *s2, long n)
{
	unsigned c1, c2;

	/*@
	  @ loop invariant 0 <= n <= \at(n, Pre);
	  @ loop invariant s1 == \at(s1, Pre) + (\at(n, Pre) - n);
	  @ loop invariant s2 == \at(s2, Pre) + (\at(n, Pre) - n);
	  @ loop invariant \forall integer i; 0 <= i < (\at(n, Pre) - n) ==>
	  @                  \at(s1, Pre)[i] == \at(s2, Pre)[i];
	  @ loop assigns n, s1, s2, c1, c2;
	  @ loop variant n;
	  @*/
	while(n > 0) {
		c1 = *s1++;
		c2 = *s2++;
		n--;
		if(c1 != c2) {
			if(c1 > c2)
				return 1;
			return -1;
		}
		if(c1 == 0)
			break;
	}
	return 0;
}
