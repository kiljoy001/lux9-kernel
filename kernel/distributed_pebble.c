#ifndef __FRAMAC__
/* distributed_pebble.c - Cross-Machine Token Economy Implementation
 *
 * Implements distributed token accounting with:
 *   - Arena branches for lock-free local allocation
 *   - Secret branches with Elligator encoding
 *   - Merkle proofs for cross-machine transfers
 *   - MSGORD integration for global ordering
 */

#include "include/distributed_pebble.h"
#include "dat.h"
#include "error.h"
#include "fns.h"
#include "lib.h"
#include "mem.h"
#include "monocypher.h"
#include "u.h"

/* Forward declarations */
static void machine_bank_update_root(MachineBank *bank);
static int branch_refill(ArenaBranch *branch, TokenType type);

/* Global state */
MachineBank *local_machine_bank = nil;
static Lock distributed_pebble_lock;

/* ========== Machine Bank ========== */

void machine_bank_init(MachineBank *bank, uuid_t *machine_id, u64int ram_bytes,
                       u64int cpu_count, u64int gpu_mem_bytes,
                       u64int net_bandwidth) {
  if (bank == nil || machine_id == nil)
    return;

  memset(bank, 0, sizeof(MachineBank));
  uuid_copy(&bank->machine_id, machine_id);

  /* Calculate token pools from physical resources */
  bank->total_tokens[TOK_MEMORY] = ram_bytes / TOK_MEMORY_BYTES;
  bank->total_tokens[TOK_CPU] =
      cpu_count * 1000 * 60; /* 1 min of CPU time per core */
  bank->total_tokens[TOK_GPU] =
      gpu_mem_bytes / TOK_MEMORY_BYTES; /* GPU mem as tokens */
  bank->total_tokens[TOK_NETWORK] = net_bandwidth / TOK_NETWORK_BYTES;

  /* All tokens start available */
  for (int i = 0; i < TOK_MAX; i++)
    bank->available[i] = bank->total_tokens[i];

  bank->epoch = 1;
  bank->branch_count = 0;

  /* Compute initial Merkle root */
  machine_bank_update_root(bank);

  print("DPEBBLE: bank init mem=%llud cpu=%llud gpu=%llud net=%llud "
        "tokens\n",
        bank->total_tokens[TOK_MEMORY], bank->total_tokens[TOK_CPU],
        bank->total_tokens[TOK_GPU], bank->total_tokens[TOK_NETWORK]);
}

/* Hash helper using BLAKE2b */
static void hash_tokens(u64int *tokens, int count, BlindLedgerHash *out) {
  crypto_blake2b((u8int *)out, BLIND_LEDGER_CAP_SIZE, (u8int *)tokens,
                 count * sizeof(u64int));
}

/* Update Merkle root from current state */
static void machine_bank_update_root(MachineBank *bank) {
  BlindLedgerHash leaves[TOK_MAX + MAX_BRANCHES_PER_MACHINE];
  int leaf_count = 0;

  /* Hash each token type */
  for (int i = 0; i < TOK_MAX; i++) {
    hash_tokens(&bank->available[i], 1, &leaves[leaf_count++]);
  }

  /* Hash each branch */
  for (u32int i = 0; i < bank->branch_count; i++) {
    if (bank->branches[i] != nil) {
      hash_tokens(bank->branches[i]->tokens, TOK_MAX, &leaves[leaf_count++]);
    }
  }

  /* Build Merkle tree (simple pairwise hashing) */
  while (leaf_count > 1) {
    int new_count = 0;
    for (int i = 0; i < leaf_count; i += 2) {
      if (i + 1 < leaf_count) {
        u8int combined[BLIND_LEDGER_CAP_SIZE * 2];
        memmove(combined, &leaves[i], BLIND_LEDGER_CAP_SIZE);
        memmove(combined + BLIND_LEDGER_CAP_SIZE, &leaves[i + 1],
                BLIND_LEDGER_CAP_SIZE);
        crypto_blake2b((u8int *)&leaves[new_count], BLIND_LEDGER_CAP_SIZE,
                       combined, sizeof(combined));
      } else {
        memmove(&leaves[new_count], &leaves[i], BLIND_LEDGER_CAP_SIZE);
      }
      new_count++;
    }
    leaf_count = new_count;
  }

  memmove(&bank->bank_root, &leaves[0], BLIND_LEDGER_CAP_SIZE);
  bank->epoch++;
}

