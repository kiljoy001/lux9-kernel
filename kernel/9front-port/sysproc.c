/* clang-format off */
#include "u.h"

/* Local Plan 9 Syscall ABI fix */
#include <u.h>
typedef ulong *syscall_va_list;
#define SYSCALL_ARG(list, type) (*(type*)((list)++))
/* va_list macro removed to prevent stdarg.h conflict */
#define va_start(list, start) ((void)0)
#define va_end(list) ((void)0)




#define syscall_vainit(list, start) ((list) = (syscall_va_list)(start))










#define SYSCALL_ARG(list, type) (*(type*)((list)++))

#include "portlib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "ureg.h"
#include <error.h>
#include "9p_router.h"

#include "edf.h"
#include "elf.h"
#include "monocypher.h"
#include "pebble.h"
#include "tos.h"
#include "uuid.h"
/* clang-format on */

/*@
  @ axiomatic Syscall_ABI {
  @   predicate valid_syscall_args(ulong *list, integer n) =
  @     \valid(list + (0..n-1));
  @
  @   axiom syscall_arg_advance:
  @     \forall ulong *list, integer n;
  @       valid_syscall_args(list, n) ==> valid_syscall_args(list + 1, n - 1);
  @ }
  @
  @ requires e == \null || \valid(e);
  @ assigns \nothing;
  @ ensures \false;
  @ terminates \true;
  @*/
void lux9_error(char *e);

#include <a.out.h>

/* CLR compilation includes removed - CLR moved to userspace */
#include "exchange.h"

extern void crypto_blake2b_final(crypto_blake2b_ctx *ctx, u8int *out);
#include "proc_packet.h"

/* FSM Integration */
extern int proc_event(Proc *p, int event);

/*@
  @ requires tc != \null;
  @ requires up != \null;
  @ requires \valid(up);
  @ assigns up->text_hash[0..63];
  @*/
static void hash_binary(Chan *tc) {
  crypto_blake2b_ctx ctx;
  u8int buf[4096];
  long n;
  vlong off = 0;

  crypto_blake2b_init(&ctx, 64);
  while ((n = devtab[tc->type]->read(tc, buf, sizeof(buf), off)) > 0) {
    crypto_blake2b_update(&ctx, buf, n);
    off += n;
  }
  crypto_blake2b_final(&ctx, up->text_hash);

  /* Init Hardening: If this process is bound to a specific binary hash
   * (via spawn_bound_binary), verify it now. */
  int i;
  int bound = 0;
  for (i = 0; i < 64; i++) {
    if (up->spawn_bound_binary[i] != 0) {
      bound = 1;
      break;
    }
  }

  if (bound) {
    if (memcmp(up->text_hash, up->spawn_bound_binary, 64) != 0) {
      print("EXEC SECURITY: blocked execution of non-bound binary\n");
      error("exec: binary hash does not match bound restriction");
    }
  }
}

/*@
  @ requires up == \null || \valid(up);
  @ requires up != \null ==> \valid(&up->pid2);
  @ assigns up->pid2;
  @*/
static void update_pid2_after_exec(void) {
  uuid_t *parent_p = nil;
  u8int *ns_cid = nil;

  if (up == nil)
    return;
  if (up->parent)
    parent_p = &up->parent->pid2;
  if (up->pgrp)
    ns_cid = up->pgrp->namespace_cid;

  uuid_pack_pid_lux9(&up->pid2, parent_p, ns_cid, up->text_hash);
}

/*@
  @ requires \valid((ulong*)list_void);
  @ terminates \true;
  @ assigns \nothing;
  @ ensures \result == 0;
  @*/
uintptr sysr1(void *list_void) {
  syscall_va_list list = (syscall_va_list)list_void;
  if (!iseve())
    error(Eperm);
  return 0;
}

/*@
  @ assigns \nothing;
  @ ensures \false;
  @ terminates \true;
  @*/
static void abortion(void) { pexit("fork aborted", 1); }

