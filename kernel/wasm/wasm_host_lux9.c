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

/*@
  @ assigns \nothing;
  @*/
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

/* ========== BigInt Host Functions (Symbolic Math) ========== */
/* ========== Ring Buffer IPC Functions ========== */

/*@
  @ requires \valid(runtime);
  @ requires \valid(up);
  @ assigns \nothing;
  @ ensures \result == 0 || \result > 0;
  @ behavior has_exchange:
  @   assumes up != \null && up->exchange_channel != \null;
  @   ensures \result == (uint64_t)up->p9uaddr;
  @ behavior has_p9page:
  @   assumes up != \null && up->exchange_channel == \null && up->p9page != \null;
  @   ensures \result == (uint64_t)up->p9uaddr;
  @ behavior no_page:
  @   assumes up == \null || (up->exchange_channel == \null && up->p9page == \null);
  @   ensures \result == 0;
  @ complete behaviors;
  @ disjoint behaviors;
  @*/
/* lux9_ring_attach() -> i64 (exchange page address or 0 on failure) */
m3ApiRawFunction(host_lux9_ring_attach) {
  m3ApiReturnType(uint64_t)

  Proc *p = up;
  if (!p || !p->exchange_channel) {
    /* Exchange channel not initialized, attempt to allocate */
    /* For now, return p9page if available as legacy fallback */
    if (p && p->p9page) {
      m3ApiReturn((uint64_t)p->p9uaddr);
    }
    m3ApiReturn(0);
  }

  /* Return user VA of exchange page */
  m3ApiReturn((uint64_t)p->p9uaddr);
}

/*@
  @ requires \valid(runtime);
  @ requires \valid(_mem);
  @ requires \valid(up);
  @ requires buf_ptr + max_len <= m3_GetMemorySize(runtime);
  @ assigns ((char*)_mem)[buf_ptr .. buf_ptr + max_len - 1];
  @ ensures \result >= -1;
  @ ensures \result <= max_len;
  @ behavior out_of_bounds:
  @   assumes buf_ptr + max_len > m3_GetMemorySize(runtime);
  @   assigns \nothing;
  @   // Traps, never returns
  @ behavior no_channel:
  @   assumes buf_ptr + max_len <= m3_GetMemorySize(runtime);
  @   assumes up == \null || up->exchange_channel == \null;
  @   ensures \result == -1;
  @ behavior success:
  @   assumes buf_ptr + max_len <= m3_GetMemorySize(runtime);
  @   assumes up != \null && up->exchange_channel != \null;
  @   ensures \result >= 0 && \result <= max_len;
  @ complete behaviors;
  @ disjoint behaviors;
  @*/
/* lux9_ring_read(buf_ptr: i32, max_len: i32) -> i32 (bytes read, -1 on error) */
m3ApiRawFunction(host_lux9_ring_read) {
  m3ApiGetArg(uint32_t, buf_ptr)
  m3ApiGetArg(uint32_t, max_len)
  m3ApiReturnType(int32_t)

  if (!wasm_ptr_in_bounds(runtime, buf_ptr, max_len)) {
    m3ApiTrap(m3Err_trapOutOfBoundsMemoryAccess);
  }

  Proc *p = up;
  if (!p || !p->exchange_channel) {
    m3ApiReturn(-1);
  }

  /* Stub: Real implementation would read from ring buffer in exchange_channel */
  /* For now, return 0 (no data available) */
  m3ApiReturn(0);
}

/*@
  @ requires \valid(runtime);
  @ requires \valid_read(_mem);
  @ requires \valid(up);
  @ requires buf_ptr + len <= m3_GetMemorySize(runtime);
  @ assigns \nothing;
  @ ensures \result >= -1;
  @ ensures \result <= len;
  @ behavior out_of_bounds:
  @   assumes buf_ptr + len > m3_GetMemorySize(runtime);
  @   assigns \nothing;
  @   // Traps, never returns
  @ behavior no_channel:
  @   assumes buf_ptr + len <= m3_GetMemorySize(runtime);
  @   assumes up == \null || up->exchange_channel == \null;
  @   ensures \result == -1;
  @ behavior success:
  @   assumes buf_ptr + len <= m3_GetMemorySize(runtime);
  @   assumes up != \null && up->exchange_channel != \null;
  @   ensures \result == len;
  @ complete behaviors;
  @ disjoint behaviors;
  @*/
/* lux9_ring_write(buf_ptr: i32, len: i32) -> i32 (bytes written, -1 on error) */
m3ApiRawFunction(host_lux9_ring_write) {
  m3ApiGetArg(uint32_t, buf_ptr)
  m3ApiGetArg(uint32_t, len)
  m3ApiReturnType(int32_t)

  if (!wasm_ptr_in_bounds(runtime, buf_ptr, len)) {
    m3ApiTrap(m3Err_trapOutOfBoundsMemoryAccess);
  }

  Proc *p = up;
  if (!p || !p->exchange_channel) {
    m3ApiReturn(-1);
  }

  /* Stub: Real implementation would write to ring buffer in exchange_channel */
  /* For now, return len (claim success) */
  m3ApiReturn((int32_t)len);
}

#include "../symbolic/mini-gmp.h"

/* Opaque Handle System for BigInts */
typedef struct BigIntHandle {
  u32int id;
  mpz_t val;
  struct BigIntHandle *next;
} BigIntHandle;

static BigIntHandle *bigint_handles = nil;
static u32int next_bigint_id = 1;

static BigIntHandle *get_bigint(u32int id) {
  BigIntHandle *h;
  for (h = bigint_handles; h; h = h->next)
    if (h->id == id)
      return h;
  return nil;
}

