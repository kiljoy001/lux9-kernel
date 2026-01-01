/* wasm_host_lux9.c - Lux9 Kernel Host Functions for WASM
 *
 * Provides kernel-native host imports under the "lux9" namespace.
 * These functions work without a filesystem and provide direct kernel access.
 *
 * Import namespace: "lux9"
 * Functions:
 *   - print(ptr, len) -> void       : Print string to kernel console
 *   - getpid() -> i64               : Get current process ID
 *   - getticks() -> i64             : Get kernel tick count
 *   - meminfo() -> i64              : Get available Pebble budget
 *   - panic(ptr, len) -> void       : Kernel panic with message
 */

/* Include exactly like wasi_lux9_shim.c does */
#include "../include/dat.h"
#include "../include/fns.h"
#include "../include/portlib.h"
#include "../include/u.h"
#include "wasi_lux9_shim.h"

#ifndef nil
#define nil ((void *)0)
#endif

static int wasm_ptr_in_bounds(IM3Runtime runtime, uint32_t ptr, uint32_t len) {
  uint32_t memsize = m3_GetMemorySize(runtime);
  if (ptr > memsize)
    return 0;
  if (len > memsize - ptr)
    return 0;
  return 1;
}

/* ========== Host Function Implementations ========== */

/* lux9_print(ptr: i32, len: i32) -> void */
m3ApiRawFunction(host_lux9_print) {
  m3ApiGetArg(uint32_t, ptr) m3ApiGetArg(uint32_t, len)

  if (!wasm_ptr_in_bounds(runtime, ptr, len)) {
    m3ApiTrap(m3Err_trapOutOfBoundsMemoryAccess);
  }

  char *str = (char *)m3ApiOffsetToPtr(ptr);

  if (len > 1024)
    len = 1024;
  char buf[1028];
  memmove(buf, str, len);
  buf[len] = '\0';
  print("%s", buf);

  m3ApiSuccess();
}

/* lux9_getpid() -> i64 */
m3ApiRawFunction(host_lux9_getpid) {
  m3ApiReturnType(uint64_t)

      Proc *p = up;
  m3ApiReturn(p ? (uint64_t)p->pid : 0);
}

/* lux9_getticks() -> i64 */
m3ApiRawFunction(host_lux9_getticks) {
  m3ApiReturnType(uint64_t)

      m3ApiReturn((uint64_t)fastticks(nil));
}

/* lux9_meminfo() -> i64 */
m3ApiRawFunction(host_lux9_meminfo) {
  m3ApiReturnType(uint64_t)

      Proc *p = up;
  if (p && p->wasm.initialized) {
    m3ApiReturn((uint64_t)p->wasm.branch.local_colorless);
  }
  m3ApiReturn(0);
}

/* lux9_panic(ptr: i32, len: i32) -> void */
m3ApiRawFunction(host_lux9_panic) {
  m3ApiGetArg(uint32_t, ptr) m3ApiGetArg(uint32_t, len)

  if (!wasm_ptr_in_bounds(runtime, ptr, len)) {
    m3ApiTrap(m3Err_trapOutOfBoundsMemoryAccess);
  }

  char *str = (char *)m3ApiOffsetToPtr(ptr);

  if (len > 256)
    len = 256;
  char buf[260];
  memmove(buf, str, len);
  buf[len] = '\0';

  panic("WASM panic: %s", buf);

  m3ApiTrap(m3Err_trapUnreachable);
}

/* lux9_sleep(ms: i32) -> void */
m3ApiRawFunction(host_lux9_sleep) {
  m3ApiGetArg(uint32_t, ms)

      if (ms > 0 && ms < 60000) {
    tsleep(&up->sleep, return0, nil, ms);
  }

  m3ApiSuccess();
}

/* lux9_random() -> i64 */
m3ApiRawFunction(host_lux9_random) {
  m3ApiReturnType(uint64_t)

      u64int val = (u64int)fastticks(nil) ^ ((u64int)up->pid << 32);
  m3ApiReturn(val);
}

/* ========== Linking Function ========== */

M3Result LinkLux9(IM3Module module) {
  M3Result result = m3Err_none;
  const char *ns = "lux9";

  result = m3_LinkRawFunction(module, ns, "print", "v(ii)", &host_lux9_print);
  if (result && result != m3Err_functionLookupFailed)
    return result;

  result = m3_LinkRawFunction(module, ns, "getpid", "I()", &host_lux9_getpid);
  if (result && result != m3Err_functionLookupFailed)
    return result;

  result =
      m3_LinkRawFunction(module, ns, "getticks", "I()", &host_lux9_getticks);
  if (result && result != m3Err_functionLookupFailed)
    return result;

  result = m3_LinkRawFunction(module, ns, "meminfo", "I()", &host_lux9_meminfo);
  if (result && result != m3Err_functionLookupFailed)
    return result;

  result = m3_LinkRawFunction(module, ns, "panic", "v(ii)", &host_lux9_panic);
  if (result && result != m3Err_functionLookupFailed)
    return result;

  result = m3_LinkRawFunction(module, ns, "sleep", "v(i)", &host_lux9_sleep);
  if (result && result != m3Err_functionLookupFailed)
    return result;

  result = m3_LinkRawFunction(module, ns, "random", "I()", &host_lux9_random);
  if (result && result != m3Err_functionLookupFailed)
    return result;

  return m3Err_none;
}
