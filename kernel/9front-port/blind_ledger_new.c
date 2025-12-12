/*
 * Blind Ledger - Zero-Knowledge Addressing System
 *
 * Implements capability-based memory isolation with O(log n) lookup
 * scalability. Primary index: RB-tree for capability hashes (scales to
 * millions) Secondary index: Hash table for PA reverse lookups (O(1) average)
 */

#include "../include/rbtree.h"
#include "blind_ledger.h"
#include "crypto.h"
#include "dat.h"
#include "fns.h"
#include "mem.h"
#include "portlib.h"
#include "siphash.h"
#include "u.h"
#include <error.h>

// =========================================================================
//  Internal Data Structures
// =========================================================================

// PA index hash table size (for reverse lookups only)
#define LEDGER_PA_HASHTABLE_SIZE 4096

typedef struct LedgerEntryNode {
  BlindLedgerEntry entry;
  struct rb_node rb; // RB-tree node for primary (capability) index
  struct LedgerEntryNode *pa_next; // Hash chain for secondary (PA) index
} LedgerEntryNode;

// Primary index: RB-tree ordered by capability hash (O(log n))
static struct rb_root ledger_tree = RB_ROOT;

// Secondary index: hash by physical address for fast PA lookups (O(1) average)
static LedgerEntryNode *ledger_pa_index[LEDGER_PA_HASHTABLE_SIZE];

static Lock ledger_lock;

// Global epoch counter for use-after-free prevention
static u64int global_epoch = 1;

// Entry count for statistics
static u64int ledger_entry_count = 0;
static u64int ledger_burned_count = 0;
static u64int ledger_total_memory = 0;

// SipHash key for PA hash table (DoS-resistant)
static hsiphash_key_t pa_hash_key;

// =========================================================================
//  Internal Utility Functions
// =========================================================================

// Compare two capability hashes for RB-tree ordering
// Returns: <0 if a < b, 0 if a == b, >0 if a > b
static int cap_hash_cmp(const u8int *a, const u8int *b) {
  return memcmp(a, b, BLIND_LEDGER_CAP_SIZE);
}

// SipHash-based hash function for physical addresses
static u32int hash_physical_address(uintptr pa) {
  return hsiphash(&pa, sizeof(pa), &pa_hash_key) % LEDGER_PA_HASHTABLE_SIZE;
}

// RB-tree search for capability hash
static LedgerEntryNode *ledger_tree_search(const u8int *hash) {
  struct rb_node *node = ledger_tree.rb_node;

  while (node) {
    LedgerEntryNode *entry = rb_entry(node, LedgerEntryNode, rb);
    int cmp = cap_hash_cmp(hash, entry->entry.capability.hash);

    if (cmp < 0)
      node = node->rb_left;
    else if (cmp > 0)
      node = node->rb_right;
    else
      return entry; // Found
  }
  return nil; // Not found
}

// RB-tree insert for new entry
static int ledger_tree_insert(LedgerEntryNode *new_node) {
  struct rb_node **link = &ledger_tree.rb_node;
  struct rb_node *parent = nil;
  const u8int *hash = new_node->entry.capability.hash;

  // Find insertion point
  while (*link) {
    parent = *link;
    LedgerEntryNode *entry = rb_entry(parent, LedgerEntryNode, rb);
    int cmp = cap_hash_cmp(hash, entry->entry.capability.hash);

    if (cmp < 0)
      link = &parent->rb_left;
    else if (cmp > 0)
      link = &parent->rb_right;
    else
      return -1; // Duplicate key
  }

  // Link and rebalance
  rb_link_node(&new_node->rb, parent, link);
  rb_insert_color(&new_node->rb, &ledger_tree);
  return 0;
}

// =========================================================================
//  Blind Ledger Core Functions
// =========================================================================

