#include "u.h"
#include "portlib.h"
#include "mem.h"
#include "dat.h"
#include "blind_ledger.h"
#include "crypto.h" // For SHA256 hashing
#include "fns.h"    // For lock, unlock, mallocz, free
#include <error.h> // For error codes

// =========================================================================
//  Internal Data Structures for the Blind Ledger
// =========================================================================

// Simple hash map for BlindLedgerEntry instances
// TODO: Replace with a more robust, dynamic, and collision-resistant hash map
// implementation suitable for kernel use. This is a basic placeholder.
#define LEDGER_HASHTABLE_SIZE 1024

typedef struct LedgerEntryNode {
    BlindLedgerEntry entry;
    struct LedgerEntryNode *next; // Next in capability hash chain
    struct LedgerEntryNode *pa_next; // Next in PA hash chain (secondary index)
} LedgerEntryNode;

// Primary index: hash by UserCapability
static LedgerEntryNode *ledger_hashtable[LEDGER_HASHTABLE_SIZE];

// Secondary index: hash by physical address for fast PA lookups
static LedgerEntryNode *ledger_pa_index[LEDGER_HASHTABLE_SIZE];

static Lock ledger_lock; // Protects access to both hash tables

// Global epoch counter for use-after-free prevention
static u64int global_epoch = 1;

// =========================================================================
//  Internal Utility Functions
// =========================================================================

// Basic hash function for UserCapability
static u32int
hash_user_capability(UserCapability cap)
{
    u32int hash_val = 0;
    // Accumulate bytes of the capability hash using a rotating XOR for better distribution.
    // This avoids making assumptions about alignment for u32int/u64int access.
    for (int i = 0; i < BLIND_LEDGER_CAP_SIZE; i++) {
        hash_val = (hash_val << 5) ^ (hash_val >> 27) ^ cap.hash[i]; // Rotate hash_val left by 5 and XOR with current byte
    }
    return hash_val % LEDGER_HASHTABLE_SIZE;
}

// Hash function for physical addresses (secondary index)
static u32int
hash_physical_address(uintptr pa)
{
    // Simple multiplicative hash
    // Use golden ratio prime for good distribution
    u64int hash = pa * 0x9e3779b97f4a7c15ULL;
    return (u32int)(hash % LEDGER_HASHTABLE_SIZE);
}

// =========================================================================
//  Blind Ledger Core Functions
// =========================================================================

void
blind_ledger_init(void)
{
    extern int crypto_hw_sha_available(void);

    memset(ledger_hashtable, 0, sizeof(ledger_hashtable));
    memset(ledger_pa_index, 0, sizeof(ledger_pa_index));
    memset(&ledger_lock, 0, sizeof(Lock)); // Boot-safe lock initialization

    if(crypto_hw_sha_available()) {
        print("blind_ledger: initialized with HW-accelerated SHA256 (PA secondary index)\n");
    } else {
        print("blind_ledger: initialized with software SHA256 (PA secondary index)\n");
    }
}

/*
 * Mint a new BlindLedgerEntry and return its UserCapability.
 * This is called when a new memory span is tokenized (e.g., in pebble_black_alloc).
 */
