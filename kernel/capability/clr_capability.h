/* clr_capability.h - Capability-Based Security for CLR Runtime
 *
 * Implements monotonic capability derivation with per-module and per-class
 * granularity. Properties proven in proofs/capability/*.v:
 *   - PermsBitmask.v: Permission bitmask operations
 *   - CapabilityModel.v: Core capability structure and derivation
 *   - DerivationChain.v: Transitive chain properties
 *   - LedgerInvariants.v: Uniqueness and preservation invariants
 *
 * Security model: Capabilities can only be derived with FEWER permissions
 * (monotonic decrease). This is enforced at derivation time and proven correct
 * in the Coq formalization.
 */

#ifndef CLR_CAPABILITY_H
#define CLR_CAPABILITY_H

#include "../include/uuid.h"

/* Plan 9 types if not in kernel context */
#ifndef _U_H_
typedef unsigned char u8int;
typedef unsigned short u16int;
typedef unsigned int u32int;
typedef unsigned long long u64int;
#endif

/* ========== Permission Bits (matching proofs/capability/PermsBitmask.v)
 * ========== */
/*
 * @coq_proof: proofs/capability/PermsBitmask.v
 * @definition: perm_read, perm_write, perm_exec, perm_transfer, perm_grant
 *
 * Permission bits form a lattice under subset ordering.
 * @theorem: perms_subset_refl, perms_subset_trans
 *
 * In kernel mode, we use the enum values from blind_ledger.h.
 * In userspace test mode, we define our own macros.
 */
#ifdef USERSPACE_TEST
#define CAP_PERM_READ 0x01     /* Read access to resource */
#define CAP_PERM_WRITE 0x02    /* Write/modify access */
#define CAP_PERM_EXEC 0x04     /* Execute/invoke access */
#define CAP_PERM_TRANSFER 0x08 /* Transfer capability to other entity */
#define CAP_PERM_GRANT 0x10    /* Grant derived capabilities */

#define CAP_PERM_ALL                                                           \
  (CAP_PERM_READ | CAP_PERM_WRITE | CAP_PERM_EXEC | CAP_PERM_TRANSFER |        \
   CAP_PERM_GRANT)
#define CAP_PERM_NONE 0x00
#else
/* Kernel mode: use values from blind_ledger.h (included via pebble.h) */
#include "../include/blind_ledger.h"
#define CAP_PERM_ALL                                                           \
  (CAP_PERM_READ | CAP_PERM_WRITE | CAP_PERM_EXEC | CAP_PERM_TRANSFER |        \
   CAP_PERM_GRANT)
#define CAP_PERM_NONE 0x00
#endif

/* ========== Capability Scope ========== */
typedef enum {
  CAP_SCOPE_MODULE, /* Assembly-level capability (root for assembly) */
  CAP_SCOPE_CLASS,  /* Class-level capability (derived from module) */
  CAP_SCOPE_METHOD  /* Method-level capability (derived from class) */
} capability_scope_t;

/* ========== Monotonic Capability Structure ========== */
/*
 * @coq_proof: proofs/capability/CapabilityModel.v
 * @record: Capability (cap_id, cap_perms, cap_parent)
 *
 * ACSL contracts derived from:
 *   @theorem: derived_perm_monotonic
 *     derived_from ct c p -> perms_subset (cap_perms c) (cap_perms p)
 */
typedef struct clr_monotonic_capability {
  /* Identity */
  uuid_t uuid;   /* Unique capability identifier (UUIDv8) */
  u32int cap_id; /* Numeric ID for fast lookup */

  /* Derivation chain */
  uuid_t parent_uuid;      /* Parent capability UUID (null if root) */
  u32int parent_id;        /* Parent numeric ID (0 if root) */
  u32int derivation_depth; /* Chain depth (0 = root, increases on derive) */

  /* Permissions (monotonic: can only decrease) */
  u32int permissions;     /* Current active permissions */
  u32int max_permissions; /* Original immutable max (set at creation) */

  /* Timing (monotonic counters) */
  u64int creation_time;   /* Monotonic counter at creation */
  u64int expiration_time; /* Optional expiration (0 = no expiry) */

  /* Scope and metadata */
  capability_scope_t scope; /* MODULE, CLASS, or METHOD */
  char *bound_metadata;     /* Immutable binding: "assembly:class:method" */
  u8int is_validated;       /* Has chain been validated? */
  u8int is_revoked;         /* Revocation flag */
} clr_monotonic_capability_t;

/* ========== Capability Manager ========== */
/*
 * @coq_proof: proofs/capability/LedgerInvariants.v
 * @definition: cap_table
 * @theorem: find_cap_unique
 *
 * The manager maintains a ledger of all capabilities with invariants:
 *   - All cap_id values are unique
 *   - Parent references point to existing capabilities
 *   - Permission chains are monotonically decreasing
 */
