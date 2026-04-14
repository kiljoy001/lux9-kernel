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
#include "hhdm.h"
#include "mem.h"
#include "borrowchecker.h"
#include "pageown.h"
#include "portlib.h"
#include "u.h"
#include "siphash.h"
#include <error.h>

static hsiphash_key_t p9va_key;
static int p9va_key_init;
extern uintptr paddr(void *);

/*@
  @ assigns \nothing;
  @*/
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

/*@
  @ requires p == \null || \valid(p);
  @ requires cap == \null || \valid(cap);
  @ assigns \nothing;
  @*/
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
    /*@ loop invariant 0 <= i <= P9_VA_REGION_PAGES;
    @ loop assigns i;
    @ loop variant P9_VA_REGION_PAGES - i;
    @*/
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

/*@
  @ requires p == \null || \valid(p);
  @ assigns \nothing;
  @*/
static int proc_find_pool_cap_by_pa(Proc *p, uintptr pa, UserCapability *out) {
  ProcAllocation *alloc;
  BlindLedgerEntry entry;

  if (p == nil || out == nil || pa == 0)
    return -1;

  alloc = get_proc_allocation(p);
  if (alloc == nil)
    return -1;

  for (uint i = 0; i < alloc->num_pages; i++) {
    if (ledger_verify(&alloc->pages[i], &entry) != BLIND_LEDGER_OK)
      continue;
    if (entry.physical_address != pa)
      continue;
    *out = alloc->pages[i];
    return 0;
  }

  return -1;
}

void proc_teardown_p9page(Proc *p) {
  Segment *s;
  void *kva;
  uintptr pa;
  int pooled;
  Proc *owner;
  UserCapability cap;
  uintptr key;

  if (p == nil || p->kp)
    return;

  s = nil;
  kva = nil;
  pa = 0;
  pooled = 0;

  qlock(&p->seglock);
  s = p->seg[P9SEG];
  if (s != nil)
    p->seg[P9SEG] = nil;
  kva = p->p9page;
  pa = p->p9page_phys;
  if (s != nil && s->pseg != nil && (s->pseg->attr & SG_POOL) != 0)
    pooled = 1;
  p->p9page = nil;
  p->p9page_phys = 0;
  qunlock(&p->seglock);

  if (pa != 0) {
    key = (uintptr)kaddr(pa);
    owner = pageown_get_owner(pa);
    if (owner != nil) {
      if (borrow_release(owner, key) != BORROW_OK)
        print("proc_teardown_p9page: borrow_release failed pid=%lud pa=%#p\n",
              p->pid, (void *)pa);
    } else if (borrow_is_owned_by_system(key, OWNER_KERNEL)) {
      if (borrow_release_system(key, OWNER_KERNEL) != BORROW_OK)
        print("proc_teardown_p9page: system release failed pid=%lud pa=%#p\n",
              p->pid, (void *)pa);
    }

    if (pooled) {
      if (proc_find_pool_cap_by_pa(p, pa, &cap) == 0) {
        if (global_pool_free_page(p, &cap) != POOL_OK) {
          print("proc_teardown_p9page: pool free failed pid=%lud pa=%#p\n",
                p->pid, (void *)pa);
        }
      } else {
        print("proc_teardown_p9page: missing pool capability pid=%lud pa=%#p\n",
              p->pid, (void *)pa);
      }
    } else if (kva != nil) {
      xfree(kva);
    }
  }

  if (s != nil)
    putseg(s);
}

/*@
  @ requires p == \null || \valid(p);
  @ assigns \nothing;
  @*/
int proc_setup_p9page(Proc *p) {
  int have_cap;

  if (p->kp)
    return 0; /* Kernel processes don't need this */

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
  memset(&cap, 0, sizeof(cap));
  have_cap = 0;

  if (global_pool != nil) {
    PoolError perr = global_pool_alloc_page(p, &cap);
    if (perr == POOL_OK) {
      have_cap = 1;
      /* Verify capability and get kernel virtual address */
      if (ledger_verify(&cap, &entry) == BLIND_LEDGER_OK) {
        kva = (void *)hhdm_virt(entry.physical_address);
        pa = entry.physical_address;
        print("proc_setup_p9page: pid=%lud pool page kva=%p pa=%#p\n", p->pid,
              kva, (void *)pa);
      } else {
        print("proc_setup_p9page: pid=%lud cap verify failed, fallback\n",
              p->pid);
        global_pool_free_page(p, &cap);
        have_cap = 0;
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
    pa = paddr(page);
    print("proc_setup_p9page: pid=%lud fallback page kva=%p pa=%#p\n", p->pid,
          kva, (void *)pa);
  }

  if (p->p9uaddr == 0) {
    if (have_cap)
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
    if (have_cap)
      global_pool_free_page(p, &cap);
    p->p9uaddr = 0;
    return -1;
  }

  /*
   * Allocate Physseg and configure with our pool-allocated physical page.
   */
  s->pseg = malloc(sizeof(Physseg));
  if (s->pseg == nil) {
    print("proc_setup_p9page: malloc failed for pseg\n");
    putseg(s);
    if (have_cap)
      global_pool_free_page(p, &cap);
    p->p9uaddr = 0;
    return -1;
  }

  /* Configure physical segment with real physical address */
  s->pseg->attr = SG_PHYSICAL | SG_CACHED | SG_NOEXEC | SG_PROCOWNED;
  if (have_cap)
    s->pseg->attr |= SG_POOL;
  s->pseg->name = "9pexchange";
  s->pseg->pa = pa; /* Physical address from pool */
  s->pseg->size = BY2PG;
  s->pseg->next = nil;
  s->pseg->prev = nil;

  /* Clear any conflicting segments in the user's address space */
    /*@ loop invariant 0 <= i <= NSEG;
    @ loop assigns i;
    @ loop variant NSEG - i;
    @*/
  for (int i = 0; i < NSEG; i++) {
    Segment *oseg = p->seg[i];
    if (oseg == nil)
      continue;
    if (p->p9uaddr >= oseg->base && p->p9uaddr < oseg->top) {
      print("proc_setup_p9page: clearing conflicting seg[%d] at base=%#p\n", i,
            oseg->base);
      p->seg[i] = nil;
      putseg(oseg);
    }
  }

  /* Assign segment to process at P9SEG slot */
  p->seg[P9SEG] = s;

  /* Store kernel virtual address for p9_handle_doorbell */
  p->p9page = kva;
  p->p9page_phys = pa;

  if (pageown_acquire(p, pa, (u64int)(uintptr)kaddr(pa)) != POWN_OK) {
    print("proc_setup_p9page: borrow acquire failed pid=%lud pa=%#p\n", p->pid,
          (void *)pa);
  }

  print("proc_setup_p9page: pid=%lud seg=%p base=%#p pa=%#p kva=%p\n", p->pid,
        s, (void *)s->base, (void *)pa, p->p9page);

  return 0;
}