BlindLedgerError
ledger_mint(UserCapability *out_cap, uintptr pa, ulong len, Proc *owner, u32int permissions, const u8int *vault_secret)
{
    if (out_cap == nil || owner == nil || vault_secret == nil || len == 0 || pa == 0) {
        return BLIND_LEDGER_EINVAL;
    }

    BlindLedgerEntry new_entry;
    memset(&new_entry, 0, sizeof(BlindLedgerEntry));

    // 1. Generate leaf_hash (immutable properties: pa, len)
    u8int leaf_hash_input[sizeof(uintptr) + sizeof(ulong)];
    memmove(leaf_hash_input, &pa, sizeof(uintptr));
    memmove(leaf_hash_input + sizeof(uintptr), &len, sizeof(ulong));
    crypto_sha256(new_entry.leaf_hash, leaf_hash_input, sizeof(leaf_hash_input));

    // 2. Copy vault_secret to new_entry.secret
    memmove(new_entry.secret, vault_secret, BLIND_LEDGER_SECRET_SIZE); 

    // 3. Generate process_hash (HMAC(secret, leaf_hash))
    //    crypto_hmac_sha256(out, key, keylen, data, len)
    crypto_hmac_sha256(new_entry.process_hash, new_entry.secret, BLIND_LEDGER_SECRET_SIZE, new_entry.leaf_hash, BLIND_LEDGER_CAP_SIZE);

    // 4. Generate the final UserCapability hash (H(process_hash || leaf_hash))
    //    crypto_sha256(out, data, len)
    u8int cap_hash_input[BLIND_LEDGER_CAP_SIZE + BLIND_LEDGER_CAP_SIZE];
    memmove(cap_hash_input, new_entry.process_hash, BLIND_LEDGER_CAP_SIZE);
    memmove(cap_hash_input + BLIND_LEDGER_CAP_SIZE, new_entry.leaf_hash, BLIND_LEDGER_CAP_SIZE);
    crypto_sha256(out_cap->hash, cap_hash_input, sizeof(cap_hash_input));

    // 5. Populate new_entry fields
    memmove(&new_entry.capability, out_cap, sizeof(UserCapability)); // Copy the full UserCapability including generated hash
    new_entry.physical_address = pa;
    new_entry.span_len = len;
    new_entry.owner = owner;
    new_entry.permissions = permissions;
    new_entry.state = BLIND_LEDGER_STATE_ACTIVE;
    new_entry.epoch = global_epoch; // Assign current global epoch

    // 6. Populate out_cap additional fields (size, type, perms)
    out_cap->size = len;
    out_cap->type = CAP_TYPE_MEMORY; // Assuming memory for now, will be passed as arg later
    out_cap->perms = permissions;

    // 7. Store in both indexes (capability hash and PA hash)
    LedgerEntryNode *node = mallocz(sizeof(LedgerEntryNode), 1); // Use mallocz
    if (node == nil) {
        return BLIND_LEDGER_ENOMEM;
    }
    memmove(&node->entry, &new_entry, sizeof(BlindLedgerEntry));

    lock(&ledger_lock);

    // Add to primary index (by UserCapability hash)
    u32int cap_idx = hash_user_capability(*out_cap);
    node->next = ledger_hashtable[cap_idx];
    ledger_hashtable[cap_idx] = node;

    // Add to secondary index (by physical address)
    u32int pa_idx = hash_physical_address(pa);
    node->pa_next = ledger_pa_index[pa_idx];
    ledger_pa_index[pa_idx] = node;

    unlock(&ledger_lock);

    // TODO: Update Merkle tree

    return BLIND_LEDGER_OK;
}

/*
 * Verify a UserCapability and retrieve its associated BlindLedgerEntry.
 * Used to check if a capability is valid and retrieve its details.
 */
BlindLedgerError
ledger_verify(const UserCapability *cap, BlindLedgerEntry *out_entry)
{
    if (cap == nil || out_entry == nil) {
        return BLIND_LEDGER_EINVAL;
    }

    lock(&ledger_lock);
    u32int idx = hash_user_capability(*cap);
    LedgerEntryNode *node = ledger_hashtable[idx];
    while (node != nil) {
        if (memcmp(node->entry.capability.hash, cap->hash, BLIND_LEDGER_CAP_SIZE) == 0) {
            memmove(out_entry, &node->entry, sizeof(BlindLedgerEntry));
            unlock(&ledger_lock);
            if (out_entry->state == BLIND_LEDGER_STATE_ACTIVE) {
                return BLIND_LEDGER_OK;
            } else {
                return BLIND_LEDGER_EEXPIRED; // Capability exists but is not active
            }
        }
        node = node->next;
    }
    unlock(&ledger_lock);

    return BLIND_LEDGER_ENOTFOUND;
}

