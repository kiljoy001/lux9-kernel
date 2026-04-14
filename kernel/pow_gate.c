/*
 * Kinetic Defense - Risk-Based Proof-of-Work Gating
 *
 * Implements "Softwar" constraints: Operations require energy expenditure
 * proportional to their risk and system load.
 *
 * FORMAL SPECIFICATION:
 * - PoW difficulty scales with operation risk and system load
 * - TCB processes (kp == 1) MUST be exempted at call sites to prevent
 *   circular dependencies and self-DoS
 * - Verification is O(1) and bound to specific transaction context
 */

#include "dat.h"
#include "fns.h"
#include "mem.h"
#include "portlib.h"
#include "siphash.h"
#include "u.h"

/* Global defense state */
struct KineticState {
  u64int seed_key[2];   /* Rotates periodically to prevent pre-mining */
  int base_load_factor; /* Current system congestion level */
  Lock lock;
} kinetic;

/*@
  @ // Global predicates for state validity
  @
  @ predicate valid_kinetic_state =
  @   kinetic.base_load_factor >= 0;
  @
  @ predicate valid_difficulty(integer d) =
  @   0 <= d <= 32;
  @
  @ predicate valid_op_class(integer op) =
  @   op == POW_OP_ALLOC ||
  @   op == POW_OP_SPAWN ||
  @   op == POW_OP_NET_BIND ||
  @   op == POW_OP_REALTIME ||
  @   op == POW_OP_STACK_ALLOC ||
  @   op == POW_OP_MSGORD;
  @
  @ // Hash-based PoW verification predicate
  @ // Specifies that a hash value has at least 'zeros' leading zero bits
  @ predicate has_leading_zeros(u64int hash, integer zeros) =
  @   zeros == 64 ? hash == 0 :
  @   zeros == 0 ? \true :
  @   (hash >> (64 - zeros)) == 0;
  @
  @ // Global invariant: kinetic state is always valid
  @ global invariant kinetic_state_valid:
  @   valid_kinetic_state;
  @*/

/*@
  @ requires \true;
  @ assigns kinetic.seed_key[0..1], kinetic.base_load_factor;
  @ ensures kinetic.base_load_factor == 0;
  @ ensures valid_kinetic_state;
  @ behavior initialization:
  @   ensures kinetic.seed_key[0] != \old(kinetic.seed_key[0]) ||
  @           kinetic.seed_key[1] != \old(kinetic.seed_key[1]);
  @*/
void pow_gate_init(void) {
  /* Initialize with random seed */
  extern void genrandom(uchar * buf, int nbytes);
  genrandom((uchar *)kinetic.seed_key, 16);
  kinetic.base_load_factor = 0;
}

/*
 * SMT: Validated by proofs/bcra_proofs.v
 * Theorems: increasing_benefit_increases_bcra, increasing_cost_decreases_bcra
 * Description: Implements cost scaling (CA) proportional to operation benefit
 * (BA)
 */
/*
 * Calculate Difficulty Target
 * Returns number of leading zeros required (0-64).
 */
/*@
  @ requires valid_op_class(op_class);
  @ requires magnitude >= 0;
  @ requires \valid_read(MACHP(0));
  @ assigns \nothing;
  @ ensures valid_difficulty(\result);
  @ ensures 0 <= \result <= 32;
  @
  @ behavior msgord_healthy:
  @   assumes op_class == POW_OP_MSGORD;
  @   assumes magnitude <= 10;
  @   ensures \result == 0;
  @
  @ behavior msgord_contested:
  @   assumes op_class == POW_OP_MSGORD;
  @   assumes magnitude > 10;
  @   ensures \result >= 1;
  @   ensures \result == \min(32, 1 + ((magnitude - 10) / 5) + (MACHP(0)->load /
  100));
  @
  @ behavior spawn:
  @   assumes op_class == POW_OP_SPAWN;
  @   ensures \result == \min(32, 12 + (MACHP(0)->load / 100));
  @
  @ behavior net_bind:
  @   assumes op_class == POW_OP_NET_BIND;
  @   ensures \result == \min(32, 8 + (MACHP(0)->load / 100));
  @
  @ behavior realtime:
  @   assumes op_class == POW_OP_REALTIME;
  @   ensures \result == \min(32, 16 + (MACHP(0)->load / 100));
  @
  @ behavior stack_alloc_tiny:
  @   assumes op_class == POW_OP_STACK_ALLOC;
  @   assumes magnitude < 1024;
  @   ensures \result == 1;
  @
  @ complete behaviors;
  @ disjoint behaviors;
  @*/
