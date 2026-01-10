#include <libc.h>
#include <u.h>

/*@
  @ requires n >= 0;
  @ requires \valid_read((uchar *)ap + (0 .. n-1));
  @ assigns \nothing;
  @ behavior found:
  @   assumes \exists integer i; 0 <= i < n && ((uchar *)ap)[i] == (uchar)(c & 0xFF);
  @   ensures \result != \null;
  @   ensures (uchar *)\result >= (uchar *)ap && (uchar *)\result < (uchar *)ap + n;
  @   ensures *((uchar *)\result) == (uchar)(c & 0xFF);
  @   ensures \forall integer i; 0 <= i < ((uchar *)\result - (uchar *)ap) ==>
  @             ((uchar *)ap)[i] != (uchar)(c & 0xFF);
  @ behavior not_found:
  @   assumes \forall integer i; 0 <= i < n ==> ((uchar *)ap)[i] != (uchar)(c & 0xFF);
  @   ensures \result == \null;
  @ complete behaviors;
  @ disjoint behaviors;
  @*/
void *memchr(const void *ap, int c, usize n) {
  const uchar *sp;

  sp = ap;
  c &= 0xFF;
  /*@
    @ loop invariant 0 <= n <= \at(n, Pre);
    @ loop invariant sp == (uchar *)ap + (\at(n, Pre) - n);
    @ loop invariant \forall integer i; 0 <= i < (\at(n, Pre) - n) ==>
    @                  ((uchar *)ap)[i] != (uchar)c;
    @ loop assigns n, sp;
    @ loop variant n;
    @*/
  while (n > 0) {
    if (*sp++ == c)
      return (void *)(sp - 1);
    n--;
  }
  return 0;
}