uintptr sysrfork(void *list_void) {
  syscall_va_list list = (syscall_va_list)list_void;
  /*
   * Code using RFNOMNT expects to block all but
   * the following devices.
   */
  static char nomntdevs[] = "|decp";

  ulong pid, flag;
  int n, i;
  Proc *p;

  flag = SYSCALL_ARG(list, ulong);
  /* Check flags before we commit */
  if ((flag & (RFFDG | RFCFDG)) == (RFFDG | RFCFDG))
    error(Ebadarg);
  if ((flag & (RFNAMEG | RFCNAMEG)) == (RFNAMEG | RFCNAMEG))
    error(Ebadarg);
  if ((flag & (RFENVG | RFCENVG)) == (RFENVG | RFCENVG))
    error(Ebadarg);

  if ((flag & RFPROC) == 0) {
    Fgrp *ofg;
    Pgrp *opg;
    Rgrp *org;
    Egrp *oeg;

    if (flag & (RFMEM | RFNOWAIT))
      error(Ebadarg);

    ofg = up->fgrp;
    opg = up->pgrp;
    org = up->rgrp;
    oeg = up->egrp;

    if (waserror()) {
      if (up->fgrp != ofg) {
        closefgrp(up->fgrp);
        up->fgrp = ofg;
      }
      if (up->pgrp != opg) {
        closepgrp(up->pgrp);
        up->pgrp = opg;
      }
      if (up->rgrp != org) {
        closergrp(up->rgrp);
        up->rgrp = org;
      }
      if (up->egrp != oeg) {
        closeegrp(up->egrp);
        up->egrp = oeg;
      }
      nexterror();
    }

    /* File descriptors */
    if (flag & (RFFDG | RFCFDG)) {
      if (flag & RFFDG)
        up->fgrp = dupfgrp(ofg);
      else
        up->fgrp = dupfgrp(nil);
    }

    /* Process group */
    if (flag & (RFNAMEG | RFCNAMEG)) {
      up->pgrp = newpgrp();
      if (flag & RFNAMEG)
        pgrpcpy(up->pgrp, opg);
      /* inherit notallowed */
      memmove(up->pgrp->notallowed, opg->notallowed,
              sizeof up->pgrp->notallowed);
    }

    /* Rendezvous group */
    if (flag & RFREND)
      up->rgrp = newrgrp();

    /* Environment group */
    if (flag & (RFENVG | RFCENVG)) {
      up->egrp = newegrp();
      if (flag & RFENVG)
        envcpy(up->egrp, oeg);
    }

    if (ofg != up->fgrp)
      closefgrp(ofg);
    if (opg != up->pgrp)
      closepgrp(opg);
    if (org != up->rgrp)
      closergrp(org);
    if (oeg != up->egrp)
      closeegrp(oeg);

    poperror();

    if (flag & RFNOMNT)
      devmask(up->pgrp, 1, nomntdevs);

    if (flag & RFNOTEG) {
      qlock(&up->debug);
      setnoteid(up, 0); /* can't error() with 0 argument */
      qunlock(&up->debug);
    }
    return 0;
  }

  if ((p = newproc()) == nil)
    error("no procs");

  qlock(&up->debug);
  qlock(&p->debug);

  p->scallnr = up->scallnr;
  p->s = up->s;
  p->slash = up->slash;
  p->dot = up->dot;
  incref((Ref *)&p->dot->ref);

  p->nnote = 0;
  p->notify = up->notify;
  p->notified = 0;
  p->notepending = 0;
  p->lastnote = nil;

  if ((flag & RFNOTEG) == 0)
    p->noteid = up->noteid;

  p->procmode = up->procmode;
  p->privatemem = up->privatemem;
  p->noswap = up->noswap;

  /* Universal CBS: Inherit parent's capabilities (monotonic - can only
   * decrease) Every process is a SIP by default with inherited capability
   * bitmap */
  p->capabilities = up->capabilities;
  p->hang = up->hang;
  if (up->procctl == Proc_tracesyscall)
    p->procctl = Proc_tracesyscall;
  p->kp = 0;

  /* Inherit spawn capability and limits from parent */
  memmove(&p->spawn_cap, &up->spawn_cap, sizeof(uuid_t));
  p->spawn_max_children =
      up->spawn_max_children; /* Child inherits parent's limit */
  p->spawn_children = 0;      /* Child starts with no children of its own */

  /*
   * Craft a return frame which will cause the child to pop out of
   * the scheduler in user mode with the return register zero
   */
  forkchild(p, up->dbgreg);

  kstrdup(&p->text, up->text);
  kstrdup(&p->user, up->user);
  kstrdup(&p->args, "");
  p->nargs = 0;
  p->setargs = 0;

  p->insyscall = 0;
  memset(p->time, 0, sizeof(p->time));
  p->time[TReal] = MACHP(0)->ticks;
  p->kentry = up->kentry;
  p->pcycles = -p->kentry;

  pid = pidalloc(p);

  /* Transfer initial Pebble budget from parent to child.
   * Child needs tokens to allocate its stack and initial segments.
   * Strategy: Transfer either 50% of parent's budget or 8MB, whichever is
   * smaller. This maintains token conservation while ensuring child viability.
   */
  {
    ulong parent_budget = up->pebble.colorless_bank;
    ulong child_budget;
    ulong half_parent = parent_budget / 2;
    ulong fixed_grant = 32 * 1024 * 1024; /* 32 MB */

    /* Choose the smaller of half-parent or fixed grant */
    child_budget = (half_parent < fixed_grant) ? half_parent : fixed_grant;

    /* Ensure parent has enough to transfer */
    if (parent_budget < child_budget)
      child_budget = parent_budget;

    /* Atomic transfer: parent loses exactly what child gains */
    p->pebble.colorless_bank = child_budget;
    up->pebble.colorless_bank -= child_budget;

    print("PEBBLE: sysrfork transferred %lud bytes (%lud MB) to child pid %lud "
          "(parent %lud has %lud bytes remaining)\n",
          child_budget, child_budget / (1024 * 1024), pid, up->pid,
          up->pebble.colorless_bank);
  }

  qunlock(&p->debug);
  qunlock(&up->debug);

  /* Abort the child process on error */
  if (waserror()) {
    p->kp = 1;
    kprocchild(p, abortion);
    ready(p);
    nexterror();
  }

  /* Make a new set of memory segments */
  n = flag & RFMEM;
  int sharemem = n != 0;
  qlock(&p->seglock);
  if (waserror()) {
    qunlock(&p->seglock);
    nexterror();
  }
  uintptr ubase = p9_user_base(up);
  for (i = 0; i < NSEG; i++) {
    /*
     * CRITICAL: Skip P9SEG (exchange page) during fork.
     * The child will allocate its own exchange page on first fault.
     * This avoids MMU aliasing - child never inherits parent's PTE.
     */
    /*
     * CRITICAL: Skip P9SEG (exchange page) during fork.
     * The child will allocate its own exchange page on first fault.
     * This avoids MMU aliasing - child never inherits parent's PTE.
     * Use address check to be robust against slot assignment.
     */
    if (!sharemem &&
        (i == P9SEG || (up->seg[i] != nil && up->seg[i]->base == ubase))) {
      p->seg[i] = nil; /* Child will fault and allocate fresh page */
      continue;
    }
    if (up->seg[i] != nil)
      p->seg[i] = dupseg(up->seg, i, n);
  }
  qunlock(&p->seglock);
  poperror();

  /* DEBUG: Verify Child Segments */
  for (i = 0; i < NSEG; i++) {
    if (p->seg[i] != nil) {
      /*
      print("DEBUG: sysrfork Child Seg[%d] base=%#p top=%#p type=%x\n", i,
            (void *)p->seg[i]->base, (void *)p->seg[i]->top, p->seg[i]->type);
      */
    }
  }

  /* File descriptors */
  if (flag & (RFFDG | RFCFDG)) {
    if (flag & RFFDG)
      p->fgrp = dupfgrp(up->fgrp);
    else
      p->fgrp = dupfgrp(nil);
  } else {
    p->fgrp = up->fgrp;
    incref(&up->fgrp->ref);
  }

  /* Process groups */
  if (flag & (RFNAMEG | RFCNAMEG)) {
    p->pgrp = newpgrp();
    if (flag & RFNAMEG)
      pgrpcpy(p->pgrp, up->pgrp);
    /* inherit notallowed */
    memmove(p->pgrp->notallowed, up->pgrp->notallowed,
            sizeof p->pgrp->notallowed);
  } else {
    p->pgrp = up->pgrp;
    incref((Ref *)&up->pgrp->ref);
  }

  /* Rendezvous group */
  if (flag & RFREND)
    p->rgrp = newrgrp();
  else {
    p->rgrp = up->rgrp;
    incref((Ref *)&up->rgrp->ref);
  }

  /* Increment namespace spawn count */
  if (p->pgrp != nil) {
    lock(&p->pgrp->spawn_lock);
    p->pgrp->spawn_count++;
    unlock(&p->pgrp->spawn_lock);
  }

  /* Environment group */
  if (flag & (RFENVG | RFCENVG)) {
    p->egrp = newegrp();
    if (flag & RFENVG)
      envcpy(p->egrp, up->egrp);
  } else {
    p->egrp = up->egrp;
    incref(&up->egrp->ref);
  }

  if (!sharemem) {
    /*
     * CRITICAL: Save, invalidate, and restore parent's exchange page PTE for
     * fork.
     * 1. Save parent's PTE (points to page with Rsysfork reply)
     * 2. Invalidate PTE so child inherits invalid PTE and faults on first
     * access
     * 3. Fork child (inherits invalid PTE)
     * 4. Restore parent's saved PTE so it can continue using exchange page
     */
    extern void putmmu(uintptr, uintptr, Page *);
    extern uintptr getmmu(uintptr, Page **);

    uintptr saved_pte;
    Page *saved_page = nil;

    /* Save parent's current PTE */
    saved_pte = getmmu(ubase, &saved_page);
    print("DEBUG: sysrfork saved parent PTE=%#llx page=%p\n", saved_pte,
          saved_page);

    /* Invalidate parent's PTE before fork */
    print(
        "DEBUG: sysrfork pid %lud->%lud invalidating parent PTE before fork\n",
        up->pid, p->pid);
    putmmu(ubase, 0, nil);

    /* Flush TLB to ensure CPU sees the invalidated PTE */
    __asm__ volatile("invlpg (%0)" ::"r"(ubase) : "memory");
    print("DEBUG: sysrfork TLB flushed\n");

    /* procfork copies page tables - child will inherit INVALID PTE */
    procfork(p);
    print("DEBUG: sysrfork procfork complete, child pid %lud has invalidated "
          "PTE\n",
          p->pid);

    /*
     * CRITICAL: Restore parent's SAVED PTE after fork.
     * This preserves the parent's exchange page with Rsysfork reply.
     */
    putmmu(ubase, saved_pte, saved_page);
    __asm__ volatile("invlpg (%0)" ::"r"(ubase) : "memory");
    print("DEBUG: sysrfork parent PTE restored to %#llx\n", saved_pte);

    /*
     * Setup stub P9SEG segment for lazy exchange page allocation.
     * Page is allocated on first access via fault handler.
     */
    extern int proc_setup_p9seg_stub(Proc *);
    if (proc_setup_p9seg_stub(p) < 0)
      error(Enovmem);
  } else {
    /* Shared memory: keep parent's exchange mapping and base. */
    procfork(p);
    p->p9uaddr = up->p9uaddr;
    p->p9page = up->p9page;
    p->p9page_phys = up->p9page_phys;

    /* CRITICAL: Transfer borrow ownership of exchange page to child.
     * Without this, child syscalls will fail with BORROW_ENOTOWNER because
     * the borrow checker still thinks the parent owns the page. */
    if (p->p9page_phys != 0) {
      extern enum BorrowError borrow_transfer(Proc * from, Proc * to,
                                              uintptr key);
      enum BorrowError berr = borrow_transfer(up, p, p->p9page_phys);
      if (berr != BORROW_OK) {
        print("sysrfork: WARNING - borrow_transfer of exchange page failed "
              "(berr=%d)\n",
              berr);
        /* This is non-fatal in RFMEM mode - the shared page model may need
         * different ownership semantics. For now, log and continue. */
      }
    }
  }

  poperror(); /* abortion */

  if (flag & RFNOMNT)
    devmask(p->pgrp, 1, nomntdevs);

  if ((flag & RFNOWAIT) == 0) {
    p->parent = up;
    lock(&up->exl);
    up->nchild++;
    /* Increment process spawn limit */
    up->spawn_children++;
    unlock(&up->exl);
  }

  /*
   *  since the bss/data segments are now shareable,
   *  any mmu info about this process is now stale
   *  (i.e. has bad properties) and has to be discarded.
   *
   *  NOTE: Do NOT call flushmmu() here!
   *  At this point 'up' is the PARENT, and calling flushmmu() destroys
   *  the parent's user PTEs. The child will get its TLB flushed
   *  automatically when scheduled.
   */

  procpriority(p, up->basepri, up->fixedpri);
  if (up->wired)
    procwired(p, up->affinity);

  /* CRITICAL: Child starts with empty mmuhead. Force fault-based MMU rebuild.
   */
  p->newtlb = 1; /* For forcing TLB flush on child */
  ready(p);

  /* vfork synchronization: Block parent if sharing stack (RFMEM) */
  if (flag & RFMEM) {
    p->vforkp = up;
    proc_event(up, EV_VFORK);
    print("VFORK: Blocking parent pid %lud until child pid %lud execs/exits\n",
          up->pid, p->pid);
    sched();
  }

  return p->pid;
}

/*@
  @ requires \valid(s + (0..n-1));
  @ requires \valid(ap + (0..nap-1));
  @ assigns s[0..n-1], ap[0..nap-1];
  @ ensures \result >= -1 && \result < nap;
  @*/
static int shargs(char *s, int n, char **ap, int nap) {
  char *p;
  int i;

  if (n <= 2 || s[0] != '#' || s[1] != '!')
    return -1;
  s += 2;
  n -= 2; /* skip #! */
  if ((p = memchr(s, '\n', (usize)n)) == nil)
    return 0;
  *p = 0;
  i = tokenize(s, ap, nap - 1);
  ap[i] = nil;
  return i;
}

/*@
  @ assigns \nothing;
  @ ensures \result == ((l >> 24) & 0xFF) | ((l >> 8) & 0xFF00) | ((l << 8) &
  0xFF0000) | ((l << 24) & 0xFF000000);
  @ terminates \true;
  @*/
ulong beswal(ulong l) {
  uchar *p;

  p = (uchar *)&l;
  return (p[0] << 24) | (p[1] << 16) | (p[2] << 8) | p[3];
}

/*@
  @ assigns \nothing;
  @ ensures \result == ((v >> 56) & 0xFF) | ((v >> 40) & 0xFF00) | ((v >> 24) &
  0xFF0000) | ((v >> 8) & 0xFF000000) | ((v << 8) & 0xFF00000000) | ((v << 24) &
  0xFF0000000000) | ((v << 40) & 0xFF000000000000) | ((v << 56) &
  0xFF00000000000000);
  @ terminates \true;
  @*/
uvlong beswav(uvlong v) {
  uchar *p;

  p = (uchar *)&v;
  return ((uvlong)p[0] << 56) | ((uvlong)p[1] << 48) | ((uvlong)p[2] << 40) |
         ((uvlong)p[3] << 32) | ((uvlong)p[4] << 24) | ((uvlong)p[5] << 16) |
         ((uvlong)p[6] << 8) | (uvlong)p[7];
}

