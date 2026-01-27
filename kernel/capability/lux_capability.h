/* lux_capability.h - Capability-Based Security for Lux9 Runtime
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

#ifndef LUX_CAPABILITY_H
#define LUX_CAPABILITY_H

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
#define LUX_CAP_PERM_READ 0x01     /* Read access to resource */
#define LUX_CAP_PERM_WRITE 0x02    /* Write/modify access */
#define LUX_CAP_PERM_EXEC 0x04     /* Execute/invoke access */
#define LUX_CAP_PERM_TRANSFER 0x08 /* Transfer capability to other entity */
#define LUX_CAP_PERM_GRANT 0x10    /* Grant derived capabilities */

#define LUX_CAP_PERM_ALL                                                       \
  (LUX_CAP_PERM_READ | LUX_CAP_PERM_WRITE | LUX_CAP_PERM_EXEC |                \
   LUX_CAP_PERM_TRANSFER | LUX_CAP_PERM_GRANT)
#define LUX_CAP_PERM_NONE 0x00
#else
/* Kernel mode: use values from blind_ledger.h (included via pebble.h) */
#include "../include/blind_ledger.h"
/* Mapping blind_ledger names to LUX_CAP names for consistency */
#define LUX_CAP_PERM_READ CAP_PERM_READ
#define LUX_CAP_PERM_WRITE CAP_PERM_WRITE
#define LUX_CAP_PERM_EXEC CAP_PERM_EXEC
#define LUX_CAP_PERM_TRANSFER CAP_PERM_TRANSFER
#define LUX_CAP_PERM_GRANT CAP_PERM_GRANT

#define LUX_CAP_PERM_ALL                                                       \
  (CAP_PERM_READ | CAP_PERM_WRITE | CAP_PERM_EXEC | CAP_PERM_TRANSFER |        \
   CAP_PERM_GRANT)
#define LUX_CAP_PERM_NONE 0x00
#endif

/* ========== Capability Scope ========== */
typedef enum {
  LUX_CAP_SCOPE_MODULE, /* Assembly-level capability (root for assembly) */
  LUX_CAP_SCOPE_CLASS,  /* Class-level capability (derived from module) */
  LUX_CAP_SCOPE_METHOD  /* Method-level capability (derived from class) */
} lux_capability_scope_t;

/* ========== Monotonic Capability Structure ========== */
/*
 * @coq_proof: proofs/capability/CapabilityModel.v
 * @record: Capability (cap_id, cap_perms, cap_parent)
 *
 * ACSL contracts derived from:
 *   @theorem: derived_perm_monotonic
 *     derived_from ct c p -> perms_subset (cap_perms c) (cap_perms p)
 */
typedef struct lux_capability {
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
  lux_capability_scope_t scope; /* MODULE, CLASS, or METHOD */
  char *bound_metadata;         /* Immutable binding: "assembly:class:method" */
  u8int is_validated;           /* Has chain been validated? */
  u8int is_revoked;             /* Revocation flag */
  void *aux;                    /* Auxiliary data (e.g., Fruity IR object) */
} lux_capability_t;

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
#define LUX_CAP_TABLE_INITIAL_SIZE 64
#define LUX_CAP_TABLE_MAX_SIZE 4096

typedef struct lux_capability_manager {
  /* Capability storage */
  lux_capability_t *capabilities; /* Dynamic array */
  u32int count;                   /* Number of capabilities */
  u32int capacity;                /* Allocated slots */

  /* Monotonic counters */
  u64int monotonic_time; /* Global monotonic timestamp */
  u32int next_cap_id;    /* Next capability ID to assign */

  /* Statistics */
  u32int derivations; /* Total derivations performed */
  u32int validations; /* Total chain validations */
  u32int rejections;  /* Failed derivations (permission violation) */
} lux_capability_manager_t;

/* ========== Core API ========== */

/*@
  @ requires \true;
  @ assigns \result \from \nothing;
  @ ensures \result == \null || \result->count == 0;
  @*/