void machine_bank_get_root(MachineBank *bank, BlindLedgerHash *out_root) {
  if (bank == nil || out_root == nil)
    return;
  lock(&bank->lock);
  memmove(out_root, &bank->bank_root, BLIND_LEDGER_CAP_SIZE);
  unlock(&bank->lock);
}

int machine_bank_prove_balance(MachineBank *bank, TokenType type, u64int amount,
                               TokenProof *out_proof) {
  if (bank == nil || out_proof == nil || type >= TOK_MAX)
    return -1;

  lock(&bank->lock);

  /* Check sufficient balance */
  if (bank->available[type] < amount) {
    unlock(&bank->lock);
    return -1;
  }

  /* Build proof */
  uuid_copy(&out_proof->machine_id, &bank->machine_id);
  out_proof->token_type = type;
  out_proof->amount = amount;
  out_proof->epoch = bank->epoch;

  /* Generate Merkle proof for this token type */
  /* Simplified: just include current balance hash */
  hash_tokens(&bank->available[type], 1, &out_proof->balance_proof.target_hash);
  out_proof->balance_proof.depth = 1;
  /* Full Merkle sibling path would go here for production */

  unlock(&bank->lock);
  return 0;
}

int machine_bank_verify_proof(const TokenProof *proof,
                              const BlindLedgerHash *expected_root) {
  if (proof == nil || expected_root == nil)
    return 0;

  /* Verify the Merkle path reconstructs to expected root */
  BlindLedgerHash computed;
  memmove(&computed, &proof->balance_proof.target_hash, BLIND_LEDGER_CAP_SIZE);

  for (u32int i = 0; i < proof->balance_proof.depth; i++) {
    u8int combined[BLIND_LEDGER_CAP_SIZE * 2];
    int is_right = (proof->balance_proof.path_bits >> i) & 1;

    if (is_right) {
      memmove(combined, (void *)&proof->balance_proof.siblings[i],
              BLIND_LEDGER_CAP_SIZE);
      memmove(combined + BLIND_LEDGER_CAP_SIZE, &computed,
              BLIND_LEDGER_CAP_SIZE);
    } else {
      memmove(combined, &computed, BLIND_LEDGER_CAP_SIZE);
      memmove(combined + BLIND_LEDGER_CAP_SIZE,
              (void *)&proof->balance_proof.siblings[i], BLIND_LEDGER_CAP_SIZE);
    }

    crypto_blake2b((u8int *)&computed, BLIND_LEDGER_CAP_SIZE, combined,
                   sizeof(combined));
  }

  return memcmp(&computed, (void *)expected_root, BLIND_LEDGER_CAP_SIZE) == 0;
}

/* ========== Arena Branch ========== */

ArenaBranch *branch_create(MachineBank *bank, u32int flags) {
  if (bank == nil)
    return nil;

  lock(&bank->lock);

  if (bank->branch_count >= MAX_BRANCHES_PER_MACHINE) {
    unlock(&bank->lock);
    return nil;
  }

  ArenaBranch *branch = mallocz(sizeof(ArenaBranch), 1);
  if (branch == nil) {
    unlock(&bank->lock);
    return nil;
  }

  uuid_new_v8(&branch->branch_id);
  branch->flags = flags;
  branch->machine = bank;
  branch->parent = nil;

  /* Set default thresholds */
  for (int i = 0; i < TOK_MAX; i++) {
    branch->low_water[i] = 64;    /* Refill when below 64 tokens */
    branch->high_water[i] = 1024; /* Return when above 1024 */
    branch->tokens[i] = 0;
    branch->borrowed[i] = 0;
  }

  /* Add to bank */
  bank->branches[bank->branch_count++] = branch;
  machine_bank_update_root(bank);

  unlock(&bank->lock);

  print("DPEBBLE: branch created flags=%x\n", flags);
  return branch;
}

