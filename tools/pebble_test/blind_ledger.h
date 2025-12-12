/*
 * Blind Ledger - Zero-Knowledge Addressing System
 *
 * Implements the "Identity is Security" paradigm.
 *
 * - UserCapability: The opaque handle held by userspace.
 * - BlindLedgerEntry: The kernel's secret mapping table.
 *
 * The "Tension Resolution":
 * - We do NOT track every 8-byte Peg with a hash.
 * - We track "Spans" (Ranges of Pegs) created during Black Token allocation.
 * - White Tokens are fungible budget; Black Tokens have Identity.
 *
 * Circular Economy:
 * - Banked tokens (Free Pegs) are Colorless.
 * - Issued Tokens (White) are Fungible Budget.
 * - Used Tokens (Black) are Reified Identity (Ledger Entry).
 * - Freed Tokens return to the Bank as Colorless Pegs.
 */

#ifndef _BLIND_LEDGER_H_
#define _BLIND_LEDGER_H_

#include "rbtree.h"
#include "u.h"

struct Proc;
typedef struct Proc Proc;

#define BLIND_LEDGER_SECRET_SIZE 32
#define BLIND_LEDGER_CAP_SIZE 32    // For BLAKE2b_256 (256 bits = 32 bytes)
#define BLIND_LEDGER_SECRET_SIZE 32 // Cryptographically strong secret size

/*
 * The Public Handle (User Space View)
 *
 * Users never see physical addresses. They hold this struct.
 * Access is granted if they can present this struct, and
 * Hash(Entry.Secret | Entry.PhysAddr) == Capability.Hash.
 */
typedef struct UserCapability {
  u8int hash[BLIND_LEDGER_CAP_SIZE]; /* The cryptographic proof of ownership */
  u64int size;                       /* Size of the object (Span) in bytes */
  u32int type;                       /* Resource Type (Memory, Channel, PCI) */
  u32int perms; /* Permissions (Read, Write, Execute, Transfer) */
} UserCapability;

/* Capability Resource Types */
enum {
  CAP_TYPE_MEMORY = 1,
  CAP_TYPE_CHANNEL = 2,
  CAP_TYPE_DEVICE = 3,
  CAP_TYPE_IPC = 4,
};

/* Capability Permissions */
enum {
  CAP_PERM_READ = 1 << 0,
  CAP_PERM_WRITE = 1 << 1,
  CAP_PERM_EXEC = 1 << 2,
  CAP_PERM_TRANSFER = 1 << 3, /* Can Mint/Send to others */
  CAP_PERM_GRANT = 1 << 4,    /* Can create sub-capabilities */
};

// States for a BlindLedgerEntry
typedef enum BlindLedgerState {
  BLIND_LEDGER_STATE_INACTIVE = 0, // Not yet active or invalid
  BLIND_LEDGER_STATE_ACTIVE = 1,   // Currently active and valid
  BLIND_LEDGER_STATE_BURNED = 2,   // Burned, no longer valid, awaiting cleanup
  BLIND_LEDGER_STATE_COW_RED = 3,  // Copy-on-Write: shared, read-only
  BLIND_LEDGER_STATE_COW_BLUE = 4, // Copy-on-Write: private, writable copy
} BlindLedgerState;

// Type for internal hashes within the ledger (e.g., leaf_hash, process_hash)
typedef u8int BlindLedgerHash[BLIND_LEDGER_CAP_SIZE];

/*
 * The Private Record (Kernel View)
 *
 * Stored in a Kernel-only Hash Map (The Ledger).
 * Lookup key is the UserCapability.hash.
 */
typedef struct BlindLedgerEntry {
  UserCapability capability; // The full capability associated with this entry
  uintptr physical_address;  /* The concrete resource (Start of Span) */
  Proc *owner;               /* The current authoritative owner process */
  u8int secret[BLIND_LEDGER_SECRET_SIZE]; /* Cryptographic secret for capability
                                             derivation */
  u64int epoch;              /* Allocation Cycle (Prevents Use-After-Free) */
  u64int span_len;           /* Length in bytes (must match Capability.size) */
  u32int permissions;        /* Current effective permissions */
  BlindLedgerState state;    /* Current state of the entry */
  BlindLedgerHash leaf_hash; /* Immutable hash of physical properties */
  BlindLedgerHash process_hash; /* Mutable hash of dynamic properties (owner,
                                   perms, state) */

  // Derivation Chain Support
  BlindLedgerHash parent_hash;    /* Parent capability hash (0 if root) */
  BlindLedgerHash derivation_sig; /* HMAC(key, parent || constraints || self) */
} BlindLedgerEntry;

