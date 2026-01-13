/* clang-format off */
#include "u.h"
#include <lib.h>
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "io.h"
#include "ureg.h"
#include "tos.h"
#include <error.h>
#include <trace.h>

#include "9p_router.h"
#include "pebble.h"
/* clang-format on */

extern int irqhandled(Ureg *, int);
extern void irqinit(void);
extern int dosyscall(ulong, Sargs *, uintptr *);
#define PSTATENAME_MAX 14
extern char *statename[];

extern uvlong touser_target_pc;
extern uvlong touser_target_sp;
extern uvlong touser_target_flags;
extern uvlong touser_attempt_count;

int intret_debug_stage = 0;

void asm_debug_print(int n) { print("%d", n); }

void asm_debug_print_hex(unsigned long n) { print("[%p]", n); }

uvlong intr_frame_addr;
uvlong intr_frame_saved_sp;
uvlong intr_frame_saved_ss;
uvlong intr_frame_saved_cs;
uvlong intr_frame_saved_flags;
uvlong intr_frame_before;
uvlong intr_frame_relocated;
uvlong intr_frame_r14;

static void debugexc(Ureg *, void *);
static void debugbpt(Ureg *, void *);
static void faultamd64(Ureg *, void *);
static void doublefault(Ureg *, void *);
static void unexpected(Ureg *, void *);
static void _dumpstack(Ureg *);

/* Temporary IDT until we can set up proper page tables */
Segdesc temp_idt[512] __attribute__((aligned(16)));

int userureg(Ureg *ureg) { return (ureg->cs & 3) == 3; }

void trapinit0(void) {
  u32int d1, v;
  uintptr vaddr;
  Segdesc *idt, *idt_start;
  uintptr ptr[2];

  uartputs("trapinit0: ENTRY - initializing IDT\n", 40);
  /* Use temp_idt in BSS instead of fixed IDTADDR until page tables are set up
   */
  idt = temp_idt;
  idt_start = idt;
  vaddr = (uintptr)vectortable;
  uartputs("trapinit0: initialized\n", 24);

  for (v = 0; v < 256; v++) {
    uintptr vec;
    u32int d0, flags;

    /* Get vector handler address */
    vec = vaddr;

    /* Build flags: SEGP | SEGIG | SEGPL(dpl) with IST implicitly 0 */
    flags = SEGP | SEGIG;
    if (v == VectorBPT || v == VectorSYSCALL)
      flags |= SEGPL(3);
    else
      flags |= SEGPL(0);

    /* Build d0: offset[15:0] | selector[31:16] */
    d0 = (vec & 0xFFFF) | (KESEL << 16);

    /* Build d1: offset[31:16] in upper half, flags[15:0] in lower half */
    d1 = (((vec >> 16) & 0xFFFF) << 16) | flags;

    /* Debug: show IDT setup for timer interrupt */
    if (v == 32) {
      char buf[128];
      int n;
      n = snprint(buf, sizeof(buf), "IDT[32]: vec=%#p d0=%#x d1=%#x IST=%d\n",
                  (void *)vec, d0, d1, d1 & 0x7);
      uartputs(buf, n);
    }

    idt->d0 = d0;
    idt->d1 = d1;
    idt++;

    idt->d0 = (vec >> 32);
    idt->d1 = 0;
    idt++;

    vaddr += 6;
  }

  uartputs("trapinit0: loop complete, loading IDT\n", 39);
  ((ushort *)&ptr[1])[-1] = sizeof(Segdesc) * 512 - 1;
  ptr[1] = (uintptr)temp_idt; /* Use temp_idt address */
  lidt(&((ushort *)&ptr[1])[-1]);
  uartputs("trapinit0: DONE\n", 17);
}

