/*
 * Bounded print for formal verification
 * All kernel messages limited to 1024 bytes for Frama-C verification
 */

#define BPRINT_MAX 1024

/*@
  @ requires \valid(fmt);
  @ terminates \true;
  @ assigns \nothing;
  @ ensures \result >= 0 && \result < BPRINT_MAX;
  @*/
int bprint(const char *fmt, ...);

/*@
  @ requires \valid(fmt);
  @ terminates \false;
  @ exits \true;
  @ assigns \nothing;
  @*/
void bpanic(const char *fmt, ...) __attribute__((noreturn));
