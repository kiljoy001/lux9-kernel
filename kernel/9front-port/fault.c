#include "9p_router.h"
#include "dat.h"
#include "fns.h"
#include "hhdm.h"
#include "mem.h"
#include "pageown.h"
#include "portlib.h"
#include "u.h"
#include <error.h>

extern int boot_verbose;

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

/*@
  @ requires s == \null || \valid(s);
  @ requires c == \null || \valid(c);
  @ assigns \nothing;
  @*/
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

/*@
  @ requires type == \null || \valid(type);
  @ requires access == \null || \valid(access);
  @ assigns \nothing;
  @*/
void faultnote(char *type, char *access, uintptr addr) {
  char buf[ERRMAX];

  checkpages();
  snprint(buf, sizeof(buf), "sys: trap: %s %s addr=%#p", type, access, addr);
  postnote(up, 1, buf, NDebug);
}

static void
cow_restore_exclusive_page(Page *p)
{
  Proc *owner;

  if (p == nil || p->pa == 0 || p->token_color != PEBBLE_COLOR_RED)
    return;
  if (pageown_get_state(p->pa) != POWN_EXCLUSIVE)
    return;

  owner = pageown_get_owner(p->pa);
  p->token_color = PEBBLE_COLOR_BLACK;
  if (owner != nil) {
    lock(&pebble_global_lock);
    if (owner->pebble.red_inuse >= BY2PG)
      owner->pebble.red_inuse -= BY2PG;
    owner->pebble.black_inuse += BY2PG;
    unlock(&pebble_global_lock);
  }
}

static void
cow_return_shared_borrow(Page *p)
{
  enum PageOwnError err;

  if (p == nil || up == nil || p->pa == 0 || p->token_color != PEBBLE_COLOR_RED)
    return;

  err = pageown_return_shared(up, p->pa);
  if (err != POWN_OK) {
    print("fixfault: pageown_return_shared failed pa=%#p pid=%lud err=%d\n",
          p->pa, up->pid, err);
    error("fixfault shared return failed");
  }

  cow_restore_exclusive_page(p);
}

/*@
  @ requires s == \null || \valid(s);
  @ requires p == \null || \valid(p);
  @ assigns \nothing;
  @*/
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
    new = lookpage(image, daddr);
    if (new != nil) {
      *p = new;
      s->used++;
      return 0;
    }