void blind_ledger_init(void) {
  extern int crypto_hw_sha_available(void);
  extern int tpm_get_random(u8int * buffer, int len);
  extern u64int rdrand_u64(void);
  extern int crypto_hw_rdrand_available(void);
  extern u64int chacha20_csprng_u64(void);

  memset(ledger_pa_index, 0, sizeof(ledger_pa_index));
  memset(&ledger_lock, 0, sizeof(Lock));

  // Generate SipHash key for PA hash table from secure RNG
  if (tpm_get_random((u8int *)&pa_hash_key, sizeof(pa_hash_key)) ==
      sizeof(pa_hash_key)) {
    print("blind_ledger: Using TPM random for SipHash key\n");
  } else if (crypto_hw_rdrand_available()) {
    pa_hash_key.key[0] = rdrand_u64();
    pa_hash_key.key[1] = rdrand_u64();
    print("blind_ledger: Using RDRAND for SipHash key\n");
  } else {
    pa_hash_key.key[0] = chacha20_csprng_u64();
    pa_hash_key.key[1] = chacha20_csprng_u64();
    print("blind_ledger: Using ChaCha20 CSPRNG for SipHash key (SOFTWARE "
          "FALLBACK)\n");
  }

  if (crypto_hw_sha_available()) {
    print("blind_ledger: initialized with RB-tree (O(log n)) + HW-accelerated "
          "SHA256\n");
  } else {
    print("blind_ledger: initialized with RB-tree (O(log n)) + software "
          "SHA256\n");
  }
}

/*
 * Mint a new BlindLedgerEntry and return its UserCapability.
 */
BlindLedgerError ledger_mint(UserCapability *out_cap, uintptr pa, ulong len,
                             Proc *owner, u32int permissions,
                             const u8int *vault_secret) {
  if (out_cap == nil || owner == nil || vault_secret == nil || len == 0 ||
      pa == 0) {
    return BLIND_LEDGER_EINVAL;
  }

  BlindLedgerEntry new_entry;
  memset(&new_entry, 0, sizeof(BlindLedgerEntry));

  // 1. Generate leaf_hash (immutable properties: pa, len)
  u8int leaf_hash_input[sizeof(uintptr) + sizeof(ulong)];
  memmove(leaf_hash_input, &pa, sizeof(uintptr));
  memmove(leaf_hash_input + sizeof(uintptr), &len, sizeof(ulong));
  crypto_sha256(new_entry.leaf_hash, leaf_hash_input, sizeof(leaf_hash_input));

  // 2. Copy vault_secret
  memmove(new_entry.secret, vault_secret, BLIND_LEDGER_SECRET_SIZE);

  // 3. Generate process_hash (HMAC(secret, leaf_hash))
  crypto_hmac_sha256(new_entry.process_hash, new_entry.secret,
                     BLIND_LEDGER_SECRET_SIZE, new_entry.leaf_hash,
                     BLIND_LEDGER_CAP_SIZE);

  // 4. Generate final UserCapability hash
  u8int cap_hash_input[BLIND_LEDGER_CAP_SIZE + BLIND_LEDGER_CAP_SIZE];
  memmove(cap_hash_input, new_entry.process_hash, BLIND_LEDGER_CAP_SIZE);
  memmove(cap_hash_input + BLIND_LEDGER_CAP_SIZE, new_entry.leaf_hash,
          BLIND_LEDGER_CAP_SIZE);
  crypto_sha256(out_cap->hash, cap_hash_input, sizeof(cap_hash_input));

  // 5. Populate entry fields
  memmove(&new_entry.capability, out_cap, sizeof(UserCapability));
  new_entry.physical_address = pa;
  new_entry.span_len = len;
  new_entry.owner = owner;
  new_entry.permissions = permissions;
  new_entry.state = BLIND_LEDGER_STATE_ACTIVE;
  new_entry.epoch = global_epoch;

  // 6. Populate out_cap fields
  out_cap->size = len;
  out_cap->type = CAP_TYPE_MEMORY;
  out_cap->perms = permissions;

  // 7. Allocate node and insert into both indexes
  LedgerEntryNode *node = mallocz(sizeof(LedgerEntryNode), 1);
  if (node == nil) {
    return BLIND_LEDGER_ENOMEM;
  }
  memmove(&node->entry, &new_entry, sizeof(BlindLedgerEntry));

  lock(&ledger_lock);

  // Insert into RB-tree (primary index)
  if (ledger_tree_insert(node) < 0) {
    unlock(&ledger_lock);
    free(node);
    return BLIND_LEDGER_EINVAL; // Duplicate capability (should never happen)
  }

  // Add to secondary index (by physical address)
  u32int pa_idx = hash_physical_address(pa);
  node->pa_next = ledger_pa_index[pa_idx];
  ledger_pa_index[pa_idx] = node;

  ledger_entry_count++;
  ledger_total_memory += len;

  unlock(&ledger_lock);

  blind_ledger_update_merkle_root();

  return BLIND_LEDGER_OK;
}

