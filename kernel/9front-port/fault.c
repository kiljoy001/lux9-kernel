#include "9p_router.h"
#include "dat.h"
#include "fns.h"
#include "mem.h"
#include "pageown.h"
#include "portlib.h"
#include "u.h"
#include <error.h>

struct Segment *seg(struct Proc *p, uintptr addr, int dolock) {
  struct Segment **s, **et, *n;

  et = &p->seg[NSEG];
  for (s = p->seg; s < et; s++) {
    if ((n = *s) == nil)
      continue;
    if (addr >= n->base && addr < n->top) {
      if (dolock == 0)
        return n;

      qlock(n);
      if (addr >= n->base && addr < n->top)
        return n;
      qunlock(n);
    }
  }

  return nil;
}

_Noreturn static void faulterror(char *s, Chan *c) {
  char buf[ERRMAX];

  if (c != nil)
    snprint(buf, sizeof buf, "sys: %s accessing %s: %s", s, chanpath(c),
            up->errstr);
  else
    snprint(buf, sizeof buf, "sys: %s", s);
  if (up->nerrlab) {
    if (up->kp == 0)
      postnote(up, 1, buf, NDebug);
    up->psstate = nil;
    error(s);
  }
  pprint("suicide: %s\n", buf);
  pexit(s, 1);
}

void faultnote(char *type, char *access, uintptr addr) {
  char buf[ERRMAX];

  checkpages();
  snprint(buf, sizeof(buf), "sys: trap: %s %s addr=%#p", type, access, addr);
  postnote(up, 1, buf, NDebug);
}

static int pio(Segment *s, uintptr addr, uintptr soff, Page **p) {
  KMap *k;
  Chan *c;
  int n, ask;
  uintptr daddr, vaddr;
  Page *loadrec, *new;
  Image *image;

retry:
  loadrec = *p;
  if (loadrec == nil) { /* from a text/data image */
    daddr = s->fstart + soff;
    image = s->image;
    print("pio: soff=%#llux fstart=%#llux daddr=%#llux flen=%#llux\n",
          (uvlong)soff, (uvlong)s->fstart, (uvlong)daddr, (uvlong)s->flen);
    new = lookpage(image, daddr);
    if (new != nil) {
      *p = new;
      s->used++;
      return 0;
    }

    ask = image->c->iounit;
    if (ask == 0)
      ask = qiomaxatomic;
    ask &= -BY2PG;
    if (ask == 0)
      ask = BY2PG;

    daddr = soff & -ask;
    if (daddr + ask > s->flen)
      ask = s->flen - daddr;
    vaddr = s->base + daddr;
    daddr += s->fstart;
  } else { /* from a swap image */
    daddr = swapaddr(loadrec);
    image = swapimage;
    new = lookpage(image, daddr);
    if (new != nil) {
      *p = new;
      s->swapped--;
      putswap(loadrec);
      return 0;
    }
    vaddr = addr;
    ask = BY2PG;
  }
  qunlock(&s->qlock);

  c = image->c;
  if (waserror()) {
    if (strcmp(up->errstr, Eintr) == 0)
      return -1;
    faulterror(Eioload, c);
  }
  if (ask <= BY2PG) {
    new = newpage(vaddr, nil);
    new->daddr = daddr;
    k = kmap(new);
    if (waserror()) {
      kunmap(k);
      putpage(new);
      nexterror();
    }
    n = devtab[c->type]->read(c, (uchar *)VA(k), ask, daddr);
    if (n != ask)
      error(Eshort);
    if (n < BY2PG)
      memset((uchar *)VA(k) + n, 0, BY2PG - n);
    /* Debug: print first bytes loaded */
    {
      uchar *data = (uchar *)VA(k);
      print(
          "pio: read %d bytes from offset %#llux, first 16: %02x %02x %02x "
          "%02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\n",
          n, (uvlong)daddr, data[0], data[1], data[2], data[3], data[4],
          data[5], data[6], data[7], data[8], data[9], data[10], data[11],
          data[12], data[13], data[14], data[15]);
    }
    kunmap(k);
    settxtflush(new, s->flushme);
    cachepage(new, image);
    putpage(new);
    poperror();
  } else {
    uintptr o;
    Block *b;

    b = devtab[c->type]->bread(c, ask, daddr);
    if (waserror()) {
      freeblist(b);
      nexterror();
    }
    for (o = 0; o < ask; o += BY2PG) {
      new = lookpage(image, daddr + o);
      if (new != nil) {
        putpage(new);
        continue;
      }
      new = newpage(vaddr + o, nil);
      new->daddr = daddr + o;
      k = kmap(new);
      n = ask - o;
      if (n > BY2PG)
        n = BY2PG;
      else if (n < BY2PG)
        memset((uchar *)VA(k) + n, 0, BY2PG - n);
      if (readblist(b, (uchar *)VA(k), n, o) != n) {
        kunmap(k);
        putpage(new);
        error(Eshort);
      }
      kunmap(k);
      settxtflush(new, s->flushme);
      cachepage(new, image);
      putpage(new);
    }
    freeblist(b);
    poperror();
  }
  poperror();

  qlock(&s->qlock);
  /*
   *  race, another proc may have gotten here first
   *  (and the pager may have run on that page) while
   *  s was unlocked
   */
  if (*p != loadrec && !pagedout(*p))
    return 0;
  goto retry;
}