#define PAGING_IO_SIZE (64 * 1024)

    ask = image->c->iounit;
    if (ask == 0)
      ask = PAGING_IO_SIZE;
    ask &= -BY2PG;
    if (ask == 0)
      ask = BY2PG;

    /* Check if device supports bread BEFORE calculating daddr alignment */
    c = image->c;
    if (c != nil && devtab[devno(c->type, 0)]->bread == nil && ask > BY2PG)
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
  if (c == nil)
    error(Eio);

  if (waserror()) {
    if (strcmp(up->errstr, Eintr) == 0)
      return -1;
    faulterror(Eioload, c);
  }

  /* If device doesn't support buffered reads (bread), limit to page size */
  if (devtab[devno(c->type, 0)]->bread == nil && ask > BY2PG)
    ask = BY2PG;

  if (ask <= BY2PG) {
    new = newpage(vaddr, nil);
    if (new == nil)
      error(Enomem);
    new->daddr = daddr;
    k = kmap(new);
    if (waserror()) {
      kunmap(k);
      putpage(new);
      nexterror();
    }
    n = devtab[devno(c->type, 0)]->read(c, (uchar *)VA(k), ask, daddr);
    if (n < 0)
      nexterror();
    if (n != ask)
      error(Eshort);
    if (n < BY2PG)
      memset((uchar *)VA(k) + n, 0, BY2PG - n);
    kunmap(k);
    settxtflush(new, s->flushme);
    cachepage(new, image);
    putpage(new);
    poperror();
  } else {
    uintptr o;
    Block *b;

    b = devtab[devno(c->type, 0)]->bread(c, ask, daddr);
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

/*@
  @ requires s == \null || \valid(s);
  @ assigns \nothing;
  @*/
int fixfault(Segment *s, uintptr addr, int read) {
  Pte **pte, *etp;
  uintptr soff, mmuphys;
  Page **pg, *old, *new;
  static int seg_trace_count;
  static int fixfault_trace_count;
  int trace = 0;

  if (boot_verbose && fixfault_trace_count < 16) {
    fixfault_trace_count++;
    trace = 1;
    print("fixfault: entry addr=%#llx type=%#x\n", (unsigned long long)addr,
          s->type);
  }

  addr &= ~(BY2PG - 1);
  soff = addr - s->base;
  if (seg_trace_count < 50 &&
      (addr == 0x617000 || addr == 0x617ff8 || addr == 0x7ffffef3aef8)) {
    seg_trace_count++;
    print("fixfault: seg pid=%lud addr=%#p type=%#x base=%#p top=%#p fstart=%#p flen=%#p image=%p\n",
          up ? up->pid : 0, (void *)addr, s->type, (void *)s->base,
          (void *)s->top, (void *)s->fstart, (void *)s->flen, s->image);
  }
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
    if (trace)
      print("ptealloc assign: s=%p idx=%ld pte=%p\n", s,
            (long)(pte - s->map), etp);
  }

  pg = &etp->pages[(soff & (PTEMAPMEM - 1)) / BY2PG];
  if (pg < etp->first)
    etp->first = pg;
  if (pg > etp->last)
    etp->last = pg;

  /* DEBUG: Trace potential corruption */
  if (*pg != nil) {
    uintptr ptrval = (uintptr)*pg;
    /* Page structs are allocated via xalloc, so they should be in kernel space.
     * We check if the pointer is in the canonical kernel address range. */
    if (ptrval < 0xFFFF800000000000ULL) {
      print("DEBUG: fixfault pid %lud addr=%#p etp=%p pg_ptr=%p *pg=%p "
            "(ILLEGAL POINTER)\n",
            up ? up->pid : 0, addr, etp, pg, *pg);
    }
  }

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
    if (boot_verbose) {
      static int stack_fault_count;
      stack_fault_count++;
      if (stack_fault_count <= 20 || stack_fault_count % 200 == 0) {
        print("FAULT-STACK: pid=%lud addr=%#p seg=%p type=%#x pg=%p\n",
              up ? up->pid : 0, addr, s, s->type, *pg);
      }
    }
    if (trace)
      print("fixfault: stack/bss addr=%#llx seg=%p type=%#x pg=%p\n",
            (unsigned long long)addr, s, s->type, *pg);
    if (*pg == nil) {
      new = newpage(addr, s);
      if (new == nil) {
        print("fixfault: newpage returned nil for addr=%#llx\n",
              (unsigned long long)addr);
        return -1;
      }
      if (boot_verbose && new != nil) {
        extern uintptr saved_limine_hhdm_offset;
        uintptr hhdm_va = new->pa + saved_limine_hhdm_offset;
        print("FAULT-STACK: newpage pa=%#p hhdm=%#p kaddr=%#p\n",
              new->pa, hhdm_va, kaddr(new->pa));
      }
      *pg = fillpage(new, (s->type & SG_TYPE) == SG_STACK ? 0xfe : 0);
      s->used++;
    }
    /* wet floor */
    /* fallthrough */
  case SG_DATA: /* Demand load/pagein/copy on write */
    if (pagedout(*pg)) {
      int pio_rc = pio(s, addr, soff, pg);
      if (seg_trace_count < 50 &&
          (addr == 0x617000 || addr == 0x617ff8 || addr == 0x7ffffef3aef8)) {
        seg_trace_count++;
        print("fixfault: pio addr=%#p soff=%#p rc=%d pg=%p\n",
              (void *)addr, (void *)soff, pio_rc, *pg);
      }
      if (pio_rc < 0)
        return -1;
    }
    /*
     *  It's only possible to copy on write if
     *  we're the only user of the segment.
     */
    if (read && conf.copymode == 0 && s->ref == 1) {
      mmuphys = PPN((*pg)->pa) | PTERONLY | PTECACHED | PTEVALID;
      if (trace)
        print("fixfault: SG_DATA mapping pa=%#llx\n", (uvlong)(*pg)->pa);
      (*pg)->modref |= PG_REF;
      break;
    }

    old = *pg;
    if (swapimage != nil && old->image == swapimage &&
        (old->ref + swapcount(old->daddr)) == 1)
      uncachepage(old);
    if (old->ref > 1 || old->image != nil) {
      /* ZERO-COPY ENFORCEMENT: (RELAXED FOR NOW) */
      new = newpage(addr, s);
      if (new == nil)
        return -1;
      copypage(old, new); /* RESTORED COPY-ON-WRITE FOR NOW */

      /* After copy, update page table link */
      putpage(old);
      cow_return_shared_borrow(old);
      *pg = new;
    } else {
      cow_restore_exclusive_page(old);
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
  if (up != nil && addr == p9_user_base(up)) {
    up->p9page = (uchar *)KADDR((*pg)->pa);
    up->p9page_phys = (*pg)->pa;
    /* print("fixfault: updated p->p9page for pid %lud to pa %#llx\n", up->pid,
     * (unsigned long long)up->p9page_phys); */
  }

  return 0;
}

/*@
  @ requires s == \null || \valid(s);
  @ assigns \nothing;
  @*/
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
    extern u64int limine_kernel_phys_base;
    uintptr kva_addr = (uintptr)kva;
    extern uintptr hhdm_base;

    print("DEBUG: mapphys kva=%p is_hhdm=%d hhdm_base=%p KZERO=%#llx\n", kva,
          is_hhdm_virt(kva), hhdm_base, (unsigned long long)KZERO);

    if (is_hhdm_virt(kva)) {
      s->pseg->pa = hhdm_phys(kva);
      print("mapphys: HHDM translation kva=%p -> pa=%#p\n", kva, s->pseg->pa);
    } else if (kva_addr >= KZERO) {
      s->pseg->pa = (kva_addr - KZERO) + limine_kernel_phys_base;
      print("mapphys: KZERO translation kva=%p -> pa=%#p\n", kva, s->pseg->pa);
    } else {
      s->pseg->pa = PADDR(kva); /* Fallback to macro */
      print("mapphys: PADDR fallback kva=%p -> pa=%#p\n", kva, s->pseg->pa);
    }

    print("mapphys: LAZY ALLOC exchange page pid=%lud kva=%p pa=%#p\n", up->pid,
          kva, s->pseg->pa);

    /* Register with borrow checker - process initially owns it */
    extern uintptr saved_limine_hhdm_offset;
    uintptr hhdm_va = s->pseg->pa + saved_limine_hhdm_offset;

    if (boot_verbose) {
      print("OWN-ACQ: pid=%lud pa=%#p hhdm=%#p kaddr=%#p off=%#p\n",
            up ? up->pid : 0, s->pseg->pa, hhdm_va, kaddr(s->pseg->pa),
            (void *)saved_limine_hhdm_offset);
    }

    if (pageown_acquire(up, s->pseg->pa, hhdm_va) != POWN_OK) {
      print("mapphys: failed to acquire ownership of exchange page\n");
    } else {
      print("mapphys: exchange owned pid=%lud pa=%#p hhdm=%#p p9page=%#p\n",
            up ? up->pid : 0, s->pseg->pa, (void *)hhdm_va,
            up ? up->p9page : nil);
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
  if (up != nil && addr == p9_user_base(up)) {
    up->p9page = (uchar *)KADDR(pg.pa);
    up->p9page_phys = pg.pa;
    print("mapphys: updated up->p9page for pid %lud to pa %#llx kva %p\n",
          up->pid, (unsigned long long)pg.pa, up->p9page);
  }

  /* Verify data at exchange page after mapping */
  if (addr >= 0x7FFFFEEFF000ULL && addr < 0x7FFFFEEFF000ULL + 0x1000) {
    uchar *data = (uchar *)kaddr(pg.pa);
    print("mapphys: VERIFY after putmmu, reading PA=0x%p first 16: ", pg.pa);
    /*@ loop invariant 0 <= i <= 16;
  @ loop assigns i;
  @ loop variant 16 - i;
  @*/
    for (int i = 0; i < 16; i++)
      print("%02x ", data[i]);
    print("\n");
  }
}

/*@
  @ assigns \nothing;
  @*/
int fault(uintptr addr, uintptr pc, int read) {
  Segment *s;
  char *sps;
  int pnd, ins, attr;
  static int fault_trace_count;
  int trace = 0;

  if (boot_verbose && fault_trace_count < 16) {
    fault_trace_count++;
    trace = 1;
    print("fault: ENTRY addr=%#llx pc=%#llx pid=%ld\n",
          (unsigned long long)addr, (unsigned long long)pc,
          up ? up->pid : 0);
  }

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
    if (trace)
      print("fault: calling seg(%p, %#llx, 1)\n", up,
            (unsigned long long)addr);

    s = seg(up, addr, 1); /* leaves s locked if seg != nil */
    if (trace)
      print("fault: seg() returned %p\n", s);
    if (s == nil) {
      print("fault: seg lookup FAILED addr=%#llx pid=%ld\n",
            (unsigned long long)addr, up->pid);
      up->psstate = sps;
      up->insyscall = ins;
      return -1;
    }
    if (trace)
      print("fault: addr=%#llx seg=%p type=%#x base=%#p top=%#p\n",
            (unsigned long long)addr, s, s->type, (void *)s->base,
            (void *)s->top);
    if (trace && (addr == 0x617000 || addr == 0x617ff8 ||
                  addr == 0x7ffffef3aef8 || addr == 0x638000 ||
                  addr == 0x637ff8 || addr == 0x7ffffefd9ef8)) {
      print("fault: seg detail pid=%ld tseg=%#p-%#p dseg=%#p-%#p bseg=%#p-%#p sseg=%#p-%#p p9seg=%#p-%#p\n",
            up ? up->pid : -1,
            up && up->seg[TSEG] ? (void *)up->seg[TSEG]->base : 0,
            up && up->seg[TSEG] ? (void *)up->seg[TSEG]->top : 0,
            up && up->seg[DSEG] ? (void *)up->seg[DSEG]->base : 0,
            up && up->seg[DSEG] ? (void *)up->seg[DSEG]->top : 0,
            up && up->seg[BSEG] ? (void *)up->seg[BSEG]->base : 0,
            up && up->seg[BSEG] ? (void *)up->seg[BSEG]->top : 0,
            up && up->seg[SSEG] ? (void *)up->seg[SSEG]->base : 0,
            up && up->seg[SSEG] ? (void *)up->seg[SSEG]->top : 0,
            up && up->seg[P9SEG] ? (void *)up->seg[P9SEG]->base : 0,
            up && up->seg[P9SEG] ? (void *)up->seg[P9SEG]->top : 0);
    }

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
/*@
  @ assigns \nothing;
  @*/
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

/*@
  @ assigns \nothing;
  @*/
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

/*@
  @ assigns \nothing;
  @*/
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
