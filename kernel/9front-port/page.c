#include "borrowchecker.h"
#include "dat.h"
#include "fns.h"
#include "mem.h"
#include "pageown.h"
#include "pebble.h"
#include "portlib.h"
#include "u.h"
#include <error.h>

extern uintptr saved_limine_hhdm_offset;
extern uintptr *mmuwalk(uintptr *, uintptr, int, int);

static inline uintptr hhdm_virt(uintptr pa) {
  return pa + saved_limine_hhdm_offset;
}

/* Validate token color matches borrowchecker state (debug only) */
static void validate_page_token_state(Page *p) {
  if (!pebble_debug || p->pa == 0)
    return;

  enum BorrowState bstate = borrow_get_state(p->pa);

  switch (p->token_color) {
  case PEBBLE_COLOR_BLACK:
    if (bstate != BORROW_EXCLUSIVE) {
      panic("page token mismatch: BLACK but not EXCLUSIVE (pa=%#p)", p->pa);
    }
    break;
  case PEBBLE_COLOR_RED:
    if (bstate != BORROW_SHARED_OWNED) {
      panic("page token mismatch: RED but not SHARED_OWNED (pa=%#p)", p->pa);
    }
    break;
  case PEBBLE_COLOR_COLORLESS:
    if (borrow_is_owned(p->pa)) {
      panic("page token mismatch: COLORLESS but still owned (pa=%#p)", p->pa);
    }
    break;
  }
}

Palloc palloc;
static Page *palloc_end;

static void dbgserial(int c) {}

void dbgserial_hex(uvlong v) {
  int started, shift, nib;

  started = 0;
  for (shift = (int)(sizeof(uvlong) * 8) - 4; shift >= 0; shift -= 4) {
    nib = (v >> shift) & 0xF;
    if (!started && nib == 0 && shift != 0)
      continue;
    started = 1;
    dbgserial(nib < 10 ? '0' + nib : 'A' + nib - 10);
  }
  if (!started)
    dbgserial('0');
}

ulong nkpages(Confmem *cm) {
  return ((cm->klimit - cm->kbase) + BY2PG - 1) / BY2PG;
}

void pageinit(void) {
  int color, i, j;
  Page *p, **t;
  Confmem *cm;
  vlong m, v, u;
  ulong skipped_unmapped, skipped_poison;

  if (palloc.pages == nil) {
    ulong np;

    np = 0;
    for (i = 0; i < nelem(conf.mem); i++) {
      cm = &conf.mem[i];
      np += cm->npage - nkpages(cm);
    }
    palloc.pages = xalloc(np * sizeof(Page));
    if (palloc.pages == nil)
      panic("pageinit");
    palloc_end = palloc.pages + np;
    print("pageinit: palloc.pages=[%p - %p) count=%ld\n", palloc.pages,
          palloc_end, np);
  }

  color = 0;
  palloc.freecount = 0;
  palloc.head = nil;
  skipped_unmapped = 0;
  skipped_poison = 0;

  t = &palloc.head;
  p = palloc.pages;

  for (i = 0; i < nelem(conf.mem); i++) {
    cm = &conf.mem[i];
    if (cm->npage == 0 || cm->base == 0)
      continue;
    dbgserial('P');
    dbgserial('B');
    dbgserial('A' + (i % 26));
    dbgserial('[');
    dbgserial_hex(cm->base);
    dbgserial(']');
    dbgserial('(');
    dbgserial_hex(cm->npage);
    dbgserial(')');
    for (j = nkpages(cm); j < cm->npage; j++) {
      memset(p, 0, sizeof *p);
      p->pa = cm->base + j * BY2PG;
      if (cankaddr(p->pa) == 0) {
        skipped_unmapped++;
        p++;
        continue;
      }
      void *kva = KADDR(p->pa);
      if (kva == nil || kva == (void *)-BY2PG) {
        skipped_poison++;
        p++;
        continue;
      }
      p->color = color;
      p->token_color = PEBBLE_COLOR_COLORLESS; /* All pages start COLORLESS */
      color = (color + 1) % NCOLOR;
      /* Note: Physical page memory will be zeroed by fillpage() in newpage()
       * when actually allocated. Don't zero here as HHDM may not cover all
       * physical memory regions at this early boot stage. */
      *t = p, t = &p->next;
      palloc.freecount++;
      p++;
    }
  }

  palloc.user = p - palloc.pages;
  u = palloc.user * BY2PG;
  v = u + conf.nswap * BY2PG;
  dbgserial('P');
  dbgserial('F');
  dbgserial_hex(palloc.freecount);
  dbgserial('U');
  dbgserial_hex(skipped_unmapped);
  dbgserial('I');
  dbgserial_hex(skipped_poison);
  dbgserial('\n');

  /* Paging numbers */
  swapalloc.highwater = (palloc.user * 5) / 100;
  swapalloc.headroom = swapalloc.highwater + (swapalloc.highwater / 4);

  m = 0;
  for (i = 0; i < nelem(conf.mem); i++)
    if (conf.mem[i].npage)
      m += conf.mem[i].npage * BY2PG;
  m += PGROUND(end - (char *)KTZERO);

  print("%lldM memory: ", (m + 1024 * 1024 - 1) / (1024 * 1024));
  print("%lldM kernel data, ", (m - u + 1024 * 1024 - 1) / (1024 * 1024));
  print("%lldM user, ", u / (1024 * 1024));
  print("%lldM swap\n", v / (1024 * 1024));
}

