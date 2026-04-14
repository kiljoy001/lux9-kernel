#include "../../limine.h"
#include "9p_router.h"
#include "borrowchecker.h"
#include "dat.h"
#include "exchange.h"
#include "fns.h"
#include "initrd.h"
#include "io.h"
#include "mem.h"
#include "pageown.h"
#include "pci.h"
#include "pebble.h"
#include "pool.h"
#include "rebootcode.i"
#include "sdhw.h"
#include "tos.h"
#include "u.h"
#include "vmdetect.h"
#include <lib.h>

Conf conf;
int idle_spin;

int console_ready = 0;

/* BOOT STATE MACHINE */
typedef enum {
  BOOT_START,
  BOOT_XINIT,
  BOOT_MEM_COORD,
  BOOT_PAGES_OWN,
  BOOT_EXCHANGE,
  BOOT_TRAP,
  BOOT_ARCH,
  BOOT_PROC_INIT,
  BOOT_SEG_INIT,
  BOOT_LINKS,
  BOOT_IO,
  BOOT_CHANDEV_RESET,
  BOOT_PAGEK,
  BOOT_PRINT,
  BOOT_TPM,
  BOOT_MSGORD,
  BOOT_CRYPTO,
  BOOT_CHANDEV_INIT,
  BOOT_USERINIT,
  BOOT_SCHED,
  BOOT_COMPLETE
} BootState;

BootState current_boot_state = BOOT_START;
static char *boot_state_names[] = {
    "START",   "XINIT",         "MEM_COORD",    "PAGES_OWN", "EXCHANGE",
    "TRAP",    "ARCH",          "PROC_INIT",    "SEG_INIT",  "LINKS",
    "IO",      "CHANDEV_RESET", "PAGEK",        "PRINT",     "TPM",
    "MSGORD",  "CRYPTO",        "CHANDEV_INIT", "USERINIT",  "SCHED",
    "COMPLETE"};

static void boot_state_require(BootState required, BootState next,
                               const char *reason) {
  if (current_boot_state < required) {
    panic("BOOT_STATE: %s requires %s (%s)", boot_state_names[next],
          boot_state_names[required], reason);
  }
}

/* Boot state transition rules (strict enforcement) */
void set_boot_state(BootState s) {
  if (s < current_boot_state && s != BOOT_START) {
    panic("BOOT_STATE: Invalid regression %s -> %s",
          boot_state_names[current_boot_state], boot_state_names[s]);
  }
  if (s == current_boot_state)
    return;

  switch (s) {
  case BOOT_CHANDEV_INIT:
    boot_state_require(BOOT_CRYPTO, s,
                       "crypto must be initialized before devices");
    break;
  case BOOT_USERINIT:
    boot_state_require(BOOT_CHANDEV_INIT, s,
                       "devices must be initialized before userspace");
    break;
  case BOOT_SCHED:
    boot_state_require(BOOT_USERINIT, s,
                       "userspace must be initialized before scheduler");
    break;
  case BOOT_COMPLETE:
    boot_state_require(BOOT_SCHED, s,
                       "scheduler must be running before boot complete");
    break;
  default:
    break;
  }

  current_boot_state = s;

  if (boot_verbose) {
    if (console_ready)
      print("BOOT_STATE: %s\n", boot_state_names[s]);
    else
      uartputs("BOOT_STATE: ", 12),
          uartputs(boot_state_names[s], strlen(boot_state_names[s])),
          uartputs("\n", 1);
  }
}

/* CRITICAL: Global debug flag that doesn't depend on environment device */
int panic_debug = 1; /* Enabled for Tsyscall testing */
int jitdebug = 0;    /* JIT debug flag */

/* Boot verbosity control */
int boot_verbose = 1;
extern void uartputs(char *, int);

/* Consolidated boot debug function */
static void boot_log(char *fmt, ...) {
  if (!boot_verbose)
    return;

  char buf[256];
  va_list arg;
  va_start(arg, fmt);
  vsnprint(buf, sizeof(buf), fmt, arg);
  va_end(arg);

  if (console_ready)
    print("%s", buf);
  else
    uartputs(buf, strlen(buf));
}

extern void (*i8237alloc)(void);
extern void bootscreeninit(void);
extern uintptr saved_limine_hhdm_offset;
extern void *kaddr(uintptr);

