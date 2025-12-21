/* qbe_buffer.h - Pebble-backed buffer for building QBE IL text
 *
 * Uses Pebble Blue tokens for memory allocation, supporting:
 * - Dynamic growth like C# List<T>
 * - Budget tracking per process
 * - Graceful failure when budget exhausted
 */

#ifndef QBE_BUFFER_H
#define QBE_BUFFER_H

#ifdef USERSPACE_TEST
#include <stddef.h>
#include <stdint.h>
typedef unsigned long usize;
typedef unsigned long ulong;
/* Mock PebbleBlue for userspace testing */
typedef struct PebbleBlue {
  void *blue_data;
  ulong blue_size;
} PebbleBlue;
#else
#include "../include/pebble.h"
#include "portlib.h"
#include "u.h"
#endif

/* Page size for buffer segments */
#define QBE_BUFFER_PAGE_SIZE 4096

/* Buffer page - one Blue token per page */
typedef struct QBEBufferPage {
  PebbleBlue *blue;           /* Blue token for this page */
  char *data;                 /* Pointer to blue_data (convenience) */
  usize used;                 /* Bytes used in this page */
  struct QBEBufferPage *next; /* Next page in chain */
} QBEBufferPage;

/* Multi-page buffer with Pebble backing */
typedef struct QBEBuffer {
  QBEBufferPage *head; /* First page */
  QBEBufferPage *tail; /* Current write page */
  usize total_len;     /* Total bytes across all pages */
  int page_count;      /* Number of pages allocated */
} QBEBuffer;

/* Initialize buffer - allocates first Blue page */
int qbe_buffer_init(QBEBuffer *buf);

/* Free buffer - releases all Blue tokens */
void qbe_buffer_free(QBEBuffer *buf);

/* Append formatted string to buffer (may grow automatically) */
int qbe_buffer_printf(QBEBuffer *buf, const char *fmt, ...);

/* Get total length of data in buffer */
usize qbe_buffer_len(QBEBuffer *buf);

/* Flatten buffer linked list into a single contiguous block (Pebble Blue
 * allocation) */
PebbleBlue *qbe_buffer_flatten(QBEBuffer *buf, usize *out_len);

/* Legacy API - for compatibility during transition */
char *qbe_buffer_data(QBEBuffer *buf);

#endif /* QBE_BUFFER_H */
