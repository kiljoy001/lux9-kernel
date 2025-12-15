/*
 * CLR Code Page System - Implementation
 *
 * Implements independent IL storage using exchange pages and borrow checker.
 */

/* Manual Plan 9 Types */
#define _U_H_
#define nil ((void *)0)
typedef unsigned char uchar;
typedef unsigned short ushort;
typedef unsigned int uint;
typedef unsigned long ulong;
typedef unsigned long long uvlong;
typedef long long vlong;
typedef unsigned long usize;
typedef unsigned long uintptr;
typedef unsigned char u8int;
typedef unsigned short u16int;
typedef unsigned int u32int;
typedef unsigned long long u64int;
typedef signed char s8int;
typedef signed short s16int;
typedef signed int s32int;
typedef signed long long s64int;
#define BY2PG 4096
#define USED(x)                                                                \
  if (x) {                                                                     \
  }

/* Struct Lock forward declaration since we use void* in handle for simple
 * include */
typedef struct Lock Lock;
struct Lock {
  int key;
};
static inline void lock(Lock *l) { (void)l; }
static inline void unlock(Lock *l) { (void)l; }

/* Memory/System imports */
extern void *mallocz(ulong size, int clr);
extern void *xalloc(ulong size);
extern void free(void *p);
extern void *memset(void *s, int c, ulong n);
extern void *memmove(void *dst, const void *src, ulong n);
extern int print(char *fmt, ...);
extern int pebble_black_alloc(ulong size, void *user_cap_out);
extern uintptr KADDR(ulong pa);

/* Pebble types (opaque) */
typedef struct PebbleBlack PebbleBlack;
struct PebbleBlack {
  ulong pa;
};
extern PebbleBlack pebble_lookup_black(void *pb_state, void *cap);
extern void *pebble_state(void);

/* Borrow checker imports */
typedef struct Proc Proc;
extern Proc *up;
extern int borrow_borrow_mut(Proc *owner, Proc *borrower, uintptr key);
extern int borrow_borrow_shared(Proc *owner, Proc *borrower, uintptr key);
extern int borrow_return_mut(Proc *borrower, uintptr key);
extern int borrow_return_shared(Proc *borrower, uintptr key);
extern int borrow_acquire(Proc *p, uintptr key);
extern int borrow_release(Proc *p, uintptr key);

#include "clr_codepage.h"

/* Global registry of code pages */
static struct {
  CodePageHandle *head;
  Lock lock;
  int initialized;
} cp_registry;

void clr_codepage_init(void) {
  memset(&cp_registry, 0, sizeof(cp_registry));
  cp_registry.initialized = 1;
  print("CLR: code page system initialized\n");
}

CodePageHandle *clr_codepage_create(u32int assembly_id) {
  CodePageHandle *h;
  CodePage *page;
  int err;

  if (!cp_registry.initialized)
    clr_codepage_init();

  h = mallocz(sizeof(CodePageHandle), 1);
  if (h == nil)
    return nil;

  h->cap = mallocz(128, 1); /* Space for capability (approx) */

  /* Allocate exchange page via Pebble */
  /* Using xalloc fallback for now until pebble headers are cleanly integrated
   */
  page = xalloc(BY2PG);
  if (page == nil) {
    free(h->cap);
    free(h);
    return nil;
  }

  h->page = page;
  memset(page, 0, BY2PG);

  /* Initialize Page Header */
  page->magic = CODEPAGE_MAGIC;
  page->assembly_id = assembly_id;
  page->method_count = 0;
  page->total_il_size = 0;

  /* Generate borrow key (use physical address or kernel address for now) */
  page->borrow_key = (uintptr)page;

  /* Acquire exclusive ownership for the kernel/loader to write */
  /* In a real scenario, this 'up' would be the loader process */
  if (up) {
    borrow_acquire(up, page->borrow_key);
  }

  /* Link code page */
  lock(&cp_registry.lock);
  h->next = cp_registry.head;
  cp_registry.head = h;
  unlock(&cp_registry.lock);

  print("CLR: created code page for assembly %d (key=0x%lx)\n", assembly_id,
        page->borrow_key);

  return h;
}

int clr_codepage_add_method(CodePageHandle *h, u32int token, void *il,
                            u32int size) {
  CodePage *page;
  int idx;

  if (h == nil || h->page == nil || il == nil)
    return -1;

  page = h->page;
  idx = page->method_count;

  if (idx >= CODEPAGE_MAX_METHODS)
    return -1;

  if (page->total_il_size + size > CODEPAGE_DATA_SIZE)
    return -1;

  /* Copy IL to page */
  memmove(&page->il_code[page->total_il_size], il, size);

  /* Update directory */
  page->methods[idx].token = token;
  page->methods[idx].offset = page->total_il_size;
  page->methods[idx].size = size;

  page->total_il_size += size;
  page->method_count++;

  return 0;
}

void clr_codepage_seal(CodePageHandle *h) {
  /* Transition from Writer (Exclusive) to Reader-Ready */
  /* In full implementation this might verify checksums or signatures */
  if (h && up) {
    /* Release exclusive lock so shared borrowers can enter */
    /* Note: This logic depends on exact borrow semantics (Auto-release vs
     * manual) */
    /* For now we assume we just hold it open for shared readers */
  }
}

int clr_codepage_acquire_read(CodePageHandle *h, void *proc) {
  if (h == nil || h->page == nil)
    return -1;
  /* Borrow shared access for the tasklet's process */
  /* borrow_borrow_shared(loader_proc, tasklet_proc, key) */
  /* For prototype we return 0 */
  return 0;
}

int clr_codepage_release_read(CodePageHandle *h, void *proc) {
  if (h == nil || h->page == nil)
    return -1;
  return 0;
}

void clr_codepage_destroy(CodePageHandle *h) {
  if (h == nil)
    return;
  /* Release memory, remove from registry */
  /* TODO: Check borrow counts */
  if (h->page)
    free(h->page);
  free(h);
}
