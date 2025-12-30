/* wasi_lux9_shim.h - WASI Snapshot Preview 1 to Lux9 9P Shim
 *
 * Maps WASI file descriptors to Lux9 9P FIDs and Capabilities.
 */

#ifndef WASI_LUX9_SHIM_H
#define WASI_LUX9_SHIM_H

#include "../include/dat.h"
#include "../include/portlib.h"
#include "../include/u.h"

/* Provide standard types required by wasm3.h */
typedef u8int uint8_t;
typedef u16int uint16_t;
typedef u32int uint32_t;
typedef u64int uint64_t;
typedef s32int int32_t;
typedef s64int int64_t;
typedef uintptr uintptr_t;

#include "wasm_runtime/wasm3/wasm3.h"

/* WASI File Descriptor Entry */
typedef struct {
  int is_open;
  u32int lux9_fid;       /* 9P FID (File ID) */
  u32int capability_idx; /* Index into capability table (if applicable) */
  u32int rights;         /* WASI rights */
  u32int rights_inheriting;
  int is_dir;
  u64int offset; /* Current offset */
} wasi_fd_entry_t;

/* WASI Context for a Process */
#define WASI_MAX_FDS 64
typedef struct {
  wasi_fd_entry_t fds[WASI_MAX_FDS];
  u32int exit_code;
} wasi_context_t;

/* Initialization */
void wasi_lux9_init_context(wasi_context_t *ctx);
void wasi_lux9_destroy_context(wasi_context_t *ctx);

/* Link WASI functions to module */
M3Result LinkWasi(IM3Module module);

#endif // WASI_LUX9_SHIM_H