int fixfault(Segment *s, uintptr addr, int read) {
  Pte **pte, *etp;
  uintptr soff, mmuphys;
  Page **pg, *old, *new;

  print("fixfault: entry addr=%#llx type=%#x\n", (unsigned long long)addr,
        s->type);

  addr &= ~(BY2PG - 1);
  soff = addr - s->base;
  pte = &s->map[soff / PTEMAPMEM];
  if ((etp = *pte) == nil) {
    etp = ptealloc();
    if (etp == nil) {
      qunlock(&s->qlock);
      if (!waserror()) {
        resrcwait("no memory for ptealloc");
        poperror();
      }
      return -1;
    }
    *pte = etp;
    print("ptealloc assign: s=%p idx=%ld pte=%p\n", s, (long)(pte - s->map),
          etp);
  }

  pg = &etp->pages[(soff & (PTEMAPMEM - 1)) / BY2PG];
  if (pg < etp->first)
    etp->first = pg;
  if (pg > etp->last)
    etp->last = pg;

  switch (s->type & SG_TYPE) {
  default:
    panic("fault");

  case SG_TEXT: /* Demand load */
    if (pagedout(*pg)) {
      if (pio(s, addr, soff, pg) < 0)
        return -1;
    }
    mmuphys = PPN((*pg)->pa) | PTECACHED | PTEVALID;
    if (s->type & SG_RONLY)
      mmuphys |= PTERONLY;
    else
      mmuphys |= PTEWRITE;
    (*pg)->modref = PG_REF;
    break;

  case SG_BSS:
  case SG_SHARED: /* fill on demand */
  case SG_STACK:
    print("fixfault: stack/bss addr=%#llx seg=%p type=%#x pg=%p\n",
          (unsigned long long)addr, s, s->type, *pg);
    if (*pg == nil) {
      new = newpage(addr, s);
      if (new == nil) {
        print("fixfault: newpage returned nil for addr=%#llx\n",
              (unsigned long long)addr);
        return -1;
      }
      *pg = fillpage(new, (s->type & SG_TYPE) == SG_STACK ? 0xfe : 0);
      s->used++;
    }
    /* wet floor */
    /* fallthrough */
  case SG_DATA: /* Demand load/pagein/copy on write */
    if (pagedout(*pg)) {
      if (pio(s, addr, soff, pg) < 0)
        return -1;
    }
    /*
     *  It's only possible to copy on write if
     *  we're the only user of the segment.
     */
    if (read && conf.copymode == 0 && s->ref == 1) {
      mmuphys = PPN((*pg)->pa) | PTERONLY | PTECACHED | PTEVALID;
      (*pg)->modref |= PG_REF;
      break;
    }

    old = *pg;
    if (swapimage != nil && old->image == swapimage &&
        (old->ref + swapcount(old->daddr)) == 1)
      uncachepage(old);
    if (old->ref > 1 || old->image != nil) {
      /* ZERO-COPY ENFORCEMENT:
       * We cannot copy pages. If a page is shared (ref > 1) and a write occurs,
       * it implies a violation of the pure Exchange/Borrow model unless it's
       * SG_SHARED intent. But SG_DATA implies private data. If we are here, it
       * means we have a write fault on a shared page. Previously we would copy.
       * Now we must forbid it.
       */
      print("fixfault: COW attempt blocked on addr=%#p type=%d ref=%ld\n", addr,
            s->type, old->ref);
      panic("fixfault: Strict No-Copy Violation - Write to shared page");
      /* copyptr(old, new); -- REMOVED */
    }
    /* wet floor */
    /* fallthrough */
  case SG_STICKY: /* Never paged out */
    mmuphys = PPN((*pg)->pa) | PTEWRITE | PTECACHED | PTEVALID;
    (*pg)->modref |=
        up->privatemem ? PG_PRIV | PG_MOD | PG_REF : PG_MOD | PG_REF;
    break;

  case SG_FIXED: /* Never paged out */
    mmuphys = PPN((*pg)->pa) | PTEWRITE | PTEUNCACHED | PTEVALID;
    (*pg)->modref |=
        up->privatemem ? PG_PRIV | PG_MOD | PG_REF : PG_MOD | PG_REF;
    break;
  }

#ifdef PTENOEXEC
  if ((s->type & SG_NOEXEC) != 0 || s->flushme == 0)
    mmuphys |= PTENOEXEC;
#endif

  /* Ensure user-space pages are accessible from user mode */
  if (addr < USTKTOP)
    mmuphys |= PTEUSER;

  qunlock(&s->qlock);

  putmmu(addr, mmuphys, *pg);

  /* CRITICAL: Update kernel mapping for exchange page if we just allocated it.
   * KADDR translates physical address to kernel virtual address (HHDM).
   * This ensures syscallentry reads arguments from the CORRECT page (child's),
   * not the stale parent page.
   */
  if (addr == EXCHANGE_PAGE_ADDR && up != nil) {
    up->p9page = (uchar *)KADDR((*pg)->pa);
    up->p9page_phys = (*pg)->pa;
    /* print("fixfault: updated p->p9page for pid %lud to pa %#llx\n", up->pid,
     * (unsigned long long)up->p9page_phys); */
  }

  return 0;
}

