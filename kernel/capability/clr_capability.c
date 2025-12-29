/* clr_capability.c - Capability-Based Security Implementation
 *
 * Implements monotonic capability derivation with properties proven in Coq.
 * See clr_capability.h for detailed documentation and Coq proof references.
 *
 * Key invariants maintained:
 *   1. Permissions can only decrease during derivation
 *   2. Derivation depth strictly increases
 *   3. UUIDs are unique across all capabilities
 *   4. Parent references form valid chains to root
 */

#include "clr_capability.h"

/* Kernel includes or userspace stubs */
#ifdef USERSPACE_TEST
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define xalloc(size) calloc(1, size)
#define xallocz(size, zero) (zero ? calloc(1, size) : malloc(size))
#define xfree(ptr) free(ptr)
#define print printf
#define snprint snprintf
#define smprint(...)                                                           \
  ({                                                                           \
    char *_buf = malloc(256);                                                  \
    if (_buf)                                                                  \
      snprintf(_buf, 256, __VA_ARGS__);                                        \
    _buf;                                                                      \
  })
#define nil NULL
#else
/* Kernel mode - use Pebble for tracked allocations */
#include "../include/pebble.h"
#include "../include/u.h"

/* Capability allocations use Pebble for resource tracking */
static void *pebble_alloc_wrapper(unsigned long size) {
  UserCapability cap;
  void *addr;

  if (pebble_alloc_with_white(size, &cap, &addr) != 0)
    return nil;

  if (addr && size > 0) {
    memset(addr, 0, size); /* Zero memory like xallocz */
  }
  return addr;
}

#define xalloc(size) pebble_alloc_wrapper(size)
#define xallocz(size, zero)                                                    \
  pebble_alloc_wrapper(size)     /* Pebble zeros by default */
#define xfree(ptr) ((void)(ptr)) /* Cleanup via Pebble process exit */

#define nil ((void *)0)
#endif

/* Global capability manager instance - initialized during kernel boot */
capability_manager_t *global_cap_manager = nil;

/* ========== Permission Utilities ========== */

/*
 * @coq_proof: proofs/capability/PermsBitmask.v
 * @definition: perms_subset
 *
 * ACSL Contract:
 *   requires \valid(a) && \valid(b);
 *   ensures \result == 1 <==> (a & b) == a;
 */
int cap_perms_subset(u32int a, u32int b) { return (a & b) == a; }

void cap_perms_to_string(u32int perms, char *buf, u32int buflen) {
  if (buflen == 0)
    return;
  buf[0] = '\0';

  char *p = buf;
  char *end = buf + buflen - 1;

  if (perms & CAP_PERM_READ) {
    int n = snprint(p, end - p, "R");
    p += n;
  }
  if (perms & CAP_PERM_WRITE) {
    int n = snprint(p, end - p, "W");
    p += n;
  }
  if (perms & CAP_PERM_EXEC) {
    int n = snprint(p, end - p, "X");
    p += n;
  }
  if (perms & CAP_PERM_TRANSFER) {
    int n = snprint(p, end - p, "T");
    p += n;
  }
  if (perms & CAP_PERM_GRANT) {
    int n = snprint(p, end - p, "G");
    p += n;
  }
  if (perms == 0) {
    snprint(buf, buflen, "-");
  }
}

/* ========== Capability Manager Lifecycle ========== */

capability_manager_t *cap_manager_create(void) {
  capability_manager_t *mgr = xalloc(sizeof(capability_manager_t));
  if (!mgr)
    return nil;

  mgr->capabilities =
      xalloc(sizeof(clr_monotonic_capability_t) * CAP_TABLE_INITIAL_SIZE);
  if (!mgr->capabilities) {
    xfree(mgr);
    return nil;
  }

  mgr->capacity = CAP_TABLE_INITIAL_SIZE;
  mgr->count = 0;
  mgr->monotonic_time = 1; /* Start at 1, 0 reserved for "no time" */
  mgr->next_cap_id = 1;    /* Start at 1, 0 reserved for "no parent" */
  mgr->derivations = 0;
  mgr->validations = 0;
  mgr->rejections = 0;

  return mgr;
}

