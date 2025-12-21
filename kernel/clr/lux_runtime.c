/*
 * lux_runtime.c - CIL GC Runtime Functions
 *
 * Runtime support for QBE-compiled CIL code.
 * Implements LIME/VANILLA/BURN operations called from generated code.
 *
 * Linked into QBE-compiled binaries to provide:
 * - $lux_alloc: LIME (allocate + first WHITE token)
 * - $lux_addref: VANILLA (issue additional WHITE token)
 * - $lux_release: BURN (release WHITE token, free if count==0)
 */

#include "clr-kernel/clr_pebble_integration.h"
#include "dat.h"
#include "error.h"
#include "fns.h"
#include "mem.h"
#include "pebble.h"
#include "portlib.h"
#include "u.h"

static clr_heap_t *lux_heap;

/*
 * $lux_alloc - Allocate CIL object (LIME operation)
 *
 * Called from QBE-compiled CIL when 'newobj' is executed.
 * Creates BLACK pebble + first WHITE token.
 *
 * Args:
 *   size: number of bytes to allocate
 *   type_token: ECMA-335 metadata token (for future type info)
 *
 * Returns: pointer to allocated object
 */
void *lux_alloc(ulong size, ulong type_token) {
  clr_object_t *obj;
  PebbleState *ps;

  /* Get/create CLR heap for current process */
  ps = pebble_state();
  if (ps == nil)
    error("lux_alloc: no pebble state");

  /* Allocate heap if this is first CIL allocation */
  if (lux_heap == nil) {
    lux_heap = mallocz(sizeof(clr_heap_t), 1);
    if (lux_heap == nil)
      error(PEBBLE_E_NOMEM);
    lux_heap->pebble = ps;
    lux_heap->objects_head = nil;
    lux_heap->objects_tail = nil;
    lux_heap->object_count = 0;
  }

  if (lux_heap == nil)
    error("lux_alloc: no CLR heap");

  /* Allocate object via Pebble (LIME: BLACK + WHITE) */
  obj = clr_object_alloc(lux_heap, size, CLR_REF);
  if (obj == nil)
    error(PEBBLE_E_NOMEM);

  /* Return pointer to object data */
  return obj->data;
}

/*
 * $lux_addref - Add reference to object (VANILLA operation)
 *
 * Called from QBE-compiled CIL when 'dup' is executed on reference type.
 * Issues additional WHITE token for same object.
 *
 * Args:
 *   ptr: pointer to object data
 *
 * Returns: same pointer (for stack convenience)
 */
void *lux_addref(void *ptr) {
  clr_object_t *obj;
  PebbleWhite *white;

  if (ptr == nil)
    return nil; /* Null references don't need refcount */

  if (lux_heap == nil)
    error("lux_addref: no CLR heap");

  /* Find object by data pointer (reverse lookup) */
  lock(&lux_heap->objects_lock);
  for (obj = lux_heap->objects_head; obj != nil; obj = obj->next) {
    if (obj->data == ptr) {
      unlock(&lux_heap->objects_lock);

      /* Issue new WHITE token (VANILLA) */
      white = clr_object_addref(lux_heap, obj);
      if (white == nil)
        error(PEBBLE_E_AGAIN);

      return ptr;
    }
  }
  unlock(&lux_heap->objects_lock);

  error("lux_addref: object not found");
  return nil;
}

/*
 * $lux_release - Release reference to object (BURN operation)
 *
 * Called from QBE-compiled CIL when 'pop' is executed on reference type.
 * Burns WHITE token, frees object if last reference.
 *
 * Args:
 *   ptr: pointer to object data
 */
void lux_release(void *ptr) {
  clr_object_t *obj;
  PebbleWhite *white;

  if (ptr == nil)
    return; /* Null references don't need release */

  if (lux_heap == nil)
    error("lux_release: no CLR heap");

  /* Find object by data pointer */
  lock(&lux_heap->objects_lock);
  for (obj = lux_heap->objects_head; obj != nil; obj = obj->next) {
    if (obj->data == ptr) {
      unlock(&lux_heap->objects_lock);

      /* Find any WHITE token for this object */
      lock(&obj->lock);
      if (obj->white_count > 0) {
        /* Use inline white or first in list */
        white = obj->inline_white.white;
        if (white == nil && obj->white_list)
          white = obj->white_list->white;
        unlock(&obj->lock);

        if (white) {
          /* Release white token (BURN) */
          clr_object_release(lux_heap, obj, white);
          /* Note: clr_object_release frees obj if white_count==0 */
        }
      } else {
        unlock(&obj->lock);
      }

      return;
    }
  }
  unlock(&lux_heap->objects_lock);

  /* Not found - could be already freed, just ignore */
}

/*
 * $lux_snapshot - Create Red snapshot for transactional semantics
 */
void *lux_snapshot(void *ptr) {
  if (ptr == nil)
    return nil;

  PebbleState *ps = pebble_state();
  if (ps == nil)
    return ptr;

  PebbleBlue *blue = nil;
  for (PebbleBlue *b = ps->blue_list; b != nil; b = b->next) {
    if (b->blue_data == ptr) {
      blue = b;
      break;
    }
  }

  if (blue == nil)
    return ptr;

  PebbleRed *red = nil;
  if (pebble_red_snapshot(blue, &red) != 0)
    return ptr;

  return red ? red->red_data : ptr;
}

/*
 * $lux_commit - Commit transactional changes (MVP no-op)
 */
void lux_commit(void *ptr) {
  USED(ptr);
}

/*
 * $lux_rollback - Rollback transactional changes (MVP no-op)
 */
void lux_rollback(void *ptr) {
  USED(ptr);
}

/*
 * $lux_alloc_array - Allocate CIL array
 *
 * Future extension for array allocation with length prefix.
 */
void *lux_alloc_array(ulong elem_size, ulong count, ulong type_token) {
  ulong size = sizeof(ulong) + (elem_size * count); /* length + elements */
  void *arr = lux_alloc(size, type_token);

  if (arr) {
    /* Store length at start */
    *(ulong *)arr = count;
  }

  return arr;
}