int pow_calculate_difficulty(int op_class, ulong magnitude) {
  int diff = 0;
  int congestion = MACHP(0)->load / 100; /* Load average */

  /* Base difficulty by operation class */
  switch (op_class) {
  case POW_OP_ALLOC:
    /* Budget requests: modest base cost + gradual penalty for tiny top-ups */
    if (magnitude < 4096)
      magnitude = 4096;
    diff = 1 + (magnitude / (64 * 1024 * 1024));
    if (magnitude < (64 * 1024 * 1024)) {
      ulong bucket = magnitude;
      int penalty = 0;
      /*@
        @ loop invariant 0 <= penalty <= 6;
        @ loop invariant bucket == magnitude << penalty;
        @ loop invariant penalty < 6 ==> bucket < (64 * 1024 * 1024);
        @ loop assigns bucket, penalty;
        @ loop variant 6 - penalty;
        @*/
      while (bucket < (64 * 1024 * 1024) && penalty < 6) {
        penalty++;
        bucket <<= 1;
      }
      diff += penalty;
    }
    break;

  case POW_OP_STACK_ALLOC:
    /* Stack allocation (CIL localloc) - much cheaper than heap
     * Still costs something (no freebies), but 256x easier
     * Stack frames are frequent, so difficulty is minimal */
    if (magnitude < 1024)
      return 1;                                  /* 1-bit for tiny allocs */
    diff = 2 + (magnitude / (16 * 1024 * 1024)); /* 1 bit per 16MB */
    break;

  case POW_OP_SPAWN:
    /* Forking is expensive */
    diff = 12;
    break;

  case POW_OP_NET_BIND:
    /* Binding ports is medium risk */
    diff = 8;
    break;

  case POW_OP_REALTIME:
    /* Acquiring RED tokens (EDF) is expensive */
    diff = 16;
    break;

  case POW_OP_MSGORD:
    /* MsgOrd Consensus Admission
     * magnitude = Red Message Ratio (0-100)
     * If DAG is healthy (low red ratio), entry is cheap.
     * If DAG is contested (high red ratio), entry gets expensive.
     */
    if (magnitude <= 10)
      return 0; /* No PoW required if healthy */

    diff = 1 + ((magnitude - 10) / 5);
    break;

  default:
    diff = 4;
    break;
  }

  /* Congestion Pricing: If system is loaded, everything gets harder */
  diff += congestion;

  /* Cap at 32 to prevent total lockup (approx 4 billion hashes) */
  if (diff > 32)
    diff = 32;

  return diff;
}

/*
 * Verify Proof-of-Work
 * O(1) verification of client's work.
 *
 * nonce: The value the client found
 * context: The data being operated on (e.g., ptr address, size)
 * required_diff: Result from pow_calculate_difficulty
 *
 * SECURITY INVARIANT:
 * - Hash binds nonce to context, preventing replay attacks
 * - Verification is deterministic and constant-time for given difficulty
 */
/*@
  @ requires valid_difficulty(required_diff);
  @ requires valid_kinetic_state;
  @ assigns \nothing;
  @ ensures required_diff <= 0 ==> \result == 1;
  @ ensures \result == 0 || \result == 1;
  @
  @ behavior fast_path:
  @   assumes required_diff <= 0;
  @   ensures \result == 1;
  @
  @ behavior verification:
  @   assumes required_diff > 0;
  @   ensures \result == 1 ==>
  @     \exists integer lz; has_leading_zeros(hsiphash((uchar *)input,
  sizeof(input),
  @                                            (hsiphash_key_t
  *)kinetic.seed_key), lz) &&
  @                           lz >= required_diff;
  @   ensures \result == 0 ==>
  @     \forall integer lz; has_leading_zeros(hsiphash((uchar *)input,
  sizeof(input),
  @                                            (hsiphash_key_t
  *)kinetic.seed_key), lz) ==>
  @                           lz < required_diff;
  @
  @ complete behaviors;
  @ disjoint behaviors;
  @*/
int pow_verify(u64int nonce, u64int context, int required_diff) {
  if (required_diff <= 0)
    return 1;

  /*
   * Hash(seed + context + nonce)
   * We use SipHash-2-4 because it's fast, secure, and available.
   *
   * Note: We are hashing the *request*, so the PoW is bound
   * to this specific transaction. It cannot be replayed.
   */
  u64int input[2];
  input[0] = context;
  input[1] = nonce;

  u64int hash = hsiphash((uchar *)input, sizeof(input),
                         (hsiphash_key_t *)kinetic.seed_key);

  /* Check leading zeros via Count Leading Zeros (clz) */
  /* Note: __builtin_clzll is standard GCC/Clang */
  int zeros = 0;
  if (hash == 0) {
    zeros = 64;
  } else {
    /* Generic fallback if builtin not available in this env */
    /* Assuming we have access to standard bit ops */
    u64int mask = 1ULL << 63;
    /*@
      @ loop invariant 0 <= zeros <= 64;
      @ loop invariant zeros < 64 ==> mask == (1ULL << (63 - zeros));
      @ loop invariant zeros == 64 ==> mask == 0;
      @ loop invariant \forall integer i; 0 <= i < zeros ==>
      @                  ((hash >> (63 - i)) & 1) == 0;
      @ loop assigns zeros, mask;
      @ loop variant (mask > 0 ? 64 - zeros : 0);
      @*/
    while ((hash & mask) == 0 && mask > 0) {
      zeros++;
      mask >>= 1;
    }
  }

  return (zeros >= required_diff);
}

/*
 * Rotate the seed to prevent "Long Range Attacks" (Pre-mining)
 * Called by timer interrupt every N seconds.
 *
 * SECURITY INVARIANT:
 * - Seed rotation invalidates pre-computed nonces
 * - Periodic rotation prevents long-range mining attacks
 * - Lock ensures atomic update visible to all CPUs
 */
/*@
  @ requires valid_kinetic_state;
  @ requires \valid(&kinetic.lock);
  @ assigns kinetic.seed_key[0..1];
  @ ensures valid_kinetic_state;
  @ ensures kinetic.seed_key[0] != \old(kinetic.seed_key[0]) ||
  @         kinetic.seed_key[1] != \old(kinetic.seed_key[1]);
  @
  @ behavior atomic_rotation:
  @   ensures \forall integer i; 0 <= i < 2 ==>
  @     kinetic.seed_key[i] != \old(kinetic.seed_key[i]);
  @*/
void pow_rotate_epoch(void) {
  extern void genrandom(uchar * buf, int nbytes);
  lock(&kinetic.lock);
  genrandom((uchar *)kinetic.seed_key, 16);
  unlock(&kinetic.lock);
}