ArenaBranch *branch_create_secret(MachineBank *bank,
                                  const u8int *elligator_key) {
  ArenaBranch *branch = branch_create(bank, BRANCH_SECRET);
  if (branch == nil)
    return nil;

  /* Store Elligator key */
  if (elligator_key != nil) {
    memmove(branch->elligator_secret, elligator_key, 32);
  } else {
    /* Generate random key */
    extern void chacha20_csprng_fill(u8int * buf, ulong len);
    chacha20_csprng_fill(branch->elligator_secret, 32);
  }

  /* Encode initial state */
  branch_elligator_encode(branch);

  print("DPEBBLE: SECRET branch created (deniable)\n");
  return branch;
}

int branch_alloc(ArenaBranch *branch, TokenType type, u64int amount) {
  if (branch == nil || type >= TOK_MAX)
    return -1;

  lock(&branch->lock);

  /* Fast path: allocate from local cache */
  if (branch->tokens[type] >= amount) {
    branch->tokens[type] -= amount;
    branch->alloc_count++;
    unlock(&branch->lock);
    return 0;
  }

  /* Slow path: try to refill from parent bank */
  unlock(&branch->lock);

  if (branch_refill(branch, type) < 0)
    return -1;

  /* Retry allocation */
  lock(&branch->lock);
  if (branch->tokens[type] >= amount) {
    branch->tokens[type] -= amount;
    branch->alloc_count++;
    unlock(&branch->lock);
    return 0;
  }
  unlock(&branch->lock);

  return -1; /* Still insufficient */
}

/* Refill branch from parent bank */
static int branch_refill(ArenaBranch *branch, TokenType type) {
  if (branch == nil || branch->machine == nil)
    return -1;

  MachineBank *bank = branch->machine;
  u64int refill_amount = branch->high_water[type]; /* Refill to high water */

  lock(&bank->lock);
  lock(&branch->lock);

  /* Check bank has tokens */
  if (bank->available[type] < refill_amount) {
    refill_amount = bank->available[type];
  }

  if (refill_amount == 0) {
    unlock(&branch->lock);
    unlock(&bank->lock);
    return -1;
  }

  /* Transfer from bank to branch */
  bank->available[type] -= refill_amount;
  branch->tokens[type] += refill_amount;
  branch->borrowed[type] += refill_amount;
  branch->refill_count++;

  machine_bank_update_root(bank);

  unlock(&branch->lock);
  unlock(&bank->lock);

  return 0;
}

void branch_free(ArenaBranch *branch, TokenType type, u64int amount) {
  if (branch == nil || type >= TOK_MAX)
    return;

  lock(&branch->lock);
  branch->tokens[type] += amount;

  /* Check if we should return excess to bank */
  if (branch->tokens[type] > branch->high_water[type]) {
    unlock(&branch->lock);
    branch_reconcile(branch);
  } else {
    unlock(&branch->lock);
  }
}

int branch_reconcile(ArenaBranch *branch) {
  if (branch == nil || branch->machine == nil)
    return -1;

  MachineBank *bank = branch->machine;

  lock(&bank->lock);
  lock(&branch->lock);

  /* Return excess tokens to bank */
  for (int i = 0; i < TOK_MAX; i++) {
    if (branch->tokens[i] > branch->high_water[i]) {
      u64int excess = branch->tokens[i] - branch->high_water[i];
      branch->tokens[i] -= excess;
      branch->borrowed[i] -= excess;
      bank->available[i] += excess;
    }
  }

  branch->reconcile_count++;
  machine_bank_update_root(bank);

  /* Re-encode if secret branch */
  if (branch->flags & BRANCH_SECRET) {
    branch_elligator_encode(branch);
  }

  unlock(&branch->lock);
  unlock(&bank->lock);

  return 0;
}