void cap_manager_destroy(capability_manager_t *manager) {
  if (!manager)
    return;

  /* Free bound_metadata strings */
  for (u32int i = 0; i < manager->count; i++) {
    if (manager->capabilities[i].bound_metadata) {
      xfree(manager->capabilities[i].bound_metadata);
    }
  }

  if (manager->capabilities) {
    xfree(manager->capabilities);
  }

  xfree(manager);
}

/* ========== Internal Helpers ========== */

/* Grow capability table if needed */
static int cap_ensure_capacity(capability_manager_t *manager) {
  if (manager->count < manager->capacity)
    return 0;

  if (manager->capacity >= CAP_TABLE_MAX_SIZE) {
    print("cap: table full, max %d capabilities\n", CAP_TABLE_MAX_SIZE);
    return -1;
  }

  u32int new_cap = manager->capacity * 2;
  if (new_cap > CAP_TABLE_MAX_SIZE)
    new_cap = CAP_TABLE_MAX_SIZE;

  clr_monotonic_capability_t *new_caps =
      xalloc(sizeof(clr_monotonic_capability_t) * new_cap);
  if (!new_caps)
    return -1;

  /* Copy existing capabilities */
  for (u32int i = 0; i < manager->count; i++) {
    new_caps[i] = manager->capabilities[i];
  }

  xfree(manager->capabilities);
  manager->capabilities = new_caps;
  manager->capacity = new_cap;

  return 0;
}

/* Allocate a new capability slot */
static clr_monotonic_capability_t *
cap_alloc_slot(capability_manager_t *manager) {
  if (cap_ensure_capacity(manager) != 0)
    return nil;

  clr_monotonic_capability_t *cap = &manager->capabilities[manager->count];
  manager->count++;

  /* Initialize with zeros */
  uuid_clear(&cap->uuid);
  uuid_clear(&cap->parent_uuid);
  cap->cap_id = 0;
  cap->parent_id = 0;
  cap->derivation_depth = 0;
  cap->permissions = 0;
  cap->max_permissions = 0;
  cap->creation_time = 0;
  cap->expiration_time = 0;
  cap->scope = CAP_SCOPE_MODULE;
  cap->bound_metadata = nil;
  cap->is_validated = 0;
  cap->is_revoked = 0;

  return cap;
}

/* ========== Core Capability Operations ========== */

/*
 * Create a module-level (root) capability.
 *
 * @coq_proof: proofs/capability/CapabilityModel.v
 *
 * ACSL Contract:
 *   requires \valid(manager) && \valid(assembly_name);
 *   ensures \result != NULL ==>
 *           \result->scope == CAP_SCOPE_MODULE &&
 *           \result->parent_id == 0 &&
 *           \result->permissions == CAP_PERM_ALL &&
 *           \result->derivation_depth == 0;
 */
clr_monotonic_capability_t *cap_create_module(capability_manager_t *manager,
                                              const char *assembly_name) {
  print("CAP: create_module ENTER\n");
  if (!manager || !assembly_name) {
    print("CAP: create_module NULL args\n");
    return nil;
  }

  print("CAP: calling cap_alloc_slot\n");
  clr_monotonic_capability_t *cap = cap_alloc_slot(manager);
  print("CAP: cap_alloc_slot returned %p\n", cap);
  if (!cap)
    return nil;

  print("CAP: calling uuid_new_v8\n");
  /* Generate unique UUID */
  uuid_new_v8(&cap->uuid);

  /* Assign capability ID */
  cap->cap_id = manager->next_cap_id++;

  /* Root capability: no parent */
  uuid_clear(&cap->parent_uuid);
  cap->parent_id = 0;
  cap->derivation_depth = 0;

  /* Full permissions for root */
  cap->permissions = CAP_PERM_ALL;
  cap->max_permissions = CAP_PERM_ALL;

  /* Timing */
  cap->creation_time = manager->monotonic_time++;
  cap->expiration_time = 0; /* No expiry */

  /* Scope and metadata */
  cap->scope = CAP_SCOPE_MODULE;
  cap->bound_metadata = smprint("module:%s", assembly_name);
  cap->is_validated = 1; /* Root is always valid */
  cap->is_revoked = 0;

  return cap;
}

