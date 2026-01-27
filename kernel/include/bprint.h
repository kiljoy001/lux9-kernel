/*
 * Bounded print for formal verification
 * All kernel messages limited to 1024 bytes for Frama-C verification
 */

#define BPRINT_MAX 1024

/*@
  @ requires \valid(fmt);
  @ assigns \nothing;
  @ ensures \result >= 0 && \result < BPRINT_MAX;
  @ terminates \true;
  @*/
int bprint(const char *fmt, ...);

/*@
  @ requires \valid(fmt);
  @ assigns \nothing;
  @ exits \nothing;
  @*/
void bpanic(const char *fmt, ...) __attribute__((noreturn));