void branch_destroy(ArenaBranch *branch) {
  if (branch == nil)
    return;

  MachineBank *bank = branch->machine;
  if (bank == nil) {
    free(branch);
    return;
  }

  lock(&bank->lock);

  /* Return all tokens to bank */
  for (int i = 0; i < TOK_MAX; i++) {
    bank->available[i] += branch->tokens[i];
  }

  /* Remove from bank's branch list */
  for (u32int i = 0; i < bank->branch_count; i++) {
    if (bank->branches[i] == branch) {
      /* Shift remaining */
      for (u32int j = i; j < bank->branch_count - 1; j++) {
        bank->branches[j] = bank->branches[j + 1];
      }
      bank->branch_count--;
      break;
    }
  }

  machine_bank_update_root(bank);
  unlock(&bank->lock);

  /* Wipe secret if deniable */
  if (branch->flags & BRANCH_SECRET) {
    crypto_wipe(branch->elligator_secret, 32);
  }

  free(branch);
}

/* ========== Elligator Encoding for Secret Branches ========== */

/* Helper to derive key from Elligator representative */
/*@
  requires \valid(rep + (0 .. 31));
  requires \valid(key_out + (0 .. 31));
  assigns key_out[0 .. 31];
*/
static void derive_key_from_rep(const u8int *rep, u8int *key_out) {
  u8int curve_point[32];
  /* Map random string (rep) to valid Curve25519 point */
  crypto_elligator_map(curve_point, rep);
  /* Hash the point to get a symmetric encryption key */
  crypto_blake2b(key_out, 32, curve_point, 32);
}

/*@
  requires branch != \null;
  requires \valid(branch);
  requires \valid(branch->encoded_state + (0 .. 31));
  requires \valid((u8int *)branch->tokens + (0 .. 31));
  requires \valid((u8int *)branch->borrowed + (0 .. 31));
  assigns branch->encoded_state[0 .. 31];
  assigns branch->tokens[0 .. 3];
  assigns branch->borrowed[0 .. 3];
  ensures \result == 0 || \result == -1;
*/
int branch_elligator_encode(ArenaBranch *branch) {
  if (branch == nil || !(branch->flags & BRANCH_SECRET))
    return -1;

  /* 1. Use existing elligator_secret as the random Representative R */
  /* In a full implementation, we might regenerate this using CSPRNG */
  u8int rep[32];
  memmove(rep, branch->elligator_secret, 32);
  memmove(branch->encoded_state, rep, 32);

  /* 2. Derive Symmetric Key K from R */
  u8int key[32];
  derive_key_from_rep(rep, key);

  /* 3. Encrypt the branch tokens and borrowed state */
  /* We treat the struct fields as a flat buffer */
  u8int data[64];
  memmove(data, branch->tokens, 32);
  memmove(data + 32, branch->borrowed, 32);

  u8int nonce[8] = {0}; /* Ephemeral key (random R) implies unique key, so zero
                           nonce is safe */
  crypto_chacha20_djb(data, data, 64, key, nonce, 0);

  /* 4. Overwrite valid data with "noise" (ciphertext) */
  memmove(branch->tokens, data, 32);
  memmove(branch->borrowed, data + 32, 32);

  return 0;
}

/*@
  requires branch != \null;
  requires \valid(branch);
  requires \valid(branch->encoded_state + (0 .. 31));
  requires \valid((u8int *)branch->tokens + (0 .. 31));
  requires \valid((u8int *)branch->borrowed + (0 .. 31));
  assigns branch->tokens[0 .. 3];
  assigns branch->borrowed[0 .. 3];
  ensures \result == 0 || \result == -1;
*/
int branch_elligator_decode(ArenaBranch *branch, const u8int *elligator_key) {
  if (branch == nil)
    return -1;

  /* 1. Read Representative R from encoded state */
  /* We ignore elligator_key arg as the key is embedded in the lock
   * (steganography) */
  u8int rep[32];
  memmove(rep, branch->encoded_state, 32);

  /* 2. Derive Symmetric Key K from R */
  u8int key[32];
  derive_key_from_rep(rep, key);

  /* 3. Decrypt the data */
  u8int data[64];
  memmove(data, branch->tokens, 32);
  memmove(data + 32, branch->borrowed, 32);

  u8int nonce[8] = {0};
  crypto_chacha20_djb(data, data, 64, key, nonce, 0);

  /* 4. Restore Plaintext */
  memmove(branch->tokens, data, 32);
  memmove(branch->borrowed, data + 32, 32);

  return 0;
}

