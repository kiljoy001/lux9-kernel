/* wasm_buffer.h - Simple buffer for emitting WASM binary format
 *
 * Provides utilities for:
 * - Dynamic buffer growth (using kernel xalloc/xrealloc if available, or simple
 * logic)
 * - LEB128 encoding (signed/unsigned)
 * - String/Vector writing
 * - Section management
 */

#ifndef WASM_BUFFER_H
#define WASM_BUFFER_H

#ifdef USERSPACE_TEST
/* Use fruity_standalone.h for types in userspace mode */
#include "../fruity/fruity_standalone.h"
#else
#include "../../include/portlib.h"
#include "../../include/u.h"
#endif

typedef struct wasm_buffer {
  u8int *data;
  ulong size;
  ulong capacity;
  int error; /* Non-zero if allocation failed */
} wasm_buffer_t;

/* Buffer management */
void wasm_buf_init(wasm_buffer_t *buf, ulong initial_cap);
void wasm_buf_free(wasm_buffer_t *buf);
void wasm_buf_check_cap(wasm_buffer_t *buf, ulong needed);

/* Primitive emitters */
void wasm_emit_u8(wasm_buffer_t *buf, u8int val);
void wasm_emit_u32(wasm_buffer_t *buf,
                   u32int val); /* Little endian fixed size */
void wasm_emit_u64(wasm_buffer_t *buf,
                   u64int val); /* Little endian fixed size */
void wasm_emit_bytes(wasm_buffer_t *buf, const void *data, ulong len);

/* LEB128 emitters */
void wasm_emit_uleb128(wasm_buffer_t *buf, u64int val);
void wasm_emit_sleb128(wasm_buffer_t *buf, s64int val);

/* High-level structures */
void wasm_emit_vec_header(wasm_buffer_t *buf, u32int count);
void wasm_emit_name(wasm_buffer_t *buf,
                    const char *name); /* length-prefixed utf8 */

#endif
