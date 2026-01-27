#include <libc.h>
#include <u.h>

/*@
  @ requires n >= 0;
  @ requires \valid_read((uchar *)a1 + (0 .. n-1));
  @ requires \valid_read((uchar *)a2 + (0 .. n-1));
  @ assigns \nothing;
  @ ensures \result == -1 || \result == 0 || \result == 1;
  @ behavior equal:
  @   assumes \forall integer i; 0 <= i < n ==> ((uchar *)a1)[i] == ((uchar *)a2)[i];
  @   ensures \result == 0;
  @ behavior less:
  @   assumes \exists integer i; 0 <= i < n &&
  @           (\forall integer j; 0 <= j < i ==> ((uchar *)a1)[j] == ((uchar *)a2)[j]) &&
  @           ((uchar *)a1)[i] < ((uchar *)a2)[i];
  @   ensures \result == -1;
  @ behavior greater:
  @   assumes \exists integer i; 0 <= i < n &&
  @           (\forall integer j; 0 <= j < i ==> ((uchar *)a1)[j] == ((uchar *)a2)[j]) &&
  @           ((uchar *)a1)[i] > ((uchar *)a2)[i];
  @   ensures \result == 1;
  @ complete behaviors;
  @ disjoint behaviors;
  @*/
int memcmp(const void *a1, const void *a2, usize n) {
  const uchar *s1, *s2;
  uint c1, c2;

  s1 = a1;
  s2 = a2;
  /*@
    @ loop invariant 0 <= n <= \at(n, Pre);
    @ loop invariant s1 == (uchar *)a1 + (\at(n, Pre) - n);
    @ loop invariant s2 == (uchar *)a2 + (\at(n, Pre) - n);
    @ loop invariant \forall integer i; 0 <= i < (\at(n, Pre) - n) ==>
    @                  ((uchar *)a1)[i] == ((uchar *)a2)[i];
    @ loop assigns n, s1, s2, c1, c2;
    @ loop variant n;
    @*/
  while (n > 0) {
    c1 = *s1++;
    c2 = *s2++;
    if (c1 != c2) {
      if (c1 > c2)
        return 1;
      return -1;
    }
    n--;
  }
  return 0;
}