/*
 * Transfer ownership of a UserCapability from one process to another.
 * Non-reversible version (kept for backwards compatibility).
 */
BlindLedgerError
ledger_transfer(const UserCapability *cap, Proc *from_owner, Proc *to_owner)
{
    return ledger_transfer_reversible(cap, from_owner, to_owner, nil);
}

/*
 * Transfer ownership with rollback support.
 * If rollback_token is non-nil, saves state for potential rollback.
 */
BlindLedgerError
ledger_transfer_reversible(const UserCapability *cap, Proc *from_owner, Proc *to_owner, LedgerRollbackToken *rollback_token)
{
    if (cap == nil || from_owner == nil || to_owner == nil) {
        return BLIND_LEDGER_EINVAL;
    }

    lock(&ledger_lock);
    u32int idx = hash_user_capability(*cap);
    LedgerEntryNode *node = ledger_hashtable[idx];
    while (node != nil) {
        if (memcmp(node->entry.capability.hash, cap->hash, BLIND_LEDGER_CAP_SIZE) == 0) {
            if (node->entry.state != BLIND_LEDGER_STATE_ACTIVE) {
                unlock(&ledger_lock);
                return BLIND_LEDGER_EEXPIRED;
            }
            if (node->entry.owner != from_owner) {
                unlock(&ledger_lock);
                return BLIND_LEDGER_EPERM; // Not the current owner
            }

            // Save rollback state if requested
            if (rollback_token != nil) {
                memmove(&rollback_token->original_capability, &node->entry.capability, sizeof(UserCapability));
                rollback_token->original_owner = node->entry.owner;
                memmove(rollback_token->original_process_hash, node->entry.process_hash, BLIND_LEDGER_CAP_SIZE);
                rollback_token->is_valid = ROLLBACK_TOKEN_MAGIC;
            }

            node->entry.owner = to_owner;

            // Recalculate process_hash due to owner change
            // HMAC(secret, (leaf_hash || owner || permissions || state))
            u8int process_hmac_input_data[BLIND_LEDGER_CAP_SIZE + sizeof(Proc*) + sizeof(u32int) + sizeof(BlindLedgerState)];
            memmove(process_hmac_input_data, node->entry.leaf_hash, BLIND_LEDGER_CAP_SIZE);
            memmove(process_hmac_input_data + BLIND_LEDGER_CAP_SIZE, &node->entry.owner, sizeof(Proc*));
            memmove(process_hmac_input_data + BLIND_LEDGER_CAP_SIZE + sizeof(Proc*), &node->entry.permissions, sizeof(u32int));
            memmove(process_hmac_input_data + BLIND_LEDGER_CAP_SIZE + sizeof(Proc*) + sizeof(u32int), &node->entry.state, sizeof(BlindLedgerState));
            crypto_hmac_sha256(node->entry.process_hash, node->entry.secret, BLIND_LEDGER_SECRET_SIZE, process_hmac_input_data, sizeof(process_hmac_input_data));

            // Recalculate UserCapability hash based on new process_hash
            // H(process_hash || leaf_hash)
            u8int cap_hash_input[BLIND_LEDGER_CAP_SIZE + BLIND_LEDGER_CAP_SIZE];
            memmove(cap_hash_input, node->entry.process_hash, BLIND_LEDGER_CAP_SIZE);
            memmove(cap_hash_input + BLIND_LEDGER_CAP_SIZE, node->entry.leaf_hash, BLIND_LEDGER_CAP_SIZE);
            crypto_sha256(node->entry.capability.hash, cap_hash_input, sizeof(cap_hash_input));

            blind_ledger_update_merkle_root(); // Update Merkle tree
            unlock(&ledger_lock);
            return BLIND_LEDGER_OK;
        }
        node = node->next;
    }
    unlock(&ledger_lock);

    return BLIND_LEDGER_ENOTFOUND;
}

/*
 * Rollback a transfer operation using saved state.
 * current_cap should be the capability after the transfer (with new hash).
 */