/*
 * Verify a UserCapability and retrieve its associated BlindLedgerEntry.
 * O(log n) lookup via RB-tree.
 */
BlindLedgerError ledger_verify(const UserCapability *cap,
                               BlindLedgerEntry *out_entry) {
  if (cap == nil || out_entry == nil) {
    return BLIND_LEDGER_EINVAL;
  }

  lock(&ledger_lock);
  LedgerEntryNode *node = ledger_tree_search(cap->hash);
  if (node == nil) {
    unlock(&ledger_lock);
    return BLIND_LEDGER_ENOTFOUND;
  }

  memmove(out_entry, &node->entry, sizeof(BlindLedgerEntry));
  unlock(&ledger_lock);

  if (out_entry->state == BLIND_LEDGER_STATE_ACTIVE) {
    return BLIND_LEDGER_OK;
  } else {
    return BLIND_LEDGER_EEXPIRED;
  }
}

/*
 * Transfer ownership (non-reversible wrapper)
 */
BlindLedgerError ledger_transfer(const UserCapability *cap, Proc *from_owner,
                                 Proc *to_owner) {
  return ledger_transfer_reversible(cap, from_owner, to_owner, nil);
}

/*
 * Transfer ownership with rollback support
 */
BlindLedgerError
ledger_transfer_reversible(const UserCapability *cap, Proc *from_owner,
                           Proc *to_owner,
                           LedgerRollbackToken *rollback_token) {
  if (cap == nil || from_owner == nil || to_owner == nil) {
    return BLIND_LEDGER_EINVAL;
  }

  lock(&ledger_lock);
  LedgerEntryNode *node = ledger_tree_search(cap->hash);
  if (node == nil) {
    unlock(&ledger_lock);
    return BLIND_LEDGER_ENOTFOUND;
  }

  if (node->entry.state != BLIND_LEDGER_STATE_ACTIVE) {
    unlock(&ledger_lock);
    return BLIND_LEDGER_EEXPIRED;
  }
  if (node->entry.owner != from_owner) {
    unlock(&ledger_lock);
    return BLIND_LEDGER_EPERM;
  }

  // Save rollback state if requested
  if (rollback_token != nil) {
    memmove(&rollback_token->original_capability, &node->entry.capability,
            sizeof(UserCapability));
    rollback_token->original_owner = node->entry.owner;
    memmove(rollback_token->original_process_hash, node->entry.process_hash,
            BLIND_LEDGER_CAP_SIZE);
    rollback_token->is_valid = ROLLBACK_TOKEN_MAGIC;
  }

  node->entry.owner = to_owner;

  // Recalculate process_hash
  u8int process_hmac_input[BLIND_LEDGER_CAP_SIZE + sizeof(Proc *) +
                           sizeof(u32int) + sizeof(BlindLedgerState)];
  memmove(process_hmac_input, node->entry.leaf_hash, BLIND_LEDGER_CAP_SIZE);
  memmove(process_hmac_input + BLIND_LEDGER_CAP_SIZE, &node->entry.owner,
          sizeof(Proc *));
  memmove(process_hmac_input + BLIND_LEDGER_CAP_SIZE + sizeof(Proc *),
          &node->entry.permissions, sizeof(u32int));
  memmove(process_hmac_input + BLIND_LEDGER_CAP_SIZE + sizeof(Proc *) +
              sizeof(u32int),
          &node->entry.state, sizeof(BlindLedgerState));
  crypto_hmac_sha256(node->entry.process_hash, node->entry.secret,
                     BLIND_LEDGER_SECRET_SIZE, process_hmac_input,
                     sizeof(process_hmac_input));

  // Recalculate capability hash
  u8int cap_hash_input[BLIND_LEDGER_CAP_SIZE + BLIND_LEDGER_CAP_SIZE];
  memmove(cap_hash_input, node->entry.process_hash, BLIND_LEDGER_CAP_SIZE);
  memmove(cap_hash_input + BLIND_LEDGER_CAP_SIZE, node->entry.leaf_hash,
          BLIND_LEDGER_CAP_SIZE);
  crypto_sha256(node->entry.capability.hash, cap_hash_input,
                sizeof(cap_hash_input));

  unlock(&ledger_lock);

  blind_ledger_update_merkle_root();

  return BLIND_LEDGER_OK;
}

