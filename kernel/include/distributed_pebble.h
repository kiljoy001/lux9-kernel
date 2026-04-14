/* distributed_pebble.h - Cross-Machine Token Economy
 *
 * Extends Pebble tokens across a cluster for CPU/memory/GPU sharing.
 * Features:
 *   - MachineBank: Per-machine token pool with Merkle proof state
 *   - ArenaBranch: Local token cache for lock-free allocation
 *   - SecretBranch: Elligator-encoded deniable branches
 *   - Token types: Memory, CPU, GPU, Network
 *   - 9P extensions: Ttoken/Rtoken for cross-machine transfer
 *   - MSGORD integration: Global ordering prevents double-spend
 *
 * Conservation law: Σ all machines' tokens = GLOBAL_CONSTANT
 */

#ifndef _DISTRIBUTED_PEBBLE_H_
#define _DISTRIBUTED_PEBBLE_H_

#include "blind_ledger.h"
#include "pebble.h"
#include "u.h"
#include "uuid.h"

/* ========== Token Types ========== */

typedef enum {
  TOK_MEMORY = 0,  /* 8 bytes per token */
  TOK_CPU = 1,     /* 1 millisecond per token */
  TOK_GPU = 2,     /* 1 millisecond per token */
  TOK_NETWORK = 3, /* 1 KB per token */
  TOK_MAX = 4
} TokenType;

/* Token unit conversions */
#define TOK_MEMORY_BYTES 8     /* 1 token = 8 bytes */
#define TOK_CPU_USEC 1000      /* 1 token = 1ms = 1000us */
#define TOK_GPU_USEC 1000      /* 1 token = 1ms */
#define TOK_NETWORK_BYTES 1024 /* 1 token = 1KB */

/* ========== Branch Flags ========== */

typedef enum {
  BRANCH_PUBLIC = 0,          /* Normal auditable branch */
  BRANCH_SECRET = (1 << 0),   /* Elligator-encoded, deniable */
  BRANCH_REMOTE = (1 << 1),   /* Backed by remote machine */
  BRANCH_BORROWED = (1 << 2), /* Contains borrowed tokens */
} BranchFlags;

/* ========== Arena Branch (Local Token Cache) ========== */
/*
 * Each arena/WASM module can have a branch for lock-free allocation.
 * Branches hold colorless tokens and reconcile with parent bank.
 *
 * Secret branches are Elligator-encoded:
 *   - Ledger entry looks like random noise
 *   - Only holder of branch secret can decode
 *   - Plausibly deniable under coercion
 *
 * DENIABILITY MODEL:
 *   - DETECTABLE: Token conservation reveals "X tokens are somewhere"
 *     (the count gap is visible - you can't hide token quantity)
 *   - UNLOCATABLE: Branch data is indistinguishable from random noise
 *     (you can't prove WHERE the tokens are or WHAT they're for)
 *
 *   Under coercion: "Some tokens are unaccounted... in transit? lost?"
 *   Adversary knows tokens exist, cannot prove hidden branch.
 */
typedef struct ArenaBranch {
  uuid_t branch_id; /* Unique branch identity */
  u32int flags;     /* BranchFlags */

  /* Token pools (per type) */
  u64int tokens[TOK_MAX];   /* Available tokens by type */
  u64int borrowed[TOK_MAX]; /* Amount borrowed from parent */

  /* Reconciliation thresholds */
  u64int low_water[TOK_MAX];  /* Refill when below */
  u64int high_water[TOK_MAX]; /* Return excess when above */

  /* Parent linkage */
  struct ArenaBranch *parent;  /* Parent branch (or NULL if root) */
  struct MachineBank *machine; /* Owning machine bank */

  /* Secret branch state (if BRANCH_SECRET) */
  u8int elligator_secret[32];    /* Elligator encoding key */
  BlindLedgerHash encoded_state; /* Looks like random noise */

  /* Statistics */
  u64int alloc_count;
  u64int refill_count;
  u64int reconcile_count;

  /* Lock for this branch (per-branch, not global) */
  Lock lock;
} ArenaBranch;

/* ========== Machine Bank ========== */
/*
 * Each machine has a bank sized to its physical resources.
 * Maintains Merkle tree of all local branches for audit proofs.
 */

#define MAX_BRANCHES_PER_MACHINE 256
#define MERKLE_MAX_DEPTH 16

typedef struct MachineBank {
  uuid_t machine_id; /* Unique machine identity */

  /* Physical resource limits */
  u64int total_tokens[TOK_MAX]; /* Max tokens by type */
  u64int available[TOK_MAX];    /* Currently available */

  /* Branch management */
  ArenaBranch *branches[MAX_BRANCHES_PER_MACHINE];
  u32int branch_count;

  /* Merkle state for proofs */
  BlindLedgerHash bank_root; /* Root of token Merkle tree */
  u64int epoch;              /* Monotonic revision counter */

  /* Cross-machine state */
  u64int pending_outbound; /* Tokens in transit out */
  u64int pending_inbound;  /* Tokens awaiting confirmation */

  /* MSGORD integration */
  u64int last_msgord_seq; /* Last processed MSGORD sequence */

  Lock lock;
} MachineBank;