void trapinit(void) {
  print("trapinit: ENTRY\n");
  irqinit();

  nmienable();

  /*
   * Special traps.
   * Syscall() is called directly without going through trap().
   */
  print("trapinit: registering trap handlers\n");
  trapenable(VectorDE, debugexc, 0, "debugexc");
  trapenable(VectorBPT, debugbpt, 0, "debugpt");
  trapenable(VectorPF, faultamd64, 0, "faultamd64");
  print("trapinit: registered faultamd64 for VectorPF=%d\n", VectorPF);
  trapenable(Vector2F, doublefault, 0, "doublefault");
  trapenable(Vector15, unexpected, 0, "unexpected");

  print("trapinit: EXIT\n");
}

static char *excname[32] = {
    "divide error",
    "debug exception",
    "nonmaskable interrupt",
    "breakpoint",
    "overflow",
    "bounds check",
    "invalid opcode",
    "coprocessor not available",
    "double fault",
    "coprocessor segment overrun",
    "invalid TSS",
    "segment not present",
    "stack exception",
    "general protection violation",
    "page fault",
    "15 (reserved)",
    "coprocessor error",
    "alignment check",
    "machine check",
    "simd error",
    "20 (reserved)",
    "21 (reserved)",
    "22 (reserved)",
    "23 (reserved)",
    "24 (reserved)",
    "25 (reserved)",
    "26 (reserved)",
    "27 (reserved)",
    "28 (reserved)",
    "29 (reserved)",
    "30 (reserved)",
    "31 (reserved)",
};

static int usertrap(Ureg *ureg, int vno) {
  char buf[ERRMAX];

  if (vno < nelem(excname)) {
    spllo();
    if (up->pid == 1)
      print("usertrap: pid=1 (init) trap=%d (%s) pc=%#p sp=%#p\n", vno,
            excname[vno], ureg->pc, ureg->sp);
    snprint(buf, sizeof(buf), "sys: trap: %s", excname[vno]);
    postnote(up, 1, buf, NDebug);
    return 1;
  }
  return 0;
}

void trap(Ureg *ureg) {
  int vno, user;
  static int trap_count = 0;
  static int trapdebug = 0;
  static int post_exec_trap = 0;

  vno = ureg->type;

  /* DEBUG: Show first few traps during boot */
  trap_count++;
  if ((trap_count <= 50 || vno < 32 || trap_count % 100 == 0) && boot_verbose) {
    uintptr pc = ureg->pc;
    print("trap[%d]: vno=%d pc=%#p sp=%#p user=%d\n", trap_count, vno, pc,
          ureg->sp, userureg(ureg));
  }

  post_exec_trap++;
  (void)trap_count;
  (void)trapdebug;
  user = kenter(ureg);
  if (user && pebble_enabled)
    pebble_auto_verify(up, ureg);
  if (vno != VectorCNA)
    fpukenter(ureg);

  if (!irqhandled(ureg, vno) && (!user || !usertrap(ureg, vno))) {
    if (!user) {
      void (*pc)(void);

      extern void _rdmsrinst(void);
      extern void _wrmsrinst(void);
      extern void _peekinst(void);

      pc = (void *)ureg->pc;
      if (pc == _rdmsrinst || pc == _wrmsrinst) {
        if (vno == VectorGPF) {
          ureg->bp = -1;
          ureg->pc += 2;
          goto out;
        }
      } else if (pc == _peekinst) {
        if (vno == VectorGPF || vno == VectorPF) {
          ureg->pc += 2;
          goto out;
        }
      }

      /* early fault before trapinit() */
      if (vno == VectorPF) {
        faultamd64(ureg, 0);
        goto out;
      }
    }

    dumpregs(ureg);
    if (!user) {
      ureg->sp = (uintptr)&ureg->sp;
      _dumpstack(ureg);
    }
    if (vno < nelem(excname))
      panic("%s", excname[vno]);
    panic("unknown trap/intr: %d", vno);
  }
out:
  intret_debug_stage = 1;
  splhi();
  intret_debug_stage = 2;
  if (user) {
    if (up->procctl || up->nnote)
      donotify(ureg);
    kexit(ureg);
  }
  intret_debug_stage = 3;
  if (vno != VectorCNA)
    fpukexit(ureg);
  intret_debug_stage = 4;
}