static void pagechaindone(void) {
  if (palloc.pwait[0].rendez.p != nil && wakeup(&palloc.pwait[0]) != nil)
    return;
  if (palloc.pwait[1].rendez.p != nil)
    wakeup(&palloc.pwait[1]);
}

/*@ requires head == \null || \valid(head);
  @ requires tail == \null || \valid(tail);
  @ assigns \everything;
  @*/
void freepages(Page *head, Page *tail, ulong np) {
  if (head == nil)
    return;

  /* Borrow Checker Validation */
  if (head < palloc.pages || head >= palloc_end)
    panic("freepages: head %p out of bounds", head);
  if (tail < palloc.pages || tail >= palloc_end)
    panic("freepages: tail %p out of bounds", tail);

  /* Release ownership for all pages in range */
  Page *p = head;
  while (p != nil) {
    if (up != nil && p->pa != 0) {
      /* Update per-process color counters before freeing */
      lock(&pebble_global_lock);
      switch (p->token_color) {
      case PEBBLE_COLOR_BLACK:
        up->pebble.black_inuse -= BY2PG;
        break;
      case PEBBLE_COLOR_RED:
        up->pebble.red_inuse -= BY2PG;
        break;
      case PEBBLE_COLOR_BLUE:
        up->pebble.blue_inuse -= BY2PG;
        break;
      }
      p->token_color = PEBBLE_COLOR_COLORLESS;
      unlock(&pebble_global_lock);

      /* Try to release, but don't banic if not owned (might be early boot) */
      /* Try to release, but don't panic if not owned (might be early boot) */
      if (pageown_is_owned(p->pa)) {
        Proc *owner = pageown_get_owner(p->pa);
        if (owner != nil) {
          if (pageown_release(up, p->pa) != POWN_OK) {
            panic("freepages: release failed pa=%#p (kaddr=%#p). OwnerPID: %d, "
                  "CurrentPID: %d.",
                  p->pa, hhdm_virt(p->pa), owner->pid, up ? up->pid : -2);
          }
        } else if (borrow_is_owned_by_system(hhdm_virt(p->pa), OWNER_KERNEL)) {
          /* System-owned (WASM) page - release using system API */
          if (borrow_release_system(hhdm_virt(p->pa), OWNER_KERNEL) !=
              BORROW_OK) {
            pageown_dump_page(p->pa);
            panic("freepages: system release failed pa=%#p (kaddr=%#p). "
                  "CurrentPID: %d.",
                  p->pa, hhdm_virt(p->pa), up ? up->pid : -2);
          }
        } else {
          /* Owned but no owner and not OWNER_KERNEL? Phantom state. */
          uintptr hhdm_va = hhdm_virt(p->pa);
          pageown_dump_page(p->pa);
          panic("freepages: phantom ownership pa=%#p (kaddr=%#p). OwnerPID: "
                "-1, CurrentPID: %d.",
                p->pa, hhdm_va, up ? up->pid : -2);
        }
      }
    }
    if (p == tail)
      break;
    p = p->next;
  }

  if (tail == nil) {
    tail = head;
    for (np = 1;; np++) {
      tail->ref = 0;
      if (tail->next == nil)
        break;
      tail = tail->next;
    }
  }
  if (head < palloc.pages || head >= palloc_end)
    panic("freepages: head %p out of bounds", head);
  if (tail < palloc.pages || tail >= palloc_end)
    panic("freepages: tail %p out of bounds", tail);

  lock(&palloc);
  if (palloc.head != nil &&
      (palloc.head < palloc.pages || palloc.head >= palloc_end))
    panic("freepages: corrupted palloc.head detected before free (%p)",
          palloc.head);

  tail->next = palloc.head;
  palloc.head = head;
  palloc.freecount += np;
  pagechaindone();
  unlock(&palloc);

  /* Update per-process color counters before freeing */
  {
    Page *p = head;
    while (p != nil) {
      if (up != nil) {
        lock(&pebble_global_lock);
        switch (p->token_color) {
        case PEBBLE_COLOR_BLACK:
          up->pebble.black_inuse -= BY2PG;
          break;
        case PEBBLE_COLOR_RED:
          up->pebble.red_inuse -= BY2PG;
          break;
        case PEBBLE_COLOR_BLUE:
          up->pebble.blue_inuse -= BY2PG;
          break;
        }
        p->token_color = PEBBLE_COLOR_COLORLESS;
        unlock(&pebble_global_lock);
      }

      if (p == tail)
        break;
      p = p->next;
    }
  }

  /* Return tokens to global pool for freed pages */
  /* Each page = BY2PG / PEBBLE_BYTES_PER_TOKEN tokens */
  {
    ulong tokens = np * (BY2PG / PEBBLE_BYTES_PER_TOKEN);
    lock(&pebble_bank_lock);
    pebble_global_colorless_bank += tokens;
    unlock(&pebble_bank_lock);
  }
}

