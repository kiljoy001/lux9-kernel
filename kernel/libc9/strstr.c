#include <u.h>
#include <libc.h>

/*
 * Return pointer to first occurrence of s2 in s1,
 * 0 if none
 */
/*@
  @ requires \valid_read(s1 + (0..));
  @ requires \valid_read(s2 + (0..));
  @ requires \exists integer n1; n1 >= 0 && s1[n1] == '\0';
  @ requires \exists integer n2; n2 >= 0 && s2[n2] == '\0';
  @ assigns \nothing;
  @ behavior empty_needle:
  @   assumes s2[0] == '\0';
  @   ensures \result == s1;
  @ behavior found:
  @   assumes s2[0] != '\0';
  @   assumes \exists integer i; 0 <= i <= \strlen(s1) - \strlen(s2) &&
  @             \forall integer j; 0 <= j <= \strlen(s2) ==> s1[i+j] == s2[j];
  @   ensures \result != \null;
  @   ensures s1 <= \result <= s1 + \strlen(s1) - \strlen(s2);
  @   ensures \forall integer j; 0 <= j <= \strlen(s2) ==> \result[j] == s2[j];
  @ behavior not_found:
  @   assumes s2[0] != '\0';
  @   assumes \forall integer i; 0 <= i <= \strlen(s1) - \strlen(s2) ==>
  @             \exists integer j; 0 <= j <= \strlen(s2) && s1[i+j] != s2[j];
  @   ensures \result == \null;
  @ complete behaviors;
  @ disjoint behaviors;
  @*/
char*
strstr(char *s1, char *s2)
{
	char *p, *pa, *pb;
	int c0, c;

	c0 = *s2;
	if(c0 == 0)
		return s1;
	s2++;
	/*@
	  @ loop invariant p == \null || (s1 <= p && \valid_read(p));
	  @ loop assigns p, pa, pb, c;
	  @*/
	for(p=strchr(s1, c0); p; p=strchr(p+1, c0)) {
		pa = p;
		/*@
		  @ loop invariant pb >= \at(s2, Here);
		  @ loop invariant pa >= p;
		  @ loop invariant \valid_read(pb);
		  @ loop invariant \valid_read(pa);
		  @ loop assigns pb, pa, c;
		  @*/
		for(pb=s2;; pb++) {
			c = *pb;
			if(c == 0)
				return p;
			if(c != *++pa)
				break;
		}
	}
	return 0;
}
