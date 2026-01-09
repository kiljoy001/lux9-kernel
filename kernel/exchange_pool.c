/*
 * Exchange Pool Implementation - Minimal Working Version
 *
 * Global pool of exchange pages for IPC message passing.
 * TODO: Add proportional load balancing in Phase 3
 * TODO: Add dynamic resizing in Phase 4
 */

/* Standard kernel includes - ORDER MATTERS! u.h first for Plan 9 types */
#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "uuid.h"
#include "blind_ledger.h"
#include "borrowchecker.h"
#include "exchange_pool.h"

// Global pool instance (accessible via exchange_pool.h extern declaration)
GlobalExchangePool *global_pool = nil;

void exchange_pool_init(uint pool_size) {
  uint i;

  global_pool = xalloc(sizeof(GlobalExchangePool));
  if (global_pool == nil) {
    print("exchange_pool_init: failed to allocate global pool\n");
    return;
  }

  memset(global_pool, 0, sizeof(GlobalExchangePool));

  // Initialize page tracking
  if (pool_size > POOL_SIZE)
    pool_size = POOL_SIZE;

  // Initialize free list
  for (i = 0; i < pool_size; i++) {
    global_pool->free_list[i] = i;
  }
  global_pool->free_count = pool_size;

  // Allocate actual pages
  for (i = 0; i < pool_size; i++) {
    void *page = xspanalloc(BY2PG, BY2PG, 0);
    if (page == nil) {
      print("exchange_pool_init: failed to allocate page %d\n", i);
      continue;
    }

    // Generate capability for this page
    u8int vault_secret[BLIND_LEDGER_SECRET_SIZE];
    BlindLedgerError err;

    err = ledger_generate_secret(vault_secret);
    if (err != BLIND_LEDGER_OK) {
      print("exchange_pool_init: failed to generate secret for page %d\n", i);
      continue;
    }

    err = ledger_mint(&global_pool->pages[i], (uintptr)page, BY2PG, nil,
                      CAP_PERM_READ | CAP_PERM_WRITE, vault_secret);
    if (err != BLIND_LEDGER_OK) {
      print("exchange_pool_init: failed to mint capability for page %d\n", i);
    }
  }

  print("exchange_pool_init: initialized with %d pages\n", pool_size);
}

void exchange_pool_shutdown(void) {
  if (global_pool == nil)
    return;

  // Free all allocated pages
  for (uint i = 0; i < POOL_SIZE; i++) {
    BlindLedgerEntry entry;
    if (ledger_verify(&global_pool->pages[i], &entry) == BLIND_LEDGER_OK) {
      // Free the physical page
      // Note: In a real system, we'd need proper page freeing
      ledger_burn(&global_pool->pages[i], nil);
    }
  }

  free(global_pool);
  global_pool = nil;

  print("exchange_pool_shutdown: pool freed\n");
}

// Get or create process allocation record
ProcAllocation *get_proc_allocation(Proc *p) {
  if (global_pool == nil || p == nil)
    return nil;

  // Search existing allocations
  ProcAllocation *pa = global_pool->proc_allocs;
  while (pa != nil) {
    if (pa->proc == p)
      return pa;
    pa = pa->next;
  }

  // Create new allocation record
  pa = xalloc(sizeof(ProcAllocation));
  if (pa == nil)
    return nil;

  memset(pa, 0, sizeof(ProcAllocation));
  pa->proc = p;
  pa->next = global_pool->proc_allocs;
  global_pool->proc_allocs = pa;

  return pa;
}

// Allocate a page from the global pool
PoolError global_pool_alloc_page(Proc *p, UserCapability *out) {
  uint pool_idx;

  if (global_pool == nil || out == nil)
    return POOL_EINVAL;

  qlock(&global_pool->pool_lock);

  if (global_pool->free_count == 0) {
    qunlock(&global_pool->pool_lock);
    return POOL_ENOMEM;
  }

  // Get a page from free list
  pool_idx = global_pool->free_list[--global_pool->free_count];
  *out = global_pool->pages[pool_idx];

  qunlock(&global_pool->pool_lock);

  // Track allocation if process provided
  if (p != nil) {
    ProcAllocation *pa = get_proc_allocation(p);
    if (pa != nil && pa->num_pages < MAX_PAGES_PER_PROCESS) {
      pa->pages[pa->num_pages++] = *out;
    }
  }

  return POOL_OK;
}

// Return a page to the global pool
PoolError global_pool_free_page(Proc *p, const UserCapability *cap) {
  if (global_pool == nil || cap == nil)
    return POOL_EINVAL;

  // Remove from process tracking if applicable
  if (p != nil) {
    ProcAllocation *pa = get_proc_allocation(p);
    if (pa != nil) {
      for (uint i = 0; i < pa->num_pages; i++) {
        if (memcmp(pa->pages[i].hash, cap->hash, BLIND_LEDGER_CAP_SIZE) == 0) {
          // Remove from list by shifting
          for (uint j = i; j < pa->num_pages - 1; j++) {
            pa->pages[j] = pa->pages[j + 1];
          }
          pa->num_pages--;
          break;
        }
      }
    }
  }

  qlock(&global_pool->pool_lock);

  // Find the page in our pool and return it
  for (uint i = 0; i < POOL_SIZE; i++) {
    if (memcmp(global_pool->pages[i].hash, cap->hash, BLIND_LEDGER_CAP_SIZE) ==
        0) {
      if (global_pool->free_count < POOL_SIZE) {
        global_pool->free_list[global_pool->free_count++] = i;
      }
      qunlock(&global_pool->pool_lock);
      return POOL_OK;
    }
  }

  qunlock(&global_pool->pool_lock);
  return POOL_EINVAL; // Page not found
}

// Cleanup when a process exits
void exchange_cleanup_process(Proc *p) {
  if (global_pool == nil || p == nil)
    return;

  ProcAllocation **prev = &global_pool->proc_allocs;
  ProcAllocation *pa = global_pool->proc_allocs;

  while (pa != nil) {
    if (pa->proc == p) {
      // Return all pages to pool
      for (uint i = 0; i < pa->num_pages; i++) {
        global_pool_free_page(nil, &pa->pages[i]);
      }

      // Remove from list
      *prev = pa->next;
      free(pa);
      return;
    }
    prev = &pa->next;
    pa = pa->next;
  }
}

// Measure process demand (stub for now)
void measure_process_demand(ProcAllocation *pa) {
  if (pa == nil)
    return;
  // TODO: Implement actual demand measurement
  pa->syscall_count++;
}

// Compute target allocations (stub for now)
void compute_target_allocations(GlobalExchangePool *pool) {
  if (pool == nil)
    return;
  // TODO: Implement proportional allocation computation
}