/*
 * Derive a class-level capability from a parent.
 *
 * @coq_proof: proofs/capability/DerivationChain.v
 * @theorem: derived_perm_monotonic
 *
 * ACSL Contract:
 *   requires \valid(manager) && \valid(parent) && \valid(class_name);
 *   requires cap_perms_subset(permission_mask, parent->permissions);
 *   ensures \result != NULL ==>
 *           \result->scope == CAP_SCOPE_CLASS &&
 *           \result->parent_id == parent->cap_id &&
 *           \result->permissions == permission_mask &&
 *           \result->derivation_depth == parent->derivation_depth + 1 &&
 *           cap_perms_subset(\result->permissions, parent->permissions);
 */
clr_monotonic_capability_t *cap_derive_class(capability_manager_t *manager,
                                             clr_monotonic_capability_t *parent,
                                             const char *class_name,
                                             u32int permission_mask) {
  if (!manager || !parent || !class_name)
    return nil;

  /* CRITICAL: Enforce monotonic permission decrease */
  /* This is the key security property proven in DerivationChain.v */
  if (!cap_perms_subset(permission_mask, parent->permissions)) {
    manager->rejections++;
    print("cap: SECURITY: rejected derivation - requested perms 0x%x "
          "exceed parent perms 0x%x\n",
          permission_mask, parent->permissions);
    return nil;
  }

  /* Parent must not be revoked */
  if (parent->is_revoked) {
    manager->rejections++;
    print("cap: rejected derivation from revoked capability\n");
    return nil;
  }

  /* Parent must have GRANT permission to derive */
  if (!(parent->permissions & CAP_PERM_GRANT)) {
    manager->rejections++;
    print("cap: rejected derivation - parent lacks GRANT permission\n");
    return nil;
  }

  clr_monotonic_capability_t *cap = cap_alloc_slot(manager);
  if (!cap)
    return nil;

  /* Generate unique UUID */
  uuid_new_v8(&cap->uuid);

  /* Assign capability ID */
  cap->cap_id = manager->next_cap_id++;

  /* Link to parent */
  uuid_copy(&cap->parent_uuid, &parent->uuid);
  cap->parent_id = parent->cap_id;
  cap->derivation_depth = parent->derivation_depth + 1;

  /* Permissions: monotonically decreased */
  cap->permissions = permission_mask;
  cap->max_permissions = permission_mask; /* Immutable from creation */

  /* Timing */
  cap->creation_time = manager->monotonic_time++;
  cap->expiration_time = 0;

  /* Scope and metadata */
  cap->scope = CAP_SCOPE_CLASS;
  cap->bound_metadata = smprint("class:%s", class_name);
  cap->is_validated = 0; /* Needs explicit validation */
  cap->is_revoked = 0;

  manager->derivations++;

  return cap;
}

/*
 * Validate a capability's derivation chain.
 *
 * @coq_proof: proofs/capability/ChainDecidability.v
 * @theorem: derived_chain_table_perms_monotonic
 *
 * Walks the chain from child to root, verifying:
 *   1. All parent references are valid
 *   2. Permission monotonicity holds at each step
 *   3. No capabilities in chain are revoked
 */