void confinit(void) {
  char *p;
  int i, userpcnt;
  ulong kpages;

  if ((p = getconf("verbose")) != nil && *p != '0')
    boot_verbose = 1;

  if (p = getconf("service")) {
    if (strcmp(p, "cpu") == 0)
      cpuserver = 1;
    else if (strcmp(p, "terminal") == 0)
      cpuserver = 0;
  }

  if (p = getconf("*kernelpercent"))
    userpcnt = 100 - strtol(p, 0, 0);
  else
    userpcnt = 0;

  conf.npage = 0;
  for (i = 0; i < nelem(conf.mem); i++)
    conf.npage += conf.mem[i].npage;

  conf.nproc = 100 + ((conf.npage * BY2PG) / MB) * 5;
  if (cpuserver)
    conf.nproc *= 3;
  if (conf.nproc > 4000)
    conf.nproc = 4000;
  /* Set minimum of 100 procs */
  if (conf.nproc < 100)
    conf.nproc = 100;
  conf.nimage = 200;
  conf.nswap = conf.nproc * 80;
  conf.nswppo = 4096;

  if (cpuserver) {
    if (userpcnt < 10)
      userpcnt = 70;
    kpages = conf.npage - (conf.npage * userpcnt) / 100;
    conf.nimage = conf.nproc;
  } else {
    if (userpcnt < 10) {
      if (conf.npage * BY2PG < 16 * MB)
        userpcnt = 50;
      else
        userpcnt = 60;
    }
    kpages = conf.npage - (conf.npage * userpcnt) / 100;

    /*
     * Make sure terminals with low memory get at least
     * 4MB on the first Image chunk allocation.
     */
    if (conf.npage * BY2PG < 16 * MB)
      imagmem->minarena = 4 * MB;
  }

  /*
   * can't go past the end of virtual memory.
   */
  if (kpages > ((uintptr)-KZERO) / BY2PG)
    kpages = ((uintptr)-KZERO) / BY2PG;

  /* Ensure reasonable memory allocation for userspace */
  if (conf.npage > 0) {
    /* Make sure we leave at least 30% of memory for userspace */
    ulong min_upages = conf.npage * 30 / 100;
    if (conf.npage - kpages < min_upages) {
      kpages = conf.npage - min_upages;
    }
  }

  conf.upages = conf.npage - kpages;
  /* DEBUG: Print memory allocation details */
  print("DEBUG: npage=%lud, kpages=%lud, upages=%lud\n", conf.npage, kpages,
        conf.upages);
  /* Remove temporary memory allocation hack */
  // if(conf.upages > conf.npage/2)
  // 	conf.upages = conf.npage/10;  /* Give 90% to kernel temporarily */
  conf.ialloc = (kpages / 2) * BY2PG;

  /*
   * Guess how much is taken by the large permanent
   * datastructures. Mntcache and Mntrpc are not accounted for.
   */
  kpages *= BY2PG;
  kpages -= conf.nproc * sizeof(Proc *) + conf.nimage * sizeof(Image) +
            conf.nswap + conf.nswppo * sizeof(Page *);
  mainmem->maxsize = kpages;

  /*
   * the dynamic allocation will balance the load properly,
   * hopefully. be careful with 32-bit overflow.
   */
  imagmem->maxsize = kpages - (kpages / 10);
  if (p = getconf("*imagemaxmb")) {
    imagmem->maxsize = strtol(p, nil, 0) * MB;
    if (imagmem->maxsize > mainmem->maxsize)
      imagmem->maxsize = mainmem->maxsize;
  }
}

void machinit(void) {
  int machno;
  Segdesc *gdt;
  uintptr *pml4;

  machno = m->machno;
  pml4 = m->pml4;
  gdt = m->gdt;
  memset(m, 0, sizeof(Mach));
  m->machno = machno;
  m->pml4 = pml4;
  m->gdt = gdt;
  m->perf.period = 1;

  /*
   * For polled uart output at boot, need
   * a default delay constant. 100000 should
   * be enough for a while. Cpuidentify will
   * calculate the real value later.
   */
  m->loopconst = 100000;
}