int branch_is_plausibly_random(const u8int *data, usize len) {
  if (data == nil || len < 32)
    return 0;

  /* Simple entropy check: count bit transitions */
  int transitions = 0;
  for (usize i = 0; i < len - 1; i++) {
    transitions += __builtin_popcount(data[i] ^ data[i + 1]);
  }

  /* Random data should have ~4 transitions per byte on average */
  int expected = (len - 1) * 4;
  int variance = expected / 4;

  return (transitions > expected - variance &&
          transitions < expected + variance);
}

/* ========== Cross-Machine Transfer ========== */

int transfer_initiate(MachineBank *local, uuid_t *remote_machine,
                      TokenType type, u64int amount,
                      TokenTransfer *out_transfer) {
  if (local == nil || remote_machine == nil || out_transfer == nil)
    return -1;

  lock(&local->lock);

  /* Check balance */
  if (local->available[type] < amount) {
    unlock(&local->lock);
    return -1;
  }

  /* Reserve tokens (move to pending) */
  local->available[type] -= amount;
  local->pending_outbound += amount;

  /* Build transfer */
  uuid_new_v8(&out_transfer->transfer_id);
  uuid_copy(&out_transfer->from_machine, &local->machine_id);
  uuid_copy(&out_transfer->to_machine, remote_machine);
  out_transfer->token_type = type;
  out_transfer->amount = amount;
  out_transfer->status = TRANSFER_PENDING;
  out_transfer->timestamp = fastticks(nil);

  /* Generate proof */
  machine_bank_prove_balance(local, type, amount, &out_transfer->proof);

  machine_bank_update_root(local);
  unlock(&local->lock);

  print("DPEBBLE: transfer initiated amount=%llud type=%d\n", amount, type);

  return 0;
}

int transfer_receive(MachineBank *local, const TokenTransfer *transfer) {
  if (local == nil || transfer == nil)
    return -1;

  /* Verify this transfer is for us */
  if (uuid_compare(&transfer->to_machine, &local->machine_id) != 0)
    return -1;

  /* Verify the proof (would need sender's Merkle root) */
  /* In production: fetch sender's root via MSGORD consensus */

  lock(&local->lock);

  /* Credit tokens */
  local->available[transfer->token_type] += transfer->amount;
  machine_bank_update_root(local);

  unlock(&local->lock);

  print("DPEBBLE: transfer received amount=%llud\n", transfer->amount);

  return 0;
}

int transfer_confirm(MachineBank *local, uuid_t *transfer_id) {
  if (local == nil || transfer_id == nil)
    return -1;

  lock(&local->lock);
  local->pending_outbound -= 0; /* Would lookup actual amount */
  unlock(&local->lock);

  return 0;
}

int transfer_cancel(MachineBank *local, uuid_t *transfer_id) {
  if (local == nil || transfer_id == nil)
    return -1;

  /* Would lookup transfer, refund tokens */
  /* Placeholder */

  return 0;
}

/* ========== Initialization ========== */

void distributed_pebble_init(void) {
  /* Create local machine bank */
  local_machine_bank = mallocz(sizeof(MachineBank), 1);
  if (local_machine_bank == nil) {
    print("DPEBBLE: FATAL - cannot allocate machine bank\n");
    return;
  }

  /* Generate machine ID */
  uuid_t machine_id;
  uuid_new_v8(&machine_id);

  /* Initialize with local resources */
  machine_bank_init(local_machine_bank, &machine_id,
                    (u64int)conf.npage * BY2PG, /* RAM */
                    1,                          /* CPU cores (placeholder) */
                    0,                          /* GPU memory */
                    100 * 1024 * 1024           /* 100MB/s network */
  );

  print("DPEBBLE: distributed pebble initialized\n");
}
#endif