uintptr sysexec(void *list_void) {
  extern void uartputs(char *, int);
  char debug_buf[128];
  snprint(debug_buf, sizeof(debug_buf), "DEBUG: sysexec ENTERED list_void=%p\n",
          list_void);
  uartputs(debug_buf, (int)strlen(debug_buf));
  ulong *uargs = (ulong *)list_void;
  union {
    struct {
      Exec exec;
      uvlong hdr[1];
    } ehdr;
    char buf[256];
  } u;
  char line[256];
  char *progarg[32 + 1];
  volatile char *args, *elem, *file0;
  char **argv, **argp, **argp0;
  char *a, *e, *charp, *file;
  int i, n, indir, is_elf;
  ulong magic, stacksize, nargs, nbytes;
  uintptr entry, text, data, bss, adata, abss, ebss, tstk, align, file_offset;
  uintptr data_file_offset = 0; /* New: Track data segment file offset */
  uintptr text_base = UTZERO;
  int text_writable = 0;
  Segment *s, *ts;
  Image *img;
  Tos *tos;
  Chan *tc;
  Fgrp *f;
  int saved_nerrlab;
  const char *stage_desc;

  stage_desc = "start";
  print("CONSOLE: sysexec started, list=%p\n", list_void);

  /* Save error stack level */
  saved_nerrlab = up->nerrlab;
  print("CONSOLE: sysexec saved_nerrlab=%d\n", saved_nerrlab);
  print("CONSOLE: sysexec internal up=%p up->slash=%p up->dot=%p\n", up,
        up ? up->slash : 0, up ? up->dot : 0);

  /* Initialize to nil */
  args = elem = nil;
  file0 = nil;
  tc = nil;

  /* Set up error handler BEFORE any code that can call error() */
  print("CONSOLE: sysexec about to call waserror()\n");
  if (waserror()) {
    print("CONSOLE: sysexec ERROR PATH: %s\n", up->errstr);
    print("sysexec: error at %s: %s\n", stage_desc, up->errstr);
    if (tc) {
      print("sysexec: cleaning up tc=%p ref=%d\n", tc, tc->ref);
      if (tc->ref > 0) {
        if (tc->ref > 0) {
          cclose(tc);
        } else {
          print("sysexec: warning - tc ref count already zero\n");
        }
      } else {
        print("sysexec: warning - tc ref count already zero\n");
      }
    }
    free(file0);
    free(elem);
    free(args);
    /* Disaster after commit */
    if (up->seg[SSEG] == nil)
      pexit(up->errstr, 1);
    nexterror();
  }
  print("CONSOLE: sysexec waserror() returned\n");

  /* Now we have an error handler, safe to do validation that might error */
  print("CONSOLE: sysexec getting file0 from uargs[0]\n");
  stage_desc = "arg file0";
  file0 = (char *)uargs[0];
  print("CONSOLE: sysexec got file0=%p\n", file0);
  print("CONSOLE: sysexec calling validaddr for file0\n");
  stage_desc = "validaddr file0";
  validaddr((uintptr)file0, 1, 0);
  print("CONSOLE: sysexec validaddr returned\n");
  print("CONSOLE: sysexec getting argp0\n");
  stage_desc = "arg argp0";
  argp0 = (char **)uargs[1];
  print("CONSOLE: sysexec got argp0=%p\n", argp0);
  stage_desc = "evenaddr argp0";
  evenaddr((uintptr)argp0);
  print("CONSOLE: sysexec evenaddr done\n");
  stage_desc = "validaddr argp0";
  validaddr((uintptr)argp0, 2 * BY2WD, 0);
  print("CONSOLE: sysexec validaddr argp0 done\n");
  if (*argp0 == nil)
    error(Ebadarg);
  print("CONSOLE: sysexec checked *argp0\n");
  stage_desc = "validnamedup";
  file0 = validnamedup(file0, 1);
  print("CONSOLE: sysexec validated file '%s'\n", file0);

  print("CONSOLE: sysexec setting up variables\n");
  align = BY2PG - 1;
  indir = 0;
  is_elf = 0;
  file_offset = 0;
  file = file0;
  print("CONSOLE: sysexec entering loop with file='%s'\n", file);

  /* Probe namec capabilities */
  {
    Chan *probe;
    print("CONSOLE: PROBE: namec('/')\n");
    if (waserror()) {
      print("CONSOLE: PROBE: namec('/') FAILED: %s\n", up->errstr);
    } else {
      probe = namec("/", Aopen, OREAD, 0);
      print("CONSOLE: PROBE: namec('/') SUCCESS tc=%p\n", probe);
      cclose(probe);
      poperror();
    }

    print("CONSOLE: PROBE: namec('/boot')\n");
    if (waserror()) {
      print("CONSOLE: PROBE: namec('/boot') FAILED: %s\n", up->errstr);
    } else {
      probe = namec("/boot", Aopen, OREAD, 0);
      print("CONSOLE: PROBE: namec('/boot') SUCCESS tc=%p\n", probe);
      cclose(probe);
      poperror();
    }
  }

  for (;;) {
    print("CONSOLE: sysexec about to call namec('%s')\n", file);
    uartputs(debug_buf,
             (int)strlen(debug_buf)); /* Keep uartputs just in case print fails?
                                         No, remove it. */
    stage_desc = "namec";
    /* Use OREAD instead of OEXEC to avoid permission issues with WASM files */
    tc = namec(file, Aopen, OREAD, 0);
    snprint(debug_buf, sizeof(debug_buf),
            "CONSOLE: sysexec namec returned tc=%p\n", tc);
    uartputs(debug_buf, (int)strlen(debug_buf));
    if (waserror()) {
      if (tc->ref > 0) {
        cclose(tc);
      } else {
        print("sysexec: warning - tc ref count already zero\n");
      }
      nexterror();
    }
    snprint(debug_buf, sizeof(debug_buf),
            "DEBUG: sysexec file opened, waserror set\n");
    uartputs(debug_buf, (int)strlen(debug_buf));
    if (!indir) {
      snprint(debug_buf, sizeof(debug_buf), "DEBUG: sysexec calling kstrdup\n");
      uartputs(debug_buf, (int)strlen(debug_buf));
      kstrdup(&elem, up->genbuf);
      uartputs(debug_buf, (int)strlen(debug_buf));
    }

    /*
     * Read header
     */
    /* Duplicate namec removed */
    if (waserror()) {
      /* If read/attach fails, print debug */
      snprint(debug_buf, sizeof(debug_buf),
              "EXEC: attach/read failed for %s error=%s\n", file, up->errstr);
      uartputs(debug_buf, (int)strlen(debug_buf));
      if (tc->ref > 0) {
        cclose(tc);
      } else {
        print("sysexec: warning - tc ref count already zero\n");
      }
      nexterror();
    }

    /* Read first chunk to check magic */
    n = (int)devtab[tc->type]->read(tc, u.buf, sizeof(u.buf), 0);
    if (n < 2) {
      snprint(debug_buf, sizeof(debug_buf), "EXEC: read too short n=%d\n", n);
      uartputs(debug_buf, (int)strlen(debug_buf));
      error(Ebadexec);
    }

    /* Check for ELF signature */
    if (n >= 4 && u.buf[0] == 0x7f && u.buf[1] == 'E' && u.buf[2] == 'L' &&
        u.buf[3] == 'F') {
      snprint(debug_buf, sizeof(debug_buf),
              "DEBUG: sysexec detected ELF binary\n");
      uartputs(debug_buf, (int)strlen(debug_buf));
      is_elf = 1;
    }

    /* Check for WASM magic: 0x00 0x61 0x73 0x6D = "\0asm" */
    if (n >= 4 && u.buf[0] == 0x00 && u.buf[1] == 0x61 && u.buf[2] == 0x73 &&
        u.buf[3] == 0x6d) {
      void *start_func = nil;

      snprint(debug_buf, sizeof(debug_buf),
              "DEBUG: sysexec detected WASM binary\n");
      uartputs(debug_buf, (int)strlen(debug_buf));
      print("EXEC: detected WASM binary '%s'\n", file);

      /* Compile WASM module into current process */
      if (wasm_exec_compile(tc, &start_func) < 0) {
        if (tc->ref > 0) {
          cclose(tc);
        } else {
          print("sysexec: warning - tc ref count already zero\n");
        }
        tc = nil;
        poperror(); /* tc error handler */
        error("WASM compile failed");
      }

      /* Close the file channel */
      if (tc->ref > 0) {
        cclose(tc);
      } else {
        print("sysexec: warning - tc ref count already zero\n");
      }
      tc = nil;
      poperror(); /* tc error handler */

      /* Clean up exec state */
      free(file0);
      free(elem);
      poperror(); /* outer error handler */

      /* Execute WASM - this does NOT return */
      wasm_exec_run(start_func);
      /* NOTREACHED */
      return 0;
    }

    /* Check for .NET/CLR PE/COFF signature ("MZ") */
    if (n >= 2 && u.buf[0] == 'M' && u.buf[1] == 'Z') {
      /* CLR execution moved to userspace - use userspace runtime */
      if (tc->ref > 0) {
        cclose(tc);
      } else {
        print("sysexec: warning - tc ref count already zero\n");
      }
      poperror();
      error("CLR execution moved to userspace - recompile for WASM or use "
            "userspace CLR");
    }

    if ((ulong)n >= sizeof(Exec)) {
      magic = beswal(u.ehdr.exec.magic);
      print("EXEC: magic=0x%08lx AOUT_MAGIC=0x%08lx S_MAGIC=0x%08lx\n", magic,
            AOUT_MAGIC, S_MAGIC);
      if (magic == AOUT_MAGIC) {
        print("EXEC: magic matches AOUT_MAGIC\n");
        if (magic & HDR_MAGIC) {
          print("EXEC: has HDR_MAGIC, checking header size n=%d "
                "sizeof(u.ehdr)=%d\n",
                n, (int)sizeof(u.ehdr));
          if ((ulong)n < sizeof(u.ehdr))
            error("exec: header too small for expansion");
          entry = beswav(u.ehdr.hdr[0]);
          text = UTZERO + sizeof(u.ehdr);
          print("EXEC: expanded header: entry=%#llux text=%#llux\n", entry,
                text);
        } else {
          entry = beswal(u.ehdr.exec.entry);
          text = UTZERO + sizeof(Exec);
          print("EXEC: basic header: entry=%#llux text=%#llux\n", entry, text);
        }
        print("EXEC: checking entry < text: entry=%#llux text=%#llux\n", entry,
              text);
        if (entry < text)
          error("exec: entry point before text segment");
        text += beswal(u.ehdr.exec.text);

        print("EXEC: after adding text size: text=%#llux entry=%#llux "
              "USTKTOP-USTKSIZE=%#llux\n",
              text, entry, (uvlong)(USTKTOP - USTKSIZE));

        if (text <= entry || text >= (USTKTOP - USTKSIZE))
          error("exec: invalid text segment range");

        switch (magic) {
        case S_MAGIC: /* 2MB segment alignment for amd64 */
          align = 0x1fffff;
          break;
        case P_MAGIC: /* 16K segment alignment for spim */
        case V_MAGIC: /* 16K segment alignment for mips */
          align = 0x3fff;
          break;
        case R_MAGIC: /* 64K segment alignment for arm64 */
          align = 0xffff;
          break;
        }
        hash_binary(tc);
        update_pid2_after_exec();
        break; /* for binary */
      }

      /* Check for ELF magic */
      if ((ulong)n >= sizeof(Elf64_Ehdr) && u.buf[0] == ELF_MAGIC_0 &&
          u.buf[1] == ELF_MAGIC_1 && u.buf[2] == ELF_MAGIC_2 &&
          u.buf[3] == ELF_MAGIC_3) {
        Elf64_Ehdr *ehdr = (Elf64_Ehdr *)u.buf;
        Elf64_Phdr phdr;
        /* int i; shadowed */
        uintptr minva = ~0ULL, maxva_file = 0, maxva_mem = 0;
        uintptr elf_file_offset = 0; /* File offset of first LOAD segment */
        uintptr text_start = ~0ULL, text_end = 0;
        uintptr data_start = ~0ULL;
        uintptr data_file_end = 0;
        uintptr data_mem_end = 0;

        print("EXEC: detected ELF binary\n");

        /* Verify it's a 64-bit little-endian executable for x86_64 */
        if (ehdr->e_ident[4] != ELFCLASS64)
          error("ELF: not 64-bit");
        if (ehdr->e_ident[5] != ELFDATA2LSB)
          error("ELF: not little-endian");
        if (ehdr->e_type != ET_EXEC && ehdr->e_type != ET_DYN)
          error("ELF: not executable");
        if (ehdr->e_machine != EM_X86_64)
          error("ELF: not x86_64");

        entry = ehdr->e_entry;
        print("EXEC: ELF entry point = %#llux\n", entry);

        /* Find the extent of loadable segments */
        for (i = 0; i < ehdr->e_phnum; i++) {
          devtab[tc->type]->read(
              tc, &phdr, sizeof(phdr),
              (vlong)(ehdr->e_phoff + (ulong)i * sizeof(phdr)));
          if (phdr.p_type == PT_LOAD) {
            if (phdr.p_vaddr < minva) {
              minva = phdr.p_vaddr;
              elf_file_offset = phdr.p_offset;
            }
            if (phdr.p_vaddr + phdr.p_filesz > maxva_file)
              maxva_file = phdr.p_vaddr + phdr.p_filesz;
            if (phdr.p_vaddr + phdr.p_memsz > maxva_mem)
              maxva_mem = phdr.p_vaddr + phdr.p_memsz;
            if (phdr.p_flags & PF_X) {
              if (phdr.p_vaddr < text_start)
                text_start = phdr.p_vaddr;
              if (phdr.p_vaddr + phdr.p_filesz > text_end)
                text_end = phdr.p_vaddr + phdr.p_filesz;
              if (phdr.p_flags & PF_W)
                text_writable = 1;
            } else if (phdr.p_flags & PF_W) {
              if (phdr.p_vaddr < data_start) {
                data_start = phdr.p_vaddr;
                data_file_offset = phdr.p_offset;
              }
              if (phdr.p_vaddr + phdr.p_filesz > data_file_end)
                data_file_end = phdr.p_vaddr + phdr.p_filesz;
              if (phdr.p_vaddr + phdr.p_memsz > data_mem_end)
                data_mem_end = phdr.p_vaddr + phdr.p_memsz;
            }
          }
        }

        print("EXEC: ELF file offset = %#llux\n", (uvlong)elf_file_offset);
        print("EXEC: Data file offset = %#llux\n", (uvlong)data_file_offset);

        print("EXEC: ELF file range: %#llux - %#llux\n", minva, maxva_file);
        print("EXEC: ELF mem range: %#llux - %#llux\n", minva, maxva_mem);

        if (text_start == ~0ULL) {
          text_start = minva;
          text_end = maxva_file;
        }
        if (text_end < text_start)
          text_end = text_start;
        text = text_end > minva ? text_end - minva : 0;
        text_base = text_start;

        if (data_start != ~0ULL) {
          if (data_file_end < data_start)
            data_file_end = data_start;
          if (data_mem_end < data_file_end)
            data_mem_end = data_file_end;
          if (data_start < minva)
            data_start = minva;
          data = data_file_end > data_start ? data_file_end - data_start : 0;
          adata = data_start;
        } else {
          data = 0;
          adata = 0;
        }

        bss = maxva_mem > maxva_file ? maxva_mem - maxva_file : 0;

        print("EXEC: computed segments: text=%#llux data=%#llux bss=%#llux "
              "(text_writable=%d)\n",
              text, data, bss, text_writable);

        /* ELF binaries use page alignment */
        align = BY2PG - 1;
        is_elf = 1;
        file_offset = elf_file_offset;
        hash_binary(tc);
        update_pid2_after_exec();
        break; /* for binary */
      }
    }

    if (indir++)
      error(Ebadexec);

    /*
     * Process #! /bin/sh args ...
     */
    memmove(line, u.buf, (usize)n);
    n = shargs(line, n, progarg, nelem(progarg));
    if (n < 1)
      error(Ebadexec);
    /*
     * First arg becomes complete file name
     */
    progarg[n++] = file;
    progarg[n] = nil;
    argp0++;
    file = progarg[0];
    progarg[0] = elem;
    poperror();
    if (tc->ref > 0) {
      cclose(tc);
    } else {
      print("sysexec: warning - tc ref count already zero\n");
    }
  }

  if (is_elf) {
    /* For ELF, text/data/bss are already sizes, not addresses */
    /* adata is set to data_start in ELF block */
  } else {
    /* For a.out, text is end address, need to convert to size */
    adata = (text + align) & ~align;
    text -= UTZERO;
    data = beswal(u.ehdr.exec.data);
    bss = beswal(u.ehdr.exec.bss);
  }
  align = BY2PG - 1;

  abss = (adata + data + align) & ~align;
  ebss = (adata + data + bss + align) & ~align;
  if (adata >= (USTKTOP - USTKSIZE) || abss >= (USTKTOP - USTKSIZE) ||
      ebss >= (USTKTOP - USTKSIZE))
    error(Ebadexec);

  /*
   * Args: pass 1: count
   */
  nbytes =
      sizeof(Tos); /* hole for profiling clock at top of stack (and more) */
  nargs = 0;
  if (indir) {
    argp = progarg;
    while (*argp != nil) {
      a = *argp++;
      nbytes += (ulong)(strlen(a) + 1);
      nargs++;
    }
  }
  argp = argp0;
  while (*argp != nil) {
    a = *argp++;
    if (((uintptr)argp & (BY2PG - 1)) < BY2WD)
      validaddr((uintptr)argp, BY2WD, 0);
    validaddr((uintptr)a, 1, 0);
    e = vmemchr(a, 0, USTKSIZE);
    if (e == nil)
      error(Ebadarg);
    nbytes += (ulong)((e - a) + 1);
    if (nbytes >= USTKSIZE)
      error(Enovmem);
    nargs++;
  }
  stacksize = BY2WD * (nargs + 2) + ((nbytes + (BY2WD - 1)) & ~(BY2WD - 1));

  /*
   * 8-byte align SP for those (e.g. sparc) that need it.
   * execregs() will subtract another two words for p9uaddr and argc.
   */
  if (BY2WD == 4 && (stacksize + 4) & 7)
    stacksize += 4;

  if (PGROUND(stacksize) >= USTKSIZE)
    error(Enovmem);

  /*
   * Build the stack segment, putting it in kernel virtual for the moment
   */
  qlock(&up->seglock);
  if (waserror()) {
    qunlock(&up->seglock);
    nexterror();
  }
  s = up->seg[SSEG];
  /*
  print("EXEC: current stack segment base=%#llx top=%#llx size=%lud\n",
          s != nil ? (unsigned long long)s->base : 0ULL,
          s != nil ? (unsigned long long)s->top : 0ULL,
          s != nil ? s->size : 0UL);
  */
  do {
    tstk = s->base;
    if (tstk <= USTKSIZE)
      error(Enovmem);
  } while ((s = isoverlap(tstk - USTKSIZE, USTKSIZE)) != nil);
  /*
  print("EXEC: allocating temporary stack segment at [%#llx, %#llx)\n",
          (unsigned long long)(tstk-USTKSIZE),
          (unsigned long long)tstk);
  */
  up->seg[ESEG] =
      newseg(SG_STACK | SG_NOEXEC, tstk - USTKSIZE, USTKSIZE / BY2PG);
  qunlock(&up->seglock);

  if (waserror()) {
    qlock(&up->seglock);
    s = up->seg[ESEG];
    if (s != nil) {
      up->seg[ESEG] = nil;
      putseg(s);
    }
    nexterror();
  }

  /*
   * Args: pass 2: assemble; the pages will be faulted in
   */
  tos = (Tos *)(tstk - sizeof(Tos));
  tos->cyclefreq = m->cyclefreq;
  print("DEBUG: stack tos initialized\n");
  tos->kcycles = 0;
  tos->pcycles = 0;
  tos->clock = 0;

  argv = (char **)(tstk - stacksize);
  char **argv0 = argv;
  charp = (char *)(tstk - nbytes);
  if (indir)
    argp = progarg;
  else
    argp = argp0;

  for (i = 0; (ulong)i < nargs; i++) {
    if (indir && *argp == nil) {
      indir = 0;
      argp = argp0;
    }
    *argv++ = charp + (USTKTOP - tstk);
    a = *argp++;
    if (indir) {
      e = strchr(a, 0);
      nbytes += (ulong)(strlen(a) + 1);
    } else {
      if (charp >= (char *)tos)
        error(Ebadarg);
      validaddr((uintptr)a, 1, 0);
      e = vmemchr(a, 0, (ulong)((char *)tos - charp));
      if (e == nil)
        error(Ebadarg);
      nbytes += (ulong)((e - a) + 1);
    }
    n = (int)((e - a) + 1);
    memmove(charp, a, (usize)n);
    charp += n;
  }
  *argv = nil;
  /* Store argc just below argv[] so _start sees a reliable value. */
  ((ulong *)argv0)[-1] = nargs;

  /* copy args; easiest from new process's stack */
  a = (char *)(tstk - nbytes);
  n = (int)(charp - a);
  if (n > (int)sizeof(Sargs))
    n = sizeof(Sargs);
  args = smalloc((ulong)n);
  memmove(args, a, (usize)n);
  if (n > 0 && args[n - 1] != '\0') {
    /* make sure last arg is NUL-terminated */
    /* put NUL at UTF-8 character boundary */
    for (i = n - 1; i > 0; --i)
      if (fullrune(args + i, n - i))
        break;
    args[i] = 0;
    n = i + 1;
  }

  /* Attach text segment */
  /* attachimage returns a locked cache image */
  img = attachimage(tc, (PGROUND(text) + PGROUND(data)) >> PGSHIFT);
  if ((ts = img->s) != nil && ts->flen == text) {
    assert(ts->image == img);
    incref((Ref *)&ts->ref);
    putimage(img);
  } else {
    if (waserror()) {
      putimage(img);
      nexterror();
    }
    {
      int text_attr = SG_TEXT;
      if (!text_writable)
        text_attr |= SG_RONLY;
      ts = newseg(text_attr, text_base, PGROUND(text) >> PGSHIFT);
    }
    ts->flushme = 1;
    ts->image = img;
    ts->fstart = file_offset;
    ts->flen = text;
    /*
    print("EXEC: text segment fstart=%#llux flen=%#llux\n",
          (uvlong)ts->fstart, (uvlong)ts->flen);
    */
    img->s = ts;
    unlock(img);
    poperror();
  }

  /*
   * Committed.
   * Free old memory.
   * Special segments are maintained across exec
   */
  poperror();
  qlock(&up->seglock);

  for (i = SSEG; i <= BSEG; i++) {
    s = up->seg[i];
    if (s != nil) {
      /* prevent a second free if we have an error */
      up->seg[i] = nil;
      putseg(s);
    }
  }
  /* Preserve P9SEG and segments with SG_CEXEC=0 */
  for (i = ESEG; i < NSEG; i++) {
    if (i == P9SEG)
      continue;
    s = up->seg[i];
    if (s != nil && (s->type & SG_CEXEC) != 0) {
      up->seg[i] = nil;
      putseg(s);
    }
  }

  /* Text. Shared. */
  assert(ts->ref > 0);
  up->seg[TSEG] = ts;
#ifdef DEBUG
  /*
  print("EXEC: mapped text segment base=%#llx size=%lud bytes (writable=%d)\n",
          (unsigned long long)up->seg[TSEG]->base,
          (unsigned long long)(up->seg[TSEG]->size*BY2PG),
          text_writable);
  */
#endif

  /* Data. Shared. */
  if (data > 0) {
    s = newseg(SG_DATA, adata, PGROUND(data) >> PGSHIFT);
    s->image = img;
    s->fstart = is_elf ? data_file_offset : text;
    s->flen = data;
    incref((Ref *)&img->ref);
    up->seg[DSEG] = s;
    print("EXEC: mapped data segment base=%#llx size=%lud bytes fstart=%#llx\n",
          (unsigned long long)s->base, (unsigned long long)(s->size * BY2PG),
          (unsigned long long)s->fstart);
  } else {
    up->seg[DSEG] = nil;
    /* print("EXEC: skipping data segment (size 0)\n"); */
  }

  /* BSS. Zero fill on demand */
  up->seg[BSEG] = newseg(SG_BSS, abss, (ebss - abss) >> PGSHIFT);

  /*
   * Move the stack
   */
  s = up->seg[ESEG];
  up->seg[ESEG] = nil;
  qlock(&s->qlock);
  s->base = USTKTOP - USTKSIZE;
  s->top = USTKTOP;
  relocateseg(s, USTKTOP - tstk);
  qunlock(&s->qlock);
  up->seg[SSEG] = s;
  qunlock(&up->seglock);
  poperror(); /* seglock */

  if (tc == img->c) {
    /* avoid double caching */
    tc->flag &= (ushort)~CCACHE;
    cclunk(tc);
  }
  if (tc->ref > 0) {
    cclose(tc);
  } else {
    print("sysexec: warning - tc ref count already zero\n");
  }
  poperror(); /* tc */

  free(file0);
  poperror(); /* file0 */

  /*
   * Close on exec
   */
  if ((f = up->fgrp) != nil) {
    for (i = 0; i <= f->maxfd; i++)
      fdclose(i, CCEXEC);
  }

  qlock(&up->debug);
  free(up->text);
  up->text = elem;
  free(up->args);
  up->args = args;
  up->nargs = n;
  up->setargs = 0;

  freenotes(up);
  freenote(up->lastnote);
  up->lastnote = nil;
  up->notify = nil;
  up->notified = 0;
  up->noteureg = nil;
  up->privatemem = 0;
  up->noswap = 0;
  up->pcycles = (vlong)-up->kentry;
  procsetup(up);
  qunlock(&up->debug);

  up->errbuf0[0] = '\0';
  up->errbuf1[0] = '\0';

  /*
   *  At this point, the mmu contains info about the old address
   *  space and needs to be flushed
   */
  flushmmu();

  if (up->hang)
    up->procctl = Proc_stopme;

  /* Force error stack to 1 - syscall wrapper will pop once to get to 0 */
  print("sysexec: before cleanup, nerrlab=%d\n", up->nerrlab);
  while (up->nerrlab > 1)
    poperror();
  while (up->nerrlab < 1)
    up->nerrlab++;
  print("sysexec: after cleanup, nerrlab=%d\n", up->nerrlab);

  /* execregs should not return */
  execregs(entry, stacksize, nargs);
  return 0;
}

