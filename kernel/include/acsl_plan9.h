/*
 * acsl_plan9.h - Global ACSL contracts for native Plan 9 kernel functions.
 * These are intended to be appended to preprocessed files to provide
 * safety properties without clashing with existing C declarations.
 */

#ifdef __FRAMAC__

#include "acsl_bounds.h"

/* reporting.c / print.c */
/*@
  @ terminates \true;
  @ assigns \nothing;
  @*/
int print(char *fmt, ...);

/*@
  @ terminates \false;
  @ assigns \nothing;
  @*/
_Noreturn void panic(const char *fmt, ...);

/* lock.c / devarch.c */
/*@
  @ terminates \true;
  @ assigns \nothing;
  @*/
void ilock(Lock *l);

/*@
  @ terminates \true;
  @ assigns \nothing;
  @*/
void iunlock(Lock *l);

/* string / memory */
/*@
  @ lemma rounding_bounds:
  @   \forall unsigned long s, sz; (sz > 0 && (sz & (sz - 1)) == 0) ==>
  @   (s <= (((s) + ((sz) - 1)) & ~((sz) - 1)) < s + sz);
  @*/

/*@
  @ terminates \true;
  @ assigns ((char *)s)[0 .. n-1] \from c;
  @ ensures \forall integer i; 0 <= i < n ==> ((char *)s)[i] == (char)c;
  @*/
void *memset(void *s, int c, unsigned long n);

/*@
  @ requires s1 == \null || valid_string(s1);
  @ requires s2 == \null || valid_string(s2);
  @ terminates \true;
  @ exits \false;
  @ assigns \nothing;
  @*/
int strcmp(char *s1, char *s2);

/*@
  @ requires s1 == \null || valid_string(s1);
  @ requires s2 == \null || valid_string(s2);
  @ terminates \true;
  @ exits \false;
  @ assigns s1[0 .. ACSL_MAXSTR-1];
  @ ensures valid_string(s1);
  @*/
char *strcpy(char *s1, char *s2);

/*@
  @ requires s != \null;
  @ requires valid_string(s);
  @ terminates \true;
  @ exits \false;
  @ assigns \nothing;
  @ ensures \result >= 0;
  @*/
long strlen(char *s);

/*@
  @ requires r != \null;
  @ requires s != \null;
  @ requires valid_string(s);
  @ terminates \true;
  @ exits \false;
  @ assigns *r \from s[0 .. ACSL_MAXSTR-1];
  @ ensures 1 <= \result <= 4;
  @*/
int chartorune(Rune *r, char *s);

/*@
  @ assigns \nothing;
  @ ensures \result != \null ==> \valid((char *)\result + (0 .. size-1));
  @ ensures \result != \null ==> (clr != 0 ==> \forall integer i; 0 <= i < size
  ==> ((char *)\result)[i] == 0);
  @*/
void *mallocz(unsigned long size, int clr);

/*@
  @ assigns \nothing;
  @*/
unsigned long msize(void *p);

/*@
  @ requires s != \null;
  @ requires src != \null;
  @ requires n >= 0;
  @ requires n == 0 || \valid(s + (0 .. n-1));
  @ requires n == 0 || \valid_read(src + (0 .. n-1));
  @ terminates \true;
  @ exits \false;
  @ assigns s[0 .. n-1] \from src[0 .. n-1], n;
  @ ensures n == 0 || \valid(s + (0 .. n-1));
  @*/
void kstrcpy(char *s, char *src, int n);

#endif /* __FRAMAC__ */