int cap_validate_chain(capability_manager_t *manager,
                       clr_monotonic_capability_t *child) {
  if (!manager || !child)
    return 0;

  manager->validations++;

  clr_monotonic_capability_t *current = child;
  u32int max_depth = CAP_TABLE_MAX_SIZE; /* Prevent infinite loops */

  while (current->parent_id != 0 && max_depth > 0) {
    /* Find parent */
    clr_monotonic_capability_t *parent =
        cap_find_by_id(manager, current->parent_id);
    if (!parent) {
      print("cap: chain validation failed - parent %d not found\n",
            current->parent_id);
      return 0;
    }

    /* Check parent not revoked */
    if (parent->is_revoked) {
      print("cap: chain validation failed - ancestor revoked\n");
      return 0;
    }

    /* Check permission monotonicity */
    if (!cap_perms_subset(current->permissions, parent->permissions)) {
      print("cap: chain validation failed - permission violation\n");
      return 0;
    }

    /* Check depth consistency */
    if (current->derivation_depth != parent->derivation_depth + 1) {
      print("cap: chain validation failed - depth inconsistency\n");
      return 0;
    }

    current = parent;
    max_depth--;
  }

  /* Reached root successfully */
  if (current->scope == CAP_SCOPE_MODULE && current->parent_id == 0) {
    child->is_validated = 1;
    return 1;
  }

  print("cap: chain validation failed - did not reach root\n");
  return 0;
}

/*
 * Check if capability has required permission.
 *
 * @coq_proof: proofs/capability/PermsBitmask.v
 * @definition: has_perm
 */
int cap_check_permission(clr_monotonic_capability_t *cap, u32int required) {
  if (!cap)
    return 0;
  if (cap->is_revoked)
    return 0;
  return (cap->permissions & required) == required;
}

/* ========== Lookup Functions ========== */

clr_monotonic_capability_t *cap_find_by_uuid(capability_manager_t *manager,
                                             const uuid_t *uuid) {
  if (!manager || !uuid)
    return nil;

  for (u32int i = 0; i < manager->count; i++) {
    if (uuid_compare(&manager->capabilities[i].uuid, uuid) == 0) {
      return &manager->capabilities[i];
    }
  }

  return nil;
}

clr_monotonic_capability_t *cap_find_by_id(capability_manager_t *manager,
                                           u32int cap_id) {
  if (!manager || cap_id == 0)
    return nil;

  for (u32int i = 0; i < manager->count; i++) {
    if (manager->capabilities[i].cap_id == cap_id) {
      return &manager->capabilities[i];
    }
  }

  return nil;
}

/* ========== Debug Functions ========== */

void cap_dump(clr_monotonic_capability_t *cap) {
  if (!cap) {
    print("cap: (null)\n");
    return;
  }

  char uuid_str[40];
  uuid_unparse(&cap->uuid, uuid_str);

  char perms_str[16];
  cap_perms_to_string(cap->permissions, perms_str, sizeof(perms_str));

  const char *scope_str = "?";
  switch (cap->scope) {
  case CAP_SCOPE_MODULE:
    scope_str = "MODULE";
    break;
  case CAP_SCOPE_CLASS:
    scope_str = "CLASS";
    break;
  case CAP_SCOPE_METHOD:
    scope_str = "METHOD";
    break;
  }

  print("cap[%d]: uuid=%s scope=%s perms=%s depth=%d parent=%d %s%s\n",
        cap->cap_id, uuid_str, scope_str, perms_str, cap->derivation_depth,
        cap->parent_id, cap->bound_metadata ? cap->bound_metadata : "",
        cap->is_revoked ? " [REVOKED]" : "");
}

void cap_manager_dump_stats(capability_manager_t *manager) {
  if (!manager) {
    print("cap_manager: (null)\n");
    return;
  }

  print("cap_manager: %d/%d capabilities, %d derivations, %d validations, "
        "%d rejections\n",
        manager->count, manager->capacity, manager->derivations,
        manager->validations, manager->rejections);
}
