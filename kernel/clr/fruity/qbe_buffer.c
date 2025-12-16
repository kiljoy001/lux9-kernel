/* qbe_buffer.c - Simple buffer implementation for QBE IL text */

#include "u.h"
#include "portlib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"

#include "qbe_buffer.h"

#define INITIAL_CAPACITY 4096
#define GROWTH_FACTOR 2

int qbe_buffer_init(QBEBuffer *buf) {
  extern void uartputs(char *, int);
  char debug_buf[128];

  snprint(debug_buf, sizeof(debug_buf), "DEBUG: qbe_buffer_init calling mallocz(%d)\n", INITIAL_CAPACITY);
  uartputs(debug_buf, strlen(debug_buf));

  buf->data = mallocz(INITIAL_CAPACITY, 0);

  snprint(debug_buf, sizeof(debug_buf), "DEBUG: qbe_buffer_init data=%p len=%d cap=%d\n", buf->data, 0, INITIAL_CAPACITY);
  uartputs(debug_buf, strlen(debug_buf));

  if (buf->data == nil) {
    buf->len = 0;
    buf->capacity = 0;
    return -1;
  }

  buf->len = 0;
  buf->capacity = INITIAL_CAPACITY;
  return 0;
}

void qbe_buffer_free(QBEBuffer *buf) {
  if (buf->data)
    free(buf->data);
  buf->data = nil;
  buf->len = 0;
  buf->capacity = 0;
}

int qbe_buffer_printf(QBEBuffer *buf, const char *fmt, ...) {
  extern void uartputs(char *, int);
  char debug_buf[128];
  va_list ap;
  int n;
  usize available;

  /* Ensure we have at least 1KB of space to start */
  available = buf->capacity - buf->len;
  if (available < 1024) {
      usize new_capacity = buf->capacity ? buf->capacity * GROWTH_FACTOR : INITIAL_CAPACITY;
      if (new_capacity < buf->capacity + 1024) new_capacity = buf->capacity + 1024;
      
      char *new_data = mallocz(new_capacity, 0);
      if (new_data == nil) return -1;
      
      if (buf->data) {
          memmove(new_data, buf->data, buf->len);
          free(buf->data);
      }
      buf->data = new_data;
      buf->capacity = new_capacity;
      available = buf->capacity - buf->len;
  }

  /* Write data */
  va_start(ap, fmt);
  n = vsnprint(buf->data + buf->len, (int)available, (char *)fmt, ap);
  va_end(ap);

  /* Check for error */
  if (n < 0) return -1;

  /* Check for truncation (if n >= available - 1, meaning it filled the buffer) */
  if ((usize)n >= available - 1) {
      /* Likely truncated. Grow and retry. */
      usize new_capacity = buf->capacity * 2;
      /* Ensure at least 4KB growth if doubling isn't enough */
      if (new_capacity < buf->capacity + 4096) new_capacity = buf->capacity + 4096;

      char *new_data = mallocz(new_capacity, 0);
      if (new_data == nil) return -1;
      
      memmove(new_data, buf->data, buf->len);
      free(buf->data);
      buf->data = new_data;
      buf->capacity = new_capacity;
      available = buf->capacity - buf->len;
      
      /* Retry print */
      va_start(ap, fmt);
      n = vsnprint(buf->data + buf->len, (int)available, (char *)fmt, ap);
      va_end(ap);
      
      if (n < 0) return -1;
  }

  buf->len += (usize)n;
  return n;
}

char *qbe_buffer_data(QBEBuffer *buf) { return buf->data; }

usize qbe_buffer_len(QBEBuffer *buf) { return buf->len; }