/* lux9_bigint_init(str_ptr: i32, str_len: i32, base: i32) -> i64 (handle) */
m3ApiRawFunction(host_lux9_bigint_init) {
  m3ApiGetArg(uint32_t, ptr) m3ApiGetArg(uint32_t, len)
      m3ApiGetArg(int32_t, base) m3ApiReturnType(uint64_t)

          if (len > 0 && !wasm_ptr_in_bounds(runtime, ptr, len)) {
    m3ApiTrap(m3Err_trapOutOfBoundsMemoryAccess);
  }

  BigIntHandle *h = malloc(sizeof(BigIntHandle));
  if (!h)
    m3ApiReturn(0);

  h->id = next_bigint_id++;
  h->next = bigint_handles;
  bigint_handles = h;

  mpz_init(h->val);

  if (len > 0) {
    char *str = (char *)m3ApiOffsetToPtr(ptr);
    char *buf = malloc(len + 1);
    memmove(buf, str, len);
    buf[len] = 0;
    mpz_set_str(h->val, buf, base);
    free(buf);
  }

  m3ApiReturn(h->id);
}

/* lux9_bigint_free(handle: i64) -> void */
m3ApiRawFunction(host_lux9_bigint_free) {
  m3ApiGetArg(uint64_t, id)

      BigIntHandle **pp,
      *p;
  for (pp = &bigint_handles; (p = *pp); pp = &p->next) {
    if (p->id == (u32int)id) {
      *pp = p->next;
      mpz_clear(p->val);
      free(p);
      break;
    }
  }
  m3ApiSuccess();
}

/* lux9_bigint_add(h_res: i64, h_a: i64, h_b: i64) -> void */
m3ApiRawFunction(host_lux9_bigint_add) {
  m3ApiGetArg(uint64_t, id_res) m3ApiGetArg(uint64_t, id_a)
      m3ApiGetArg(uint64_t, id_b)

          BigIntHandle *res = get_bigint((u32int)id_res);
  BigIntHandle *a = get_bigint((u32int)id_a);
  BigIntHandle *b = get_bigint((u32int)id_b);

  if (res && a && b) {
    mpz_add(res->val, a->val, b->val);
  }
  m3ApiSuccess();
}

/* lux9_bigint_mul(h_res: i64, h_a: i64, h_b: i64) -> void */
m3ApiRawFunction(host_lux9_bigint_mul) {
  m3ApiGetArg(uint64_t, id_res) m3ApiGetArg(uint64_t, id_a)
      m3ApiGetArg(uint64_t, id_b)

          BigIntHandle *res = get_bigint((u32int)id_res);
  BigIntHandle *a = get_bigint((u32int)id_a);
  BigIntHandle *b = get_bigint((u32int)id_b);

  if (res && a && b) {
    mpz_mul(res->val, a->val, b->val);
  }
  m3ApiSuccess();
}

/* lux9_bigint_tostring(handle: i64, ptr: i32, len: i32, base: i32) -> i32
 * (written) */
m3ApiRawFunction(host_lux9_bigint_tostring) {
  m3ApiGetArg(uint64_t, id) m3ApiGetArg(uint32_t, ptr)
      m3ApiGetArg(uint32_t, len) m3ApiGetArg(int32_t, base)
          m3ApiReturnType(uint32_t)

              if (!wasm_ptr_in_bounds(runtime, ptr, len)) {
    m3ApiTrap(m3Err_trapOutOfBoundsMemoryAccess);
  }

  BigIntHandle *h = get_bigint((u32int)id);
  if (!h)
    m3ApiReturn(0);

  /* Get string representation kernel-side first */
  char *str = mpz_get_str(nil, base, h->val);
  if (!str)
    m3ApiReturn(0);

  int slen = strlen(str);
  int to_copy = (slen < len) ? slen : len;

  char *out_ptr = (char *)m3ApiOffsetToPtr(ptr);
  memmove(out_ptr, str, to_copy);
  if (to_copy < len)
    out_ptr[to_copy] = 0; // Null terminate if room

  /* mpz_get_str allocates with default allocator (kernel malloc), free it */
  free(str);

  m3ApiReturn(to_copy);
}

/* ========== Linking Function ========== */

/*@
  @ assigns \nothing;
  @*/
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

  /* Ring Buffer IPC */
  result = m3_LinkRawFunction(module, ns, "ring_attach", "I()",
                              &host_lux9_ring_attach);
  if (result && result != m3Err_functionLookupFailed)
    return result;

  result = m3_LinkRawFunction(module, ns, "ring_read", "i(ii)",
                              &host_lux9_ring_read);
  if (result && result != m3Err_functionLookupFailed)
    return result;

  result = m3_LinkRawFunction(module, ns, "ring_write", "i(ii)",
                              &host_lux9_ring_write);
  if (result && result != m3Err_functionLookupFailed)
    return result;

  /* BigInt */
  result = m3_LinkRawFunction(module, ns, "bigint_init", "I(iii)",
                              &host_lux9_bigint_init);
  if (result && result != m3Err_functionLookupFailed)
    return result;

  result = m3_LinkRawFunction(module, ns, "bigint_free", "v(I)",
                              &host_lux9_bigint_free);
  if (result && result != m3Err_functionLookupFailed)
    return result;

  result = m3_LinkRawFunction(module, ns, "bigint_add", "v(III)",
                              &host_lux9_bigint_add);
  if (result && result != m3Err_functionLookupFailed)
    return result;

  result = m3_LinkRawFunction(module, ns, "bigint_mul", "v(III)",
                              &host_lux9_bigint_mul);
  if (result && result != m3Err_functionLookupFailed)
    return result;

  result = m3_LinkRawFunction(module, ns, "bigint_tostring", "i(Iiii)",
                              &host_lux9_bigint_tostring);
  if (result && result != m3Err_functionLookupFailed)
    return result;

  return m3Err_none;
}