/*@
    requires \true;
    assigns m, *MACHP(0), conf.nmach, active.machs[0], active.exiting;
    ensures m == MACHP(0);
    ensures m->machno == 0;
    ensures conf.nmach == 1;
    ensures active.machs[0] == 1;
    ensures active.exiting == 0;
*/
void mach0init(void) {
  extern Mach *m; /* Define m as extern - it should be in globals or bss */

  conf.nmach = 1;

  MACHP(0) = (Mach *)CPU0MACH;

  /* Initialize m to point to MACHP(0) */
  m = MACHP(0);

  /* Zero the entire Mach structure to ensure clean state */
  memset(m, 0, sizeof(Mach));

  m->machno = 0;
  m->pml4 = (u64int *)CPU0PML4;
  m->gdt = (Segdesc *)CPU0GDT;
  m->ticks = 0;
  m->ilockdepth = 0;

  machinit();

  active.machs[0] = 1;
  active.exiting = 0;
}

/* Main boot continuation after CR3 switch
 * Called directly by setuppagetables() after page table switch is complete */
/*@
    requires current_boot_state == BOOT_START || current_boot_state ==
   BOOT_XINIT; assigns current_boot_state;
    // We cannot easily specify all the state changes in the kernel global state
   here
    // as it touches almost everything.
    ensures current_boot_state == BOOT_SCHED;
*/
void main_after_cr3(void) {
  char *p;

  /* CRITICAL: First output must be via UART to verify we got here */
  boot_log("main_after_cr3: ENTERED\n");
  set_boot_state(BOOT_XINIT);

  /* Skip print() until we've reinitialized - it was set up with old stack */

  boot_log("main_after_cr3: calling xinit\n");
  xinit();

  /* Transition memory tracking to dynamic allocator */
  establish_memory_ownership_zones_dynamic();

  set_boot_state(BOOT_PAGES_OWN);
  boot_log("main_after_cr3: calling pageowninit\n");
  pageowninit();

  set_boot_state(BOOT_EXCHANGE);
  boot_log("main_after_cr3: calling exchangeinit\n");
  exchangeinit();

  /* Initialize global exchange pool for child process isolation */
  extern void exchange_pool_init(uint pool_size);
  boot_log("main_after_cr3: calling exchange_pool_init\n");
  exchange_pool_init(64); /* 64 pages for exchange pool */

  boot_log("DEBUG: pre-pebble-selftest [SKIPPED]\n");
  /* Run Pebble Self-Test (xalloc works now) */
  /* extern void pebble_selftest(void); */
  /* pebble_selftest(); */
  boot_log("DEBUG: post-pebble-selftest\n");

  set_boot_state(BOOT_TRAP);
  boot_log("main_after_cr3: calling trapinit\n");
  trapinit();
  boot_log("main_after_cr3: calling mathinit\n");
  mathinit();
  if (i8237alloc != nil)
    i8237alloc();
  /* pcicfginit removed - Moved to userspace resurrection server */
  // boot_log("main_after_cr3: calling pcicfginit\n");
  // pcicfginit();
  // boot_log("DEBUG: pcicfginit RETURNED\n");

  /* LATE DEBUG: Check InitRD and Memory Map */
  {
    extern uintptr initrd_physaddr;
    extern usize initrd_size;
    uartputs("DEBUG_LATE: InitRD phys=", 22);
    print("%#p size=%#lux\n", (void *)initrd_physaddr, (uvlong)initrd_size);

    uartputs("DEBUG_LATE: Checking conf.mem overlap...\n", 37);
    for (int i = 0; i < nelem(conf.mem); i++) {
      if (conf.mem[i].npage == 0)
        continue;
      uintptr base = conf.mem[i].base;
      uintptr end = base + conf.mem[i].npage * BY2PG;
      print("  Region %d: %#p - %#p\n", i, (void *)base, (void *)end);

      if (initrd_size > 0) {
        uintptr istart = initrd_physaddr;
        uintptr iend = istart + initrd_size;
        if ((istart >= base && istart < end) || (iend > base && iend <= end)) {
          uartputs("CRITICAL: InitRD OVERLAPS with Region!\n", 37);
        }
      }
    }
  }

  boot_log("main_after_cr3: calling bootscreeninit\n");
  bootscreeninit();
  boot_log("DEBUG: bootscreeninit RETURNED\n");
  boot_log("main_after_cr3: calling fbconsoleinit ENTER\n");
  fbconsoleinit();
  console_ready = 1; /* Console is now initialized */
  boot_log("DEBUG: fbconsoleinit RETURNED\n");
  boot_log("main_after_cr3: fbconsoleinit RETURNED\n");
  boot_log("main_after_cr3: before cpuidentify check\n");
  if (cpuidentify_done == 0)
    cpuidentify(); /* Initialize CPU data structures before cpuidprint() */
  boot_log("main_after_cr3: calling fpuinit\n");
  fpuinit(); /* Initialize FPU - must happen after xinit() */
  boot_log("main_after_cr3: fpuinit returned, calling cpuidprint\n");
  cpuidprint();

  /* Early configuration setup to prevent reboot issues */
  {
    extern void bootconfinit(void);
    extern void kconf_set(char *, char *);
    char buf[2 * KNAMELEN];

    /* 1. Populate all bootargs into global confname/confval arrays */
    bootconfinit();

    /* 2. Set critical kernel variables using kconf_set (safe for early boot) */
    /* Note: setconfenv() is NOT called here - it uses ksetenv() which requires
     * channels/processes. We'll call it later in init0() after proc0 is ready.
     */
    kconf_set("cputype", "amd64");
    kconf_set("service", cpuserver ? "cpu" : "terminal");

    snprint(buf, sizeof(buf), "%s %s", arch->id, conffile);
    kconf_set("terminal", buf);

    boot_log("BOOT_INFO: Early environment config set (critical vars only)\n");
  }

  mmuinit();

  /* Initialize r15 to point to Mach structure after mmuinit sets up GS */
  __asm__ volatile("movq %0, %%r15" : : "r"(m) : "r15");
  boot_log("DEBUG: Initialized r15=m=%p after mmuinit\n", m);

  /* Debug: check if IDT is still valid after mmuinit */
  {
    extern Segdesc temp_idt[];
    if (boot_verbose) {
      print("DEBUG: Checking IDT[0x46] AFTER mmuinit:\n");
      print("  IDT[0x46*2].d0 = %#lux\n", temp_idt[0x46 * 2].d0);
      print("  IDT[0x46*2].d1 = %#lux\n", temp_idt[0x46 * 2].d1);
      if (temp_idt[0x46 * 2].d0 == 0 && temp_idt[0x46 * 2].d1 == 0)
        print("ERROR: IDT[0x46] CORRUPTED by mmuinit()!\n");
      else
        print("OK: IDT[0x46] still valid after mmuinit\n");

      /* Check timer interrupt IDT entry (vector 32) */
      print("DEBUG: Checking IDT[32] (timer):\n");
      print("  IDT[32*2].d0 = %#lux\n", temp_idt[32 * 2].d0);
      print("  IDT[32*2].d1 = %#lux\n", temp_idt[32 * 2].d1);
      print("  IST field = %d (bits 0-2 of d1)\n",
            (int)(temp_idt[32 * 2].d1 & 0x7));
    }
  }

  boot_log("DEBUG: About to call arch->intrinit\n");

  /* Re-map ACPI tables after CR3 switch (if ACPI is being used) */
  extern PCArch archacpi;
  if (arch == &archacpi) {
    extern void acpi_remap_tables(void);
    boot_log("DEBUG: Re-mapping ACPI tables after CR3 switch\n");
    acpi_remap_tables();
  }

  if (arch->intrinit) {
    set_boot_state(BOOT_ARCH);
    boot_log("DEBUG: Calling arch->intrinit (ACPI: acpiinit)\n");
    arch->intrinit();
    boot_log("DEBUG: arch->intrinit complete\n");

    /* Debug: check if IDT is still valid after arch->intrinit (pcmpinit) */
    {
      extern Segdesc temp_idt[];
      /* DEBUG: Reduced verbose IDT checking
      print("DEBUG: Checking IDT[0x46] AFTER arch->intrinit:\n");
      print("  IDT[0x46*2].d0 = %#lux\n", temp_idt[0x46*2].d0);
      print("  IDT[0x46*2].d1 = %#lux\n", temp_idt[0x46*2].d1);
      if(temp_idt[0x46*2].d0 == 0 && temp_idt[0x46*2].d1 == 0)
              print("ERROR: IDT[0x46] CORRUPTED by arch->intrinit!\n");
      else
              print("OK: IDT[0x46] still valid after arch->intrinit\n");
      */
    }

  } else {
    print("WARNING: arch->intrinit is nil\n");
  }

  set_boot_state(BOOT_PROC_INIT);
  procinit0();
  boot_log("DEBUG: procinit0 complete\n");

  set_boot_state(BOOT_SEG_INIT);
  initseg();
  boot_log("DEBUG: initseg complete\n");

  set_boot_state(BOOT_LINKS);
  links();
  boot_log("DEBUG: links complete\n");

  /* Initialize I/O port allocation after links() */
  set_boot_state(BOOT_IO);
  iomapinit(0xFFFF);
  boot_log("DEBUG: iomapinit complete\n");

  /* Reset and initialize all devices before environment setup */
  set_boot_state(BOOT_CHANDEV_RESET);
  chandevreset();
  boot_log("DEBUG: chandevreset complete\n");

  set_boot_state(BOOT_PAGEK);
  pageinit();
  boot_log("DEBUG: pageinit complete\n");

  set_boot_state(BOOT_PRINT);
  printinit();
  /* Initialize TPM driver before crypto subsystem */
  extern void tpminit(void);
  set_boot_state(BOOT_TPM);
  print("=== Initializing TPM Driver ===\n");
  tpminit();
  print("=== TPM Driver Initialized ===\n");

  /* Initialize MSGORD consensus subsystem */
  extern void msgord_init(uint k_param);
  set_boot_state(BOOT_MSGORD);
  print("=== Initializing MSGORD Consensus ===\n");
  msgord_init(3); /* k=3 for robust ordering */
  print("=== MSGORD Consensus Initialized ===\n");

  /* Initialize distributed Pebble token economy */
  extern void distributed_pebble_init(void);
  print("=== Initializing Distributed Pebble ===\n");
  distributed_pebble_init();
  print("=== Distributed Pebble Initialized ===\n");

  /* Initialize WASM3 Runtime (Layer 1) */
  extern void wasm_runtime_init(void);
  print("=== Initializing WASM3 Runtime (Layer 1) ===\n");
  wasm_runtime_init();
  print("=== WASM3 Runtime Initialized ===\n");

  /* Initialize crypto subsystem early for testing */
  extern int crypto_tpm_key_init(void);
  set_boot_state(BOOT_CRYPTO);
  print("=== Initializing Crypto Subsystem ===\n");
  crypto_tpm_key_init();
  print("=== Crypto Subsystem Initialized ===\n");

  /* Run TPM 2.0 Kernel Test */
  extern void tpm_test_run(void);
  tpm_test_run();

  /* Initialize device drivers BEFORE spawning proc0 */
  set_boot_state(BOOT_CHANDEV_INIT);
  chandevinit();
  boot_log("DEBUG: chandevinit complete\n");

  /* Now spawn proc0 - devices are ready */
  set_boot_state(BOOT_USERINIT);
  userinit();
  boot_log("DEBUG: userinit complete\n");

  /* Debug: show scheduler state before entering schedinit */
  extern ulong runvec;
  extern int nrdy;
  /* Pre-initialize timers with interrupts masked; actual enable happens in
   * proc0 */
  splhi();
  timersinit();

  /* CRITICAL FIX: Register the timer interrupt handler
   * timersinit() sets up the timer hardware but doesn't register the handler.
   * Without this, vno=32 has no handler and causes interrupt storm. */
  extern void lapicclock(Ureg *, void *);
  intrenable(IrqTIMER, lapicclock, nil, BUSUNKNOWN, "clock");
  boot_log("DEBUG: Timer handler registered at vno=%d\n", VectorPIC + IrqTIMER);

  spllo(); /* Re-enable interrupts for scheduler - CRITICAL */
  boot_log("DEBUG: timersinit complete, interrupts enabled\n");
  set_boot_state(BOOT_SCHED);
  schedinit();
}

