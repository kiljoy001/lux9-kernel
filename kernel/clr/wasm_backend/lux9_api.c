/* lux9_api.c - Lux9 System Interface for WASM
 *
 * Implements the Host Functions (Imports) that WASM modules use to talk to the
 * kernel. The primary interface is 9P over Exchange Pages.
 */

#include "../../include/dat.h"
#include "../../include/error.h"
#include "../../include/fns.h"
#include "../../include/mem.h"
#include "../../include/portlib.h"
#include "../../include/u.h"
#include "../wasm_runtime/wasm3/m3_core.h"
#include "../wasm_runtime/wasm3/wasm3.h"

/* Error checking macro for WASM linking */
#define _(x)                                                                   \
  {                                                                            \
    M3Result _res = (x);                                                       \
    if (_res)                                                                  \
      return _res;                                                             \
  }

/* Forward declaration for 9P router hook */
extern long p9_route_message(int pid, void *msg, ulong len);

/*
 * lux9_send_9p(ptr: i32, len: i32) -> i32
 *
 * Copies 'len' bytes from WASM memory at 'ptr' to the process's Exchange Page.
 * Then calls the kernel 9P router.
 */
m3ApiRawFunction(lux9_send_9p) {
  m3ApiReturnType(uint32_t) m3ApiGetArgMem(u8int *, msg_ptr);
  m3ApiGetArg(u32int, msg_len);

  if (msg_len > 4096) { /* Exchange page size limit */
    m3ApiReturn(-1);
  }

  /* Check if process has an exchange page */
  if (!up->p9page) {
    /* Lazy allocation could happen here, or fail */
    m3ApiReturn(-2);
  }

  /* Copy from WASM memory to Kernel Exchange Page */
  /* Note: msg_ptr is already a pointer into the WASM linear memory */

  memmove(up->p9page, msg_ptr, msg_len);

  /* Route the message */
  long res = p9_route_message(up->pid, up->p9page, msg_len);

  m3ApiReturn((u32int)res);
}

/*
 * lux9_yield() -> void
 *
 * Yields the CPU.
 */
m3ApiRawFunction(lux9_yield) {
  m3ApiReturnType(void) sched();
  m3ApiSuccess();
}

/*
 * lux9_debug_print(ptr: i32, len: i32) -> void
 *
 * Prints to kernel console (kprint).
 */
m3ApiRawFunction(lux9_debug_print) {
  m3ApiReturnType(void) m3ApiGetArgMem(char *, str);
  m3ApiGetArg(u32int, len);

  if (len > 256)
    len = 256;

  char buf[257];
  memmove(buf, str, len);
  buf[len] = 0;

  print("%s", buf);

  m3ApiSuccess();
}

extern int clr_execute_assembly(void *dll_data, ulong dll_size);

static void kexec_trampoline(void *arg) {
  char *path = (char *)arg;
  Chan *c = nil;
  void *asm_data = nil;

  if (waserror()) {
    print("lux9_spawn: failed to spawn %s: %s\n", path, up->errstr);
    if (c)
      cclose(c);
    if (asm_data)
      free(asm_data);
    free(path); // Free the kstrdup'd path
    pexit("spawn failed", 1);
  }

  c = namec(path, Aopen, OEXEC, 0);

  // Get file size
  Dir *dir = dirchanstat(c);
  if (dir == nil)
    error(Eio);
  ulong fsize = dir->length;
  free(dir);

  // Read file
  asm_data = malloc(fsize);
  if (asm_data == nil)
    error(Enomem);

  devtab[c->type]->read(c, asm_data, fsize, 0);
  cclose(c);
  c = nil;

  // Execute
  print("lux9_spawn: executing %s\n", path);
  int ret = clr_execute_assembly(asm_data, fsize);

  print("lux9_spawn: %s exited with %d\n", path, ret);

  free(asm_data);
  free(path);
  pexit("child exit", 0);
}

/*
 * lux9_spawn(ptr: i32) -> i32
 * Spawns a new process executing the CLR assembly at 'ptr'.
 */
m3ApiRawFunction(lux9_spawn) {
  m3ApiReturnType(uint32_t) m3ApiGetArgMem(char *, path);

  // Copy path to kernel heap for the child
  char *kpath;
  kstrdup(&kpath, path); // Allocated in pool, needs freeing in child

  // kproc now returns int (pid)
  int pid = kproc("child", kexec_trampoline, kpath);
  m3ApiReturn((u32int)pid);
}

/*
 * lux9_sleep(ms: i32) -> void
 */
m3ApiRawFunction(lux9_sleep) {
  m3ApiReturnType(void) m3ApiGetArg(u32int, ms);

  if (ms > 0) {
    tsleep(&up->sleep, return0, 0, ms);
  }

  m3ApiSuccess();
}

/* Linker function to bind these to a module */
M3Result lux9_link_wasi(IM3Module module) {
  M3Result result = m3Err_none;

  _(m3_LinkRawFunction(module, "env", "lux9_send_9p", "i(ii)", &lux9_send_9p));
  _(m3_LinkRawFunction(module, "env", "lux9_yield", "v()", &lux9_yield));
  _(m3_LinkRawFunction(module, "env", "lux9_debug_print", "v(ii)",
                       &lux9_debug_print));
  _(m3_LinkRawFunction(module, "env", "lux9_spawn", "i(i)", &lux9_spawn));
  _(m3_LinkRawFunction(module, "env", "lux9_sleep", "v(i)", &lux9_sleep));

  return result;
}
