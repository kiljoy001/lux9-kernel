/*
 * pebble_kernel.c - Lightweight Pebble Allocation for Kernel
 *
 * Provides MSGORD-tracked allocations for kernel code with consistent
 * WHITE/BLACK color semantics, but without Blind Ledger overhead.
 *
 * Token Flow:
 *   COLORLESS → WHITE (reserve) → BLACK (activate) → COLORLESS (free)
 *
 * MSGORD Integration:
 *   - All allocations tracked in DAG for ordering
 *   - RED/BLUE consensus orthogonal to WHITE/BLACK state
 */

#include "dat.h"
#include "error.h"
#include "fns.h"
#include "mem.h"
#include "msgord.h"
#include "pebble.h"
#include "portlib.h"
#include "u.h"

/* Pebble operation types for MSGORD tracking */
typedef enum {
  PEBBLE_OP_RESERVE = 1,  /* COLORLESS → WHITE */
  PEBBLE_OP_ACTIVATE = 2, /* WHITE → BLACK */
  PEBBLE_OP_FREE = 3,     /* BLACK → COLORLESS */
} PebbleOpType;

/* Pebble operation payload for MSGORD */
typedef struct PebbleOp {
  PebbleOpType type;
  void *ptr;
  ulong size;
  uvlong timestamp;
} PebbleOp;

/* Kernel allocation tracking structure */
typedef struct PebbleKernelAlloc {
  void *ptr;          /* Physical address */
  ulong size;         /* Size in bytes (8-byte aligned) */
  PebbleWhite *white; /* WHITE token (reservation) */
  int is_black;       /* 0 = WHITE (reserved), 1 = BLACK (used) */
  uint msgord_id;     /* MSGORD message ID for tracking */

  /* List linkage */
  struct PebbleKernelAlloc *next;
  struct PebbleKernelAlloc *prev;
} PebbleKernelAlloc;

/* Global kernel allocation list */
static PebbleKernelAlloc *kernel_allocs_head = nil;
static PebbleKernelAlloc *kernel_allocs_tail = nil;
static Lock kernel_allocs_lock;

/* Statistics */
static struct {
  uvlong total_reserves;
  uvlong total_activates;
  uvlong total_frees;
  uvlong current_white;
  uvlong current_black;
  uvlong peak_white;
  uvlong peak_black;
} kernel_pebble_stats;

static void pebble_kernel_log_msgord_failure(const char *where, void *ptr,
                                             ulong size) {
  uvlong total = 0;
  uvlong blue = 0;
  uvlong red = 0;

  print("pebble_kernel_%s: MSGORD submit failed ptr=%p size=%lud\n", where, ptr,
        size);
  if (msgord == nil) {
    print("pebble_kernel_%s: MSGORD unavailable (nil)\n", where);
    return;
  }

  msgord_stats(msgord, &total, &blue, &red);
  print("pebble_kernel_%s: MSGORD id=%d init=%d count=%ud total=%llud "
        "blue=%llud red=%llud next_id=%ud global_seq=%llud\n",
        where, msgord->gd_id, msgord->gd_initialized, msgord->gd_count, total,
        blue, red, msgord->gd_next_id, msgord->gd_global_seq);
  print("pebble_kernel_%s: pebble reserves=%llud activates=%llud frees=%llud "
        "white=%llud black=%llud peak_white=%llud peak_black=%llud\n",
        where, kernel_pebble_stats.total_reserves,
        kernel_pebble_stats.total_activates, kernel_pebble_stats.total_frees,
        kernel_pebble_stats.current_white, kernel_pebble_stats.current_black,
        kernel_pebble_stats.peak_white, kernel_pebble_stats.peak_black);
}

/*
 * pebble_kernel_reserve - Reserve memory (COLORLESS → WHITE)
 *
 * Allocates memory from system pool and issues WHITE token.
 * Memory is reserved but not yet in active use.
 */
