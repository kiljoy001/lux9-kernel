#include "u.h"
#include "portlib.h"

/*@
  @ requires \valid_read(s1 + (0..));
  @ requires \valid_read(s2 + (0..));
  @ requires \exists integer n1; n1 >= 0 && s1[n1] == '\0';
  @ requires \exists integer n2; n2 >= 0 && s2[n2] == '\0';
  @ assigns \nothing;
  @ ensures \result == -1 || \result == 0 || \result == 1;
  @ ensures \result == 0 <==> \strcmp(s1, s2) == 0;
  @ ensures \result > 0 <==> \strcmp(s1, s2) > 0;
  @ ensures \result < 0 <==> \strcmp(s1, s2) < 0;
  @*/
int
strcmp(char *s1, char *s2)
{
	unsigned c1, c2;

	/*@
	  @ loop invariant \valid_read(s1);
	  @ loop invariant \valid_read(s2);
	  @ loop invariant s1 >= \at(s1, Pre);
	  @ loop invariant s2 >= \at(s2, Pre);
	  @ loop invariant s1 - \at(s1, Pre) == s2 - \at(s2, Pre);
	  @ loop invariant \forall integer i; 0 <= i < (s1 - \at(s1, Pre)) ==>
	  @                  \at(s1, Pre)[i] == \at(s2, Pre)[i];
	  @ loop assigns s1, s2, c1, c2;
	  @*/
	for(;;) {
		c1 = *s1++;
		c2 = *s2++;
		if(c1 != c2) {
			if(c1 > c2)
				return 1;
			return -1;
		}
		if(c1 == 0)
			return 0;
	}
}