static void mapphys(Segment *s, uintptr addr, int attr) {
  uintptr mmuphys;
  Page pg = {0};

  /* Debug: check if this is exchange page mapping */
  if (addr >= 0x7FFFFEEFF000ULL && addr < 0x7FFFFEEFF000ULL + 0x1000) {
    print("mapphys: EXCHANGE PAGE addr=%#p pseg->pa=%#p\n", addr, s->pseg->pa);
  }

  addr &= ~(BY2PG - 1);

  /*
   * LAZY ALLOCATION: If pseg->pa is 0, this is a demand-paged exchange page.
   * Allocate a fresh physical page now.
   */
  if (s->pseg->pa == 0) {
    void *kva = mallocalign(BY2PG, BY2PG, 0, 0);
    if (kva == nil) {
      print("mapphys: mallocalign failed for demand-paged exchange page\n");
      qunlock(&s->qlock);
      error(Enovmem);
    }
    memset(kva, 0, BY2PG);

    /* Store physical address in pseg */
    s->pseg->pa = PADDR(kva);

    print("mapphys: LAZY ALLOC exchange page pid=%lud kva=%p pa=%#p\n", up->pid,
          kva, s->pseg->pa);

    /* Register with borrow checker - process initially owns it */
    extern uintptr saved_limine_hhdm_offset;
    uintptr hhdm_va = s->pseg->pa + saved_limine_hhdm_offset;

    if (pageown_acquire(up, s->pseg->pa, hhdm_va) != POWN_OK) {
      print("mapphys: failed to acquire ownership of exchange page\n");
    }
  }

  pg.ref = 1;
  pg.va = addr;
  pg.pa = s->pseg->pa + (addr - s->base);
  settxtflush(&pg, s->flushme);

  mmuphys = (pg.pa & ~(1ull << 63 | (BY2PG - 1))) | PTEVALID;
  if (addr < USTKTOP)
    mmuphys |= PTEUSER;
  if ((attr & SG_RONLY) == 0)
    mmuphys |= PTEWRITE;
  else
    mmuphys |= PTERONLY;

#ifdef PTENOEXEC
  if ((attr & SG_NOEXEC) != 0 || s->flushme == 0)
    mmuphys |= PTENOEXEC;
#endif

#ifdef PTEDEVICE
  if ((attr & SG_DEVICE) != 0)
    mmuphys |= PTEDEVICE;
  else
#endif
      if ((attr & SG_CACHED) == 0)
    mmuphys |= PTEUNCACHED;
  else
    mmuphys |= PTECACHED;

  /* Debug: Verify flags before putmmu at exchange page */
  if (addr >= 0x7FFFFEEFF000ULL && addr < 0x7FFFFEEFF000ULL + 0x1000) {
    print(
        "mapphys: PRE-PUTMMU flags check: mmuphys=%#p (NX=%d W=%d VALID=%d)\n",
        mmuphys, (mmuphys & PTENOEXEC) ? 1 : 0, (mmuphys & PTEWRITE) ? 1 : 0,
        (mmuphys & PTEVALID) ? 1 : 0);
  }

  qunlock(&s->qlock);

  putmmu(addr, mmuphys, &pg);

  /* CRITICAL: Update kernel mapping for exchange page if we just allocated it.
   * This ensures the kernel reads from the CORRECT physical page.
   * Without this, up->p9page remains nil or points to the wrong page,
   * causing syscalls to be silently ignored.
   */
  if (addr == EXCHANGE_PAGE_ADDR && up != nil) {
    up->p9page = (uchar *)KADDR(pg.pa);
    up->p9page_phys = pg.pa;
    print("mapphys: updated up->p9page for pid %lud to pa %#llx kva %p\n",
          up->pid, (unsigned long long)pg.pa, up->p9page);
  }

  /* Verify data at exchange page after mapping */
  if (addr >= 0x7FFFFEEFF000ULL && addr < 0x7FFFFEEFF000ULL + 0x1000) {
    uchar *data = (uchar *)kaddr(pg.pa);
    print("mapphys: VERIFY after putmmu, reading PA=0x%p first 16: ", pg.pa);
    for (int i = 0; i < 16; i++)
      print("%02x ", data[i]);
    print("\n");
  }
}