/*@ assigns \everything;
  @*/
ulong pagereclaim(Image *i) {
  Page **h, **e, **l, **x, *p;
  Page *fh, *ft;
  ulong mp, np;

  if (i == nil)
    return 0;

  lock(i);
  mp = i->pgref;
  if (mp == 0) {
    unlock(i);
    return 0;
  }
  np = 0;
  fh = ft = nil;
  e = &i->pghash[i->pghsize];
  for (h = i->pghash; h < e; h++) {
    l = h;
    x = nil;
    for (p = *l; p != nil; p = p->next) {
      if (p->ref == 0)
        x = l;
      l = &p->next;
    }
    if (x == nil)
      continue;

    p = *x;
    *x = p->next;
    p->next = nil;
    p->image = nil;
    p->daddr = ~0;

    if (fh == nil)
      fh = p;
    else
      ft->next = p;
    ft = p;
    np++;

    if (--i->pgref == 0) {
      putimage(i);
      goto Done;
    }
    decref((Ref *)&i->ref);
  }
  unlock(i);
Done:
  freepages(fh, ft, np);
  return np;
}

static int ispages(void *) {
  return palloc.freecount > swapalloc.highwater ||
         up->noswap && palloc.freecount > 0;
}

/*@ requires seg == \null || \valid(seg);
    assigns \everything;
    ensures \result == \null || \valid(\result);
*/
Page *newpage(uintptr va, Segment *seg) {
  Page *p, **l;
  int color;
  QLock *locked = nil;

  if (seg != nil)
    locked = &seg->qlock;

  /* Minimal debug - just show VA and free count */
  static int newpage_count = 0;
  newpage_count++;
  if (newpage_count <= 5 || newpage_count % 100 == 0)
    print("newpage[%d]: va=%p free=%lud\n", newpage_count, va,
          palloc.freecount);

  /* Pebble: Check and consume budget for page allocation (userspace only) */
  /* Budget is in tokens; 1 token = PEBBLE_BYTES_PER_TOKEN bytes */
  if (up != nil && !(seg && (seg->type & SG_WASM))) {
    ulong tokens_needed = BY2PG / PEBBLE_BYTES_PER_TOKEN;
    lock(&pebble_global_lock);
    if (up->pebble.colorless_bank < tokens_needed) {
      unlock(&pebble_global_lock);
      if (pebble_debug)
        print("PEBBLE: insufficient tokens for page va=%#p (need %lu, have "
              "%lu)\n",
              va, tokens_needed, up->pebble.colorless_bank);
      return nil; /* Insufficient budget */
    }
    up->pebble.colorless_bank -= tokens_needed;
    unlock(&pebble_global_lock);
  }

  lock(&palloc);

  while (!ispages(nil)) {
    unlock(&palloc);
    if (locked)
      qunlock(locked);

    if (!waserror()) {
      Rendezq *q;

      q = &palloc.pwait[!up->noswap];
      eqlock(q);
      if (!waserror()) {
        kickpager();
        sleep(q, ispages, nil);
        poperror();
      }
      qunlock(q);
      poperror();
    }

    /*
     * If called from fault and we lost the lock from
     * underneath don't waste time allocating and freeing
     * a page. Fault will call newpage again when it has
     * reacquired the locks
     */
    if (locked)
      return nil;

    lock(&palloc);
  }

  if (palloc.head && (palloc.head < palloc.pages || palloc.head >= palloc_end))
    panic("newpage: CORRUPT palloc.head %p", palloc.head);

  /* First try for our colour */
  color = getpgcolor(va);
  l = &palloc.head;
  for (p = *l; p != nil; p = p->next) {
    if (p->color == color)
      break;
    l = &p->next;
  }

  if (p == nil) {
    l = &palloc.head;
    p = *l;
  }

  if (p < palloc.pages || p >= palloc_end)
    panic("newpage: p %p out of bounds (head=%p)", p, palloc.head);

  *l = p->next;
  p->next = nil;
  palloc.freecount--;
  unlock(&palloc);

  p->ref = 1;
  p->va = va;
  p->modref = 0;
  inittxtflush(p);

  /* Zero physical page memory to prevent garbage data */
  fillpage(p, 0);

  if ((p->pa & 0xFFF) != 0) {
    print("newpage: FATAL p->pa UNALIGNED pa=%#p p=%p\n", p->pa, p);
    panic("newpage: p->pa unaligned");
  }

  /* Automatically acquire ownership for the current process */
  if (up != nil && p->pa != 0) {
    extern uintptr saved_limine_hhdm_offset;
    uintptr hhdm_va = p->pa + saved_limine_hhdm_offset;
    if (seg && (seg->type & SG_WASM)) {
      if (borrow_acquire_system(hhdm_va, OWNER_KERNEL) != BORROW_OK)
        panic("newpage: failed to acquire wasm ownership pa=%#p", p->pa);
    } else {
      if (pageown_acquire(up, p->pa, hhdm_va) != POWN_OK)
        panic("newpage: failed to acquire page ownership pa=%#p", p->pa);
    }

    /* Set token color to BLACK (exclusive ownership) */
    p->token_color = PEBBLE_COLOR_BLACK;
    lock(&pebble_global_lock);
    up->pebble.black_inuse += BY2PG;
    unlock(&pebble_global_lock);
  }

  return p;
}

