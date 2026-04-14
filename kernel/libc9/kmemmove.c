#include "portlib.h"
#include "u.h"

/*@
  @ requires \valid((char *)a1 + (0 .. n-1));
  @ requires \valid_read((char *)a2 + (0 .. n-1));
  @ assigns ((char *)a1)[0 .. n-1];
  @ ensures \forall integer i; 0 <= i < n ==>
  @           ((char *)a1)[i] == \old(((char *)a2)[i]);
  @ ensures \result == a1;
  @ behavior forward:
  @   assumes (char *)a2 >= (char *)a1 || (char *)a2 + n <= (char *)a1;
  @   assigns ((char *)a1)[0 .. n-1];
  @ behavior backward:
  @   assumes (char *)a2 < (char *)a1 && (char *)a2 + n > (char *)a1;
  @   assigns ((char *)a1)[0 .. n-1];
  @ complete behaviors;
  @ disjoint behaviors;
  @*/
void *memmove(void *a1, const void *a2, usize n) {
  char *s1;
  const char *s2;

  s1 = a1;
  s2 = a2;
  if ((s2 < s1) && (s2 + n > s1))
    goto back;
  /*@
    @ loop invariant 0 <= n <= \at(n, Pre);
    @ loop invariant s1 == (char *)a1 + (\at(n, Pre) - n);
    @ loop invariant s2 == (char *)a2 + (\at(n, Pre) - n);
    @ loop invariant \forall integer i; 0 <= i < (\at(n, Pre) - n) ==>
    @                  ((char *)a1)[i] == \at(((char *)a2)[i], Pre);
    @ loop assigns n, s1, s2, ((char *)a1)[0 .. \at(n, Pre)-1];
    @ loop variant n;
    @*/
  while (n > 0) {
    *s1++ = *s2++;
    n--;
  }
  return a1;

back:
  s1 += n;
  s2 += n;
  /*@
    @ loop invariant 0 <= n <= \at(n, Pre);
    @ loop invariant s1 == (char *)a1 + n;
    @ loop invariant s2 == (char *)a2 + n;
    @ loop invariant \forall integer i; n <= i < \at(n, Pre) ==>
    @                  ((char *)a1)[i] == \at(((char *)a2)[i], Pre);
    @ loop assigns n, s1, s2, ((char *)a1)[0 .. \at(n, Pre)-1];
    @ loop variant n;
    @*/
  while (n > 0) {
    *--s1 = *--s2;
    n--;
  }
  return a1;
}

/*@
  @ requires \valid((char *)a1 + (0 .. n-1));
  @ requires \valid_read((char *)a2 + (0 .. n-1));
  @ requires \separated((char *)a1 + (0 .. n-1), (char *)a2 + (0 .. n-1));
  @ assigns ((char *)a1)[0 .. n-1];
  @ ensures \forall integer i; 0 <= i < n ==>
  @           ((char *)a1)[i] == \old(((char *)a2)[i]);
  @ ensures \result == a1;
  @*/
void *memcpy(void *a1, const void *a2, usize n) { return memmove(a1, a2, n); }