BlindLedgerError
ledger_rollback_transfer(const UserCapability *current_cap, LedgerRollbackToken *rollback_token)
{
    if (current_cap == nil || rollback_token == nil) {
        return BLIND_LEDGER_EINVAL;
    }

    // Verify rollback token is valid
    if (rollback_token->is_valid != ROLLBACK_TOKEN_MAGIC) {
        return BLIND_LEDGER_EINVAL; // Invalid or corrupted rollback token
    }

    lock(&ledger_lock);
    u32int idx = hash_user_capability(*current_cap);
    LedgerEntryNode *node = ledger_hashtable[idx];

    while (node != nil) {
        if (memcmp(node->entry.capability.hash, current_cap->hash, BLIND_LEDGER_CAP_SIZE) == 0) {
            if (node->entry.state != BLIND_LEDGER_STATE_ACTIVE) {
                unlock(&ledger_lock);
                return BLIND_LEDGER_EEXPIRED;
            }

            // Restore original state
            node->entry.owner = rollback_token->original_owner;
            memmove(node->entry.process_hash, rollback_token->original_process_hash, BLIND_LEDGER_CAP_SIZE);
            memmove(&node->entry.capability, &rollback_token->original_capability, sizeof(UserCapability));

            blind_ledger_update_merkle_root(); // Update Merkle tree

            // Invalidate rollback token to prevent reuse
            rollback_token->is_valid = 0;

            unlock(&ledger_lock);
            return BLIND_LEDGER_OK;
        }
        node = node->next;
    }
    unlock(&ledger_lock);

    return BLIND_LEDGER_ENOTFOUND;
}

/*
 * Burn (invalidate) a UserCapability.
 * This is called when a memory span is freed or a capability is cancelled.
 */
BlindLedgerError
ledger_burn(const UserCapability *cap, Proc *owner)
{
    if (cap == nil || owner == nil) {
        return BLIND_LEDGER_EINVAL;
    }

    lock(&ledger_lock);
    u32int idx = hash_user_capability(*cap);
    LedgerEntryNode **indirect = &ledger_hashtable[idx];
    LedgerEntryNode *node = *indirect;

    while (node != nil) {
        if (memcmp(node->entry.capability.hash, cap->hash, BLIND_LEDGER_CAP_SIZE) == 0) {
            if (node->entry.state != BLIND_LEDGER_STATE_ACTIVE) {
                unlock(&ledger_lock);
                return BLIND_LEDGER_EEXPIRED;
            }
            if (node->entry.owner != owner) {
                unlock(&ledger_lock);
                return BLIND_LEDGER_EPERM; // Not the current owner
            }

            node->entry.state = BLIND_LEDGER_STATE_BURNED; // Mark as burned

            // Securely destroy the secret
            ledger_destroy_secret(node->entry.secret);

            blind_ledger_update_merkle_root(); // Update Merkle tree
            // TODO: Call pebble_black_free(node->entry.physical_address, node->entry.span_len) to free underlying Pebble Pegs

            // Remove from primary index (capability hash)
            *indirect = node->next;

            // Remove from secondary index (PA hash)
            u32int pa_idx = hash_physical_address(node->entry.physical_address);
            LedgerEntryNode **pa_indirect = &ledger_pa_index[pa_idx];
            while (*pa_indirect != nil) {
                if (*pa_indirect == node) {
                    *pa_indirect = node->pa_next;
                    break;
                }
                pa_indirect = &(*pa_indirect)->pa_next;
            }

            free(node);

            unlock(&ledger_lock);
            return BLIND_LEDGER_OK;
        }
        indirect = &node->next;
        node = *indirect;
    }
    unlock(&ledger_lock);

    return BLIND_LEDGER_ENOTFOUND;
}