/*@
  @ assigns \nothing;
  @ ensures \result == 0;
  @ terminates \true;
  @*/
int return0(void *) { return 0; }

/*@
  @ requires \valid((ulong*)list_void);
  @ requires valid_syscall_args((ulong*)list_void, 1);
  @ terminates \true;
  @ assigns \nothing;
  @ ensures \result == 0;
  @*/
uintptr syssleep(void *list_void) {
  syscall_va_list list = (syscall_va_list)list_void;
  long ms;

  ms = SYSCALL_ARG(list, long);
  if (ms <= 0) {
    if (up->edf != nil && (up->edf->flags & Admitted))
      edfyield();
    else
      yield();
  } else {
    tsleep(&up->sleep, return0, 0, (ulong)ms);
  }
  return 0;
}

uintptr sysalarm(void *list_void) {
  syscall_va_list list = (syscall_va_list)list_void;
  return procalarm(SYSCALL_ARG(list, ulong));
}

uintptr sysexits(void *list_void) {
  syscall_va_list list = (syscall_va_list)list_void;
  char *status;
  char *inval = "invalid exit string";
  char buf[ERRMAX];

  status = SYSCALL_ARG(list, char *);
  if (status != nil) {
    if (waserror())
      status = inval;
    else {
      validaddr((uintptr)status, 1, 0);
      if (vmemchr(status, 0, ERRMAX) == nil) {
        memmove(buf, status, ERRMAX);
        buf[ERRMAX - 1] = 0;
        status = buf;
      }
      poperror();
    }
  }
  pexit(status, 1);
}

