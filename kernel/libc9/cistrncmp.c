#include "u.h"
#include "portlib.h"

/*@
  @ requires n >= 0;
  @ requires \valid_read(s1 + (0..));
  @ requires \valid_read(s2 + (0..));
  @ requires \exists integer k1; k1 >= 0 && s1[k1] == '\0';
  @ requires \exists integer k2; k2 >= 0 && s2[k2] == '\0';
  @ assigns \nothing;
  @ ensures \result >= -255 && \result <= 255;
  @ behavior equal_prefix:
  @   assumes \forall integer i; 0 <= i < n && s1[i] != '\0' ==>
  @             (s1[i] == s2[i] ||
  @              (s1[i] >= 'A' && s1[i] <= 'Z' && s1[i] + ('a'-'A') == s2[i]) ||
  @              (s2[i] >= 'A' && s2[i] <= 'Z' && s1[i] == s2[i] + ('a'-'A')) ||
  @              (s1[i] >= 'A' && s1[i] <= 'Z' && s2[i] >= 'A' && s2[i] <= 'Z' &&
  @               s1[i] == s2[i]));
  @   ensures \result == 0 || \result == (int)(-(uchar)*s2);
  @ complete behaviors;
  @*/
int
cistrncmp(char *s1, char *s2, int n)
{
	int c1, c2;

	/*@
	  @ loop invariant 0 <= n <= \at(n, Pre);
	  @ loop invariant s1 >= \at(s1, Pre);
	  @ loop invariant s2 >= \at(s2, Pre);
	  @ loop invariant s1 - \at(s1, Pre) == s2 - \at(s2, Pre);
	  @ loop invariant s1 - \at(s1, Pre) == \at(n, Pre) - n;
	  @ loop assigns n, s1, s2, c1, c2;
	  @*/
	while(*s1 && n-- > 0){
		c1 = *(uchar*)s1++;
		c2 = *(uchar*)s2++;

		if(c1 == c2)
			continue;

		if(c1 >= 'A' && c1 <= 'Z')
			c1 -= 'A' - 'a';

		if(c2 >= 'A' && c2 <= 'Z')
			c2 -= 'A' - 'a';

		if(c1 != c2)
			return c1 - c2;
	}
	if(n <= 0)
		return 0;
	return -*s2;
}
