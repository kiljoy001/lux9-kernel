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

  print("DPEBBLE: bank init machine=%H mem=%llu cpu=%llu gpu=%llu net=%llu "
        "tokens\n",
        machine_id->data, bank->total_tokens[TOK_MEMORY],
        bank->total_tokens[TOK_CPU], bank->total_tokens[TOK_GPU],
        bank->total_tokens[TOK_NETWORK]);
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
  BlindLedgerHash computed = proof->balance_proof.target_hash;

  for (u32int i = 0; i < proof->balance_proof.depth; i++) {
    u8int combined[BLIND_LEDGER_CAP_SIZE * 2];
    int is_right = (proof->balance_proof.path_bits >> i) & 1;

    if (is_right) {
      memmove(combined, &proof->balance_proof.siblings[i],
              BLIND_LEDGER_CAP_SIZE);
      memmove(combined + BLIND_LEDGER_CAP_SIZE, &computed,
              BLIND_LEDGER_CAP_SIZE);
    } else {
      memmove(combined, &computed, BLIND_LEDGER_CAP_SIZE);
      memmove(combined + BLIND_LEDGER_CAP_SIZE,
              &proof->balance_proof.siblings[i], BLIND_LEDGER_CAP_SIZE);
    }

    crypto_blake2b((u8int *)&computed, BLIND_LEDGER_CAP_SIZE, combined,
                   sizeof(combined));
  }

  return memcmp(&computed, expected_root, BLIND_LEDGER_CAP_SIZE) == 0;
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

  print("DPEBBLE: branch created id=%H flags=%x\n", branch->branch_id.data,
        flags);
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

int branch_elligator_encode(ArenaBranch *branch) {
  if (branch == nil || !(branch->flags & BRANCH_SECRET))
    return -1;

  /* Create state blob to encode */
  u8int state[sizeof(branch->tokens) + sizeof(branch->borrowed)];
  memmove(state, branch->tokens, sizeof(branch->tokens));
  memmove(state + sizeof(branch->tokens), branch->borrowed,
          sizeof(branch->borrowed));

  /* Use Elligator to map state to curve point that looks random */
  /* For now, use keyed hash as placeholder (real Elligator in monocypher) */
  crypto_blake2b_keyed(branch->encoded_state, BLIND_LEDGER_CAP_SIZE,
                       branch->elligator_secret, 32, state, sizeof(state));

  return 0;
}

int branch_elligator_decode(ArenaBranch *branch, const u8int *elligator_key) {
  if (branch == nil || elligator_key == nil)
    return -1;

  /* Verify key matches */
  if (memcmp(branch->elligator_secret, elligator_key, 32) != 0)
    return -1;

  /* Branch is already decoded in memory; this just validates key */
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

  print("DPEBBLE: transfer initiated id=%H amount=%llu type=%d\n",
        out_transfer->transfer_id.data, amount, type);

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

  print("DPEBBLE: transfer received id=%H amount=%llu\n",
        transfer->transfer_id.data, transfer->amount);

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
  extern ulong physmem; /* From kernel memory init */

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
  /* CPU tokens: estimate from nelem(conf.mem) or similar */
  machine_bank_init(local_machine_bank, &machine_id, physmem, /* RAM */
                    1,                /* CPU cores (placeholder) */
                    0,                /* GPU memory */
                    100 * 1024 * 1024 /* 100MB/s network */
  );

  print("DPEBBLE: distributed pebble initialized\n");
}
