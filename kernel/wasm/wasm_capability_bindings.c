/* wasm_capability_bindings.c - WASM Capability System Bindings
 *
 * Implements WASM import functions that expose the kernel's capability
 * system to WASM modules. Provides fine-grained access control through
 * monotonic capability derivation.
 */

#include "wasm_capability_bindings.h"
#include "../include/dat.h"
#include "../include/fns.h"
#include "../include/portlib.h"
#include "../include/u.h"

#ifndef nil
#define nil ((void *)0)
#endif

/* Global capability manager - defined in kernel init or lux_capability.c */
extern lux_capability_manager_t *global_cap_manager;

/* ========== Capability Handle Table Management ========== */

/*@
  @ requires table == \null || \valid(table);
  @ assigns \nothing;
  @*/
void wasm_cap_table_init(wasm_cap_table_t *table) {
  if (!table)
    return;

    /*@ loop invariant 0 <= i <= WASM_CAP_TABLE_SIZE;
    @ loop assigns i;
    @ loop variant WASM_CAP_TABLE_SIZE - i;
    @*/
  for (u32int i = 0; i < WASM_CAP_TABLE_SIZE; i++) {
    table->caps[i] = nil;
  }
  table->next_handle = 1; /* 0 is reserved for invalid handle */
}

/*@
  @ requires table == \null || \valid(table);
  @ assigns \nothing;
  @*/
void wasm_cap_table_destroy(wasm_cap_table_t *table) {
  if (!table)
    return;

  /* Capabilities themselves are owned by the global manager or LUx system */
  /* We just clear our references */
    /*@ loop invariant 0 <= i <= WASM_CAP_TABLE_SIZE;
    @ loop assigns i;
    @ loop variant WASM_CAP_TABLE_SIZE - i;
    @*/
  for (u32int i = 0; i < WASM_CAP_TABLE_SIZE; i++) {
    table->caps[i] = nil;
  }
}

wasm_cap_handle_t wasm_cap_table_insert(wasm_cap_table_t *table,
                                        lux_capability_t *cap) {
  if (!table || !cap)
    return WASM_CAP_INVALID_HANDLE;

  /* Find free slot */
    /*@ loop invariant 0 <= i <= WASM_CAP_TABLE_SIZE;
    @ loop assigns i;
    @ loop variant WASM_CAP_TABLE_SIZE - i;
    @*/
  for (u32int i = 1; i < WASM_CAP_TABLE_SIZE; i++) {
    if (table->caps[i] == nil) {
      table->caps[i] = cap;
      return i;
    }
  }

  /* Table full */
  return WASM_CAP_INVALID_HANDLE;
}

lux_capability_t *wasm_cap_table_lookup(wasm_cap_table_t *table,
                                        wasm_cap_handle_t handle) {
  if (!table || handle == WASM_CAP_INVALID_HANDLE ||
      handle >= WASM_CAP_TABLE_SIZE)
    return nil;

  return table->caps[handle];
}

/*@
  @ requires table == \null || \valid(table);
  @ assigns \nothing;
  @*/
void wasm_cap_table_remove(wasm_cap_table_t *table, wasm_cap_handle_t handle) {
  if (!table || handle == WASM_CAP_INVALID_HANDLE ||
      handle >= WASM_CAP_TABLE_SIZE)
    return;

  table->caps[handle] = nil;
}

/* ========== WASM Linear Memory Access Helper ========== */

static int wasm_get_string(IM3Runtime runtime, uint32_t ptr, uint32_t len,
                           char *buf, uint32_t bufsize) {
  /* m3ApiOffsetToPtr requires _mem to be in scope, usually from m3ApiGetMemory
   */
  /* Since we're in a helper, we use m3_GetMemory directly */
  uint8_t *_mem = m3_GetMemory(runtime, NULL, 0);
  uint32_t memsize = m3_GetMemorySize(runtime);
  if (!_mem || ptr > memsize || len > memsize - ptr || len >= bufsize)
    return -1;

  char *wasm_mem = (char *)(_mem + ptr);
  memmove(buf, wasm_mem, len);
  buf[len] = '\0';
  return 0;
}

/* ========== Host Function Implementations ========== */