PebbleKernelAlloc *pebble_kernel_reserve(ulong size) {
  PebbleKernelAlloc *alloc;
  PebbleState *ps;
  void *buf;
  PebbleWhite *white;
  PebbleOp op;
  int msg_id;

  /* Enforce 8-byte alignment (Pebble quantum) */
  if (size < PEBBLE_MIN_ALLOC)
    size = PEBBLE_MIN_ALLOC;
  if (size % PEBBLE_MEM_PER_TOKEN != 0)
    size = ROUNDUP(size, PEBBLE_MEM_PER_TOKEN);

  /* Allocate from COLORLESS bank (system memory) */
  buf = xallocz(size, 1);
  if (buf == nil)
    error(PEBBLE_E_NOMEM);

  /* Get Pebble state (kernel or current process) */
  ps = pebble_state();
  if (ps == nil) {
    xfree(buf);
    error("pebble state not initialized");
  }

  /* Issue WHITE token (reservation) */
  white = pebble_issue_white(ps, buf, size);
  if (white == nil) {
    xfree(buf);
    error(PEBBLE_E_AGAIN);
  }

  /* Create allocation tracking structure */
  alloc = mallocz(sizeof(PebbleKernelAlloc), 1);
  if (alloc == nil) {
    white->token = 0; /* Invalidate white */
    xfree(buf);
    error(PEBBLE_E_NOMEM);
  }

  alloc->ptr = buf;
  alloc->size = size;
  alloc->white = white;
  alloc->is_black = 0; /* Still WHITE (reserved) */

  /* Track in MSGORD for ordering - SKIP during boot when scheduler not ready */
  if (up != nil && msgord != nil) {
    op.type = PEBBLE_OP_RESERVE;
    op.ptr = buf;
    op.size = size;
    op.timestamp = fastticks(nil);

    msg_id = msgord_submit_raw(msgord, up, &op, sizeof(op));
    if (msg_id < 0) {
      /* MSGORD submission failed, continue anyway */
      pebble_kernel_log_msgord_failure("reserve", buf, size);
      msg_id = 0;
    }
  } else {
    msg_id = 0; /* Boot-time allocation, no MSGORD tracking */
  }
  alloc->msgord_id = (uint)msg_id;

  /* Add to global tracking list */
  lock(&kernel_allocs_lock);
  alloc->next = kernel_allocs_head;
  alloc->prev = nil;
  if (kernel_allocs_head)
    kernel_allocs_head->prev = alloc;
  else
    kernel_allocs_tail = alloc;
  kernel_allocs_head = alloc;

  /* Update statistics */
  kernel_pebble_stats.total_reserves++;
  kernel_pebble_stats.current_white++;
  if (kernel_pebble_stats.current_white > kernel_pebble_stats.peak_white)
    kernel_pebble_stats.peak_white = kernel_pebble_stats.current_white;

  unlock(&kernel_allocs_lock);

  return alloc;
}

/*
 * pebble_kernel_activate - Activate allocation (WHITE → BLACK)
 *
 * Transitions reserved memory to active use.
 */
void pebble_kernel_activate(PebbleKernelAlloc *alloc) {
  PebbleOp op;

  if (alloc == nil)
    error(PEBBLE_E_BADARG);

  lock(&kernel_allocs_lock);

  if (alloc->is_black) {
    unlock(&kernel_allocs_lock);
    return; /* Already BLACK, idempotent */
  }

  /* Transition WHITE → BLACK */
  alloc->is_black = 1;

  /* Update statistics */
  kernel_pebble_stats.total_activates++;
  kernel_pebble_stats.current_white--;
  kernel_pebble_stats.current_black++;
  if (kernel_pebble_stats.current_black > kernel_pebble_stats.peak_black)
    kernel_pebble_stats.peak_black = kernel_pebble_stats.current_black;

  unlock(&kernel_allocs_lock);

  /* Track transition in MSGORD - SKIP during boot */
  if (up != nil && msgord != nil) {
    op.type = PEBBLE_OP_ACTIVATE;
    op.ptr = alloc->ptr;
    op.size = alloc->size;
    op.timestamp = fastticks(nil);

    if (msgord_submit_raw(msgord, up, &op, sizeof(op)) < 0)
      pebble_kernel_log_msgord_failure("activate", alloc->ptr, alloc->size);
  }
}

/*
 * pebble_kernel_free - Free allocation (BLACK → COLORLESS)
 *
 * Burns WHITE token and returns memory to system pool.
 */
