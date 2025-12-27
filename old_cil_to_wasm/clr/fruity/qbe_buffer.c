/* qbe_buffer.c - Pebble-backed buffer for QBE IL text
 *
 * Implements a dynamically growing buffer backed by Pebble Blue tokens.
 * Each page is a separate Blue allocation, allowing:
 * - No contiguous memory requirement
 * - Budget tracking per process
 * - Graceful failure when budget exhausted
 */

#ifdef USERSPACE_TEST
#include "qbe_buffer.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Mocks for userspace testing */
#define snprint snprintf
#define vsnprint vsnprintf
#define nil NULL
#define print printf

/* Mock Pebble functions */
static PebbleBlue *pebble_blue_alloc(ulong size) {
  PebbleBlue *b = malloc(sizeof(PebbleBlue));
  if (!b)
    return nil;
  b->blue_data = malloc(size);
  if (!b->blue_data) {
    free(b);
    return nil;
  }
  b->blue_size = size;
  memset(b->blue_data, 0, size);
  return b;
}
static int pebble_blue_free(PebbleBlue *b) {
  if (b) {
    free(b->blue_data);
    free(b);
  }
  return 0;
}

#else
/* Kernel includes */
#include "dat.h"
#include "fns.h"
#include "mem.h"
#include "portlib.h"
#include "u.h"

#include "qbe_buffer.h"
#endif

/* Allocate a new buffer page using Pebble Blue */
static QBEBufferPage *alloc_page(void) {
  QBEBufferPage *page;
  PebbleBlue *blue;

  /* Allocate Blue token for the page data */
  blue = pebble_blue_alloc(QBE_BUFFER_PAGE_SIZE);
  if (blue == nil)
    return nil;

  /* Allocate page metadata (small, use kernel allocator) */
#ifdef USERSPACE_TEST
  page = malloc(sizeof(QBEBufferPage));
#else
  page = mallocz(sizeof(QBEBufferPage), 1);
#endif
  if (page == nil) {
    pebble_blue_free(blue);
    return nil;
  }

  page->blue = blue;
  page->data = (char *)blue->blue_data;
  page->used = 0;
  page->next = nil;

  return page;
}

/* Free a single page */
static void free_page(QBEBufferPage *page) {
  if (page == nil)
    return;
  if (page->blue)
    pebble_blue_free(page->blue);
#ifdef USERSPACE_TEST
  free(page);
#else
  free(page);
#endif
}

/* Initialize buffer - allocates first Blue page */
int qbe_buffer_init(QBEBuffer *buf) {
  QBEBufferPage *first_page;

  if (buf == nil)
    return -1;

  first_page = alloc_page();
  if (first_page == nil) {
    buf->head = nil;
    buf->tail = nil;
    buf->total_len = 0;
    buf->page_count = 0;
    return -1;
  }

  buf->head = first_page;
  buf->tail = first_page;
  buf->total_len = 0;
  buf->page_count = 1;

  return 0;
}

/* Free buffer - releases all Blue tokens */
void qbe_buffer_free(QBEBuffer *buf) {
  QBEBufferPage *page, *next;

  if (buf == nil)
    return;

  page = buf->head;
  while (page != nil) {
    next = page->next;
    free_page(page);
    page = next;
  }

  buf->head = nil;
  buf->tail = nil;
  buf->total_len = 0;
  buf->page_count = 0;
}

/* Grow buffer by adding a new page */
static int qbe_buffer_grow(QBEBuffer *buf) {
  QBEBufferPage *new_page;

  new_page = alloc_page();
  if (new_page == nil)
    return -1;

  /* Append to chain */
  if (buf->tail)
    buf->tail->next = new_page;
  else
    buf->head = new_page;

  buf->tail = new_page;
  buf->page_count++;

  return 0;
}

/* Append formatted string to buffer */
int qbe_buffer_printf(QBEBuffer *buf, const char *fmt, ...) {
  va_list ap;
  int n;
  usize available;
  char temp[1024]; /* Temp buffer for formatting */

  if (buf == nil || buf->tail == nil)
    return -1;

  /* Format to temp buffer first to know length */
  va_start(ap, fmt);
  n = vsnprint(temp, sizeof(temp), (char *)fmt, ap);
  va_end(ap);

  if (n < 0)
    return -1;

  /* Check if current page has space */
  available = QBE_BUFFER_PAGE_SIZE - buf->tail->used;

  if ((usize)n >= available) {
    /* Need to span pages or grow */
    usize written = 0;
    char *src = temp;

    while (written < (usize)n) {
      available = QBE_BUFFER_PAGE_SIZE - buf->tail->used;

      if (available == 0) {
        /* Current page full, grow */
        if (qbe_buffer_grow(buf) < 0)
          return -1;
        available = QBE_BUFFER_PAGE_SIZE;
      }

      /* Copy as much as fits in current page */
      usize to_copy = (usize)n - written;
      if (to_copy > available)
        to_copy = available;

      memmove(buf->tail->data + buf->tail->used, src, to_copy);
      buf->tail->used += to_copy;
      buf->total_len += to_copy;
      written += to_copy;
      src += to_copy;
    }
  } else {
    /* Fits in current page */
    memmove(buf->tail->data + buf->tail->used, temp, n);
    buf->tail->used += n;
    buf->total_len += n;
  }

  return n;
}

/* Get total length */
usize qbe_buffer_len(QBEBuffer *buf) {
  if (buf == nil)
    return 0;
  return buf->total_len;
}

/* Flatten buffer to contiguous allocation for JIT input */
PebbleBlue *qbe_buffer_flatten(QBEBuffer *buf, usize *out_len) {
  PebbleBlue *flat_blue;
  char *flat_data;
  char *dst;
  QBEBufferPage *page;

  if (buf == nil || buf->total_len == 0) {
    if (out_len)
      *out_len = 0;
    return nil;
  }

  /* Allocate contiguous Blue buffer */
  flat_blue =
      pebble_blue_alloc(buf->total_len + 1); /* +1 for null terminator */
  if (flat_blue == nil) {
    if (out_len)
      *out_len = 0;
    return nil;
  }

  flat_data = (char *)flat_blue->blue_data;
  dst = flat_data;

  /* Copy all pages */
  for (page = buf->head; page != nil; page = page->next) {
    if (page->used > 0) {
      memmove(dst, page->data, page->used);
      dst += page->used;
    }
  }

  /* Null terminate */
  *dst = '\0';

  if (out_len)
    *out_len = buf->total_len;

  return flat_blue;
}

/* Legacy API - flatten and return data pointer
 * WARNING: This allocates memory that the caller must track!
 * For new code, use qbe_buffer_flatten() directly.
 */
char *qbe_buffer_data(QBEBuffer *buf) {
  usize len;
  return qbe_buffer_flatten(buf, &len);
}