void dumpregs(Ureg *ureg) {
  if (up)
    iprint("cpu%d: registers for %s %lud\n", m->machno, up->text, up->pid);
  else
    iprint("cpu%d: registers for kernel\n", m->machno);

  iprint("  AX %.16lluX  BX %.16lluX  CX %.16lluX\n", ureg->ax, ureg->bx,
         ureg->cx);
  iprint("  DX %.16lluX  SI %.16lluX  DI %.16lluX\n", ureg->dx, ureg->si,
         ureg->di);
  iprint("  BP %.16lluX  R8 %.16lluX  R9 %.16lluX\n", ureg->bp, ureg->r8,
         ureg->r9);
  iprint(" R10 %.16lluX R11 %.16lluX R12 %.16lluX\n", ureg->r10, ureg->r11,
         ureg->r12);
  iprint(" R13 %.16lluX R14 %.16lluX R15 %.16lluX\n", ureg->r13, ureg->r14,
         ureg->r15);
  iprint("  CS %.4lluX   SS %.4lluX    PC %.16lluX  SP %.16lluX\n",
         ureg->cs & 0xffff, ureg->ss & 0xffff, ureg->pc, ureg->sp);
  iprint("TYPE %.2lluX  ERROR %.4lluX FLAGS %.8lluX\n", ureg->type & 0xff,
         ureg->error & 0xffff, ureg->flags & 0xffffffff);

  /*
   * Processor control registers.
   * If machine check exception, time stamp counter, page size extensions
   * or enhanced virtual 8086 mode extensions are supported, there is a
   * CR4. If there is a CR4 and machine check extensions, read the machine
   * check address and machine check type registers if RDMSR supported.
   */
  iprint(" CR0 %8.8llux CR2 %16.16llux CR3 %16.16llux", getcr0(), getcr2(),
         getcr3());
  if (m->cpuiddx & (Mce | Tsc | Pse | Vmex)) {
    iprint(" CR4 %16.16llux\n", getcr4());
    if (ureg->type == 18)
      dumpmcregs();
  }
  iprint("  ur %#p up %#p\n", ureg, up);
}

/*
 * Fill in enough of Ureg to get a stack trace, and call a function.
 * Used by debugging interface rdb.
 */
void callwithureg(void (*fn)(Ureg *)) {
  Ureg ureg;
  ureg.pc = getcallerpc(&fn);
  ureg.sp = (uintptr)&fn;
  fn(&ureg);
}

static void doublefault(Ureg *, void *) { panic("double fault"); }

static void unexpected(Ureg *ureg, void *) {
  iprint("unexpected trap %llud\n", ureg->type);
  panic("unexpected");
}

static void _dumpstack(Ureg *ureg) {
  uintptr l, v, i, estack;
  extern char etext[];
  int x;
  char *s;
  char *pname;

  iprint("DBG\n");
  pname = up ? up->text : "<nil>";
  iprint("dumpstack\n");
  if ((s = getconf("*nodumpstack")) != nil && strcmp(s, "0") != 0) {
    iprint("dumpstack disabled\n");
    return;
  }
  x = 0;
  x += iprint("ktrace /kernel/path %#p %#p <<EOF\n", ureg->pc, ureg->sp);
  i = 0;
  if (up && (uintptr)&l >= (uintptr)up - KSTACK && (uintptr)&l <= (uintptr)up)
    estack = (uintptr)up;
  else if ((uintptr)&l >= (uintptr)m->stack &&
           (uintptr)&l <= (uintptr)m + MACHSIZE)
    estack = (uintptr)m + MACHSIZE;
  else
    return;
  x += iprint("estackx %p\n", estack);

  for (l = (uintptr)&l; l < estack; l += sizeof(uintptr)) {
    v = *(uintptr *)l;
    if (KTZERO < v && v < (uintptr)&etext) {
      /*
       * Could Pick off general CALL (((uchar*)v)[-5] == 0xE8)
       * and CALL indirect through AX
       * (((uchar*)v)[-2] == 0xFF && ((uchar*)v)[-2] == 0xD0),
       * but this is too clever and misses faulting address.
       */
      x += iprint("%.8lux=%.8lux ", (ulong)l, (ulong)v);
      i++;
    }
    if (i == 4) {
      i = 0;
      x += iprint("\n");
    }
  }
  if (i)
    iprint("\n");
  iprint("EOF\n");

  if (ureg->type != VectorNMI)
    return;

  i = 0;
  for (l = (uintptr)&l; l < estack; l += sizeof(uintptr)) {
    iprint("%.8p ", *(uintptr *)l);
    if (++i == 8) {
      i = 0;
      iprint("\n");
    }
  }
  if (i)
    iprint("\n");
}