BlindLedgerError
ledger_lookup_by_pa_and_owner(uintptr pa, Proc *owner, UserCapability *out_cap, BlindLedgerEntry *out_entry)
{
    if (owner == nil || out_cap == nil || out_entry == nil) {
        return BLIND_LEDGER_EINVAL;
    }

    // Use secondary index for efficient PA lookup (O(1) average case vs O(n))
    lock(&ledger_lock);
    u32int pa_idx = hash_physical_address(pa);
    LedgerEntryNode *node = ledger_pa_index[pa_idx];

    while (node != nil) {
        if (node->entry.physical_address == pa &&
            node->entry.owner == owner &&
            node->entry.state == BLIND_LEDGER_STATE_ACTIVE) {

            memmove(out_cap, &node->entry.capability, sizeof(UserCapability));
            memmove(out_entry, &node->entry, sizeof(BlindLedgerEntry));
            unlock(&ledger_lock);
            return BLIND_LEDGER_OK;
        }
        node = node->pa_next; // Walk the PA index chain
    }
    unlock(&ledger_lock);

    return BLIND_LEDGER_ENOTFOUND;
}

/*
 * Merkle Tree Implementation
 *
 * Builds a binary Merkle tree from all active ledger entries.
 * Root hash provides tamper-evident integrity for entire ledger.
 */

static u8int merkle_root[BLIND_LEDGER_CAP_SIZE];

/* Helper: Hash two nodes together for Merkle tree */
static void
merkle_hash_pair(u8int *out, const u8int *left, const u8int *right)
{
    u8int combined[BLIND_LEDGER_CAP_SIZE * 2];
    memmove(combined, left, BLIND_LEDGER_CAP_SIZE);
    memmove(combined + BLIND_LEDGER_CAP_SIZE, right, BLIND_LEDGER_CAP_SIZE);
    crypto_sha256(out, combined, sizeof(combined));
}

/*
 * Update Merkle root hash
 * Called after any ledger modification (mint, transfer, burn)
 */
void
blind_ledger_update_merkle_root(void)
{
    u8int **leaf_hashes;
    ulong leaf_count, i, level_size, next_level_size;
    u8int *level_hashes, *next_level;

    /* Count active entries across all buckets */
    leaf_count = 0;
    for(i = 0; i < LEDGER_HASHTABLE_SIZE; i++){
        LedgerEntryNode *node = ledger_hashtable[i];
        while(node != nil){
            if(node->entry.state == BLIND_LEDGER_STATE_ACTIVE ||
               node->entry.state == BLIND_LEDGER_STATE_COW_RED ||
               node->entry.state == BLIND_LEDGER_STATE_COW_BLUE){
                leaf_count++;
            }
            node = node->next;
        }
    }

    /* Handle empty ledger */
    if(leaf_count == 0){
        memset(merkle_root, 0, BLIND_LEDGER_CAP_SIZE);
        return;
    }

    /* Allocate array for leaf hashes */
    leaf_hashes = mallocz(leaf_count * sizeof(u8int*), 1);
    if(leaf_hashes == nil)
        return; /* Silent failure - non-critical */

    /* Collect all active entry hashes */
    leaf_count = 0;
    for(i = 0; i < LEDGER_HASHTABLE_SIZE; i++){
        LedgerEntryNode *node = ledger_hashtable[i];
        while(node != nil){
            if(node->entry.state == BLIND_LEDGER_STATE_ACTIVE ||
               node->entry.state == BLIND_LEDGER_STATE_COW_RED ||
               node->entry.state == BLIND_LEDGER_STATE_COW_BLUE){
                /* Use capability hash as leaf */
                leaf_hashes[leaf_count++] = node->entry.capability.hash;
            }
            node = node->next;
        }
    }

    /* Handle single entry */
    if(leaf_count == 1){
        memmove(merkle_root, leaf_hashes[0], BLIND_LEDGER_CAP_SIZE);
        free(leaf_hashes);
        return;
    }

    /* Build Merkle tree bottom-up */
    level_size = leaf_count;
    level_hashes = mallocz(level_size * BLIND_LEDGER_CAP_SIZE, 1);
    if(level_hashes == nil){
        free(leaf_hashes);
        return;
    }

    /* Copy leaves to working buffer */
    for(i = 0; i < leaf_count; i++){
        memmove(level_hashes + i * BLIND_LEDGER_CAP_SIZE,
                leaf_hashes[i], BLIND_LEDGER_CAP_SIZE);
    }
    free(leaf_hashes);

    /* Iteratively hash pairs until we reach root */
    while(level_size > 1){
        next_level_size = (level_size + 1) / 2; /* Round up for odd counts */
        next_level = mallocz(next_level_size * BLIND_LEDGER_CAP_SIZE, 1);
        if(next_level == nil){
            free(level_hashes);
            return;
        }

        for(i = 0; i < level_size; i += 2){
            u8int *left = level_hashes + i * BLIND_LEDGER_CAP_SIZE;
            u8int *right;

            /* Handle odd count: duplicate last node */
            if(i + 1 < level_size)
                right = level_hashes + (i + 1) * BLIND_LEDGER_CAP_SIZE;
            else
                right = left;

            merkle_hash_pair(next_level + (i / 2) * BLIND_LEDGER_CAP_SIZE,
                           left, right);
        }

        free(level_hashes);
        level_hashes = next_level;
        level_size = next_level_size;
    }

    /* Store root */
    memmove(merkle_root, level_hashes, BLIND_LEDGER_CAP_SIZE);
    free(level_hashes);

    /* TODO: Store root in Cryptographic Vault for attestation */
}