#define CAP_TABLE_INITIAL_SIZE 64
#define CAP_TABLE_MAX_SIZE 4096

typedef struct capability_manager {
  /* Capability storage */
  clr_monotonic_capability_t *capabilities; /* Dynamic array */
  u32int count;                             /* Number of capabilities */
  u32int capacity;                          /* Allocated slots */

  /* Monotonic counters */
  u64int monotonic_time; /* Global monotonic timestamp */
  u32int next_cap_id;    /* Next capability ID to assign */

  /* Statistics */
  u32int derivations; /* Total derivations performed */
  u32int validations; /* Total chain validations */
  u32int rejections;  /* Failed derivations (permission violation) */
} capability_manager_t;

/* ========== Core API ========== */

/*
 * @requires: manager != NULL
 * @ensures: \result != NULL ==> manager->count == 0
 */
capability_manager_t *cap_manager_create(void);

/*
 * @requires: manager != NULL
 * @ensures: all capabilities freed
 */
void cap_manager_destroy(capability_manager_t *manager);

/*
 * Create a module-level (root) capability for an assembly.
 *
 * @coq_proof: proofs/capability/CapabilityModel.v
 * @requires: manager != NULL && name != NULL
 * @ensures: \result != NULL ==>
 *           \result->scope == CAP_SCOPE_MODULE &&
 *           \result->parent_id == 0 &&
 *           \result->permissions == CAP_PERM_ALL
 */
clr_monotonic_capability_t *cap_create_module(capability_manager_t *manager,
                                              const char *assembly_name);

/*
 * Derive a class-level capability from a module capability.
 *
 * @coq_proof: proofs/capability/DerivationChain.v
 * @theorem: derived_perm_monotonic
 *
 * @requires: manager != NULL && parent != NULL && class_name != NULL
 * @requires: (permission_mask & parent->permissions) == permission_mask
 *            // Requested permissions must be subset of parent
 * @ensures: \result != NULL ==>
 *           \result->scope == CAP_SCOPE_CLASS &&
 *           \result->parent_id == parent->cap_id &&
 *           \result->permissions == permission_mask &&
 *           \result->derivation_depth == parent->derivation_depth + 1
 */
clr_monotonic_capability_t *cap_derive_class(capability_manager_t *manager,
                                             clr_monotonic_capability_t *parent,
                                             const char *class_name,
                                             u32int permission_mask);

/*
 * Validate a capability derivation chain.
 *
 * @coq_proof: proofs/capability/ChainDecidability.v
 * @theorem: derived_chain_table_perms_monotonic
 *
 * @requires: manager != NULL && child != NULL
 * @ensures: \result == 1 ==>
 *           for all ancestors: perms_subset(child->perms, ancestor->perms)
 */
int cap_validate_chain(capability_manager_t *manager,
                       clr_monotonic_capability_t *child);

/*
 * Check if a capability has a specific permission.
 *
 * @coq_proof: proofs/capability/PermsBitmask.v
 * @definition: has_perm
 *
 * @requires: cap != NULL
 * @ensures: \result == ((cap->permissions & required) == required)
 */
int cap_check_permission(clr_monotonic_capability_t *cap, u32int required);

/*
 * Find capability by UUID.
 *
 * @coq_proof: proofs/capability/LedgerInvariants.v
 * @theorem: find_cap_unique
 *
 * @requires: manager != NULL
 * @ensures: \result != NULL ==> uuid_compare(&\result->uuid, uuid) == 0
 */
clr_monotonic_capability_t *cap_find_by_uuid(capability_manager_t *manager,
                                             const uuid_t *uuid);

/*
 * Find capability by numeric ID.
 *
 * @requires: manager != NULL
 * @ensures: \result != NULL ==> \result->cap_id == cap_id
 */
clr_monotonic_capability_t *cap_find_by_id(capability_manager_t *manager,
                                           u32int cap_id);

/* ========== Permission Utilities ========== */

/*
 * Check if permissions a are a subset of permissions b.
 *
 * @coq_proof: proofs/capability/PermsBitmask.v
 * @definition: perms_subset
 *
 * @ensures: \result == 1 <==> (a & b) == a
 */
int cap_perms_subset(u32int a, u32int b);

/*
 * Format permissions as human-readable string.
 * @ensures: writes at most buflen-1 chars to buf
 */
void cap_perms_to_string(u32int perms, char *buf, u32int buflen);

/* ========== Debug/Diagnostics ========== */

/*
 * Dump capability details to kernel console.
 */
void cap_dump(clr_monotonic_capability_t *cap);

/*
 * Dump manager statistics to kernel console.
 */
void cap_manager_dump_stats(capability_manager_t *manager);

#endif /* CLR_CAPABILITY_H */