void dumpstack(void) { callwithureg(_dumpstack); }

static void debugexc(Ureg *ureg, void *) {
  u64int dr6, m;
  char buf[ERRMAX];
  char *p, *e;
  int i;

  dr6 = getdr6();
  if (up == nil)
    panic("kernel debug exception dr6=%#.8ullx", dr6);
  putdr6(up->dr[6]);
  if (userureg(ureg))
    qlock(&up->debug);
  else if (!canqlock(&up->debug))
    return;
  m = up->dr[7];
  m = (m >> 4 | m >> 3) & 8 | (m >> 3 | m >> 2) & 4 | (m >> 2 | m >> 1) & 2 |
      (m >> 1 | m) & 1;
  m &= dr6;
  if (m == 0) {
    snprint(buf, sizeof(buf), "sys: debug exception dr6=%#.8ullx", dr6);
    postnote(up, 0, buf, NDebug);
  } else {
    p = buf;
    e = buf + sizeof(buf);
    p = seprint(p, e, "sys: watchpoint ");
    for (i = 0; i < 4; i++)
      if ((m & 1 << i) != 0)
        p = seprint(p, e, "%d%s", i, (m >> i + 1 != 0) ? "," : "");
    postnote(up, 0, buf, NDebug);
  }
  qunlock(&up->debug);
}

static void debugbpt(Ureg *ureg, void *) {
  if (up == 0)
    panic("kernel bpt");
  /* restore pc to instruction that caused the trap */
  ureg->pc--;
  postnote(up, 1, "sys: breakpoint", NDebug);
}