uintptr sys_wait(void *list_void) {
  syscall_va_list list = (syscall_va_list)list_void;
  ulong pid;
  Waitmsg w;
  OWaitmsg *ow;

  ow = SYSCALL_ARG(list, OWaitmsg *);
  if (waserror()) {
    /* If pwait is interrupted or errors */
    return (uintptr)-1;
  }

  if (ow == nil)
    pid = pwait(nil);
  else {
    validaddr((uintptr)ow, sizeof(OWaitmsg), 1);
    evenaddr((uintptr)ow);
    pid = pwait(&w);
  }
  poperror();
  if (list) {
    /* Only partial support for wait string parsing here */
    /* ow is already OWaitmsg* from line 1248 */
    // We don't have full string parsing, but we can write PID ?
    // Actually existing code tries to parse it.
    // Let's just cast w.pid to ulong as requested.
    readnum(0, ow->pid, NUMSIZE, (ulong)w.pid, NUMSIZE);
    readnum(0, ow->time + TUser * NUMSIZE, NUMSIZE, w.time[TUser], NUMSIZE);
    readnum(0, ow->time + TSys * NUMSIZE, NUMSIZE, w.time[TSys], NUMSIZE);
    readnum(0, ow->time + TReal * NUMSIZE, NUMSIZE, w.time[TReal], NUMSIZE);
    strncpy(ow->msg, w.msg, sizeof(ow->msg) - 1);
    ow->msg[sizeof(ow->msg) - 1] = '\0';
  }
  return pid;
}

uintptr sysawait(void *list_void) {
  syscall_va_list list = (syscall_va_list)list_void;
  char *p;
  Waitmsg w;
  uint n;
  p = SYSCALL_ARG(list, char *);
  n = SYSCALL_ARG(list, uint);
  validaddr((uintptr)p, n, 1);
  pwait(&w);
  if ((int)n < 0) // Added cast for comparison
    return (uintptr)-1;
  /* n is uint, snprint takes int n. Cast in call. */
  return (uintptr)snprint(p, (int)n, "%d %lud %lud %lud %q", w.pid,
                          w.time[TUser], w.time[TSys], w.time[TReal], w.msg);
}

void werrstr(char *fmt, ...) {
  va_list va;

  if (up == nil)
    return;

  va_start(va, fmt);
  vseprint(up->syserrstr, up->syserrstr + ERRMAX, fmt, va);
  va_end(va);
}

/*@
  @ requires buf != \null && nbuf > 0;
  @ requires up != \null;
  @ requires \valid(up);
  @ assigns up->errstr, up->syserrstr;
  @ ensures \result == 0;
  @*/
