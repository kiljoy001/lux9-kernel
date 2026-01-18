/*
 * Setup 9P exchange page for a user process - POOL-BASED ALLOCATION.
 *
 * EXCHANGE POOL MODEL:
 * ====================
 * Each process gets its own physical page allocated from the global
 * exchange pool. This provides:
 * 1. Complete parent/child isolation - no shared pages after fork
 * 2. Capability-based access control via BlindLedger
 * 3. Proper resource tracking and cleanup on process exit
 *
 * The page is mapped at a per-process virtual address (p->p9uaddr) for
 * userspace compatibility, backed by a unique physical page from the pool.
 */
#include "9p_router.h"
#include "dat.h"
#include "exchange_pool.h"
#include "fns.h"
#include "mem.h"
#include "pageown.h"
#include "portlib.h"
#include "u.h"
#include "siphash.h"
#include <error.h>

static hsiphash_key_t p9va_key;
static int p9va_key_init;

static void p9va_init_key(void) {
  extern int tpm_get_random(u8int *buf, int len);
  extern u64int rdrand_u64(void);
  extern int crypto_hw_rdrand_available(void);
  extern u64int chacha20_csprng_u64(void);

  if (p9va_key_init)
    return;

  if (tpm_get_random((u8int *)&p9va_key, sizeof(p9va_key)) ==
      sizeof(p9va_key)) {
    print("p9va: Using TPM random for VA key\n");
  } else if (crypto_hw_rdrand_available()) {
    p9va_key.key[0] = rdrand_u64();
    p9va_key.key[1] = rdrand_u64();
    print("p9va: Using RDRAND for VA key\n");
  } else {
    p9va_key.key[0] = chacha20_csprng_u64();
    p9va_key.key[1] = chacha20_csprng_u64();
    print("p9va: Using ChaCha20 CSPRNG for VA key (fallback)\n");
  }
  p9va_key_init = 1;
}

uintptr p9_pick_uaddr(Proc *p, const UserCapability *cap) {
  u8int buf[BLIND_LEDGER_CAP_SIZE + 16];
  int len = 0;

  if (p == nil)
    return EXCHANGE_PAGE_ADDR;

  p9va_init_key();

  memmove(buf, p->pid2.data, sizeof(p->pid2.data));
  len = sizeof(p->pid2.data);

  if (cap != nil) {
    memmove(buf + len, cap->hash, BLIND_LEDGER_CAP_SIZE);
    len += BLIND_LEDGER_CAP_SIZE;
  }

  u32int h = hsiphash(buf, len, &p9va_key);
  for (u32int i = 0; i < P9_VA_REGION_PAGES; i++) {
    uintptr slot = (h + i) % P9_VA_REGION_PAGES;
    uintptr va = P9_VA_REGION_BASE + (slot * BY2PG);
    if (va < UTZERO || va >= (USTKTOP - USTKSIZE))
      continue;
    if (isoverlap(va, BY2PG) == nil)
      return va;
  }

  return EXCHANGE_PAGE_ADDR;
}

