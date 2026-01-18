/* wasm_capability_bindings.c - WASM Capability System Bindings
 *
 * Implements WASM import functions that expose the kernel's capability
 * system to WASM modules. Provides fine-grained access control through
 * monotonic capability derivation.
 */

#include "wasm_capability_bindings.h"
#include "../include/u.h"

#ifndef nil
#define nil ((void *)0)
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
  if (!table || handle == WASM_CAP_INVALID_HANDLE ||
      handle >= WASM_CAP_TABLE_SIZE)
    return nil;

  return table->caps[handle];
}

void wasm_cap_table_remove(wasm_cap_table_t *table, wasm_cap_handle_t handle) {
  if (!table || handle == WASM_CAP_INVALID_HANDLE ||
      handle >= WASM_CAP_TABLE_SIZE)
    return;

  table->caps[handle] = nil;
}

/* ========== WASM Linear Memory Access Helper ========== */

/* Read string from WASM linear memory with bounds checking
 *
 * @param linear_mem: Pointer to WASM linear memory base
 * @param mem_size: Size of linear memory in bytes
 * @param ptr: Offset in linear memory where string starts
 * @param len: Length of string to read
 * @param buf: Output buffer for string (should include space for null
 * terminator)
 * @param buf_size: Size of output buffer
 * @returns: 0 on success, -1 on error (out of bounds or buffer too small)
 */
static int wasm_read_string_from_memory(u8int *linear_mem, u32int mem_size,
                                        u32int ptr, u32int len, char *buf,
                                        u32int buf_size) {
  if (!linear_mem || !buf)
    return -1;

  /* Check if string fits in WASM memory */
  if (ptr >= mem_size || len > mem_size || ptr + len > mem_size)
    return -1;

  /* Check if output buffer is big enough (including null terminator) */
  if (len >= buf_size)
    return -1;

  /* Copy string from WASM memory */
  for (u32int i = 0; i < len; i++) {
    buf[i] = (char)linear_mem[ptr + i];
  }
  buf[len] = '\0'; /* Null terminate */

  return 0;
}

/* ========== WASM Import Functions ========== */

wasm_cap_handle_t wasm_import_cap_create_module(
    wasm_cap_table_t *table, capability_manager_t *manager, u8int *linear_mem,
    u32int mem_size, u32int name_ptr, u32int name_len) {
  char module_name[128];

  if (!table || !manager)
    return WASM_CAP_INVALID_HANDLE;

  /* Read module name from WASM linear memory */
  if (wasm_read_string_from_memory(linear_mem, mem_size, name_ptr, name_len,
                                   module_name, sizeof(module_name)) < 0) {
    print("wasm_cap: failed to read module name from WASM memory\n");
    return WASM_CAP_INVALID_HANDLE;
  }

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
                                         u8int *linear_mem, u32int mem_size,
                                         u32int perms, u32int scope,
                                         u32int name_ptr, u32int name_len) {
  char derived_name[128];

  if (!table || !manager)
    return WASM_CAP_INVALID_HANDLE;

  /* Lookup parent capability */
  clr_monotonic_capability_t *parent =
      wasm_cap_table_lookup(table, parent_handle);
  if (!parent)
    return WASM_CAP_INVALID_HANDLE;

  /* Read derived capability name from WASM linear memory */
  if (wasm_read_string_from_memory(linear_mem, mem_size, name_ptr, name_len,
                                   derived_name, sizeof(derived_name)) < 0) {
    print("wasm_cap: failed to read derived name from WASM memory\n");
    return WASM_CAP_INVALID_HANDLE;
  }

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

u32int wasm_import_cap_check(wasm_cap_table_t *table, wasm_cap_handle_t handle,
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
                                  wasm_cap_handle_t handle, uuid_t *uuid_out) {
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