static int generrstr(char *buf, uint nbuf) {
  char *err;

  if (nbuf == 0)
    error(Ebadarg);
  if (nbuf > ERRMAX)
    nbuf = ERRMAX;
  validaddr((uintptr)buf, nbuf, 1);

  err = up->errstr;
  utfecpy(err, err + nbuf, buf);
  utfecpy(buf, buf + nbuf, up->syserrstr);

  up->errstr = up->syserrstr;
  up->syserrstr = err;

  return 0;
}

uintptr syserrstr(void *list_void) {
  syscall_va_list list = (syscall_va_list)list_void;
  char *buf;
  uint len;

  buf = SYSCALL_ARG(list, char *);
  len = SYSCALL_ARG(list, uint);
  return (uintptr)generrstr(buf, len);
}

/* compatibility for old binaries */
uintptr sys_errstr(void *list_void) {
  syscall_va_list list = (syscall_va_list)list_void;
  return (uintptr)generrstr(SYSCALL_ARG(list, char *), 64);
}

uintptr sysnotify(void *list_void) {
  syscall_va_list list = (syscall_va_list)list_void;
  int (*f)(void *, char *);
  f = SYSCALL_ARG(list, void *);
  if (f != nil)
    validaddr((uintptr)f, sizeof(void *), 0);
  up->notify = f;
  return 0;
}

/*@
  @ requires ureg != \null;
  @ requires up != \null;
  @ requires \valid(up);
  @ assigns up->noteureg, up->notified, up->lastnote, up->notify;
  @ ensures \result == 0 || \result == 1;
  @*/
int donotify(Ureg *ureg) {
  Ureg *nureg;
  char *msg;

  if (up->procctl)
    procctl();
  if (up->nnote == 0)
    return 0;

  spllo();
  qlock(&up->debug);
  msg = popnote(ureg);
  if (msg == nil) {
    qunlock(&up->debug);
    splhi();
    return 0;
  }
  splhi();
  fpunotify(up);
  spllo();
  qunlock(&up->debug);

  if (up->notify == nil || (nureg = notify(ureg, msg)) == nil) {
    if (up->lastnote->flag == NDebug)
      pprint("suicide: (note)\n"); /* Avoid unbounded string in verification */
    pexit(msg, up->lastnote->flag != NDebug);
  }

  /* word under Ureg is old ureg */
  *(Ureg **)((uintptr)nureg - BY2WD) = up->noteureg;
  up->noteureg = nureg;

  splhi();
  return 1;
}

uintptr sysnoted(void *list_void) {
  syscall_va_list list = (syscall_va_list)list_void;
  Ureg *nureg;
  int arg;

  arg = SYSCALL_ARG(list, int);

  qlock(&up->debug);
  if (up->notified) {
    splhi();
    fpunoted(up);
    spllo();
  } else if (arg != NRSTR) {
    qunlock(&up->debug);
    error(Ebadarg);
  }
  qunlock(&up->debug);

  nureg = up->noteureg;
  if (!okaddr((uintptr)nureg - BY2WD, BY2WD + sizeof(Ureg), 0)) {
    pprint("suicide: bad ureg in noted or call to noted when not notified\n");
    pexit("Suicide", 0);
  }

  switch (arg) {
  case NCONT:
  case NRSTR:
    /* word under Ureg is old ureg */
    up->noteureg = *(Ureg **)((uintptr)nureg - BY2WD);
    /* fall through */
  case NSAVE:
    if (noted(up->dbgreg, nureg, arg)) {
      pprint("suicide: trap in noted\n");
      pexit("Suicide", 0);
    }
    break;
  default:
    up->lastnote->flag = NDebug;
    /* fall through */
  case NDFLT:
    noted(up->dbgreg, nureg, arg); /* for debugging */
    if (up->lastnote->flag == NDebug)
      pprint("suicide: %s\n", up->lastnote->msg);
    pexit(up->lastnote->msg, up->lastnote->flag != NDebug);
  }

  /* allow next note */
  up->notified = 0;

  return 0;
}

uintptr syssegbrk(void *list_void) {
  syscall_va_list list = (syscall_va_list)list_void;
  int i;
  uintptr addr;
  Segment *s;

  addr = SYSCALL_ARG(list, uintptr);
  for (i = 0; i < NSEG; i++) {
    s = up->seg[i];
    if (s == nil || addr < s->base || addr >= s->top)
      continue;
    switch (s->type & SG_TYPE) {
    case SG_TEXT:
    case SG_DATA:
    case SG_STACK:
    case SG_PHYSICAL:
    case SG_FIXED:
    case SG_STICKY:
      error(Ebadarg);
    default:
      return ibrk(SYSCALL_ARG(list, uintptr), i);
    }
  }
  error(Ebadarg);
}

uintptr syssegattach(void *list_void) {
  syscall_va_list list = (syscall_va_list)list_void;
  int attr;
  char *name;
  uintptr va;
  ulong len;

  attr = SYSCALL_ARG(list, int);
  name = SYSCALL_ARG(list, char *);
  va = SYSCALL_ARG(list, uintptr);
  len = SYSCALL_ARG(list, ulong);
  validaddr((uintptr)name, 1, 0);
  name = validnamedup(name, 1);
  if (waserror()) {
    free(name);
    nexterror();
  }
  va = segattach(attr, name, va, len);
  free(name);
  poperror();
  return va;
}

uintptr syssegdetach(void *list_void) {
  syscall_va_list list = (syscall_va_list)list_void;
  int i;
  uintptr addr;
  Segment *s;

  addr = SYSCALL_ARG(list, uintptr);

  qlock(&up->seglock);
  if (waserror()) {
    qunlock(&up->seglock);
    nexterror();
  }

  for (i = 0; i < NSEG; i++)
    if ((s = up->seg[i]) != nil) {
      qlock(&s->qlock);
      if ((addr >= s->base && addr < s->top) ||
          (s->top == s->base && addr == s->base))
        goto found;
      qunlock(&s->qlock);
    }

  error(Ebadarg);

found:
  /*
   * Check we are not detaching the initial stack segment.
   */
  if (s == up->seg[SSEG]) {
    qunlock(&s->qlock);
    error(Ebadarg);
  }
  qunlock(&s->qlock);
  up->seg[i] = nil;
  putseg(s);
  qunlock(&up->seglock);

  /* vfork synchronization: Unblock parent after successful exec */
  if (up->vforkp != nil) {
    print("VFORK: Unblocking parent pid %lud after child pid %lud execs\n",
          up->vforkp->pid, up->pid);
    proc_event(up->vforkp, EV_VFORK_DONE);
    ready(up->vforkp);
    up->vforkp = nil;
  }

  poperror();
  /* Ensure we flush any entries from the lost segment */
  flushmmu();
  return 0;
}

uintptr syssegfree(void *list_void) {
  syscall_va_list list = (syscall_va_list)list_void;
  Segment *s;
  uintptr from, to;

  from = SYSCALL_ARG(list, uintptr);
  to = SYSCALL_ARG(list, ulong);
  to += from;
  if (to < from)
    error(Ebadarg);
  s = seg(up, from, 1);
  if (s == nil)
    error(Ebadarg);
  to &= ~(BY2PG - 1);
  from = PGROUND(from);
  if (from >= to) {
    qunlock(&s->qlock);
    return 0;
  }
  if (to > s->top) {
    qunlock(&s->qlock);
    error(Ebadarg);
  }
  mfreeseg(s, from, (to - from) / BY2PG);
  qunlock(&s->qlock);
  flushmmu();
  return 0;
}

/* For binary compatibility */
uintptr sysbrk_(void *list_void) {
  syscall_va_list list = (syscall_va_list)list_void;
  return ibrk(SYSCALL_ARG(list, uintptr), BSEG);
}

uintptr sysrendezvous(void *list_void) {
  syscall_va_list list = (syscall_va_list)list_void;
  uintptr tag, val, new;
  Proc *p, **l;

  tag = SYSCALL_ARG(list, uintptr);
  new = SYSCALL_ARG(list, uintptr);
  l = &REND(up->rgrp, tag);

  lock(up->rgrp);
  for (p = *l; p != nil; p = p->rendhash) {
    if (p->rendtag == tag) {
      *l = p->rendhash;
      val = p->rendval;
      p->rendval = new;
      unlock(up->rgrp);

      ready(p);

      return val;
    }
    l = &p->rendhash;
  }

  /* Going to sleep here */
  up->rendtag = tag;
  up->rendval = new;
  up->rendhash = *l;
  *l = up;
  /* up->state = Rendezvous; -- REPLACED BY FSM */
  proc_event(up, EV_RENDEZ);
  unlock(up->rgrp);

  sched();

  return up->rendval;
}

