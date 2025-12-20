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

#include "clr/clr-kernel/clr_pebble_integration.h"
#include "dat.h"
#include "error.h"
#include "fns.h"
#include "mem.h"
#include "pebble.h"
#include "portlib.h"
#include "u.h"

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
  clr_heap_t *heap;
  PebbleState *ps;

  /* Get/create CLR heap for current process */
  ps = pebble_state();
  if (ps == nil)
    error("lux_alloc: no pebble state");

  /* Allocate heap if this is first CIL allocation */
  if (up && up->clr_heap == nil) {
    heap = mallocz(sizeof(clr_heap_t), 1);
    if (heap == nil)
      error(PEBBLE_E_NOMEM);
    heap->pebble = ps;
    heap->objects_head = nil;
    heap->objects_tail = nil;
    heap->object_count = 0;
    up->clr_heap = heap;
  } else {
    heap = up ? up->clr_heap : nil;
  }

  if (heap == nil)
    error("lux_alloc: no CLR heap");

  /* Allocate object via Pebble (LIME: BLACK + WHITE) */
  obj = clr_object_alloc(heap, size, CLR_TYPE_OBJECT);
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
  clr_heap_t *heap;
  PebbleWhite *white;

  if (ptr == nil)
    return nil; /* Null references don't need refcount */

  heap = up ? up->clr_heap : nil;
  if (heap == nil)
    error("lux_addref: no CLR heap");

  /* Find object by data pointer (reverse lookup) */
  lock(&heap->objects_lock);
  for (obj = heap->objects_head; obj != nil; obj = obj->next) {
    if (obj->data == ptr) {
      unlock(&heap->objects_lock);

      /* Issue new WHITE token (VANILLA) */
      white = clr_object_addref(heap, obj);
      if (white == nil)
        error(PEBBLE_E_AGAIN);

      return ptr;
    }
  }
  unlock(&heap->objects_lock);

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
  clr_heap_t *heap;
  PebbleWhite *white;

  if (ptr == nil)
    return; /* Null references don't need release */

  heap = up ? up->clr_heap : nil;
  if (heap == nil)
    error("lux_release: no CLR heap");

  /* Find object by data pointer */
  lock(&heap->objects_lock);
  for (obj = heap->objects_head; obj != nil; obj = obj->next) {
    if (obj->data == ptr) {
      unlock(&heap->objects_lock);

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
          clr_object_release(heap, obj, white);
          /* Note: clr_object_release frees obj if white_count==0 */
        }
      } else {
        unlock(&obj->lock);
      }

      return;
    }
  }
  unlock(&heap->objects_lock);

  /* Not found - could be already freed, just ignore */
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