int fault(uintptr addr, uintptr pc, int read) {
  Segment *s;
  char *sps;
  int pnd, ins, attr;

  print("fault: ENTRY addr=%#llx pc=%#llx pid=%ld\n", (unsigned long long)addr,
        (unsigned long long)pc, up ? up->pid : 0);

  if (up == nil)
    panic("fault: no user process pc=%#p addr=%#p", pc, addr);

  if (up->nlocks) {
    Lock *l = up->lastlock;
    print("fault: nlocks %d, proc %lud %s, addr %#p, lock %#p, lpc %#p\n",
          up->nlocks, up->pid, up->text, addr, l, l ? l->pc : 0);
  }

  pnd = up->notepending;
  ins = up->insyscall;
  up->insyscall = 1;
  sps = up->psstate;
  up->psstate = "Fault";

  m->pfault++;

  for (;;) {
    /* NOTE: spllo() removed here. Previously it re-enabled interrupts
     * which could trigger a timer interrupt -> sched() -> reschedule.
     * For newly forked children, this caused them to be rescheduled
     * mid-fault, never completing their TEXT page fix.
     * Fault handling must run to completion before allowing reschedule. */
    print("fault: calling seg(%p, %#llx, 1)\n", up, (unsigned long long)addr);

    s = seg(up, addr, 1); /* leaves s locked if seg != nil */
    print("fault: seg() returned %p\n", s);
    if (s == nil) {
      print("fault: seg lookup FAILED addr=%#llx pid=%ld\n",
            (unsigned long long)addr, up->pid);
      up->psstate = sps;
      up->insyscall = ins;
      return -1;
    }
    print("fault: addr=%#llx seg=%p type=%#x base=%#p top=%#p\n",
          (unsigned long long)addr, s, s->type, (void *)s->base,
          (void *)s->top);

    attr = s->type;
    if ((attr & SG_TYPE) == SG_PHYSICAL)
      attr |= s->pseg->attr;

    if ((attr & SG_FAULT) != 0 || read
            ? ((attr & SG_NOEXEC) != 0 || s->flushme == 0) &&
                  (addr & -BY2PG) == (pc & -BY2PG)
            : (attr & SG_RONLY) != 0) {
      qunlock(&s->qlock);
      up->psstate = sps;
      up->insyscall = ins;
      if (up->kp && up->nerrlab) /* for segio */
        error(Eio);
      return -1;
    }

    if ((attr & SG_TYPE) == SG_PHYSICAL) {
      mapphys(s, addr, attr);
      break;
    }

    if (fixfault(s, addr, read) == 0)
      break;

    splhi();
    switch (up->procctl) {
    case Proc_exitme:
    case Proc_exitbig:
      if (up->nerrlab) {
        up->psstate = sps;
        up->insyscall = ins;
        up->notepending |= pnd;
        error(up->procctl == Proc_exitbig
                  ? "Killed: Insufficient physical memory"
                  : "Killed");
      }
      procctl();
      break;
    }
  }

  up->psstate = sps;
  up->insyscall = ins;
  up->notepending |= pnd;

  /* Re-enable interrupts after fault is fully resolved.
   * This allows timer interrupts for scheduler preemption. */
  if (up && m && up->nlocks == 0)
    spllo();

  return 0;
}