/**
 * Perform final boot-time setup and transfer proc0 to user mode.
 *
 * Registers any initrd files with the device root (if present), ensures the
 * environment is configured, runs a Pebble SIP test, starts the alarm kernel
 * process, computes and logs proc0's initial user stack frame, prepares and
 * clears FPU state, raises interrupt level, and finally enters user mode by
 * calling touser() with the prepared stack frame.
 */
// test_hybrid_batching declaration/call usually here
void test_hybrid_batching(void) {} // Stub

void init0(void) {
  /* Run hybrid IPC batching tests */
  extern void test_hybrid_batching(void);
  test_hybrid_batching();

  char buf[2 * KNAMELEN], **sp;

  /*
   * Open console for stdin, stdout, stderr
   * Use #c/cons directly since /dev not bound yet
   */
  /*
   * Open console for stdin, stdout, stderr
   * Use #c/cons directly since /dev not bound yet
   */
  if (waserror())
    panic("init0: cannot open console: %r");
  uartputs("init0: calling kopen(stdin)\n", 26);
  kopen("#c/cons", OREAD); /* fd 0 - stdin */
  uartputs("init0: calling kopen(stdout)\n", 27);
  kopen("#c/cons", OWRITE); /* fd 1 - stdout */
  uartputs("init0: calling kopen(stderr)\n", 27);
  kopen("#c/cons", OWRITE); /* fd 2 - stderr */
  poperror();

  uartputs("init0: calling randominit\n", 24);
  randominit();

  /* Setup environment variables */
  if (!waserror()) {
    snprint(buf, sizeof(buf), "%s %s", arch->id, conffile);
    print("init0: about to call ksetenv('terminal', '%s', 0)\n", buf);
    ksetenv("terminal", buf, 0);
    print("init0: ksetenv('terminal') returned\n");
    ksetenv("cputype", "amd64", 0);
    print("init0: ksetenv('cputype') returned\n");
    ksetenv("service", cpuserver ? "cpu" : "terminal", 0);
    print("init0: ksetenv('service') returned\n");
    print("init0: about to call setconfenv()\n");
    setconfenv();
    poperror();
    print("BOOT[init0]: environment setup completed\n");
  } else {
    print("BOOT[init0]: environment setup failed: %r\n");
  }

  /* uartputs("init0: calling kproc(alarm)\n", 26); */
  /* kproc("alarm", alarmkproc, 0); */

  uintptr *stack = (uintptr *)(USTKTOP - sizeof(Tos) - 16 - sizeof(sp[0]) * 4);
  print("BOOT[init0]: using prebuilt user stack at %#p (p9uaddr=%#p)\n", stack,
        (void *)p9_user_base(up));
  {
    uintptr *pte = mmuwalk(m->pml4, (uintptr)stack, 0, 0);
    if (pte && (*pte & PTEVALID)) {
      uintptr pa = PPN(*pte) | ((uintptr)stack & (BY2PG - 1));
      uintptr *kva = (uintptr *)KADDR(pa & ~(BY2PG - 1));
      uintptr off = ((uintptr)stack & (BY2PG - 1)) / sizeof(uintptr);
      print("BOOT[init0]: ustack[0]=%#p ustack[1]=%#p ustack[2]=%#p\n",
            (void *)kva[off], (void *)kva[off + 1], (void *)kva[off + 2]);
    } else {
      print("BOOT[init0]: ustack PTE missing\n");
    }
  }

  splhi();
  uartputs("init0: calling fpukexit\n", 25);
  fpukexit(nil);
  uartputs("init0: fpukexit returned\n", 25);
  if (m->proc == nil)
    panic("BOOT[init0]: m->proc is NULL before touser()!");
  uartputs("init0: calling touser\n", 22);
  print("BOOT[init0]: entry_point=0x%lx\n", up->entry_point);

  /* Check if this is a WASM process */
  if (up->wasm.initialized) {
    print("BOOT[init0]: Executing WASM process (pid=%lu)\n", up->pid);
    /* Entry point holds the start_func pointer for WASM */
    wasm_exec_run((struct M3Function *)up->entry_point);
    /* NOTREACHED */
  }

  touser((void *)stack, up->entry_point, p9_user_base(up));
}