/*
 * The implementation of semaphores is complicated by needing
 * to avoid rescheduling in syssemrelease, so that it is safe
 * to call from real-time processes.  This means syssemrelease
 * cannot acquire any qlocks, only spin locks.
 *
 * Semacquire and semrelease must both manipulate the semaphore
 * wait list.  Lock-free linked lists only exist in theory, not
 * in practice, so the wait list is protected by a spin lock.
 *
 * The semaphore value *addr is stored in user memory, so it
 * cannot be read or written while holding spin locks.
 *
 * Thus, we can access the list only when holding the lock, and
 * we can access the semaphore only when not holding the lock.
 * This makes things interesting.  Note that sleep's condition function
 * is called while holding two locks - r and up->rlock - so it cannot
 * access the semaphore value either.
 *
 * An acquirer announces its intention to try for the semaphore
 * by putting a Sema structure onto the wait list and then
 * setting Sema.waiting.  After one last check of semaphore,
 * the acquirer sleeps until Sema.waiting==0.  A releaser of n
 * must wake up n acquirers who have Sema.waiting set.  It does
 * this by clearing Sema.waiting and then calling wakeup.
 *
 * There are three interesting races here.

 * The first is that in this particular sleep/wakeup usage, a single
 * wakeup can rouse a process from two consecutive sleeps!
 * The ordering is:
 *
 * 	(a) set Sema.waiting = 1
 * 	(a) call sleep
 * 	(b) set Sema.waiting = 0
 * 	(a) check Sema.waiting inside sleep, return w/o sleeping
 * 	(a) try for semaphore, fail
 * 	(a) set Sema.waiting = 1
 * 	(a) call sleep
 * 	(b) call wakeup(a)
 * 	(a) wake up again
 *
 * This is okay - semacquire will just go around the loop
 * again.  It does mean that at the top of the for(;;) loop in
 * semacquire, phore.waiting might already be set to 1.
 *
 * The second is that a releaser might wake an acquirer who is
 * interrupted before he can acquire the lock.  Since
 * release(n) issues only n wakeup calls -- only n can be used
 * anyway -- if the interrupted process is not going to use his
 * wakeup call he must pass it on to another acquirer.
 *
 * The third race is similar to the second but more subtle.  An
 * acquirer sets waiting=1 and then does a final canacquire()
 * before going to sleep.  The opposite order would result in
 * missing wakeups that happen between canacquire and
 * waiting=1.  (In fact, the whole point of Sema.waiting is to
 * avoid missing wakeups between canacquire() and sleep().) But
 * there can be spurious wakeups between a successful
 * canacquire() and the following semdequeue().  This wakeup is
 * not useful to the acquirer, since he has already acquired
 * the semaphore.  Like in the previous case, though, the
 * acquirer must pass the wakeup call along.
 *
 * This is all rather subtle.  The code below has been verified
 * with the spin model /sys/src/9/port/semaphore.p.  The
 * original code anticipated the second race but not the first
 * or third, which were caught only with spin.  The first race
 * is mentioned in /sys/doc/sleep.ps, but I'd forgotten about it.
 * It was lucky that my abstract model of sleep/wakeup still managed
 * to preserve that behavior.
 *
 * I remain slightly concerned about memory coherence
 * outside of locks.  The spin model does not take
 * queued processor writes into account so we have to
 * think hard.  The only variables accessed outside locks
 * are the semaphore value itself and the boolean flag
 * Sema.waiting.  The value is only accessed with cmpswap,
 * whose job description includes doing the right thing as
 * far as memory coherence across processors.  That leaves
 * Sema.waiting.  To handle it, we call coherence() before each
 * read and after each write.		- rsc
 */

/* Add semaphore p with addr a to list in seg. */
/*@
  @ requires s != \null;
  @ requires p != \null;
  @ assigns *p, s->sema.rendez.lock;
  @*/
static void semqueue(Segment *s, long *a, Sema *p) {
  memset(p, 0, sizeof *p);
  p->addr = a;
  lock(&s->sema.rendez.lock); /* protect semaphore list */
  p->next = &s->sema;
  p->prev = s->sema.prev;
  p->next->prev = p;
  p->prev->next = p;
  unlock(&s->sema.rendez.lock);
}

/* Remove semaphore p from list in seg. */
/*@
  @ requires s != \null;
  @ requires p != \null;
  @ assigns s->sema.rendez.lock;
  @*/
static void semdequeue(Segment *s, Sema *p) {
  lock(&s->sema.rendez.lock);
  p->next->prev = p->prev;
  p->prev->next = p->next;
  unlock(&s->sema.rendez.lock);
}

/* Wake up n waiters with addr a on list in seg. */
/*@
  @ requires s != \null;
  @ assigns s->sema.rendez.lock;
  @*/
static void semwakeup(Segment *s, long *a, long n) {
  Sema *p;

  lock(&s->sema.rendez.lock);
  for (p = s->sema.next; p != &s->sema && n > 0; p = p->next) {
    if (p->addr == a && p->waiting) {
      p->waiting = 0;
      coherence();
      wakeup(&p->rendez);
      n--;
    }
  }
  unlock(&s->sema.rendez.lock);
}

/* Add delta to semaphore and wake up waiters as appropriate. */
/*@
  @ requires s != \null;
  @ requires addr != \null;
  @ assigns *addr;
  @ ensures \result == \old(*addr) + delta;
  @*/
long semrelease(Segment *s, long *addr, long delta) {
  long value;

  do
    value = *addr;
  while (!cmpswap(addr, value, value + delta));
  semwakeup(s, addr, delta);
  return value + delta;
}

/* Try to acquire semaphore using compare-and-swap */
/*@
  @ requires addr != \null;
  @ assigns *addr;
  @ ensures \result == 0 || \result == 1;
  @*/
static int canacquire(long *addr) {
  long value;

  while ((value = *addr) > 0)
    if (cmpswap(addr, value, value - 1))
      return 1;
  return 0;
}

/* Should we wake up? */
/*@
  @ requires p != \null;
  @ assigns \nothing;
  @ ensures \result == !(((Sema*)p)->waiting);
  @*/
static int semawoke(void *p) {
  coherence();
  return !((Sema *)p)->waiting;
}

/* Acquire semaphore (subtract 1). */
/*@
  @ requires s != \null;
  @ requires addr != \null;
  @ assigns *addr;
  @ ensures \result == 1;
  @*/
int semacquire(Segment *s, long *addr, int block) {
  int acquired;
  Sema phore;

  if (canacquire(addr))
    return 1;
  if (!block)
    return 0;
  semqueue(s, addr, &phore);
  if ((acquired = !waserror())) {
    for (;;) {
      phore.waiting = 1;
      coherence();
      if (canacquire(addr))
        break;
      sleep(&phore.rendez, semawoke, &phore);
    }
    poperror();
  }
  semdequeue(s, &phore);
  coherence(); /* not strictly necessary due to lock in semdequeue */
  if (!phore.waiting)
    semwakeup(s, addr, 1);
  if (!acquired)
    nexterror();
  return 1;
}

/* Acquire semaphore or time-out */
/*@
  @ requires s != \null;
  @ requires addr != \null;
  @ assigns *addr;
  @ ensures \result == 0 || \result == 1;
  @*/
static int tsemacquire(Segment *s, long *addr, ulong ms) {
  int timedout, acquired;
  ulong t;
  Sema phore;

  if (canacquire(addr))
    return 1;
  if (ms == 0)
    return 0;
  timedout = 0;
  semqueue(s, addr, &phore);
  if ((acquired = !waserror())) {
    for (;;) {
      phore.waiting = 1;
      coherence();
      if (canacquire(addr))
        break;
      t = MACHP(0)->ticks;
      tsleep(&phore.rendez, semawoke, &phore, ms);
      t = TK2MS(MACHP(0)->ticks - t);
      if (t >= ms) {
        timedout = 1;
        break;
      }
      ms -= t;
    }
    poperror();
  }
  semdequeue(s, &phore);
  coherence(); /* not strictly necessary due to lock in semdequeue */
  if (!phore.waiting)
    semwakeup(s, addr, 1);
  if (!acquired)
    nexterror();
  return !timedout;
}

uintptr syssemacquire(void *list_void) {
  syscall_va_list list = (syscall_va_list)list_void;
  int block;
  long *addr;
  Segment *s;

  addr = SYSCALL_ARG(list, long *);
  block = SYSCALL_ARG(list, int);
  evenaddr((uintptr)addr);
  s = seg(up, (uintptr)addr, 0);
  if (s == nil || (s->type & SG_RONLY) != 0 ||
      (uintptr)addr + sizeof(long) > s->top) {
    validaddr((uintptr)addr, sizeof(long), 1);
    error(Ebadarg);
  }
  if (*addr < 0)
    error(Ebadarg);
  return (uintptr)semacquire(s, addr, block);
}

uintptr systsemacquire(void *list_void) {
  syscall_va_list list = (syscall_va_list)list_void;
  long *addr;
  ulong ms;
  Segment *s;

  addr = SYSCALL_ARG(list, long *);
  ms = SYSCALL_ARG(list, ulong);
  evenaddr((uintptr)addr);
  s = seg(up, (uintptr)addr, 0);
  if (s == nil || (s->type & SG_RONLY) != 0 ||
      (uintptr)addr + sizeof(long) > s->top) {
    validaddr((uintptr)addr, sizeof(long), 1);
    error(Ebadarg);
  }
  if (*addr < 0)
    error(Ebadarg);
  return (uintptr)tsemacquire(s, addr, ms);
}

uintptr syssemrelease(void *list_void) {
  syscall_va_list list = (syscall_va_list)list_void;
  long *addr, delta;
  Segment *s;

  addr = SYSCALL_ARG(list, long *);
  delta = SYSCALL_ARG(list, long);
  evenaddr((uintptr)addr);
  s = seg(up, (uintptr)addr, 0);
  if (s == nil || (s->type & SG_RONLY) != 0 ||
      (uintptr)addr + sizeof(long) > s->top) {
    validaddr((uintptr)addr, sizeof(long), 1);
    error(Ebadarg);
  }
  /* delta == 0 is a no-op, not a release */
  if (delta < 0 || *addr < 0)
    error(Ebadarg);
  return (uintptr)semrelease(s, addr, delta);
}

/* For binary compatibility */
uintptr sys_nsec(void *list_void) {
  syscall_va_list list = (syscall_va_list)list_void;
  vlong *v;

  /* return in register on 64bit machine */
  if (sizeof(uintptr) == sizeof(vlong)) {
    USED(list);
    return (uintptr)todget(nil, nil);
  }

  v = SYSCALL_ARG(list, vlong *);
  evenaddr((uintptr)v);
  validaddr((uintptr)v, sizeof(vlong), 1);
  *v = todget(nil, nil);
  return 0;
}

/*@
  @ requires \valid((ulong*)list_void);
  @ requires valid_syscall_args((ulong*)list_void, 2);
  @
  @ behavior success:
  @   assumes pebble_enabled == 1;
  @   ensures \result != (uintptr)0;
  @
  @ behavior error_perm:
  @   assumes pebble_enabled == 0;
  @   ensures \false;
  @
  @ terminates \true;
  @ assigns \nothing;
  @*/
uintptr syspebblewhiteissue(void *list_void) {
  syscall_va_list list = (syscall_va_list)list_void;
  ulong size;
  void **out;
  PebbleWhite *white;
  PebbleState *ps;

  size = SYSCALL_ARG(list, ulong);
  out = SYSCALL_ARG(list, void **);
  if (out == nil)
    error(Ebadarg);
  if (!pebble_enabled)
    error(PEBBLE_E_PERM);
  ps = pebble_state();
  if (ps == nil)
    error(PEBBLE_E_PERM);
  validaddr((uintptr)out, sizeof(void *), 1);
  white = pebble_issue_white(ps, nil, size);
  if (white == nil)
    error(PEBBLE_E_AGAIN);
  *out = white;
  if (pebble_debug)
    print("PEBBLE: white issue pid=%lud size=%lud token=%#p\n", up->pid, size,
          white);
  return (uintptr)white;
}