/*
 * Called only in a system call
 */
int okaddr(uintptr addr, ulong len, int write) {
  Segment *s;
  int iterations = 0;

  /* DEBUG: Disabled verbose okaddr tracing
  print("okaddr: checking addr=%#p len=%lud write=%d\n", addr, len, write);
  */

  if ((long)len >= 0 && len <= -addr) {
    for (;;) {
      iterations++;
      if (iterations > 10) {
        /* DEBUG: Disabled verbose okaddr tracing
        print("okaddr: too many iterations! addr=%#p len=%lud\n", addr, len);
        */
        break;
      }

      s = seg(up, addr, 0);
      /* DEBUG: Disabled verbose okaddr tracing
      print("okaddr: iteration %d, seg=%p addr=%#p len=%lud\n",
            iterations, s, addr, len);
      */
      if (s == nil || (write && (s->type & SG_RONLY)))
        break;

      /* DEBUG: Disabled verbose okaddr tracing
      print("okaddr: segment base=%#p top=%#p\n", s->base, s->top);
      */

      if (addr + len > s->top) {
        /* DEBUG: Disabled verbose okaddr tracing
        print("okaddr: addr+len (%#p) > s->top (%#p), continuing\n",
              addr+len, s->top);
        */
        len -= s->top - addr;
        addr = s->top;
        continue;
      }
      /* DEBUG: Disabled verbose okaddr tracing
      print("okaddr: address is valid\n");
      */
      return 1;
    }
  }
  /* DEBUG: Disabled verbose okaddr tracing
  print("okaddr: address is INVALID\n");
  */
  return 0;
}

void validaddr(uintptr addr, ulong len, int write) {
  if (!okaddr(addr, len, write)) {
    pprint("suicide: invalid address %#p/%lud in sys call pc=%#p\n", addr, len,
           userpc());
    postnote(up, 1, "sys: bad address in syscall", NDebug);
    error(Ebadarg);
  }
}

/*
 * &s[0] is known to be a valid address.
 */
void *vmemchr(void *s, int c, ulong n) {
  uintptr a;
  ulong sz;
  void *t;

  a = (uintptr)s;
  for (;;) {
    sz = BY2PG - (a & (BY2PG - 1));
    if (n <= sz)
      break;
    /* spans pages; handle this page */
    t = memchr((void *)a, c, sz);
    if (t != nil)
      return t;
    a += sz;
    n -= sz;
    if (a < KZERO)
      validaddr(a, 1, 0);
  }

  /* fits in one page */
  return memchr((void *)a, c, n);
}

extern void checkmmu(uintptr, uintptr);

void checkpages(void) {
  uintptr addr, off;
  Pte *p;
  Page *pg;
  Segment **sp, **ep, *s;

  if (up == nil)
    return;

  for (sp = up->seg, ep = &up->seg[NSEG]; sp < ep; sp++) {
    if ((s = *sp) == nil)
      continue;
    qlock(&s->qlock);
    if (s->mapsize > 0) {
      for (addr = s->base; addr < s->top; addr += BY2PG) {
        off = addr - s->base;
        if ((p = s->map[off / PTEMAPMEM]) == nil)
          continue;
        pg = p->pages[(off & (PTEMAPMEM - 1)) / BY2PG];
        if (pagedout(pg))
          continue;
        checkmmu(addr, pg->pa);
      }
    }
    qunlock(&s->qlock);
  }
}
