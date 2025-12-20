#ifndef SECURITY_H
#define SECURITY_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/*
 * Security context for operations
 */
typedef struct SecurityContext {
    uint64_t process_id;
    uint64_t thread_id;
    uint64_t trust_level;
    uint64_t capabilities;
    bool verified;
} SecurityContext;

/*
 * HMAC implementation using SHA-256 or BLAKE2s
 */
typedef struct HMACKey {
    uint8_t key[32];           // 256-bit HMAC key
    uint8_t hash[32];          // Last hash value
    uint64_t operation_count;  // Performance tracking
    enum {
        HMAC_SHA256_HW,        // Hardware-accelerated SHA-256
        HMAC_SHA256_SW,        // Software SHA-256
        HMAC_BLAKE2S,          // BLAKE2s
        HMAC_SHA3_256          // SHA-3-256
    } algorithm;
} HMACKey;

/*
 * Borrow checker integration
 */
typedef struct BorrowTracker {
    void *owners[1024];        // Ownership tracking
    uint64_t owner_ids[1024];  // Owner IDs
    uint32_t count;            // Active tracking entries
} BorrowTracker;

/*
 * Pebble chain verification
 */
typedef struct PebbleChain {
    uint8_t chain_hash[32];    // Current chain state
    uint64_t sequence_number;  // Operation sequence
    uint64_t budget;           // Pebble budget tracking
} PebbleChain;

/*
 * Security operations
 */
int security_init_hmac(HMACKey *key);
uint64_t security_compute_hmac(HMACKey *key, const void *data, size_t len);
bool security_verify_hmac(HMACKey *key, const void *data, size_t len, uint64_t expected_tag);
int security_register_borrow(BorrowTracker *tracker, void *ptr, uint64_t owner_id);
bool security_validate_borrow(BorrowTracker *tracker, void *ptr, uint64_t requester_id);
int security_update_pebble(PebbleChain *pebble, const void *data, size_t len);
bool security_verify_pebble(PebbleChain *pebble, uint64_t expected_sequence);

/*
 * Utility functions
 */
bool security_has_sha_ni(void);
uint64_t security_rdtsc(void);  // Performance monitoring
void security_perf_report(const HMACKey *key);

#endif /* SECURITY_H */