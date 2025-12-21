/* wasm_buffer.c - WASM Binary Format Emitter implementation */

#include "wasm_buffer.h"

#ifdef USERSPACE_TEST
#include <stdlib.h>
#include <string.h>
extern void *xalloc(size_t size);
extern void xfree(void *ptr);
#else
#include "../../include/fns.h"
#include "../../include/lib.h" /* For memmove, memset */
#endif

/* Buffer management */

void wasm_buf_init(wasm_buffer_t *buf, ulong initial_cap) {
  if (initial_cap == 0)
    initial_cap = 64;
  buf->data = xalloc(initial_cap);
  if (!buf->data) {
    buf->error = 1;
    buf->size = 0;
    buf->capacity = 0;
    return;
  }
  buf->size = 0;
  buf->capacity = initial_cap;
  buf->error = 0;
}

void wasm_buf_free(wasm_buffer_t *buf) {
  if (buf->data) {
    xfree(buf->data);
    buf->data = nil;
  }
  buf->size = 0;
  buf->capacity = 0;
}

void wasm_buf_check_cap(wasm_buffer_t *buf, ulong needed) {
  if (buf->error)
    return;

  if (buf->size + needed > buf->capacity) {
    ulong new_cap = buf->capacity * 2;
    if (new_cap < buf->size + needed)
      new_cap = buf->size + needed + 64;

    void *new_data = realloc(buf->data, new_cap);
    if (!new_data) {
      buf->error = 1;
      return;
    }
    buf->data = new_data;
    buf->capacity = new_cap;
  }
}

/* Primitive emitters */

void wasm_emit_u8(wasm_buffer_t *buf, u8int val) {
  wasm_buf_check_cap(buf, 1);
  if (buf->error)
    return;
  buf->data[buf->size++] = val;
}

void wasm_emit_u32(wasm_buffer_t *buf, u32int val) {
  wasm_buf_check_cap(buf, 4);
  if (buf->error)
    return;
  /* Little endian */
  buf->data[buf->size++] = val & 0xFF;
  buf->data[buf->size++] = (val >> 8) & 0xFF;
  buf->data[buf->size++] = (val >> 16) & 0xFF;
  buf->data[buf->size++] = (val >> 24) & 0xFF;
}

void wasm_emit_u64(wasm_buffer_t *buf, u64int val) {
  wasm_buf_check_cap(buf, 8);
  if (buf->error)
    return;
  /* Little endian */
  for (int i = 0; i < 8; i++) {
    buf->data[buf->size++] = (val >> (i * 8)) & 0xFF;
  }
}

void wasm_emit_bytes(wasm_buffer_t *buf, const void *data, ulong len) {
  wasm_buf_check_cap(buf, len);
  if (buf->error)
    return;
  memmove(buf->data + buf->size, data, len);
  buf->size += len;
}

/* LEB128 emitters */

void wasm_emit_uleb128(wasm_buffer_t *buf, u64int val) {
  do {
    u8int byte = val & 0x7F;
    val >>= 7;
    if (val != 0) {
      byte |= 0x80;
    }
    wasm_emit_u8(buf, byte);
  } while (val != 0);
}

void wasm_emit_sleb128(wasm_buffer_t *buf, s64int val) {
  int more = 1;
  while (more) {
    u8int byte = val & 0x7F;
    val >>= 7;
    /* Check sign bit of byte (0x40) against remaining val */
    /* If val is 0 and sign bit is 0, we are done */
    /* If val is -1 and sign bit is 1, we are done */
    if ((val == 0 && (byte & 0x40) == 0) || (val == -1 && (byte & 0x40) != 0)) {
      more = 0;
    } else {
      byte |= 0x80;
    }
    wasm_emit_u8(buf, byte);
  }
}

/* High-level structures */

void wasm_emit_vec_header(wasm_buffer_t *buf, u32int count) {
  wasm_emit_uleb128(buf, count);
}

void wasm_emit_name(wasm_buffer_t *buf, const char *name) {
  ulong len = 0;
  const char *p = name;
  while (*p++)
    len++;

  wasm_emit_uleb128(buf, len);
  wasm_emit_bytes(buf, name, len);
}