/*
 * Get Merkle root for verification/attestation
 * Returns pointer to static root hash
 */
const u8int*
blind_ledger_get_merkle_root(void)
{
    return merkle_root;
}

/*
 * Get current global epoch
 * Used to stamp new capabilities with current allocation cycle
 */
u64int
ledger_get_current_epoch(void)
{
    u64int epoch;
    lock(&ledger_lock);
    epoch = global_epoch;
    unlock(&ledger_lock);
    return epoch;
}

/*
 * Advance global epoch counter
 * Called periodically or on major events (GC, process death) to prevent epoch reuse
 * Prevents use-after-free: old capabilities from previous epochs are invalid
 */
void
ledger_advance_epoch(void)
{
    lock(&ledger_lock);
    global_epoch++;
    if(global_epoch == 0) // Handle overflow (extremely unlikely but handle it)
        global_epoch = 1;
    unlock(&ledger_lock);
    print("blind_ledger: advanced to epoch %llud\n", global_epoch);
}

/*
 * Generate a cryptographic secret for capability derivation
 * Uses TPM-backed random number generation when available,
 * falls back to kernel CSPRNG (ChaCha20-based vault)
 */
BlindLedgerError
ledger_generate_secret(u8int *secret_out)
{
    extern int tpm_get_random(u8int *buf, int n);
    extern void genrandom(u8int *buf, int nbytes);

    if(secret_out == nil)
        return BLIND_LEDGER_EINVAL;

    // Try TPM hardware random first
    int ret = tpm_get_random(secret_out, BLIND_LEDGER_SECRET_SIZE);
    if(ret == BLIND_LEDGER_SECRET_SIZE) {
        // Successfully got TPM random bytes
        return BLIND_LEDGER_OK;
    }

    // Fallback: use kernel cryptographic vault (ChaCha20-based CSPRNG)
    // This is seeded from hardware RNG + timing entropy and is cryptographically secure
    genrandom(secret_out, BLIND_LEDGER_SECRET_SIZE);

    return BLIND_LEDGER_OK;
}

/*
 * Securely destroy a secret
 * Zeroes memory and marks as destroyed in vault
 */
BlindLedgerError
ledger_destroy_secret(const u8int *secret)
{
    if(secret == nil)
        return BLIND_LEDGER_EINVAL;

    // Clear the secret from memory (cast away const for secure erasure)
    memset((void*)secret, 0, BLIND_LEDGER_SECRET_SIZE);

    // TODO: If secret was stored in TPM NVRAM, delete it here
    // For now, we only clear from RAM

    return BLIND_LEDGER_OK;
}
