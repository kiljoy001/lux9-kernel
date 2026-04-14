/* qbe_buffer.c - Simple buffer implementation for QBE IL text */

#include <u.h>
#include "portlib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"

#include "qbe_buffer.h"

#define INITIAL_CAPACITY 4096
#define GROWTH_FACTOR 2

void qbe_buffer_init(QBEBuffer *buf) {
  buf->data = mallocz(INITIAL_CAPACITY, 0);
  buf->len = 0;
  buf->capacity = INITIAL_CAPACITY;
}

void qbe_buffer_free(QBEBuffer *buf) {
  if (buf->data)
    free(buf->data);
  buf->data = nil;
  buf->len = 0;
  buf->capacity = 0;
}

int qbe_buffer_printf(QBEBuffer *buf, const char *fmt, ...) {
  va_list ap;
  int needed;
  usize available;

  /* Check how much space we need */
  va_start(ap, fmt);
  needed = vsnprint(nil, 0, (char *)fmt, ap);
  va_end(ap);

  if (needed < 0)
    return -1;

  /* Grow buffer if needed */
  available = buf->capacity - buf->len;
  if ((usize)needed + 1 > available) {
    usize new_capacity = buf->capacity;
    char *new_data;

    while (new_capacity - buf->len < (usize)needed + 1)
      new_capacity *= GROWTH_FACTOR;

    new_data = mallocz(new_capacity, 0);
    if (new_data == nil)
      return -1;

    memmove(new_data, buf->data, buf->len);
    free(buf->data);
    buf->data = new_data;
    buf->capacity = new_capacity;
  }

  /* Actually write the data */
  va_start(ap, fmt);
  vsnprint(buf->data + buf->len, (int)(buf->capacity - buf->len), (char *)fmt,
           ap);
  va_end(ap);

  buf->len += (usize)needed;
  return needed;
}

char *qbe_buffer_data(QBEBuffer *buf) { return buf->data; }

usize qbe_buffer_len(QBEBuffer *buf) { return buf->len; }
