/* wasm_host_fruity.c - Fruity IR Host Bindings for WASM
 *
 * Implements host functions for manipulating Fruity IR from WASM modules.
 * Every IR object (module, function, etc.) is managed as a Lux Capability.
 */

#include "wasm_host_fruity.h"
#include "../include/dat.h"
#include "../include/fns.h"
#define atoi lux_atoi
#include "../include/portlib.h"
#undef atoi
#include "../include/u.h"
#include "wasm_runtime.h"

/* Resolve conflicts between kernel and WASM3 headers */
#undef atoi
#undef abs

#ifndef nil
#define nil ((void *)0)
#endif

/* External capability manager */
extern lux_capability_manager_t *global_cap_manager;

/* ========== Helper Functions ========== */

/*@
  @ requires \valid(runtime);
  @ requires \valid(buf + (0..bufsize-1));
  @ assigns buf[0..len], \result \from runtime, ptr, len, bufsize;
  @ ensures \result == 0 ==> (
  @   buf[len] == '\0' &&
  @   len < bufsize
  @ );
  @ ensures \result == -1 ==> (
  @   len >= bufsize ||
  @   ptr > m3_GetMemorySize(runtime)
  @ );
  @*/
static int wasm_get_string(IM3Runtime runtime, uint32_t ptr, uint32_t len,
                           char *buf, uint32_t bufsize) {
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

/* fruity_module_create(name_ptr: i32, name_len: i32) -> cap_handle_t */
m3ApiRawFunction(host_fruity_module_create) {
  m3ApiGetArg(uint32_t, name_ptr) m3ApiGetArg(uint32_t, name_len)
      m3ApiReturnType(wasm_cap_handle_t)

      /* Check permissions */
      if (!(up->wasm.permissions & PERM_WASM_FRUITY)) {
    m3ApiReturn(WASM_CAP_INVALID_HANDLE);
  }

  char name[128];
  if (wasm_get_string(runtime, name_ptr, name_len, name, sizeof(name)) < 0) {
    m3ApiReturn(WASM_CAP_INVALID_HANDLE);
  }

  /* Wrap in Lux Capability */
  lux_capability_t *cap = lux_cap_create_module(global_cap_manager, name);
  if (!cap) {
    m3ApiReturn(WASM_CAP_INVALID_HANDLE);
  }

  /* Create Fruity IR Module */
  fruity_module_t *mod = fruity_module_create(name);
  if (!mod) {
    /* TODO: free cap */
    m3ApiReturn(WASM_CAP_INVALID_HANDLE);
  }
  cap->aux = mod;

  wasm_cap_table_t *table = (wasm_cap_table_t *)up->wasm.cap_table;
  m3ApiReturn(wasm_cap_table_insert(table, cap));
}

/* fruity_module_add_function(mod_h: i32, name_ptr: i32, name_len: i32, token:
 * i32) -> cap_handle_t */
m3ApiRawFunction(host_fruity_module_add_function) {
  m3ApiGetArg(wasm_cap_handle_t, mod_h) m3ApiGetArg(uint32_t, name_ptr)
      m3ApiGetArg(uint32_t, name_len) m3ApiGetArg(uint32_t, method_token)
          m3ApiReturnType(wasm_cap_handle_t)

              wasm_cap_table_t *table = (wasm_cap_table_t *)up->wasm.cap_table;
  lux_capability_t *mod_cap = wasm_cap_table_lookup(table, mod_h);
  if (!mod_cap || !mod_cap->aux)
    m3ApiReturn(WASM_CAP_INVALID_HANDLE);

  fruity_module_t *mod = (fruity_module_t *)mod_cap->aux;

  char name[128];
  if (wasm_get_string(runtime, name_ptr, name_len, name, sizeof(name)) < 0) {
    m3ApiReturn(WASM_CAP_INVALID_HANDLE);
  }

  /* Create Fruity Function */
  fruity_function_t *func = fruity_module_add_function(mod, name, method_token);
  if (!func)
    m3ApiReturn(WASM_CAP_INVALID_HANDLE);

  /* Derive function capability from module */
  lux_capability_t *func_cap = lux_cap_derive_class(global_cap_manager, mod_cap,
                                                    name, mod_cap->permissions);
  if (!func_cap) {
    /* TODO: free func */
    m3ApiReturn(WASM_CAP_INVALID_HANDLE);
  }
  func_cap->aux = func;

  m3ApiReturn(wasm_cap_table_insert(table, func_cap));
}

/* fruity_function_add_block(func_h: i32) -> cap_handle_t */
m3ApiRawFunction(host_fruity_function_add_block) {
  m3ApiGetArg(wasm_cap_handle_t, func_h) m3ApiReturnType(wasm_cap_handle_t)

      wasm_cap_table_t *table = (wasm_cap_table_t *)up->wasm.cap_table;
  lux_capability_t *func_cap = wasm_cap_table_lookup(table, func_h);
  if (!func_cap || !func_cap->aux)
    m3ApiReturn(WASM_CAP_INVALID_HANDLE);

  fruity_function_t *func = (fruity_function_t *)func_cap->aux;

  /* Create Fruity Block */
  fruity_basic_block_t *block = fruity_function_add_block(func);
  if (!block)
    m3ApiReturn(WASM_CAP_INVALID_HANDLE);

  lux_capability_t *block_cap = lux_cap_derive_class(
      global_cap_manager, func_cap, "block", func_cap->permissions);
  if (!block_cap) {
    /* TODO: free block */
    m3ApiReturn(WASM_CAP_INVALID_HANDLE);
  }
  block_cap->aux = block;

  m3ApiReturn(wasm_cap_table_insert(table, block_cap));
}

/* fruity_block_add_instr(block_h: i32, op: i32) -> cap_handle_t */
m3ApiRawFunction(host_fruity_block_add_instr) {
  m3ApiGetArg(wasm_cap_handle_t, block_h) m3ApiGetArg(uint32_t, op)
      m3ApiReturnType(wasm_cap_handle_t)

          wasm_cap_table_t *table = (wasm_cap_table_t *)up->wasm.cap_table;
  lux_capability_t *block_cap = wasm_cap_table_lookup(table, block_h);
  if (!block_cap || !block_cap->aux)
    m3ApiReturn(WASM_CAP_INVALID_HANDLE);

  fruity_basic_block_t *block = (fruity_basic_block_t *)block_cap->aux;

  /* Create Instruction */
  fruity_instruction_t *instr = fruity_instruction_create((fruity_opcode_t)op);
  if (!instr)
    m3ApiReturn(WASM_CAP_INVALID_HANDLE);

  fruity_basic_block_add_instruction(block, instr);

  lux_capability_t *instr_cap = lux_cap_derive_class(
      global_cap_manager, block_cap, "instr", block_cap->permissions);
  if (!instr_cap) {
    /* TODO: free instr */
    m3ApiReturn(WASM_CAP_INVALID_HANDLE);
  }
  instr_cap->aux = instr;

  m3ApiReturn(wasm_cap_table_insert(table, instr_cap));
}

/* fruity_compile(mod_h: i32) -> i32 (status) */
m3ApiRawFunction(host_fruity_compile) {
  m3ApiGetArg(wasm_cap_handle_t, mod_h) m3ApiReturnType(uint32_t)

      wasm_cap_table_t *table = (wasm_cap_table_t *)up->wasm.cap_table;
  lux_capability_t *mod_cap = wasm_cap_table_lookup(table, mod_h);
  if (!mod_cap || !mod_cap->aux)
    m3ApiReturn(1); /* Error */

  fruity_module_t *mod = (fruity_module_t *)mod_cap->aux;

  /* Verification Fast-Path:
   * 1. CFG analysis
   * 2. White Balance verification
   * 3. Transaction nesting verification
   */

  if (fruity_module_verify(mod) != 0) {
    print("fruity: verification failed for module %s\n", mod->name);
    m3ApiReturn(2); /* Verification error */
  }

  print("fruity: verified and executed IR module '%s' for pid=%lu\n", mod->name,
        up->pid);

  m3ApiReturn(0); /* Success */
}

/* ========== Linking Function ========== */

M3Result LinkFruity(IM3Module module) {
  M3Result result = m3Err_none;
  const char *ns = "fruity";

  result = m3_LinkRawFunction(module, ns, "module_create", "i(ii)",
                              &host_fruity_module_create);
  if (result && result != m3Err_functionLookupFailed)
    return result;

  result = m3_LinkRawFunction(module, ns, "module_add_function", "i(iiii)",
                              &host_fruity_module_add_function);
  if (result && result != m3Err_functionLookupFailed)
    return result;

  result = m3_LinkRawFunction(module, ns, "function_add_block", "i(i)",
                              &host_fruity_function_add_block);
  if (result && result != m3Err_functionLookupFailed)
    return result;

  result = m3_LinkRawFunction(module, ns, "block_add_instr", "i(ii)",
                              &host_fruity_block_add_instr);
  if (result && result != m3Err_functionLookupFailed)
    return result;

  result =
      m3_LinkRawFunction(module, ns, "compile", "i(i)", &host_fruity_compile);
  if (result && result != m3Err_functionLookupFailed)
    return result;

  return m3Err_none;
}