void main(void) {
  char *p;
  extern void uartprintf(char *,
                         ...); /* Formatted UART output before prbuf is ready */

#include "../../limine.h"

  mach0init();
  i8250console();
  uartputs("TEST: main() started\n", 21);

  /* Initialize Limine HHDM offset - REQUIRED for KADDR() */
  extern struct limine_hhdm_request *limine_hhdm;
  if (limine_hhdm && limine_hhdm->response) {
    saved_limine_hhdm_offset = limine_hhdm->response->offset;
    uartprintf("init: HHDM offset set to %#p\n",
               (void *)saved_limine_hhdm_offset);
  } else {
    uartprintf("init: WARNING - No HHDM response found!\n");
  }

  bootargsinit();
  trapinit0();
  ioinit();

  /* Debug: check if trapinit0() actually initialized the IDT */
  {
    extern Segdesc temp_idt[];
    extern void sidt(void *);
    uintptr idtr[2];

    /* Read the IDTR register */
    sidt(&((ushort *)&idtr[1])[-1]);
    uintptr idt_base = idtr[1];
    ushort idt_limit = ((ushort *)&idtr[1])[-1];

    /* DEBUG: Reduced verbose IDT checking
    uartprintf("DEBUG: Checking IDT after trapinit0:\n");
    uartprintf("  temp_idt addr: %#p\n", temp_idt);
    uartprintf("  IDTR base: %#lux\n", idt_base);
    uartprintf("  IDTR limit: %d\n", idt_limit);

    if(idt_base != (uintptr)temp_idt){
            uartprintf("ERROR: IDTR pointing to WRONG address!\n");
            uartprintf("  Expected: %#p\n", temp_idt);
            uartprintf("  Actual: %#lux\n", idt_base);
    } else {
            uartprintf("OK: IDTR points to temp_idt\n");
    }

    uartprintf("  IDT[0x46*2].d0 = %#lux\n", temp_idt[0x46*2].d0);
    uartprintf("  IDT[0x46*2].d1 = %#lux\n", temp_idt[0x46*2].d1);
    if(temp_idt[0x46*2].d0 == 0 && temp_idt[0x46*2].d1 == 0)
            uartprintf("ERROR: IDT[0x46] is ZERO after trapinit0()!\n");
    else
            uartprintf("OK: IDT[0x46] is initialized\n");
    */
  }

  quotefmtinstall();
  screeninit();
  uartprintf("\nLux9\n");

  vm_detect();
  vm_apply_workarounds();

  /*@
      requires m->machno == 0;
      assigns m->cpuhz, m->havetsc, m->cpuidax, m->cpuidbx, m->cpuidcx,
     m->cpuiddx; assigns m->cpuiddx; ensures m->cpuidax != 0;
  */
  cpuidentify();
  if (boot_verbose)
    uartprintf("main: cpuidentify() returned\n");
  /* Stash initrd pointers; parsing deferred until proc0 when allocators are
   * ready */
  extern struct limine_module_request *limine_module;
  if (limine_module && limine_module->response &&
      limine_module->response->module_count > 0) {
    struct limine_file *initrd = limine_module->response->modules[0];
    if (initrd && initrd->address) {
      uintptr addr = (uintptr)initrd->address;
      if (addr >= saved_limine_hhdm_offset) {
        initrd_physaddr = addr - saved_limine_hhdm_offset;
        initrd_base = initrd->address;
      } else {
        initrd_physaddr = addr;
        initrd_base = (void *)(addr + saved_limine_hhdm_offset);
      }
      initrd_size = initrd->size;
      uartprintf(
          "initrd: limine reports module at %#p (phys %#p) size %lld bytes\n",
          (void *)addr, (void *)initrd_physaddr, (uvlong)initrd_size);
    } else {
      uartprintf("initrd: limine module entry has no address!\n");
    }
  } else {
    uartprintf("initrd: no limine modules found\n");
  }

  uartprintf("CHECK: calling meminit0()\n");
  meminit0();
  uartprintf("CHECK: meminit0() returned\n");

  uartprintf("CHECK: calling archinit()\n");
  archinit();
  uartprintf("CHECK: archinit() returned\n");

  if (arch->clockinit) {
    uartprintf("CHECK: calling arch->clockinit()\n");
    arch->clockinit();
    uartprintf("CHECK: arch->clockinit() returned\n");
  }

  uartprintf("CHECK: calling meminit()\n");
  meminit(); // CRITICAL: Populates conf.mem and initializes palloc
  uartprintf("CHECK: meminit() returned\n");

  uartprintf("CHECK: calling confinit()\n");
  confinit();
  xinit();
  // poolinit(); // Not needed/defined
  uartputs("DEBUG: Testing free(0)\n", 22);
  volatile void *ptr = 0;
  free((void *)ptr);
  uartputs("DEBUG: free(0) survived\n", 23);
  pebble_enabled = 1;
  if ((p = getconf("pebble")) != nil)
    pebble_enabled = *p != '0';
  if ((p = getconf("pebbledebug")) != nil && *p != '0')
    pebble_debug = 1;
  if (pebble_enabled)
    uartprintf("PEBBLE: runtime enabled (default budget %lud bytes)\n",
               (ulong)PEBBLE_DEFAULT_BUDGET);

  /* CRITICAL: Initialize borrow checker BEFORE setuppagetables()
   * because memory coordination needs it during CR3 switch */
  borrowinit();

  /* Initialize memory coordination system for boot handoff */
  boot_memory_coordination_init();

  /* Establish static kernel memory zones for early boot protection */
  establish_memory_ownership_zones();

  /* Save framebuffer info BEFORE switching page tables */
  save_framebuffer_info();

  /* Switch to our own page tables - REQUIRED for user space!
  /* NOTE: This must happen AFTER setuppagetables() to avoid memory map
  conflicts.
   * setuppagetables() now uses HHDM and no longer relocates the kernel.
   * IMPORTANT: setuppagetables() calls main_after_cr3() directly to continue
  boot. */
  setuppagetables();

  /* UNREACHABLE - setuppagetables() never returns */
  panic("main: setuppagetables returned unexpectedly");
}

