/* wasm_runtime.h - Isolated WASM3 Runtime Interface
 *
 * Layer 1 API for WASM execution.
 * Accessible ONLY via Tsyscall messages.
 */

#pragma once

#include "../include/fcall.h" // For Fcall and SYS_WASM_* defines
#include "../include/u.h"
typedef struct Proc Proc;

/* Capability permissions for WASM */
#define PERM_WASM_COMPILE (1 << 16) /* Can compile WASM modules */
#define PERM_WASM_EXECUTE (1 << 17) /* Can execute WASM functions */
#define PERM_WASM_NET (1 << 18)     /* Allow WASI sockets/poll */
#define PERM_WASM_POSIX (1 << 19)   /* Allow /wasm/posix preopen */
#define PERM_WASM_FRUITY (1 << 20)  /* Allow Fruity IR generation */

/*@
  @ logic integer PERM_WASM_FRUITY = 1048576;
  @*/

/* Runtime initialization (called at boot) */
void wasm_runtime_init(void);

/* Tsyscall handlers (called from 9P router) */
int sys_wasm_compile(Fcall *tx, Fcall *rx);
int sys_wasm_execute(Fcall *tx, Fcall *rx);
int sys_wasm_destroy(Fcall *tx, Fcall *rx);
void wasm_runtime_cleanup_process(Proc *p);

/* Statistics */
void wasm_runtime_stats(void);