void pebble_kernel_free(PebbleKernelAlloc *alloc) {
  PebbleOp op;

  if (alloc == nil)
    return;

  lock(&kernel_allocs_lock);

  /* Remove from tracking list */
  if (alloc->prev)
    alloc->prev->next = alloc->next;
  else
    kernel_allocs_head = alloc->next;

  if (alloc->next)
    alloc->next->prev = alloc->prev;
  else
    kernel_allocs_tail = alloc->prev;

  /* Update statistics */
  kernel_pebble_stats.total_frees++;
  if (alloc->is_black)
    kernel_pebble_stats.current_black--;
  else
    kernel_pebble_stats.current_white--;

  unlock(&kernel_allocs_lock);

  /* Track free in MSGORD - SKIP during boot */
  if (up != nil && msgord != nil) {
    op.type = PEBBLE_OP_FREE;
    op.ptr = alloc->ptr;
    op.size = alloc->size;
    op.timestamp = fastticks(nil);

    if (msgord_submit_raw(msgord, up, &op, sizeof(op)) < 0)
      pebble_kernel_log_msgord_failure("free", alloc->ptr, alloc->size);
  }

  /* Burn WHITE token */
  if (alloc->white)
    alloc->white->token = 0;

  /* Return to COLORLESS bank */
  xfree(alloc->ptr);
  free(alloc);
}

/*
 * pebble_kernel_alloc - Convenience: Reserve + Activate
 *
 * Single-call allocation for common use case.
 */
void *pebble_kernel_alloc(ulong size) {
  PebbleKernelAlloc *alloc;

  alloc = pebble_kernel_reserve(size);
  pebble_kernel_activate(alloc);

  return alloc->ptr;
}

/*
 * pebble_kernel_stats - Get kernel Pebble statistics
 */
void pebble_kernel_stats(uvlong *reserves, uvlong *activates, uvlong *frees,
                         uvlong *white, uvlong *black) {
  lock(&kernel_allocs_lock);
  if (reserves)
    *reserves = kernel_pebble_stats.total_reserves;
  if (activates)
    *activates = kernel_pebble_stats.total_activates;
  if (frees)
    *frees = kernel_pebble_stats.total_frees;
  if (white)
    *white = kernel_pebble_stats.current_white;
  if (black)
    *black = kernel_pebble_stats.current_black;
  unlock(&kernel_allocs_lock);
}

/*
 * pebble_kernel_init - Initialize kernel Pebble subsystem
 */
/*
 * pebble_kernel_free_ptr - Free allocation by pointer (BLACK → COLORLESS)
 *
 * Looks up the allocation struct by pointer and frees it.
 * This is O(N) but required for compatibility with standard malloc/free
 * interfaces.
 */
void pebble_kernel_free_ptr(void *ptr) {
  PebbleKernelAlloc *curr;

  if (ptr == nil)
    return;

  lock(&kernel_allocs_lock);
  curr = kernel_allocs_head;
  while (curr) {
    if (curr->ptr == ptr) {
      /* Found match, unlock and free (free re-locks, so we must be careful) */
      /* Actually pebble_kernel_free takes a lock, so we should separate lookup
         from free to avoid deadlock? pebble_kernel_free takes the lock. We are
         holding it. We should probably inline the free logic or return the
         alloc to free outside. Or just duplicate the removal logic here.
      */
      break;
    }
    curr = curr->next;
  }
  unlock(&kernel_allocs_lock);

  if (curr) {
    pebble_kernel_free(curr);
  } else {
    /* Pointer not found in Pebble list - implies it wasn't allocated by us */
    /* Fallback to standard free? Or warn? */
    /* Since we are migrating the pipeline, we assume it should be here. */
    /* But m3 might mix allocations? Unlikely if we replace m3_Malloc globally.
     */
    /* Safest to just return if not found, or panic if strict. */
    print("pebble_kernel_free_ptr: warning: pointer %p not found in pebble "
          "list\n",
          ptr);
    /* Try legacy free just in case? No, double free risk. */
  }
}

void pebble_kernel_init(void) {
  memset(&kernel_pebble_stats, 0, sizeof(kernel_pebble_stats));
  memset(&kernel_allocs_lock, 0, sizeof(Lock));
  print("pebble_kernel: initialized (WHITE/BLACK tracking + MSGORD)\n");
}