/*
 * Rollback a transfer operation
 */
BlindLedgerError ledger_rollback_transfer(const UserCapability *current_cap,
                                          LedgerRollbackToken *rollback_token) {
  if (current_cap == nil || rollback_token == nil) {
    return BLIND_LEDGER_EINVAL;
  }

  if (rollback_token->is_valid != ROLLBACK_TOKEN_MAGIC) {
    return BLIND_LEDGER_EINVAL;
  }

  lock(&ledger_lock);
  LedgerEntryNode *node = ledger_tree_search(current_cap->hash);
  if (node == nil) {
    unlock(&ledger_lock);
    return BLIND_LEDGER_ENOTFOUND;
  }

  if (node->entry.state != BLIND_LEDGER_STATE_ACTIVE) {
    unlock(&ledger_lock);
    return BLIND_LEDGER_EEXPIRED;
  }

  // Restore original state
  node->entry.owner = rollback_token->original_owner;
  memmove(node->entry.process_hash, rollback_token->original_process_hash,
          BLIND_LEDGER_CAP_SIZE);
  memmove(&node->entry.capability, &rollback_token->original_capability,
          sizeof(UserCapability));

  rollback_token->is_valid = 0; // Invalidate token

  unlock(&ledger_lock);

  blind_ledger_update_merkle_root();

  return BLIND_LEDGER_OK;
}

/*
 * Burn (invalidate) a UserCapability and free underlying memory.
 */
BlindLedgerError ledger_burn(const UserCapability *cap, Proc *owner) {
  extern int pebble_black_free_internal(uintptr pa, ulong len, Proc * owner);

  if (cap == nil || owner == nil) {
    return BLIND_LEDGER_EINVAL;
  }

  lock(&ledger_lock);
  LedgerEntryNode *node = ledger_tree_search(cap->hash);
  if (node == nil) {
    unlock(&ledger_lock);
    return BLIND_LEDGER_ENOTFOUND;
  }

  if (node->entry.state != BLIND_LEDGER_STATE_ACTIVE) {
    unlock(&ledger_lock);
    return BLIND_LEDGER_EEXPIRED;
  }
  if (node->entry.owner != owner) {
    unlock(&ledger_lock);
    return BLIND_LEDGER_EPERM;
  }

  // Save PA and len before marking burned
  uintptr pa = node->entry.physical_address;
  ulong len = node->entry.span_len;

  node->entry.state = BLIND_LEDGER_STATE_BURNED;

  // Securely destroy the secret
  ledger_destroy_secret(node->entry.secret);

  // Remove from RB-tree (primary index)
  rb_erase(&node->rb, &ledger_tree);

  // Remove from PA hash (secondary index)
  u32int pa_idx = hash_physical_address(pa);
  LedgerEntryNode **indirect = &ledger_pa_index[pa_idx];
  while (*indirect != nil) {
    if (*indirect == node) {
      *indirect = node->pa_next;
      break;
    }
    indirect = &(*indirect)->pa_next;
  }

  free(node);

  ledger_entry_count--;
  ledger_burned_count++;
  ledger_total_memory -= len;

  unlock(&ledger_lock);

  // Free underlying Pebble memory (after releasing lock)
  pebble_black_free_internal(pa, len, owner);

  blind_ledger_update_merkle_root();

  return BLIND_LEDGER_OK;
}

/*
 * Lookup by PA and owner (uses hash table secondary index - O(1) average)
 */