/* ========== Merkle Proof ========== */

typedef struct MerkleProof {
  BlindLedgerHash target_hash;                /* Hash being proven */
  u32int depth;                               /* Proof depth */
  BlindLedgerHash siblings[MERKLE_MAX_DEPTH]; /* Sibling hashes */
  u32int path_bits; /* Left(0)/Right(1) at each level */
} MerkleProof;

/* Token transfer with proof */
typedef struct TokenProof {
  uuid_t machine_id;         /* Source machine */
  TokenType token_type;      /* Memory, CPU, GPU, Network */
  u64int amount;             /* Tokens being proven/transferred */
  u64int epoch;              /* State epoch (for freshness) */
  MerkleProof balance_proof; /* Proof of sufficient balance */
} TokenProof;

/* ========== Cross-Machine Transfer ========== */

typedef enum {
  TRANSFER_PENDING = 0,
  TRANSFER_CONFIRMED = 1,
  TRANSFER_REJECTED = 2,
  TRANSFER_TIMEOUT = 3,
} TransferStatus;

typedef struct TokenTransfer {
  uuid_t transfer_id;  /* Unique transfer ID */
  uuid_t from_machine; /* Source machine */
  uuid_t to_machine;   /* Destination machine */
  TokenType token_type;
  u64int amount;
  TokenProof proof; /* Sender's balance proof */

  /* MSGORD ordering */
  u64int msgord_seq; /* Global sequence number */

  /* Status */
  TransferStatus status;
  u64int timestamp; /* Request time */
} TokenTransfer;

/* ========== API: Machine Bank ========== */

/* Initialize machine bank based on physical resources */
void machine_bank_init(MachineBank *bank, uuid_t *machine_id, u64int ram_bytes,
                       u64int cpu_count, u64int gpu_mem_bytes,
                       u64int net_bandwidth);

/* Get current Merkle root for audit */
void machine_bank_get_root(MachineBank *bank, BlindLedgerHash *out_root);

/* Generate proof of token balance */
int machine_bank_prove_balance(MachineBank *bank, TokenType type, u64int amount,
                               TokenProof *out_proof);

/* Verify a balance proof from another machine */
int machine_bank_verify_proof(const TokenProof *proof,
                              const BlindLedgerHash *expected_root);

/* ========== API: Arena Branch ========== */

/* Create a new branch (public or secret) */
ArenaBranch *branch_create(MachineBank *bank, u32int flags);

/* Create secret (deniable) branch */
ArenaBranch *branch_create_secret(MachineBank *bank,
                                  const u8int *elligator_key);

/* Allocate tokens from branch (lock-free fast path) */
int branch_alloc(ArenaBranch *branch, TokenType type, u64int amount);

/* Free tokens back to branch */
void branch_free(ArenaBranch *branch, TokenType type, u64int amount);

/* Reconcile branch with parent bank */
int branch_reconcile(ArenaBranch *branch);

/* Destroy branch, return all tokens to parent */
void branch_destroy(ArenaBranch *branch);

/* ========== API: Cross-Machine Transfer ========== */

/* Initiate token transfer to remote machine */
int transfer_initiate(MachineBank *local, uuid_t *remote_machine,
                      TokenType type, u64int amount,
                      TokenTransfer *out_transfer);

/* Process incoming transfer (called by 9P handler) */
int transfer_receive(MachineBank *local, const TokenTransfer *transfer);

/* Confirm transfer completion (after MSGORD ordering) */
int transfer_confirm(MachineBank *local, uuid_t *transfer_id);

/* Cancel pending transfer, refund tokens */
int transfer_cancel(MachineBank *local, uuid_t *transfer_id);

/* ========== API: Secret Branch (Elligator) ========== */

/* Encode branch state so it looks like random noise */
int branch_elligator_encode(ArenaBranch *branch);

/* Decode branch state (requires elligator_secret) */
int branch_elligator_decode(ArenaBranch *branch, const u8int *elligator_key);

/* Check if data could be a secret branch (probabilistic) */
int branch_is_plausibly_random(const u8int *data, usize len);

/* ========== 9P Token Messages ========== */
/*
 * New 9P message types for token operations.
 * These extend the standard 9P2000 protocol.
 */

#define Ttoken 80  /* Transfer tokens */
#define Rtoken 81  /* Token transfer response */
#define Tbudget 82 /* Query remote budget */
#define Rbudget 83 /* Budget query response */

/* Ttoken message structure */
typedef struct TtokenMsg {
  u16int tag;
  TokenType token_type;
  u64int amount;
  TokenProof proof;
} TtokenMsg;

/* Rtoken message structure */
typedef struct RtokenMsg {
  u16int tag;
  uuid_t transfer_id;
  u64int new_balance;       /* Receiver's new balance */
  BlindLedgerHash new_root; /* Updated Merkle root */
} RtokenMsg;

/* ========== Global State ========== */

/* The local machine's bank (initialized at boot) */
extern MachineBank *local_machine_bank;

/* Initialize distributed pebble subsystem */
void distributed_pebble_init(void);

#endif /* _DISTRIBUTED_PEBBLE_H_ */
