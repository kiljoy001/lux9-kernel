/*
 * Bounded print implementation for formal verification
 */

#include "dat.h"
#include "fns.h"
#include "portlib.h"
#include "u.h"

#define BPRINT_MAX 1024

/*@
  @ requires \valid(fmt);
  @ assigns \nothing;
  @ ensures \result >= 0 && \result < BPRINT_MAX;
  @ terminates \true;
  @*/
int bprint(const char *fmt, ...) {
  char buf[BPRINT_MAX];
  va_list args;
  int n;

  /*@ assert \valid(buf + (0..BPRINT_MAX-1)); */

  va_start(args, fmt);
  n = vsnprint(buf, sizeof(buf), (char *)fmt, args);
  va_end(args);

  if (n >= 0 && n < BPRINT_MAX) {
    print("%s", buf);
  }

  return n;
}

/*@
  @ requires \valid(fmt);
  @ assigns \nothing;
  @ exits \nothing;
  @*/
void bpanic(const char *fmt, ...) {
  char buf[BPRINT_MAX];
  va_list args;

  /*@ assert \valid(buf + (0..BPRINT_MAX-1)); */

  va_start(args, fmt);
  vsnprint(buf, sizeof(buf), (char *)fmt, args);
  va_end(args);

  panic("%s", buf);
}
