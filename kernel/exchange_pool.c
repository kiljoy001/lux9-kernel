/*
 * Exchange Pool Implementation - Working Version with Load Balancing
 *
 * Global pool of exchange pages for IPC message passing.
 * ✅ Proportional load balancing implemented (measure_process_demand,
 * compute_target_allocations)
 * TODO: Add dynamic resizing in Phase 4
 */

/* Standard kernel includes - ORDER MATTERS! u.h first for Plan 9 types */
#include "exchange_pool.h"
#include "../port/lib.h"
#include "blind_ledger.h"
#include "borrowchecker.h"
#include "dat.h"
#include "fns.h"
#include "mem.h"
#include "u.h"
#include "uuid.h"

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

// Measure process demand based on syscall rate
void measure_process_demand(ProcAllocation *pa) {
  if (pa == nil)
    return;

  uvlong now = fastticks(nil);
  uvlong elapsed;

  // Initialize on first call
  if (pa->last_measurement == 0) {
    pa->last_measurement = now;
    pa->syscall_count = 0;
    pa->syscall_rate = 0.0;
    return;
  }

  // Calculate time elapsed since last measurement
  elapsed = now - pa->last_measurement;

  // Convert fastticks to milliseconds (approximate)
  // Assume fastticks is in nanoseconds or similar high-resolution unit
  uvlong elapsed_ms = fastticks2us(elapsed) / 1000;

  // Avoid division by zero and measure only after meaningful time
  if (elapsed_ms < 100) { // Less than 100ms, too soon
    pa->syscall_count++;
    return;
  }

  // Calculate syscalls per second using integer math (avoid float/SSE)
  // syscall_rate = (syscall_count * 1000) / elapsed_ms
  // Store as scaled integer (syscalls * 1000 per second)
  if (elapsed_ms > 0) {
    pa->syscall_rate = (pa->syscall_count * 1000) / elapsed_ms;
  }

  // Reset for next measurement period
  pa->last_measurement = now;
  pa->syscall_count = 0;
}

// Compute target allocations based on proportional demand
void compute_target_allocations(GlobalExchangePool *pool) {
  if (pool == nil)
    return;

  ProcAllocation *pa;
  uvlong total_demand = 0;
  uint active_procs = 0;
  uint available_pages;

  // First pass: measure total demand
  for (pa = pool->proc_allocs; pa != nil; pa = pa->next) {
    if (pa->proc != nil && pa->syscall_rate > 0) {
      total_demand += pa->syscall_rate;
      active_procs++;
    }
  }

  // If no active demand, distribute equally among active processes
  if (total_demand < 10 || active_procs == 0) {
    // Equal distribution fallback
    for (pa = pool->proc_allocs; pa != nil; pa = pa->next) {
      if (pa->proc != nil) {
        pa->target_pages = MIN_PAGES_PER_PROCESS;
      }
    }
    return;
  }

  // Calculate available pages for distribution
  qlock(&pool->pool_lock);
  available_pages = pool->free_count;
  qunlock(&pool->pool_lock);

  // Reserve minimum allocation for all processes
  uint reserved = active_procs * MIN_PAGES_PER_PROCESS;
  if (available_pages < reserved) {
    available_pages = reserved;
  }

  // Second pass: assign proportional targets using integer math
  for (pa = pool->proc_allocs; pa != nil; pa = pa->next) {
    if (pa->proc == nil || pa->syscall_rate < 10) {
      pa->target_pages = MIN_PAGES_PER_PROCESS;
      continue;
    }

    // Calculate proportional share using integer math
    // share = (syscall_rate * available_pages) / total_demand
    uint target = (pa->syscall_rate * available_pages) / total_demand;

    // Clamp to valid range
    if (target < MIN_PAGES_PER_PROCESS) {
      target = MIN_PAGES_PER_PROCESS;
    }
    if (target > MAX_PAGES_PER_PROCESS) {
      target = MAX_PAGES_PER_PROCESS;
    }

    pa->target_pages = target;
  }
}