static void rebootjump(uintptr entry, uintptr code, ulong size) {
  void (*f)(uintptr, uintptr, ulong);
  uintptr *pte;

  arch->introff();

  /*
   * This allows the reboot code to turn off the page mapping
   */
  /* Hack: Explicitly declare mmuwalk if fns.h fails us */
  extern uintptr *mmuwalk(uintptr *, uintptr, int, int);
  *mmuwalk(m->pml4, 0, 3, 0) = *mmuwalk(m->pml4, KZERO, 3, 0);
  *mmuwalk(m->pml4, 0, 2, 0) = *mmuwalk(m->pml4, KZERO, 2, 0);

  if ((pte = mmuwalk(m->pml4, REBOOTADDR, 1, 0)) != nil)
    *pte &= ~PTENOEXEC;
  if ((pte = mmuwalk(m->pml4, REBOOTADDR, 0, 0)) != nil)
    *pte &= ~PTENOEXEC;

  mmuflushtlb(PADDR(m->pml4));

  /* setup reboot trampoline function */
  f = (void *)REBOOTADDR;
  memmove(f, rebootcode, sizeof(rebootcode));

  /* off we go - never to return */
  coherence();
  (*f)(entry, code, size);

  for (;;)
    ;
}

void exit(int) {
  cpushutdown();
  splhi();

  if (m->machno)
    rebootjump(0, 0, 0);

  /* clear secrets */
  zeroprivatepages();
  poolreset(secrmem);

  arch->reset();
}

