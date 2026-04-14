#include "u.h"
#include "portlib.h"

/*@
  @ requires \valid((char *)ap + (0 .. n-1));
  @ assigns ((char *)ap)[0 .. n-1];
  @ ensures \forall integer i; 0 <= i < n ==> ((char *)ap)[i] == (char)c;
  @ ensures \result == ap;
  @ behavior empty:
  @   assumes n == 0;
  @   assigns \nothing;
  @   ensures \result == ap;
  @ behavior nonempty:
  @   assumes n > 0;
  @   assigns ((char *)ap)[0 .. n-1];
  @   ensures \forall integer i; 0 <= i < n ==> ((char *)ap)[i] == (char)c;
  @ complete behaviors;
  @ disjoint behaviors;
  @*/
void*
memset(void *ap, int c, usize n)
{
	char *p;

	p = ap;
	/*@
	  @ loop invariant 0 <= n <= \at(n, Pre);
	  @ loop invariant p == (char *)ap + (\at(n, Pre) - n);
	  @ loop invariant \forall integer i; 0 <= i < (\at(n, Pre) - n) ==>
	  @                  ((char *)ap)[i] == (char)c;
	  @ loop assigns n, p, ((char *)ap)[0 .. \at(n, Pre)-1];
	  @ loop variant n;
	  @*/
	while(n > 0) {
		*p++ = c;
		n--;
	}
	return ap;
}