static void faultamd64(Ureg *ureg, void *) {
  uintptr addr;
  int read, user;
  static int fault_count = 0;
  extern int borrow_is_owned(uintptr key);
  extern Proc *borrow_get_owner(uintptr key);

  addr = getcr2();
  read = !(ureg->error & 2);
  user = userureg(ureg);

  /* Enhanced debug - show detailed fault info */
  fault_count++;
  if ((fault_count <= 20 || fault_count % 100 == 0) && boot_verbose) {
    print("\n=== PAGE FAULT #%d ===\n", fault_count);
    print("  Address:    %#p\n", addr);
    print("  PC:         %#p\n", ureg->pc);
    print("  SP:         %#p\n", ureg->sp);
    print("  Mode:       %s\n", user ? "user" : "kernel");
    print("  Access:     %s\n", read ? "READ" : "WRITE");
    print("  Error code: %#lux ", ureg->error);
    if (ureg->error & 1)
      print("[P] ");
    if (ureg->error & 2)
      print("[W] ");
    if (ureg->error & 4)
      print("[U] ");
    if (ureg->error & 8)
      print("[RSVD] ");
    if (ureg->error & 16)
      print("[I] ");
    print("\n");

    /* Check borrow checker ownership */
    uintptr pa = 0;
    if (user) {
      /* Walk user page table to get PA */
      extern uintptr *mmuwalk(uintptr *, uintptr, int, int);
      uintptr *pte = mmuwalk(m->pml4, addr, 0, 0);
      if (pte && (*pte & PTEVALID))
        pa = PPN(*pte);
    } else {
      /* Kernel address - check range before calling PADDR */
      if (addr >= KZERO) {
        pa = PADDR(addr); /* Simple approximation for direct map */
      } else {
        /* Kernel accessed user address (fault) - walk PT */
        extern uintptr *mmuwalk(uintptr *, uintptr, int, int);
        uintptr *pte = mmuwalk(m->pml4, addr, 0, 0);
        if (pte && (*pte & PTEVALID))
          pa = PPN(*pte);
      }
    }

    if (pa != 0 && borrow_is_owned(pa)) {
      Proc *owner = borrow_get_owner(pa);
      if (owner)
        print("  Borrow:     Page owned by proc '%s' (pid=%lu)\n",
              owner->text ? owner->text : "???", owner->pid);
      else
        print("  Borrow:     Page tracked by system/unknown\n");
    } else {
      if (up == nil)
        print("  Borrow:     Page not tracked (up==nil, early boot)\n");
      else
        print("  Borrow:     Page not tracked (not yet allocated or map "
              "failed)\n");
    }

    print("========================\n\n");
  }
  if (!user) {
    extern void _peekinst(void);

    if ((void (*)(void))ureg->pc == _peekinst) {
      ureg->pc += 2;
      return;
    }
    if (addr >= USTKTOP)
      panic("kernel fault: bad address pc=%#p addr=%#p", ureg->pc, addr);
    if (up == nil)
      panic("kernel fault: no user process pc=%#p addr=%#p", ureg->pc, addr);
    if (waserror()) {
      if (up->nerrlab == 0) {
        pprint("suicide: sys: %s\n", up->errstr);
        pexit(up->errstr, 1);
      }
      /* skipping bottom of trap(), so do it outselfs */
      splhi();
      fpukexit(ureg);
      spllo();
      nexterror();
    }
  }

  if (fault(addr, ureg->pc, read) < 0) {
    if (!user) {
      /* If kernel touched a user VA, blame the process instead of panicking */
      if (up != nil && addr < USTKTOP) {
        faultnote("fault", read ? "read" : "write", addr);
        poperror();
        return;
      }

      /* DEGRADED MODE: Log detailed info and attempt recovery */
      print("\n!!! KERNEL FAULT - ATTEMPTING RECOVERY !!!\n");
      dumpregs(ureg);

      /* Check if this is during boot/initialization */
      extern int xinit_done;
      if (!xinit_done) {
        print("FAULT: During early boot - attempting to continue\n");
        /* Zero out fault address in case it's a read */
        if (read && up != nil) {
          print("FAULT: Setting AX=0 and skipping instruction\n");
          ureg->ax = 0;
          ureg->pc += 4; /* Skip faulting instruction (approximate) */
          poperror();
          return;
        }
      }

      /* If we have a process context, try to kill it instead of panicking */
      if (up != nil) {
        print("FAULT: Killing process '%s' (pid=%d) instead of panic\n",
              up->text, up->pid);
        print("FAULT: This is a DEGRADED recovery - system may be unstable\n");
        poperror();
        pexit("kernel fault", 1);
        return;
      }

      /* Last resort: panic */
      panic("kernel fault: %s addr=%#p", read ? "read" : "write", addr);
    }
    faultnote("fault", read ? "read" : "write", addr);
  }

  print("faultamd64: DONE pid=%ld addr=%#llx user=%d\n", up ? up->pid : -1,
        (unsigned long long)addr, user);

  if (!user)
    poperror();
}

/*
 *  Syscall is called directly from assembler without going through trap().
 */