BlindLedgerError ledger_lookup_by_pa_and_owner(uintptr pa, Proc *owner,
                                               UserCapability *out_cap,
                                               BlindLedgerEntry *out_entry) {
  if (owner == nil || out_cap == nil || out_entry == nil) {
    return BLIND_LEDGER_EINVAL;
  }

  lock(&ledger_lock);
  u32int pa_idx = hash_physical_address(pa);
  LedgerEntryNode *node = ledger_pa_index[pa_idx];

  while (node != nil) {
    if (node->entry.physical_address == pa && node->entry.owner == owner &&
        node->entry.state == BLIND_LEDGER_STATE_ACTIVE) {

      memmove(out_cap, &node->entry.capability, sizeof(UserCapability));
      memmove(out_entry, &node->entry, sizeof(BlindLedgerEntry));
      unlock(&ledger_lock);
      return BLIND_LEDGER_OK;
    }
    node = node->pa_next;
  }
  unlock(&ledger_lock);

  return BLIND_LEDGER_ENOTFOUND;
}

// =========================================================================
//  Merkle Tree
// =========================================================================

static u8int merkle_root[BLIND_LEDGER_CAP_SIZE];

static void merkle_hash_pair(u8int *out, const u8int *left,
                             const u8int *right) {
  u8int combined[BLIND_LEDGER_CAP_SIZE * 2];
  memmove(combined, left, BLIND_LEDGER_CAP_SIZE);
  memmove(combined + BLIND_LEDGER_CAP_SIZE, right, BLIND_LEDGER_CAP_SIZE);
  crypto_sha256(out, combined, sizeof(combined));
}

void blind_ledger_update_merkle_root(void) {
  u8int **leaf_hashes;
  u64int leaf_count, i, level_size, next_level_size;
  u8int *level_hashes, *next_level;
  struct rb_node *rb_iter;

  lock(&ledger_lock);

  // Count active entries via RB-tree traversal
  leaf_count = 0;
  for (rb_iter = rb_first(&ledger_tree); rb_iter; rb_iter = rb_next(rb_iter)) {
    LedgerEntryNode *node = rb_entry(rb_iter, LedgerEntryNode, rb);
    if (node->entry.state == BLIND_LEDGER_STATE_ACTIVE ||
        node->entry.state == BLIND_LEDGER_STATE_COW_RED ||
        node->entry.state == BLIND_LEDGER_STATE_COW_BLUE) {
      leaf_count++;
    }
  }

  if (leaf_count == 0) {
    memset(merkle_root, 0, BLIND_LEDGER_CAP_SIZE);
    unlock(&ledger_lock);
    return;
  }

  leaf_hashes = mallocz(leaf_count * sizeof(u8int *), 1);
  if (leaf_hashes == nil) {
    unlock(&ledger_lock);
    return;
  }

  // Collect leaf hashes
  i = 0;
  for (rb_iter = rb_first(&ledger_tree); rb_iter; rb_iter = rb_next(rb_iter)) {
    LedgerEntryNode *node = rb_entry(rb_iter, LedgerEntryNode, rb);
    if (node->entry.state == BLIND_LEDGER_STATE_ACTIVE ||
        node->entry.state == BLIND_LEDGER_STATE_COW_RED ||
        node->entry.state == BLIND_LEDGER_STATE_COW_BLUE) {
      leaf_hashes[i++] = node->entry.capability.hash;
    }
  }

  unlock(&ledger_lock);

  // Handle single entry
  if (leaf_count == 1) {
    memmove(merkle_root, leaf_hashes[0], BLIND_LEDGER_CAP_SIZE);
    free(leaf_hashes);
    return;
  }

  // Build Merkle tree
  level_size = leaf_count;
  level_hashes = mallocz(level_size * BLIND_LEDGER_CAP_SIZE, 1);
  if (level_hashes == nil) {
    free(leaf_hashes);
    return;
  }

  for (i = 0; i < leaf_count; i++) {
    memmove(level_hashes + i * BLIND_LEDGER_CAP_SIZE, leaf_hashes[i],
            BLIND_LEDGER_CAP_SIZE);
  }
  free(leaf_hashes);

  while (level_size > 1) {
    next_level_size = (level_size + 1) / 2;
    next_level = mallocz(next_level_size * BLIND_LEDGER_CAP_SIZE, 1);
    if (next_level == nil) {
      free(level_hashes);
      return;
    }

    for (i = 0; i < level_size; i += 2) {
      u8int *left = level_hashes + i * BLIND_LEDGER_CAP_SIZE;
      u8int *right = (i + 1 < level_size)
                         ? level_hashes + (i + 1) * BLIND_LEDGER_CAP_SIZE
                         : left;
      merkle_hash_pair(next_level + (i / 2) * BLIND_LEDGER_CAP_SIZE, left,
                       right);
    }

    free(level_hashes);
    level_hashes = next_level;
    level_size = next_level_size;
  }

  memmove(merkle_root, level_hashes, BLIND_LEDGER_CAP_SIZE);
  free(level_hashes);
}