lux_capability_manager_t *lux_cap_manager_create(void);

/*@
  @ requires manager != \null;
  @ assigns manager->capabilities, manager->count \from manager->capabilities,
  manager->count;
  @ ensures \true;
  @*/
void lux_cap_manager_destroy(lux_capability_manager_t *manager);

/*@
  @ requires manager != \null;
  @ requires assembly_name != \null && \valid_read(assembly_name);
  @ assigns manager->capabilities, manager->count \from manager->capabilities,
  manager->count, assembly_name;
  @ ensures \result != \null ==> (
  @   \result->scope == LUX_CAP_SCOPE_MODULE &&
  @   \result->parent_id == 0 &&
  @   \result->permissions == LUX_CAP_PERM_ALL
  @ );
  @*/
lux_capability_t *lux_cap_create_module(lux_capability_manager_t *manager,
                                        const char *assembly_name);

/*@
  @ requires manager != \null;
  @ requires parent != \null && \valid(parent);
  @ requires class_name != \null && \valid_read(class_name);
  @ requires (permission_mask & parent->permissions) == permission_mask;
  @ assigns manager->capabilities, manager->count \from manager->capabilities,
  manager->count, parent, class_name, permission_mask;
  @ ensures \result != \null ==> (
  @   \result->scope == LUX_CAP_SCOPE_CLASS &&
  @   \result->parent_id == parent->cap_id &&
  @   \result->permissions == permission_mask &&
  @   \result->derivation_depth == parent->derivation_depth + 1
  @ );
  @*/
lux_capability_t *lux_cap_derive_class(lux_capability_manager_t *manager,
                                       lux_capability_t *parent,
                                       const char *class_name,
                                       u32int permission_mask);

/*@
  @ requires manager != \null;
  @ requires child != \null && \valid(child);
  @ assigns \result \from manager->capabilities[0..manager->count-1], child;
  @ ensures \result == 1 ==> (
  @   \forall integer i; 0 <= i < child->derivation_depth ==>
  @     lux_cap_perms_subset(child->permissions,
  manager->capabilities[i].permissions)
  @ );
  @*/
int lux_cap_validate_chain(lux_capability_manager_t *manager,
                           lux_capability_t *child);

/*@
  @ requires cap != \null && \valid(cap);
  @ assigns \result \from cap->permissions, required;
  @ ensures \result == ((cap->permissions & required) == required);
  @*/
int lux_cap_check_permission(lux_capability_t *cap, u32int required);

/*@
  @ requires manager != \null && \valid(manager);
  @ requires uuid != \null && \valid_read(uuid);
  @ assigns \result \from manager->capabilities[0..manager->count-1], uuid;
  @ ensures \result != \null ==> (
  @   \result->uuid.data[0] == uuid->data[0] &&
  @   \result->uuid.data[15] == uuid->data[15]
  @ );
  @*/
lux_capability_t *lux_cap_find_by_uuid(lux_capability_manager_t *manager,
                                       const uuid_t *uuid);

/*
 * Find capability by numeric ID.
 *
 * @requires: manager != NULL
 * @ensures: \result != NULL ==> \result->cap_id == cap_id
 */
lux_capability_t *lux_cap_find_by_id(lux_capability_manager_t *manager,
                                     u32int cap_id);

/* ========== Permission Utilities ========== */

/*@
  @ assigns \result \from a, b;
  @ ensures \result == 1 <==> (a & b) == a;
  @*/
int lux_cap_perms_subset(u32int a, u32int b);

/*
 * Format permissions as human-readable string.
 * @ensures: writes at most buflen-1 chars to buf
 */
void lux_cap_perms_to_string(u32int perms, char *buf, u32int buflen);

/* ========== Debug/Diagnostics ========== */

/*
 * Dump capability details to kernel console.
 */
void lux_cap_dump(lux_capability_t *cap);

/*
 * Dump manager statistics to kernel console.
 */
void lux_cap_manager_dump_stats(lux_capability_manager_t *manager);

#endif /* LUX_CAPABILITY_H */