/* Syscall name lookup table for debug output */
static char *syscallnames[] = {
    [0] = "SYSR1",   [1] = "ERRSTR",   [2] = "BIND",    [3] = "CHDIR",
    [4] = "CLOSE",   [5] = "DUP",      [6] = "ALARM",   [7] = "EXEC",
    [8] = "EXITS",   [9] = "FSESSION", [10] = "FAUTH",  [11] = "FSTAT",
    [12] = "SEGBRK", [13] = "MOUNT",   [14] = "OPEN",   [15] = "READ",
    [16] = "OSEEK",  [17] = "SLEEP",   [18] = "STAT",   [19] = "RFORK",
    [20] = "WRITE",  [21] = "PIPE",    [22] = "CREATE", [23] = "FD2PATH",
    [24] = "BRK",    [25] = "REMOVE",  [39] = "SEEK",   [41] = "ERRSTR",
    [42] = "STAT",   [43] = "FSTAT",   [46] = "MOUNT",  [47] = "AWAIT",
    [50] = "PREAD",  [51] = "PWRITE",
};

void syscall(Ureg *ureg) {
  static int syscall_count = 0;

  syscall_count++;
  if (!kenter(ureg))
    panic("syscall: cs 0x%4.4lluX", ureg->cs);
  if (pebble_enabled)
    pebble_auto_verify(up, ureg);
  fpukenter(ureg);

  /* PURE MESSAGE-BASED ARCHITECTURE
   * ===============================
   * Syscall instruction is ONLY a doorbell trigger.
   * All operations come from 9P messages in exchange page.
   * NO RBP reading - NO legacy Plan 9 syscall ABI.
   */

  /* Print clean syscall entry */
  if ((syscall_count <= 50 || syscall_count % 100 == 0) && boot_verbose)
    print("SYSCALL[%d]: PURE-9P-DOORBELL pid=%ld\n", syscall_count,
          up ? up->pid : -1);

  /* PURE DOORBELL TRIGGER - No RBP reading */
  int result = p9_handle_doorbell(up, ureg);

  if (result < 0) {
    /* No valid message in exchange page */
    print("SYSCALL: Pure 9P mode - no valid message\n");
    ureg->ax = -1;
  } else {
    /* Message processed via 9P - result written to reply */
    print("SYSCALL: 9P message processed successfully\n");
    /* ureg->ax already set by p9_handle_doorbell */
  }

  /* FORCE INTERRUPTS ENABLED ON RETURN */
  /* Ensure R11 (for sysret) and Flags (for iret) have IF=1 */
  ureg->r11 |= 0x200;
  ureg->flags |= 0x200;

  /* if we delayed sched because we held a lock, sched now */
  if (up->delaysched) {
    print("syscall: calling sched()\n");
    sched();
    print("syscall: sched() returned\n");
  }

  /* Initialize stack slot to 0 for fast SYSRET path */
  ((void **)ureg)[-1] = nil;

  kexit(ureg);
  fpukexit(ureg);
}

Ureg *notify(Ureg *ureg, char *msg) {
  Ureg *nureg;
  uintptr sp;

  sp = ureg->sp;
  sp -= 256; /* debugging: preserve context causing problem */
  sp -= sizeof(Ureg);

  if (!okaddr(sp - ERRMAX - 4 * BY2WD, sizeof(Ureg) + ERRMAX + 4 * BY2WD, 1))
    return nil;

  nureg = (void *)sp;
  memmove(nureg, ureg, sizeof(Ureg));
  sp -= BY2WD + ERRMAX;
  memmove((char *)sp, msg, ERRMAX);
  sp -= 3 * BY2WD;
  ((uintptr *)sp)[2] = sp + 3 * BY2WD; /* arg2 string */
  ((uintptr *)sp)[1] = (uintptr)nureg; /* arg1 is ureg* */
  ((uintptr *)sp)[0] = 0;              /* arg0 is pc */

  ureg->sp = sp;
  ureg->pc = (uintptr)up->notify;
  ureg->bp = (uintptr)nureg; /* arg1 passed in RARG */
  ureg->cs = UESEL;
  ureg->ss = UD64SEL;

  return nureg;
}

/*
 *   Return user to state before notify()
 */