/* cap_module_create(name_ptr: i32, name_len: i32) -> cap_handle_t */
m3ApiRawFunction(host_cap_module_create) {
  m3ApiGetArg(uint32_t, name_ptr) m3ApiGetArg(uint32_t, name_len)
      m3ApiReturnType(wasm_cap_handle_t)

          char name[128];
  if (wasm_get_string(runtime, name_ptr, name_len, name, sizeof(name)) < 0) {
    m3ApiReturn(WASM_CAP_INVALID_HANDLE);
  }

  if (!global_cap_manager) {
    m3ApiReturn(WASM_CAP_INVALID_HANDLE);
  }

  lux_capability_t *cap = lux_cap_create_module(global_cap_manager, name);
  if (!cap) {
    m3ApiReturn(WASM_CAP_INVALID_HANDLE);
  }

  wasm_cap_table_t *table = (wasm_cap_table_t *)up->wasm.cap_table;
  if (!table) {
    /* This should have been initialized in sys_wasm_compile */
    m3ApiReturn(WASM_CAP_INVALID_HANDLE);
  }

  m3ApiReturn(wasm_cap_table_insert(table, cap));
}

/* cap_derive(parent_h: i32, perms: i32, scope: i32, name_ptr: i32, name_len:
 * i32) -> cap_handle_t */
m3ApiRawFunction(host_cap_derive) {
  m3ApiGetArg(wasm_cap_handle_t, parent_h) m3ApiGetArg(u32int, perms)
      m3ApiGetArg(u32int, scope) m3ApiGetArg(uint32_t, name_ptr)
          m3ApiGetArg(uint32_t, name_len) m3ApiReturnType(wasm_cap_handle_t)

              wasm_cap_table_t *table = (wasm_cap_table_t *)up->wasm.cap_table;
  if (!table)
    m3ApiReturn(WASM_CAP_INVALID_HANDLE);

  lux_capability_t *parent = wasm_cap_table_lookup(table, parent_h);
  if (!parent)
    m3ApiReturn(WASM_CAP_INVALID_HANDLE);

  char name[128];
  if (wasm_get_string(runtime, name_ptr, name_len, name, sizeof(name)) < 0) {
    m3ApiReturn(WASM_CAP_INVALID_HANDLE);
  }

  lux_capability_t *derived =
      lux_cap_derive_class(global_cap_manager, parent, name, perms);
  if (!derived)
    m3ApiReturn(WASM_CAP_INVALID_HANDLE);

  m3ApiReturn(wasm_cap_table_insert(table, derived));
}

/* cap_check(handle: i32, perms: i32) -> i32 */
m3ApiRawFunction(host_cap_check) {
  m3ApiGetArg(wasm_cap_handle_t, handle) m3ApiGetArg(u32int, perms)
      m3ApiReturnType(u32int)

          wasm_cap_table_t *table = (wasm_cap_table_t *)up->wasm.cap_table;
  if (!table)
    m3ApiReturn(0);

  lux_capability_t *cap = wasm_cap_table_lookup(table, handle);
  if (!cap)
    m3ApiReturn(0);

  m3ApiReturn(lux_cap_check_permission(cap, perms));
}

/* cap_validate(handle: i32) -> i32 */
m3ApiRawFunction(host_cap_validate) {
  m3ApiGetArg(wasm_cap_handle_t, handle) m3ApiReturnType(u32int)

      wasm_cap_table_t *table = (wasm_cap_table_t *)up->wasm.cap_table;
  if (!table)
    m3ApiReturn(0);

  lux_capability_t *cap = wasm_cap_table_lookup(table, handle);
  if (!cap)
    m3ApiReturn(0);

  m3ApiReturn(lux_cap_validate_chain(global_cap_manager, cap));
}

/* ========== Linking Function ========== */

/*@
  @ assigns \nothing;
  @*/
M3Result LinkCapabilities(IM3Module module) {
  M3Result result = m3Err_none;
  const char *ns = "cap";

  result = m3_LinkRawFunction(module, ns, "module_create", "i(ii)",
                              &host_cap_module_create);
  if (result && result != m3Err_functionLookupFailed)
    return result;

  result =
      m3_LinkRawFunction(module, ns, "derive", "i(iiiii)", &host_cap_derive);
  if (result && result != m3Err_functionLookupFailed)
    return result;

  result = m3_LinkRawFunction(module, ns, "check", "i(ii)", &host_cap_check);
  if (result && result != m3Err_functionLookupFailed)
    return result;

  result =
      m3_LinkRawFunction(module, ns, "validate", "i(i)", &host_cap_validate);
  if (result && result != m3Err_functionLookupFailed)
    return result;

  return m3Err_none;
}
