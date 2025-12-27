/* wasm_capability_bindings.h - WASM Import Bindings for Capability System
 *
 * Exposes the kernel's capability-based security system to WASM modules.
 * WASM modules can create, derive, validate, and check capabilities for
 * fine-grained access control.
 *
 * Security model:
 *   - Each WASM module gets a root MODULE capability on load
 *   - Modules derive CLASS/METHOD capabilities with reduced permissions
 *   - Capabilities are referenced by handles (like file descriptors)
 *   - Kernel validates all capability operations
 *
 * Integration:
 *   - Uses kernel/capability/clr_capability.{c,h} for actual capability logic
 *   - Maintains per-process capability handle table
 *   - Handles are process-local, not transferable directly (use IPC)
 */

#ifndef WASM_CAPABILITY_BINDINGS_H
#define WASM_CAPABILITY_BINDINGS_H

#include "../capability/clr_capability.h"

/* WASM uses 32-bit handles for capability references */
typedef u32int wasm_cap_handle_t;

#define WASM_CAP_INVALID_HANDLE 0

/* Maximum capabilities per WASM process */
#define WASM_CAP_TABLE_SIZE 256

/* ========== Per-Process Capability Handle Table ========== */
/*
 * Each WASM process has its own capability handle table.
 * Handles are small integers (1-256) that map to actual capabilities.
 * This provides isolation: process A's handle 5 != process B's handle 5.
 */
typedef struct wasm_cap_table {
  clr_monotonic_capability_t *caps[WASM_CAP_TABLE_SIZE];
  u32int next_handle; /* Next available handle */
} wasm_cap_table_t;

/* Initialize capability table for new WASM process */
void wasm_cap_table_init(wasm_cap_table_t *table);

/* Clean up capability table on process exit */
void wasm_cap_table_destroy(wasm_cap_table_t *table);

/* Insert capability and return handle */
wasm_cap_handle_t wasm_cap_table_insert(wasm_cap_table_t *table,
                                        clr_monotonic_capability_t *cap);

/* Lookup capability by handle */
clr_monotonic_capability_t *wasm_cap_table_lookup(wasm_cap_table_t *table,
                                                  wasm_cap_handle_t handle);

/* Remove capability from table */
void wasm_cap_table_remove(wasm_cap_table_t *table, wasm_cap_handle_t handle);

/* ========== WASM Import Functions ========== */
/*
 * These functions are exposed as WASM imports and callable from WASM code.
 * They translate between WASM types and kernel capability operations.
 *
 * WASM signature conventions:
 *   - Strings: (ptr: i32, len: i32) - pointer to WASM linear memory
 *   - Returns: i32 for handles/error codes, i64 for success/failure
 *   - Error codes: 0 = success, negative = error
 */

/* Create root MODULE capability for WASM module
 *
 * @param table: Process capability table
 * @param manager: Global capability manager
 * @param name_ptr: Pointer to module name string in WASM memory
 * @param name_len: Length of module name
 * @returns: Capability handle, or WASM_CAP_INVALID_HANDLE on error
 *
 * WASM signature: (i32, i32) -> i32
 */
wasm_cap_handle_t wasm_import_cap_create_module(wasm_cap_table_t *table,
                                                capability_manager_t *manager,
                                                u32int name_ptr, u32int name_len);

/* Derive child capability with reduced permissions
 *
 * @param table: Process capability table
 * @param manager: Global capability manager
 * @param parent_handle: Handle to parent capability
 * @param perms: Requested permissions (must be subset of parent)
 * @param scope: New scope (CAP_SCOPE_CLASS or CAP_SCOPE_METHOD)
 * @param name_ptr: Pointer to derived capability name in WASM memory
 * @param name_len: Length of name
 * @returns: New capability handle, or WASM_CAP_INVALID_HANDLE on error
 *
 * WASM signature: (i32, i32, i32, i32, i32) -> i32
 */
wasm_cap_handle_t wasm_import_cap_derive(wasm_cap_table_t *table,
                                         capability_manager_t *manager,
                                         wasm_cap_handle_t parent_handle,
                                         u32int perms,
                                         u32int scope,
                                         u32int name_ptr,
                                         u32int name_len);

/* Check if capability has required permissions
 *
 * @param table: Process capability table
 * @param handle: Capability handle to check
 * @param required_perms: Required permission bits
 * @returns: 1 if capability has permissions, 0 otherwise
 *
 * WASM signature: (i32, i32) -> i32
 */
u32int wasm_import_cap_check(wasm_cap_table_t *table,
                             wasm_cap_handle_t handle,
                             u32int required_perms);

/* Validate capability derivation chain
 *
 * @param table: Process capability table
 * @param manager: Global capability manager
 * @param handle: Capability handle to validate
 * @returns: 1 if chain is valid, 0 otherwise
 *
 * WASM signature: (i32) -> i32
 */
u32int wasm_import_cap_validate(wasm_cap_table_t *table,
                                capability_manager_t *manager,
                                wasm_cap_handle_t handle);

/* Get capability permissions (for inspection)
 *
 * @param table: Process capability table
 * @param handle: Capability handle
 * @returns: Permission bitmask, or 0 if handle invalid
 *
 * WASM signature: (i32) -> i32
 */
u32int wasm_import_cap_get_perms(wasm_cap_table_t *table,
                                 wasm_cap_handle_t handle);

/* Revoke capability (and all derived from it)
 *
 * @param table: Process capability table
 * @param handle: Capability handle to revoke
 * @returns: 1 if revoked, 0 if handle invalid
 *
 * WASM signature: (i32) -> i32
 */
u32int wasm_import_cap_revoke(wasm_cap_table_t *table,
                              wasm_cap_handle_t handle);

/* ========== IPC Integration ========== */
/*
 * For message-based IPC, capabilities need to be:
 *   1. Serialized into messages (as UUIDs)
 *   2. Deserialized on receive (lookup by UUID, insert into receiver's table)
 *   3. Validated on transfer (sender must have TRANSFER permission)
 */

/* Serialize capability to UUID for IPC
 *
 * @param table: Process capability table
 * @param handle: Capability handle to serialize
 * @param uuid_out: Output buffer for UUID (16 bytes)
 * @returns: 1 on success, 0 on error
 */
u32int wasm_cap_serialize_for_ipc(wasm_cap_table_t *table,
                                  wasm_cap_handle_t handle,
                                  uuid_t *uuid_out);

/* Deserialize capability from UUID received via IPC
 *
 * @param table: Process capability table
 * @param manager: Global capability manager
 * @param uuid: UUID from IPC message
 * @returns: New handle in receiver's table, or WASM_CAP_INVALID_HANDLE
 */
wasm_cap_handle_t wasm_cap_deserialize_from_ipc(wasm_cap_table_t *table,
                                                capability_manager_t *manager,
                                                const uuid_t *uuid);

#endif /* WASM_CAPABILITY_BINDINGS_H */
