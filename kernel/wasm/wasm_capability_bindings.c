/* wasm_capability_bindings.c - WASM Capability System Bindings
 *
 * Implements WASM import functions that expose the kernel's capability
 * system to WASM modules. Provides fine-grained access control through
 * monotonic capability derivation.
 */

#include "wasm_capability_bindings.h"
#include "../include/u.h"

#ifndef nil
#define nil ((void*)0)
#endif

/* ========== Capability Handle Table Management ========== */

void wasm_cap_table_init(wasm_cap_table_t *table) {
  if (!table)
    return;

  for (u32int i = 0; i < WASM_CAP_TABLE_SIZE; i++) {
    table->caps[i] = nil;
  }
  table->next_handle = 1; /* 0 is reserved for invalid handle */
}

void wasm_cap_table_destroy(wasm_cap_table_t *table) {
  if (!table)
    return;

  /* Capabilities themselves are owned by the global manager */
  /* We just clear our references */
  for (u32int i = 0; i < WASM_CAP_TABLE_SIZE; i++) {
    table->caps[i] = nil;
  }
}

wasm_cap_handle_t wasm_cap_table_insert(wasm_cap_table_t *table,
                                        clr_monotonic_capability_t *cap) {
  if (!table || !cap)
    return WASM_CAP_INVALID_HANDLE;

  /* Find free slot */
  for (u32int i = 1; i < WASM_CAP_TABLE_SIZE; i++) {
    if (table->caps[i] == nil) {
      table->caps[i] = cap;
      return i;
    }
  }

  /* Table full */
  return WASM_CAP_INVALID_HANDLE;
}

clr_monotonic_capability_t *wasm_cap_table_lookup(wasm_cap_table_t *table,
                                                  wasm_cap_handle_t handle) {
  if (!table || handle == WASM_CAP_INVALID_HANDLE || handle >= WASM_CAP_TABLE_SIZE)
    return nil;

  return table->caps[handle];
}

void wasm_cap_table_remove(wasm_cap_table_t *table, wasm_cap_handle_t handle) {
  if (!table || handle == WASM_CAP_INVALID_HANDLE || handle >= WASM_CAP_TABLE_SIZE)
    return;

  table->caps[handle] = nil;
}

/* ========== WASM Import Functions ========== */

wasm_cap_handle_t wasm_import_cap_create_module(wasm_cap_table_t *table,
                                                capability_manager_t *manager,
                                                u32int name_ptr,
                                                u32int name_len) {
  if (!table || !manager)
    return WASM_CAP_INVALID_HANDLE;

  /* TODO: Access WASM linear memory to read name string */
  /* For now, use a placeholder name */
  /* This requires integration with WASM runtime to access linear memory */
  const char *module_name = "wasm_module";

  /* Create root capability via kernel API */
  clr_monotonic_capability_t *cap = cap_create_module(manager, module_name);
  if (!cap)
    return WASM_CAP_INVALID_HANDLE;

  /* Insert into process table and return handle */
  return wasm_cap_table_insert(table, cap);
}

wasm_cap_handle_t wasm_import_cap_derive(wasm_cap_table_t *table,
                                         capability_manager_t *manager,
                                         wasm_cap_handle_t parent_handle,
                                         u32int perms,
                                         u32int scope,
                                         u32int name_ptr,
                                         u32int name_len) {
  if (!table || !manager)
    return WASM_CAP_INVALID_HANDLE;

  /* Lookup parent capability */
  clr_monotonic_capability_t *parent = wasm_cap_table_lookup(table, parent_handle);
  if (!parent)
    return WASM_CAP_INVALID_HANDLE;

  /* TODO: Access WASM linear memory to read name string */
  const char *derived_name = "derived_cap";

  /* Derive capability based on scope */
  clr_monotonic_capability_t *derived = nil;

  if (scope == CAP_SCOPE_CLASS) {
    derived = cap_derive_class(manager, parent, derived_name, perms);
  } else {
    /* For METHOD scope, we'd need cap_derive_method (not yet implemented) */
    /* For now, use class derivation */
    derived = cap_derive_class(manager, parent, derived_name, perms);
  }

  if (!derived)
    return WASM_CAP_INVALID_HANDLE;

  /* Insert into process table */
  return wasm_cap_table_insert(table, derived);
}

u32int wasm_import_cap_check(wasm_cap_table_t *table,
                             wasm_cap_handle_t handle,
                             u32int required_perms) {
  if (!table)
    return 0;

  clr_monotonic_capability_t *cap = wasm_cap_table_lookup(table, handle);
  if (!cap)
    return 0;

  return cap_check_permission(cap, required_perms);
}

u32int wasm_import_cap_validate(wasm_cap_table_t *table,
                                capability_manager_t *manager,
                                wasm_cap_handle_t handle) {
  if (!table || !manager)
    return 0;

  clr_monotonic_capability_t *cap = wasm_cap_table_lookup(table, handle);
  if (!cap)
    return 0;

  return cap_validate_chain(manager, cap);
}

u32int wasm_import_cap_get_perms(wasm_cap_table_t *table,
                                 wasm_cap_handle_t handle) {
  if (!table)
    return 0;

  clr_monotonic_capability_t *cap = wasm_cap_table_lookup(table, handle);
  if (!cap)
    return 0;

  return cap->permissions;
}

u32int wasm_import_cap_revoke(wasm_cap_table_t *table,
                              wasm_cap_handle_t handle) {
  if (!table)
    return 0;

  clr_monotonic_capability_t *cap = wasm_cap_table_lookup(table, handle);
  if (!cap)
    return 0;

  /* Mark as revoked */
  cap->is_revoked = 1;

  /* Remove from process table */
  wasm_cap_table_remove(table, handle);

  return 1;
}

/* ========== IPC Integration ========== */

u32int wasm_cap_serialize_for_ipc(wasm_cap_table_t *table,
                                  wasm_cap_handle_t handle,
                                  uuid_t *uuid_out) {
  if (!table || !uuid_out)
    return 0;

  clr_monotonic_capability_t *cap = wasm_cap_table_lookup(table, handle);
  if (!cap)
    return 0;

  /* Check if capability has TRANSFER permission */
  if (!cap_check_permission(cap, CAP_PERM_TRANSFER))
    return 0;

  /* Copy UUID for transmission */
  uuid_copy(uuid_out, &cap->uuid);

  return 1;
}

wasm_cap_handle_t wasm_cap_deserialize_from_ipc(wasm_cap_table_t *table,
                                                capability_manager_t *manager,
                                                const uuid_t *uuid) {
  if (!table || !manager || !uuid)
    return WASM_CAP_INVALID_HANDLE;

  /* Lookup capability by UUID in global manager */
  clr_monotonic_capability_t *cap = cap_find_by_uuid(manager, uuid);
  if (!cap)
    return WASM_CAP_INVALID_HANDLE;

  /* Validate that capability hasn't been revoked */
  if (cap->is_revoked)
    return WASM_CAP_INVALID_HANDLE;

  /* Insert into receiver's capability table */
  return wasm_cap_table_insert(table, cap);
}
