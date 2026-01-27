/*
 * verify.h - Central definitions for Formal Verification
 *
 * This header bridges the gap between Plan 9 C code and:
 * 1. Frama-C / ACSL (Static Analysis)
 * 2. Coq Models (Functional Correctness)
 */

#ifndef _VERIFY_H_
#define _VERIFY_H_

#ifdef __FRAMAC__
#include "framac_plan9.h"

/*
 * Standard logical definitions for verification.
 * These are only visible to the Frama-C prover.
 */

/*@
  // Force a predicate to be true (like admit in Coq)
  logic boolean admitted(boolean p) = \true;

  // Power of two function for complexity bounds
  logic integer pow2(integer n) =
    (n <= 0) ? 1 : 2 * pow2(n-1);
*/

#else
/* When compiling with GCC/Plan9 compilers, these are ignored */
#define loop_invariant(x)
#define loop_variant(x)
#define loop_assigns(x)
#define ghost_code(x)
#endif

#endif /* _VERIFY_H_ */
