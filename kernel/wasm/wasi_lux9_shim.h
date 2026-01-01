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
  int backend;           /* WASI backend selector */
  char *base_path;       /* Preopen/base path for dirfds */
  u64int offset; /* Current offset */
} wasi_fd_entry_t;

/* WASI Context for a Process */
#define WASI_MAX_FDS 64
typedef struct {
  wasi_fd_entry_t fds[WASI_MAX_FDS];
  u32int exit_code;
  int argc;
  char **argv;
  int envc;
  char **envv;
} wasi_context_t;

/* WASI allowlist flags */
#define WASI_ALLOW_ARGS 0x00000001u
#define WASI_ALLOW_CLOCK 0x00000002u
#define WASI_ALLOW_RANDOM 0x00000004u
#define WASI_ALLOW_FD 0x00000008u
#define WASI_ALLOW_PATH 0x00000010u
#define WASI_ALLOW_DIR 0x00000020u
#define WASI_ALLOW_PROC 0x00000040u
#define WASI_ALLOW_POLL 0x00000080u
#define WASI_ALLOW_SOCK 0x00000100u

#define WASI_ALLOW_DEFAULT                                                     \
  (WASI_ALLOW_ARGS | WASI_ALLOW_CLOCK | WASI_ALLOW_RANDOM | WASI_ALLOW_FD |    \
   WASI_ALLOW_PATH | WASI_ALLOW_DIR | WASI_ALLOW_PROC | WASI_ALLOW_POLL)

/* Initialization */
void wasi_lux9_init_context(wasi_context_t *ctx, Proc *p);
void wasi_lux9_destroy_context(wasi_context_t *ctx);

/* Link WASI functions to module */
M3Result LinkWasi(IM3Module module, u32int allow_mask);

#endif // WASI_LUX9_SHIM_H