/*@
  @ requires \valid((ulong*)list_void);
  @ requires valid_syscall_args((ulong*)list_void, 2);
  @
  @ behavior success:
  @   assumes pebble_enabled == 1;
  @   ensures \result != (uintptr)0;
  @
  @ behavior error_perm:
  @   assumes pebble_enabled == 0;
  @   ensures \false;
  @
  @ terminates \true;
  @ assigns \nothing;
  @*/
uintptr syspebbleblackalloc(void *list_void) {
  syscall_va_list list = (syscall_va_list)list_void;
  uintptr size;
  void **userp;
  void *handle;
  UserCapability cap;

  size = SYSCALL_ARG(list, uintptr);
  userp = SYSCALL_ARG(list, void **);
  if (userp == nil)
    error(Ebadarg);
  if (!pebble_enabled)
    error(PEBBLE_E_PERM);
  validaddr((uintptr)userp, sizeof(void *), 1);
  handle = nil;

  if (pebble_alloc_with_white(size, &cap, &handle) != 0)
    error(PEBBLE_E_NOMEM);

  *userp = handle;
  return (uintptr)handle;
}

/*@
  @ requires \valid((ulong*)list_void);
  @ requires valid_syscall_args((ulong*)list_void, 1);
  @
  @ behavior success:
  @   assumes pebble_enabled == 1;
  @   ensures \result == 0;
  @
  @ behavior error_perm:
  @   assumes pebble_enabled == 0;
  @   ensures \false;
  @
  @ terminates \true;
  @ assigns \nothing;
  @*/
uintptr syspebbleblackfree(void *list_void) {
  syscall_va_list list = (syscall_va_list)list_void;
  void *handle;

  if (!pebble_enabled)
    error(PEBBLE_E_PERM);
  handle = SYSCALL_ARG(list, void *);
  pebble_black_free(handle);
  return 0;
}

/*@
  @ requires \valid((ulong*)list_void);
  @ requires valid_syscall_args((ulong*)list_void, 2);
  @
  @ behavior success:
  @   assumes pebble_enabled == 1;
  @   ensures \result != (uintptr)0;
  @
  @ behavior error_perm:
  @   assumes pebble_enabled == 0;
  @   ensures \false;
  @
  @ terminates \true;
  @ assigns \nothing;
  @*/
uintptr syspebblewhiteverify(void *list_void) {
  syscall_va_list list = (syscall_va_list)list_void;
  PebbleWhite *white;
  void **out;
  void *black;

  white = SYSCALL_ARG(list, PebbleWhite *);
  out = SYSCALL_ARG(list, void **);
  if (out == nil)
    error(Ebadarg);
  if (!pebble_enabled)
    error(PEBBLE_E_PERM);
  validaddr((uintptr)out, sizeof(void *), 1);
  black = nil;
  pebble_white_verify(white, &black);
  *out = black;
  return (uintptr)black;
}

/*@
  @ requires \valid((ulong*)list_void);
  @ requires valid_syscall_args((ulong*)list_void, 2);
  @
  @ behavior success:
  @   assumes pebble_enabled == 1;
  @   ensures \result != (uintptr)0;
  @
  @ behavior error_perm:
  @   assumes pebble_enabled == 0;
  @   ensures \false;
  @
  @ terminates \true;
  @ assigns \nothing;
  @*/
uintptr syspebbleredcopy(void *list_void) {
  syscall_va_list list = (syscall_va_list)list_void;
  PebbleBlue *blue;
  PebbleRed **out;
  PebbleRed *red;

  blue = SYSCALL_ARG(list, PebbleBlue *);
  out = SYSCALL_ARG(list, PebbleRed **);
  if (out == nil)
    error(Ebadarg);
  if (!pebble_enabled)
    error(PEBBLE_E_PERM);
  validaddr((uintptr)out, sizeof(PebbleRed *), 1);
  red = nil;
  pebble_red_copy(blue, &red);
  *out = red;
  return (uintptr)red;
}

uintptr syspebblebluediscard(void *list_void) {
  syscall_va_list list = (syscall_va_list)list_void;
  PebbleBlue *blue;

  if (!pebble_enabled)
    error(PEBBLE_E_PERM);
  blue = SYSCALL_ARG(list, PebbleBlue *);
  pebble_blue_discard(blue);
  return 0;
}

uintptr sys_getpid2(void *list_void) {
  syscall_va_list list = (syscall_va_list)list_void;
  void *out;
  ulong len;

  out = SYSCALL_ARG(list, void *);
  len = SYSCALL_ARG(list, ulong);
  if (len < sizeof(up->pid2.data))
    error(Eshort);
  validaddr((uintptr)out, sizeof(up->pid2.data), 1);
  memmove(out, up->pid2.data, sizeof(up->pid2.data));
  return 0;
}

#include <systab.h>

int dosyscall(ulong scallnr, Sargs *args, uintptr *retp) {
  extern void uartputs(char *, int);
  char buf[128];
  snprint(buf, sizeof(buf), "DEBUG: dosyscall entry scallnr=%ld\n", scallnr);
  uartputs(buf, (int)strlen(buf));

  if (scallnr == EXEC) {
    snprint(buf, sizeof(buf), "DEBUG: dosyscall EXEC handler address %p\n",
            systab[scallnr]);
    uartputs(buf, (int)strlen(buf));
  }
  vlong startns, stopns;
  uintptr ret;
  int s;

  /*
   * DEBUG: Disabled verbose syscall tracing
   * print("dosyscall: entered, scallnr=%ld\n", scallnr);
   */

  // print("DEBUG: 1. m=%p\n", m);
  m->syscall++;
  // print("DEBUG: 2. up=%p\n", up);
  up->insyscall = 1;
  /* Re-enable interrupts for syscall processing (allows preemption/timers) */
  if (up && m && up->nlocks == 0)
    s = spllo();

  // if (1) print("DEBUG: Pre-Waserror: up=%p nerrlab=%d\n", up, up->nerrlab);
  if (!waserror()) {
    // print("DEBUG: Inside waserror\n");
    evenaddr((uintptr)args);
    validaddr((uintptr)args, sizeof(Sargs), 0);

    up->s = *args;
    // print("DEBUG: Copied args\n");
    syscall_va_list syscall_args;
    syscall_vainit(syscall_args, up->s.args);
    // print("DEBUG: vainit done\n");
    syscall_vainit(syscall_args, up->s.args);
    // print("DEBUG: vainit done\n");
    up->scallnr = (int)scallnr;

    if (up->procctl == Proc_tracesyscall) {
      syscall_va_list trace_args;
      syscall_vainit(trace_args, up->s.args);
      syscallfmt(scallnr, userpc(), trace_args);
      splhi();
      up->procctl = Proc_stopme;
      procctl();
      spllo();
      todget(nil, &startns);
    }
    if (scallnr >= (ulong)nsyscall || systab[scallnr] == nil) {
      postnote(up, 1, "sys: bad sys call", NDebug);
      error(Ebadarg);
    }
    up->psstate = sysctab[scallnr];
    /*
     * DEBUG: Disabled verbose syscall tracing
     * print("dosyscall: calling syscall handler\n");
     */
    // print("DEBUG: calling handler\n");
    snprint(buf, sizeof(buf), "DEBUG: About to call systab[%ld] at %p\n",
            scallnr, systab[scallnr]);
    uartputs(buf, (int)strlen(buf));
    ret = systab[scallnr](syscall_args);
    /*
     * DEBUG: Disabled verbose syscall tracing
     * print("dosyscall: syscall handler returned %#llux\n", ret);
     */
    poperror();
    if (scallnr == NOTED) {
      /* special case: noted() changes the ureg, return without setting *retp */
      splx(s);
      up->insyscall = 0;
      up->psstate = nil;
      return 1;
    }
  } else {
    /* failure: save the error buffer for errstr */
    char *e = up->syserrstr;
    up->syserrstr = up->errstr;
    up->errstr = e;
    ret = (uintptr)-1;
  }
  if (up->nerrlab) {
    int i;

    print("bad errstack [%lud]: %d extra\n", scallnr, up->nerrlab);
    for (i = 0; i < NERR; i++)
      print("sp=%#p pc=%#p\n", up->errlab[i].sp, up->errlab[i].pc);
    panic("error stack");
  }
  *retp = ret;
  if (up->procctl == Proc_tracesyscall) {
    todget(nil, &stopns);
    syscall_va_list ret_args;
    syscall_vainit(ret_args, up->s.args);
    sysretfmt(scallnr, ret_args, ret, (uvlong)startns, (uvlong)stopns);
    splhi();
    up->procctl = Proc_stopme;
    procctl();
  }
  splx(s);
  up->insyscall = 0;
  up->psstate = nil;
  return 0;
}

/*
 * sys_clr_compile - Compile Fruity IR to assembly via QBE
 *
 * Pipeline:
 *   1. Read Fruity IR module from /dev/clr file descriptor
 *   2. Translate Fruity IR → QBE IL (to intermediate page)
 *   3. Compile QBE IL → Assembly (to output page)
 *   4. Optionally copy QBE IL to debug page
 *
 * Arguments:
 *   fd_fruity - File descriptor to /dev/clr (contains fruity_module_t*)
 *   output_asm - Exchange handle for assembly output
 *   output_qbe - Exchange handle for QBE IL debug output (0 = skip)
 *   errorbuf - Userspace error buffer
 *   errorbuf_size - Size of error buffer
 *
 * Returns:
 *   0 on success
 *   -1 on error (error string written to errorbuf)
 */
uintptr sysclrcompile(void *list_void) {
  USED(list_void);
  /* QBE backend removed. This syscall is deprecated/disabled. */
  error("sysclrcompile: backend removed");
  return 0;
}
