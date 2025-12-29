/* wasm_runtime.h - Isolated WASM3 Runtime Interface
 *
 * Layer 1 API for WASM execution.
 * Accessible ONLY via Tsyscall messages.
 */

#pragma once

#include "../include/u.h"
#include "../include/fcall.h"

/* Syscall numbers for WASM operations */
#define SYS_WASM_COMPILE  100  /* Compile WASM module */
#define SYS_WASM_EXECUTE  101  /* Execute WASM function */
#define SYS_WASM_DESTROY  102  /* Destroy WASM instance */

/* Capability permissions for WASM */
#define PERM_WASM_COMPILE  (1 << 16)  /* Can compile WASM modules */
#define PERM_WASM_EXECUTE  (1 << 17)  /* Can execute WASM functions */

/* Runtime initialization (called at boot) */
void wasm_runtime_init(void);

/* Tsyscall handlers (called from 9P router) */
int sys_wasm_compile(Fcall *tx, Fcall *rx);
int sys_wasm_execute(Fcall *tx, Fcall *rx);
int sys_wasm_destroy(Fcall *tx, Fcall *rx);

/* Statistics */
void wasm_runtime_stats(void);
