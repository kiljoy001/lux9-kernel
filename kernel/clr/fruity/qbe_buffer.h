/* qbe_buffer.h - Simple buffer for building QBE IL text */

#ifndef QBE_BUFFER_H
#define QBE_BUFFER_H

#ifdef USERSPACE_TEST
#include <stddef.h>
#include <stdint.h>
typedef unsigned long usize;
#else
#include "portlib.h"
#include "u.h"
#endif

/* Simple dynamically growing buffer for text */
typedef struct QBEBuffer {
  char *data;
  usize len;      /* Current length */
  usize capacity; /* Allocated capacity */
} QBEBuffer;

/* Initialize buffer */
int qbe_buffer_init(QBEBuffer *buf);

/* Free buffer */
void qbe_buffer_free(QBEBuffer *buf);

/* Append formatted string to buffer */
int qbe_buffer_printf(QBEBuffer *buf, const char *fmt, ...);

/* Get final data */
char *qbe_buffer_data(QBEBuffer *buf);
usize qbe_buffer_len(QBEBuffer *buf);

#endif /* QBE_BUFFER_H */