/*
 *  deadpage() decrements the page refcount
 *  and returns the page when it becomes freeable.
 */
/*@ requires p == \null || \valid(p);
  @ assigns \everything;
  @*/
Page *deadpage(Page *p) {
  if (p->image != nil) {
    decref(p);
    return nil;
  }
  if (decref(p) != 0)
    return nil;

  /* Transition any color → COLORLESS when ref reaches 0 */
  p->token_color = PEBBLE_COLOR_COLORLESS;
  /* freepages() will update per-process counters and return tokens */

  return p;
}

/*@ requires p == \null || \valid(p);
  @ assigns \everything;
  @*/
void putpage(Page *p) {
  /* Release ownership before freeing */
  /* TEMPORARILY DISABLED - pageown lock is broken */
  if (0 && p != nil && up != nil && p->pa != 0) {
    pageown_release(up, p->pa);
  }

  p = deadpage(p);
  if (p != nil)
    freepages(p, p, 1);
}

/*
void copypage(Page *f, Page *t) {
  KMap *ks, *kd;

  ks = kmap(f);
  kd = kmap(t);
  memmove((void *)VA(kd), (void *)VA(ks), BY2PG);
  kunmap(ks);
  kunmap(kd);
}
*/

/*@ requires p == \null || \valid(p);
    assigns \everything;
    ensures \result == p;
*/
Page *fillpage(Page *p, int c) {
  KMap *k;

  if (p != nil) {
    k = kmap(p);
    memset((void *)VA(k), c, BY2PG);
    kunmap(k);
  }
  return p;
}