int proc_setup_p9page(Proc *p) {
  print("DEBUG:proc_setup_p9page ENTRY p=%p\n", p);
  print("DEBUG:proc_setup_p9page reading p->kp...\n");
  if (p->kp)
    return 0; /* Kernel processes don't need this */

  print("DEBUG:proc_setup_p9page reading p->pid (offset in struct)...\n");
  ulong test_pid = p->pid;
  print("DEBUG:proc_setup_p9page p->pid=%lud\n", test_pid);

  if (p->seg[P9SEG] != nil)
    return 0; /* Already set up */

  /*
   * POOL-BASED ALLOCATION:
   * Allocate a physical page from the global exchange pool.
   * This ensures each process has its own isolated page.
   */
  UserCapability cap;
  BlindLedgerEntry entry;
  uintptr pa = 0;
  void *kva = nil; /* Kernel virtual address for p9page */

  if (global_pool != nil) {
    PoolError perr = global_pool_alloc_page(p, &cap);
    if (perr == POOL_OK) {
      /* Verify capability and get kernel virtual address */
      if (ledger_verify(&cap, &entry) == BLIND_LEDGER_OK) {
        /* NOTE: BlindLedger stores KADDR, not physical address!
         * The pool was allocated via xspanalloc() which returns KADDR.
         */
        kva = (void *)entry.physical_address; /* This is actually KADDR */
        pa = PADDR(kva); /* Convert to real physical address for MMU */
        print("proc_setup_p9page: pid=%lud pool page kva=%p pa=%#p\n", p->pid,
              kva, (void *)pa);
      } else {
        print("proc_setup_p9page: pid=%lud cap verify failed, fallback\n",
              p->pid);
        pa = 0;
      }
    } else {
      print("proc_setup_p9page: pid=%lud pool alloc failed (%d), fallback\n",
            p->pid, perr);
    }
  } else {
    print("proc_setup_p9page: global_pool not initialized, using fallback\n");
  }

  /*
   * FALLBACK: If pool allocation fails, allocate a raw page.
   * This shouldn't happen in normal operation but prevents boot failure.
   */
  if (pa == 0) {
    void *page = xspanalloc(BY2PG, BY2PG, 0);
    if (page == nil) {
      print("proc_setup_p9page: fallback xspanalloc failed\n");
      return -1;
    }
    kva = page; /* xspanalloc returns KADDR */
    pa = PADDR(page);
    print("proc_setup_p9page: pid=%lud fallback page kva=%p pa=%#p\n", p->pid,
          kva, (void *)pa);
  }

  if (p->p9uaddr == 0) {
    if (pa != 0)
      p->p9uaddr = p9_pick_uaddr(p, &cap);
    else
      p->p9uaddr = p9_pick_uaddr(p, nil);
  }
  if (p->p9uaddr == 0)
    p->p9uaddr = EXCHANGE_PAGE_ADDR;

  /* Zero the page to prevent information leakage (use KADDR) */
  memset(kva, 0, BY2PG);

  /*
   * Create segment for the exchange page at fixed virtual address.
   * Physical page is already allocated (from pool or fallback).
   */
  Segment *s = newseg(SG_PHYSICAL, p->p9uaddr, 1);
  if (s == nil) {
    print("proc_setup_p9page: newseg failed\n");
    /* TODO: Return page to pool on failure */
    return -1;
  }

  /*
   * Allocate Physseg and configure with our pool-allocated physical page.
   */
  s->pseg = malloc(sizeof(Physseg));
  if (s->pseg == nil) {
    print("proc_setup_p9page: malloc failed for pseg\n");
    putseg(s);
    return -1;
  }

  /* Configure physical segment with real physical address */
  s->pseg->attr = SG_PHYSICAL | SG_CACHED;
  s->pseg->name = "9pexchange";
  s->pseg->pa = pa; /* Physical address from pool */
  s->pseg->size = BY2PG;
  s->pseg->next = nil;
  s->pseg->prev = nil;

  /* Clear any conflicting segments in the user's address space */
  for (int i = 0; i < NSEG; i++) {
    Segment *oseg = p->seg[i];
    if (oseg == nil)
      continue;
    if (p->p9uaddr >= oseg->base && p->p9uaddr < oseg->top) {
      print("proc_setup_p9page: clearing conflicting seg[%d] at base=%#p\n", i,
            oseg->base);
      p->seg[i] = nil;
      putseg(oseg);
      print("DEBUG:proc_setup_p9page post-clear-seg[%d] p->pid=%lud\n", i,
            p->pid);
    }
  }
  print("DEBUG:proc_setup_p9page post-loop p->pid=%lud\n", p->pid);

  /* Assign segment to process at P9SEG slot */
  p->seg[P9SEG] = s;
  print("DEBUG:proc_setup_p9page post-assignment p->pid=%lud\n", p->pid);

  /* Store kernel virtual address for p9_handle_doorbell */
  p->p9page = kva;
  p->p9page_phys = pa;

  print("proc_setup_p9page: pid=%lud seg=%p base=%#p pa=%#p kva=%p\n", p->pid,
        s, (void *)s->base, (void *)pa, p->p9page);

  return 0;
}