int noted(Ureg *ureg, Ureg *nureg, int arg0) {
  uintptr oureg, sp;

  oureg = (uintptr)nureg;

  /* don't let user change system flags or segment registers */
  setregisters(ureg, (char *)ureg, (char *)nureg, sizeof(Ureg));

  switch (arg0) {
  case NCONT:
  case NRSTR:
    if (!okaddr(ureg->pc, 1, 0) || !okaddr(ureg->sp, BY2WD, 0))
      return -1;
    break;

  case NSAVE:
    sp = oureg - 4 * BY2WD - ERRMAX;
    if (!okaddr(ureg->pc, 1, 0) || !okaddr(sp, 4 * BY2WD, 1))
      return -1;
    ureg->sp = sp;
    ureg->bp = oureg;           /* arg 1 passed in RARG */
    ((uintptr *)sp)[1] = oureg; /* arg 1 0(FP) is ureg* */
    ((uintptr *)sp)[0] = 0;     /* arg 0 is pc */
    break;
  }
  return 0;
}

uintptr execregs(uintptr entry, ulong ssize, ulong nargs) {
  uintptr *sp;
  Ureg *ureg;

  sp = (uintptr *)(USTKTOP - ssize);
  *--sp = nargs;
  ureg = up->dbgreg;
  ureg->sp = (uintptr)sp;
  ureg->pc = entry;
  ureg->cs = UESEL;
  ureg->ss = UD64SEL;
  ureg->r14 = ureg->r15 = 0; /* extern user registers */

  print("execregs: entry=%#p sp=%#p cs=%#x ss=%#x flags=%#llux\n", entry,
        ureg->sp, ureg->cs, ureg->ss, ureg->flags);
  print("execregs: ureg=%#p ureg->pc=%#p UESEL=%#x UD64SEL=%#x\n", ureg,
        ureg->pc, UESEL, UD64SEL);

  return (uintptr)USTKTOP -
         sizeof(Tos); /* address of kernel/user shared data */
}

/*
 *  return the userpc the last exception happened at
 */
uintptr userpc(void) {
  Ureg *ureg;

  ureg = (Ureg *)up->dbgreg;
  return ureg->pc;
}

/* This routine must save the values of registers the user is not permitted
 * to write from devproc and noted() and then restore the saved values before
 * returning.
 */
void setregisters(Ureg *ureg, char *pureg, char *uva, int n) {
  u64int flags;

  flags = ureg->flags;
  memmove(pureg, uva, n);
  ureg->cs = UESEL;
  ureg->ss = UD64SEL;
  ureg->flags = (ureg->flags & 0x00ff) | (flags & 0xff00);
  ureg->pc &= UADDRMASK;
}

void kprocchild(Proc *p, void (*entry)(void)) {
  /*
   * gotolabel() needs a word on the stack in
   * which to place the return PC used to jump
   * to linkproc().
   * Stack grows down from p->kstack + KSTACK.
   */
  p->sched.pc = (uintptr)entry;
  p->sched.sp = (uintptr)p->kstack + KSTACK - BY2WD;
}

void forkchild(Proc *p, Ureg *ureg) {
  Ureg *cureg;

  /*
   * Add 2*BY2WD to the stack to account for
   *  - the return PC
   *  - trap's argument (ur)
   */
  p->sched.sp = (uintptr)p - (sizeof(Ureg) + 2 * BY2WD);
  p->sched.pc = (uintptr)forkret;

  cureg = (Ureg *)(p->sched.sp + 2 * BY2WD);
  memmove(cureg, ureg, sizeof(Ureg));

  cureg->ax = 0;
  cureg->r14 = (uintptr)p;
}

/* Give enough context in the ureg to produce a kernel stack for
 * a sleeping process
 */
void setkernur(Ureg *ureg, Proc *p) {
  ureg->pc = p->sched.pc;
  ureg->sp = p->sched.sp + 8;
  ureg->r14 = (uintptr)p;
}

uintptr dbgpc(Proc *p) {
  Ureg *ureg;

  ureg = p->dbgreg;
  if (ureg == nil)
    return 0;
  return ureg->pc;
}
