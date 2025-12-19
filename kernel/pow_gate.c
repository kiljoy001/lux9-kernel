/*
 * Kinetic Defense - Risk-Based Proof-of-Work Gating
 * 
 * Implements "Softwar" constraints: Operations require energy expenditure
 * proportional to their risk and system load.
 */

#include "u.h"
#include "portlib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "siphash.h"

/* Global defense state */
struct KineticState {
    u64int seed_key[2];      /* Rotates periodically to prevent pre-mining */
    int base_load_factor;    /* Current system congestion level */
    Lock lock;
} kinetic;

void pow_gate_init(void) {
    /* Initialize with random seed */
    extern void genrandom(uchar *buf, int nbytes);
    genrandom((uchar*)kinetic.seed_key, 16);
    kinetic.base_load_factor = 0;
}

/*
 * Calculate Difficulty Target
 * Returns number of leading zeros required (0-64).
 */
int pow_calculate_difficulty(int op_class, ulong magnitude) {
    int diff = 0;
    int congestion = MACHP(0)->load / 100; /* Load average */

    /* Base difficulty by operation class */
    switch (op_class) {
        case POW_OP_ALLOC:
            /* Linear scaling with memory size: 1 bit per 64MB */
            if (magnitude < 4096) return 0; /* Free for small allocs */
            diff = 4 + (magnitude / (64 * 1024 * 1024));
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
            
        default:
            diff = 4;
            break;
    }

    /* Congestion Pricing: If system is loaded, everything gets harder */
    diff += congestion;

    /* Cap at 32 to prevent total lockup (approx 4 billion hashes) */
    if (diff > 32) diff = 32;
    
    return diff;
}

/*
 * Verify Proof-of-Work
 * O(1) verification of client's work.
 * 
 * nonce: The value the client found
 * context: The data being operated on (e.g., ptr address, size)
 * required_diff: Result from pow_calculate_difficulty
 */
int pow_verify(u64int nonce, u64int context, int required_diff) {
    if (required_diff <= 0) return 1;

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
    
    u64int hash = hsiphash((uchar*)input, sizeof(input), (hsiphash_key_t*)kinetic.seed_key);
    
    /* Check leading zeros via Count Leading Zeros (clz) */
    /* Note: __builtin_clzll is standard GCC/Clang */
    int zeros = 0;
    if (hash == 0) {
        zeros = 64;
    } else {
        /* Generic fallback if builtin not available in this env */
        /* Assuming we have access to standard bit ops */
        u64int mask = 1ULL << 63;
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
 */
void pow_rotate_epoch(void) {
    extern void genrandom(uchar *buf, int nbytes);
    lock(&kinetic.lock);
    genrandom((uchar*)kinetic.seed_key, 16);
    unlock(&kinetic.lock);
}
