#include <portlib.h>
#include <u.h>

/*@
  @ requires n >= 0;
  @ requires \valid((uchar *)a1 + (0 .. n-1));
  @ requires \valid_read((uchar *)a2 + (0 .. n-1));
  @ assigns ((uchar *)a1)[0 .. n-1];
  @ behavior found:
  @   assumes \exists integer i; 0 <= i < n && ((uchar *)a2)[i] == (uchar)(c & 0xFF);
  @   ensures \result != \null;
  @   ensures (uchar *)\result > (uchar *)a1 && (uchar *)\result <= (uchar *)a1 + n;
  @   ensures ((uchar *)\result)[-1] == (uchar)(c & 0xFF);
  @   ensures \forall integer i; 0 <= i < ((uchar *)\result - (uchar *)a1) ==>
  @             ((uchar *)a1)[i] == ((uchar *)a2)[i];
  @   ensures \forall integer i; 0 <= i < ((uchar *)\result - (uchar *)a1) - 1 ==>
  @             ((uchar *)a2)[i] != (uchar)(c & 0xFF);
  @ behavior not_found:
  @   assumes \forall integer i; 0 <= i < n ==> ((uchar *)a2)[i] != (uchar)(c & 0xFF);
  @   ensures \result == \null;
  @   ensures \forall integer i; 0 <= i < n ==> ((uchar *)a1)[i] == ((uchar *)a2)[i];
  @ complete behaviors;
  @ disjoint behaviors;
  @*/
void *memccpy(void *a1, const void *a2, int c, usize n) {
  uchar *s1;
  const uchar *s2;

  s1 = a1;
  s2 = a2;
  c &= 0xFF;
  /*@
    @ loop invariant 0 <= n <= \at(n, Pre);
    @ loop invariant s1 == (uchar *)a1 + (\at(n, Pre) - n);
    @ loop invariant s2 == (uchar *)a2 + (\at(n, Pre) - n);
    @ loop invariant \forall integer i; 0 <= i < (\at(n, Pre) - n) ==>
    @                  ((uchar *)a1)[i] == ((uchar *)a2)[i];
    @ loop invariant \forall integer i; 0 <= i < (\at(n, Pre) - n) ==>
    @                  ((uchar *)a2)[i] != (uchar)c;
    @ loop assigns n, s1, s2, ((uchar *)a1)[0 .. \at(n, Pre)-1];
    @ loop variant n;
    @*/
  while (n > 0) {
    if ((*s1++ = *s2++) == c)
      return s1;
    n--;
  }
  return 0;
}