/*@ requires p == \null || \valid(p);
  @ assigns \everything;
  @*/
void cachepage(Page *p, Image *i) {
  Page *x, **h;
  uintptr daddr;

  daddr = p->daddr;
  h = &PGHASH(i, daddr);
  lock(i);
  for (x = *h; x != nil; x = x->next)
    if (x->daddr == daddr)
      goto done;
  if (p->image != nil)
    goto done;
  p->image = i;
  p->next = *h;
  *h = p;
  incref((Ref *)&i->ref);
  i->pgref++;
done:
  unlock(i);
}

/*@ requires p == \null || \valid(p);
  @ assigns \everything;
  @*/
void uncachepage(Page *p) {
  Page **l, *x;
  Image *i;

  i = p->image;
  if (i == nil)
    return;
  l = &PGHASH(i, p->daddr);
  lock(i);
  if (p->image != i)
    goto done;
  for (x = *l; x != nil; x = x->next) {
    if (x == p) {
      *l = p->next;
      p->next = nil;
      p->image = nil;
      p->daddr = ~0;
      i->pgref--;
      putimage(i);
      return;
    }
    l = &x->next;
  }
done:
  unlock(i);
}

/*@ assigns \everything;
  @ ensures \result == \null || \valid(\result);
  @*/
Page *lookpage(Image *i, uintptr daddr) {
  Page *p, **h, **l;

  l = h = &PGHASH(i, daddr);
  lock(i);
  for (p = *l; p != nil; p = p->next) {
    if (p->daddr == daddr) {
      *l = p->next;
      p->next = *h;
      *h = p;
      incref((Ref *)&p->ref);
      unlock(i);
      return p;
    }
    l = &p->next;
  }
  unlock(i);

  return nil;
}

/*@ assigns \everything;
  @*/
void cachedel(Image *i, uintptr daddr) {
  Page *p;

  if ((p = lookpage(i, daddr)) != nil) {
    uncachepage(p);
    putpage(p);
  }
}

/*@ assigns \everything;
  @*/
void zeroprivatepages(void) {
  Page *p, *pe;

  /*
   * in case of a panic, we might not have a process
   * context to do the clearing of the private pages.
   */
  if (up == nil) {
    assert(panicking);
    return;
  }

  lock(&palloc);
  pe = palloc.pages + palloc.user;
  for (p = palloc.pages; p != pe; p++) {
    if (p->modref & PG_PRIV) {
      incref((Ref *)&p->ref);
      fillpage(p, 0);
      decref((Ref *)&p->ref);
    }
  }
  unlock(&palloc);
}

/*
 * Map a user page at a specific virtual address.
 * This function creates the necessary page table entries
 * and links them into the current process's mmuhead.
 */
/*@ assigns \everything;
  @*/
void userpmap(uintptr va, uintptr pa, int perms) {
  uintptr *pte;
  int x;
  extern int current_boot_state;
  /* Mathematical State Verification: User memory mapping requires User Init
   * phase */
  /* BOOT_USERINIT is state 18. BOOT_CHANDEV_INIT is 17. */
  if (current_boot_state < 18) { /* BOOT_USERINIT */
    print("userpmap: VIOLATION state=%d < BOOT_USERINIT(18)\n",
          current_boot_state);
    panic("userpmap: Illegal State Transition - User mapping before User Init");
  }
  print("userpmap: ENTER state=%d va=%#p pa=%#p\n", current_boot_state, va, pa);

  x = splhi();
  pte = mmuwalk(m->pml4, va, 0, 1); // level=0 (PTE), create=1
  if (pte == nil) {
    splx(x);
    panic("userpmap: out of memory for page tables");
  }
  *pte = pa | perms;

  /* Invalidate TLB for this VA to ensure mapping takes effect immediately.
   * This is critical when userpmap() is called after mmuswitch(), as the
   * CPU may have cached a "not present" TLB entry for this address. */
  __asm__ volatile("invlpg (%0)" ::"r"(va) : "memory");

  print("userpmap: va=%#p pa=%#p perms=%#ux pte=%#llux\n", va, pa, perms,
        (uvlong)*pte);
  splx(x);
  print("userpmap: EXIT\n");
}