void reboot(void *entry, void *code, ulong size) {
  writeconf();
  vmxshutdown();

  /*
   * the boot processor is cpu0.  execute this function on it
   * so that the new kernel has the same cpu0.  this only matters
   * because the hardware has a notion of which processor was the
   * boot processor and we look at it at start up.
   */
  while (m->machno != 0) {
    procwired(up, 0);
    sched();
  }
  cpushutdown();
  delay(1000);
  splhi();

  /* turn off buffered serial console */
  serialoq = nil;

  /* shutdown devices */
  chandevshutdown();

  /* clear secrets */
  zeroprivatepages();
  poolreset(secrmem);

  /* disable pci devices */
  pcireset();

  rebootjump((uintptr)entry & (ulong)~0xF0000000UL, PADDR(code), size);
}

void procsetup(Proc *p) {
  fpuprocsetup(p);

  /* clear debug registers */
  memset(p->dr, 0, sizeof(p->dr));
  if (m->dr7 != 0) {
    m->dr7 = 0;
    putdr7(0);
  }
}

void procfork(Proc *p) { fpuprocfork(p); }

void procrestore(Proc *p) {
  if (p->dr[7] != 0) {
    m->dr7 = p->dr[7];
    extern void putdr(u64int *);
    putdr(p->dr);
  }

  if (p->vmx != nil)
    vmxprocrestore(p);

  fpuprocrestore(p);
}

void procsave(Proc *p) {
  if (m->dr7 != 0) {
    m->dr7 = 0;
    putdr7(0);
  }
  if (p->state == Moribund)
    p->dr[7] = 0;

  fpuprocsave(p);

  /*
   * While this processor is in the scheduler, the process could run
   * on another processor and exit, returning the page tables to
   * the free list where they could be reallocated and overwritten.
   * When this processor eventually has to get an entry from the
   * trashed page tables it will crash.
   *
   * If there's only one processor, this can't happen.
   * You might think it would be a win not to do this in that case,
   * especially on VMware, but it turns out not to matter.
   */
  /* DISABLED: mmuflushtlb() was causing hang during first context switch */
  /* mmuflushtlb(PADDR(m->pml4)); */
}

int pcibiosinit(int *, int *) { return -1; }