// Error codes for Blind Ledger operations
typedef enum BlindLedgerError {
  BLIND_LEDGER_OK = 0,
  BLIND_LEDGER_EINVAL = 1,    // Invalid arguments
  BLIND_LEDGER_ENOMEM = 2,    // Out of memory
  BLIND_LEDGER_EPERM = 3,     // Permission denied (e.g., not owner)
  BLIND_LEDGER_ENOTFOUND = 4, // Capability not found
  BLIND_LEDGER_EEXPIRED = 5,  // Capability found but not active/expired
  BLIND_LEDGER_EFAULT = 6,    // General internal fault
  BLIND_LEDGER_EBUSY = 7,     // Resource is busy, cannot perform operation
} BlindLedgerError;

// Function prototypes
void blind_ledger_init(void);
BlindLedgerError ledger_mint(UserCapability *out_cap, uintptr pa, ulong len,
                             Proc *owner, u32int permissions,
                             const u8int *vault_secret);
BlindLedgerError ledger_verify(const UserCapability *cap,
                               BlindLedgerEntry *out_entry);
BlindLedgerError ledger_transfer(const UserCapability *cap, Proc *from_owner,
                                 Proc *to_owner);
BlindLedgerError ledger_burn(const UserCapability *cap, Proc *owner);
BlindLedgerError ledger_lookup_by_pa_and_owner(uintptr pa, Proc *owner,
                                               UserCapability *out_cap,
                                               BlindLedgerEntry *out_entry);
void blind_ledger_update_merkle_root(void);
const u8int *blind_ledger_get_merkle_root(void);

// Epoch management for preventing use-after-free
u64int ledger_get_current_epoch(void);
void ledger_advance_epoch(void);

// Vault secret generation (TPM-backed)
BlindLedgerError ledger_generate_secret(u8int *secret_out);
BlindLedgerError ledger_destroy_secret(const u8int *secret);

// Atomic rollback support for exchange operations
typedef struct LedgerRollbackToken {
  UserCapability
      original_capability; // Original capability hash before transfer
  Proc *original_owner;    // Original owner before transfer
  BlindLedgerHash
      original_process_hash; // Original process_hash before transfer
  u64int is_valid;           // Magic value to verify token validity
} LedgerRollbackToken;

#define ROLLBACK_TOKEN_MAGIC 0x524F4C4C4241434BULL // "ROLLBACK" in hex

BlindLedgerError
ledger_transfer_reversible(const UserCapability *cap, Proc *from_owner,
                           Proc *to_owner, LedgerRollbackToken *rollback_token);
// Statistics
typedef struct BlindLedgerStats {
  u64int active_entries;
  u64int burned_entries;
  u64int total_memory_tracked;
  u64int tree_depth;
  u64int epoch;
} BlindLedgerStats;

BlindLedgerError blind_ledger_get_stats(BlindLedgerStats *stats);

// Attestation
BlindLedgerError blind_ledger_attest_root(u8int *out_signature,
                                          u32int *out_len);

// =========================================================================
// Derivation Chain Proofs
// =========================================================================

#define MAX_DERIVATION_DEPTH 16

typedef struct DerivationStep {
  BlindLedgerHash parent_hash;
  BlindLedgerHash derivation_sig;
  u32int constraints;
} DerivationStep;

typedef struct DerivationProof {
  BlindLedgerHash target_hash;
  u32int chain_length;
  DerivationStep chain[MAX_DERIVATION_DEPTH];
} DerivationProof;

BlindLedgerError ledger_derive(const UserCapability *parent_cap, Proc *owner,
                               u32int child_constraints,
                               UserCapability *out_child_cap);

BlindLedgerError ledger_get_derivation_proof(const UserCapability *cap,
                                             Proc *owner,
                                             DerivationProof *out_proof);

BlindLedgerError ledger_verify_derivation_proof(const DerivationProof *proof);

#endif /* BLIND_LEDGER_H */