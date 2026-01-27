#ifndef __FRAMAC__
#include "acsl_bounds.h"
#include "fmtdef.h"
#include <libc.h>
#include <u.h>

/*@
  @ requires \valid(f);
  @ assigns *f;
  @ ensures \result == 0 || \result == 1;
  @ behavior null_start:
  @   assumes f->start == \null;
  @   ensures \result == 0;
  @ behavior valid_start:
  @   assumes f->start != \null;
  @   ensures \result == 0 || \result == 1;
  @ complete behaviors;
  @ disjoint behaviors;
  @*/
static int fmtStrFlush(Fmt *f) {
  char *s;
  int n;

  if (f->start == nil)
    return 0;
  n = (int)(uintptr)f->farg;
  n *= 2;
  s = f->start;
  f->start = realloc(s, n);
  if (f->start == nil) {
    f->farg = nil;
    f->to = nil;
    f->stop = nil;
    free(s);
    return 0;
  }
  f->farg = (void *)n;
  f->to = (char *)f->start + ((char *)f->to - s);
  f->stop = (char *)f->start + n - 1;
  return 1;
}

/*@
  @ requires \valid(f);
  @ assigns *f;
  @ ensures \result == 0 || \result == -1;
  @ ensures \result == 0 ==> f->start != \null;
  @ ensures \result == -1 ==> f->start == \null;
  @*/
int fmtstrinit(Fmt *f) {
  int n;

  memset(f, 0, sizeof *f);
  f->runes = 0;
  n = 32;
  f->start = malloc(n);
  if (f->start == nil)
    return -1;
  f->to = f->start;
  f->stop = (char *)f->start + n - 1;
  f->flush = fmtStrFlush;
  f->farg = (void *)n;
  f->nfmt = 0;
  return 0;
}

/*
 * print into an allocated string buffer
 */
/*@
  @ requires \valid_read(fmt + (0..ACSL_MAX_FMT_LEN-1));
  @ requires \exists integer n; 0 <= n < ACSL_MAX_FMT_LEN && fmt[n] == '\0';
  @ assigns \nothing;
  @ ensures \result == \null || \valid(\result);
  @ ensures \result != \null ==> \exists integer k; 0 <= k < ACSL_MAXSTR &&
  \result[k] == '\0';
  @*/
char *vsmprint(char *fmt, va_list args) {
  Fmt f;
  int n;

  if (fmtstrinit(&f) < 0)
    return nil;
  va_copy(f.args, args);
  n = dofmt(&f, fmt);
  va_end(f.args);
  if (f.start == nil)
    return nil;
  if (n < 0) {
    free(f.start);
    return nil;
  }
  *(char *)f.to = '\0';
  return f.start;
}
#endif

#ifdef __FRAMAC__
/*@ ensures \true; */ void framac_pass_dummy_vsmprint_c(void) {}
#endif