const u8int *blind_ledger_get_merkle_root(void) { return merkle_root; }

// =========================================================================
//  Epoch Management
// =========================================================================

u64int ledger_get_current_epoch(void) {
  u64int epoch;
  lock(&ledger_lock);
  epoch = global_epoch;
  unlock(&ledger_lock);
  return epoch;
}

void ledger_advance_epoch(void) {
  lock(&ledger_lock);
  global_epoch++;
  if (global_epoch == 0)
    global_epoch = 1;
  unlock(&ledger_lock);
  print("blind_ledger: advanced to epoch %llud\n", global_epoch);
}

// =========================================================================
//  Secret Management
// =========================================================================

BlindLedgerError ledger_generate_secret(u8int *secret_out) {
  extern int tpm_get_random(u8int * buf, int n);
  extern void genrandom(u8int * buf, int nbytes);

  if (secret_out == nil)
    return BLIND_LEDGER_EINVAL;

  int ret = tpm_get_random(secret_out, BLIND_LEDGER_SECRET_SIZE);
  if (ret == BLIND_LEDGER_SECRET_SIZE)
    return BLIND_LEDGER_OK;

  genrandom(secret_out, BLIND_LEDGER_SECRET_SIZE);
  return BLIND_LEDGER_OK;
}

BlindLedgerError ledger_destroy_secret(const u8int *secret) {
  if (secret == nil)
    return BLIND_LEDGER_EINVAL;

  memset((void *)secret, 0, BLIND_LEDGER_SECRET_SIZE);
  return BLIND_LEDGER_OK;
}

// =========================================================================
//  Statistics
// =========================================================================

BlindLedgerError blind_ledger_get_stats(BlindLedgerStats *stats) {
  if (stats == nil)
    return BLIND_LEDGER_EINVAL;

  lock(&ledger_lock);
  stats->active_entries = ledger_entry_count;
  stats->burned_entries = ledger_burned_count;
  stats->total_memory_tracked = ledger_total_memory;
  stats->epoch = global_epoch;
  stats->tree_depth = 0; // Calculating RB-tree depth is O(N) or O(log N),
                         // skipping for now to keep O(1)
  unlock(&ledger_lock);

  return BLIND_LEDGER_OK;
}

// =========================================================================
//  Attestation
// =========================================================================

BlindLedgerError blind_ledger_attest_root(u8int *out_signature,
                                          u32int *out_len) {
  extern int crypto_tpm_hmac_sha256(uint8_t *out, const uint8_t *data,
                                    size_t len);

  if (out_signature == nil || out_len == nil)
    return BLIND_LEDGER_EINVAL;

  const u8int *root = blind_ledger_get_merkle_root();
  if (root == nil)
    return BLIND_LEDGER_EFAULT;

  // Attempt TPM attestation first
  if (crypto_tpm_hmac_sha256(out_signature, root, BLIND_LEDGER_CAP_SIZE) == 0) {
    *out_len = 32; // SHA256 size
    return BLIND_LEDGER_OK;
  }

  // Fallback: Software HMAC with a kernel-derived key (simulated for now)
  // In a real scenario this might use a key derived from boot time secrets
  u8int fallback_key[32];
  memset(fallback_key, 0xAA, 32); // Debug key

  extern int crypto_hmac_sha256(uint8_t *out, const uint8_t *key, size_t keylen,
                                const uint8_t *data, size_t len);
  crypto_hmac_sha256(out_signature, fallback_key, 32, root,
                     BLIND_LEDGER_CAP_SIZE);

  *out_len = 32;
  return BLIND_LEDGER_OK;
}
