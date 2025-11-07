#include	"u.h"
#include	"tos.h"
#include <lib.h>
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"borrowchecker.h"
#include	"pageown.h"
#include	"exchange.h"
#include	"pebble.h"
#include	"io.h"
#include	"pci.h"
#include	"ureg.h"
#include	"pool.h"
#include	"rebootcode.i"
#include	"initrd.h"
#include	"../../limine.h"
#include	"sdhw.h"

Conf conf;
int idle_spin;

/* CRITICAL: Global debug flag that doesn't depend on environment device */
int panic_debug = 1;  /* Default to debug mode to prevent reboot during early boot */

extern void (*i8237alloc)(void);
extern void bootscreeninit(void);
extern uintptr saved_limine_hhdm_offset;
extern void* kaddr(uintptr);

void
confinit(void)
{
	char *p;
	int i, userpcnt;
	ulong kpages;

	if(p = getconf("service")){
	if(strcmp(p, "cpu") == 0)
		cpuserver = 1;
	else if(strcmp(p,"terminal") == 0)
		cpuserver = 0;
	}

	if(p = getconf("*kernelpercent"))
	userpcnt = 100 - strtol(p, 0, 0);
	else
	userpcnt = 0;

	conf.npage = 0;
	for(i=0; i<nelem(conf.mem); i++)
	conf.npage += conf.mem[i].npage;

	conf.nproc = 100 + ((conf.npage*BY2PG)/MB)*5;
	if(cpuserver)
	conf.nproc *= 3;
	if(conf.nproc > 4000)
	conf.nproc = 4000;
	/* Temporary: limit to 100 procs for early boot debugging */
	if(conf.nproc > 100)
	conf.nproc = 100;
	conf.nimage = 200;
	conf.nswap = conf.nproc*80;
	conf.nswppo = 4096;

	if(cpuserver) {
	if(userpcnt < 10)
		userpcnt = 70;
	kpages = conf.npage - (conf.npage*userpcnt)/100;
	conf.nimage = conf.nproc;
	} else {
	if(userpcnt < 10) {
		if(conf.npage*BY2PG < 16*MB)
			userpcnt = 50;
		else
			userpcnt = 60;
	}
	kpages = conf.npage - (conf.npage*userpcnt)/100;

	/*
	 * Make sure terminals with low memory get at least
	 * 4MB on the first Image chunk allocation.
	 */
	if(conf.npage*BY2PG < 16*MB)
		imagmem->minarena = 4*MB;
	}

	/*
	 * can't go past the end of virtual memory.
	 */
	if(kpages > ((uintptr)-KZERO)/BY2PG)
	kpages = ((uintptr)-KZERO)/BY2PG;

	/* Ensure reasonable memory allocation for userspace */
	if(conf.npage > 0) {
	/* Make sure we leave at least 30% of memory for userspace */
	ulong min_upages = conf.npage * 30 / 100;
	if(conf.npage - kpages < min_upages) {
		kpages = conf.npage - min_upages;
	}
	}
	
	conf.upages = conf.npage - kpages;
	/* DEBUG: Print memory allocation details */
	print("DEBUG: npage=%lud, kpages=%lud, upages=%lud\n", conf.npage, kpages, conf.upages);
	/* Remove temporary memory allocation hack */
	// if(conf.upages > conf.npage/2)
	// 	conf.upages = conf.npage/10;  /* Give 90% to kernel temporarily */
	conf.ialloc = (kpages/2)*BY2PG;

	/*
	 * Guess how much is taken by the large permanent
	 * datastructures. Mntcache and Mntrpc are not accounted for.
	 */
	kpages *= BY2PG;
	kpages -= conf.nproc*sizeof(Proc*)
	+ conf.nimage*sizeof(Image)
	+ conf.nswap
	+ conf.nswppo*sizeof(Page*);
	mainmem->maxsize = kpages;

	/*
	 * the dynamic allocation will balance the load properly,
	 * hopefully. be careful with 32-bit overflow.
	 */
	imagmem->maxsize = kpages - (kpages/10);
	if(p = getconf("*imagemaxmb")){
	imagmem->maxsize = strtol(p, nil, 0)*MB;
	if(imagmem->maxsize > mainmem->maxsize)
		imagmem->maxsize = mainmem->maxsize;
	}
}

void
machinit(void)
{
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

void
mach0init(void)
{
	extern Mach *m;  /* Define m as extern - it should be in globals or bss */

	conf.nmach = 1;

	MACHP(0) = (Mach*)CPU0MACH;

	/* Initialize m to point to MACHP(0) */
	m = MACHP(0);

	/* Zero the entire Mach structure to ensure clean state */
	memset(m, 0, sizeof(Mach));

	m->machno = 0;
	m->pml4 = (u64int*)CPU0PML4;
	m->gdt = (Segdesc*)CPU0GDT;
	m->ticks = 0;
	m->ilockdepth = 0;

	machinit();

	active.machs[0] = 1;
	active.exiting = 0;
}

/* Main boot continuation after CR3 switch
 * Called directly by setuppagetables() after page table switch is complete */
void
main_after_cr3(void)
{
	char *p;

	/* CRITICAL: First output must be via UART to verify we got here */
	extern void uartputs(char*, int);
	uartputs("main_after_cr3: ENTERED\n", 24);

	/* Skip print() until we've reinitialized - it was set up with old stack */
	uartputs("BOOT: switched to kernel-managed page tables\n", 46);

	uartputs("main_after_cr3: calling xinit\n", 31);
	xinit();
	uartputs("main_after_cr3: calling pageowninit\n", 37);
	pageowninit();
	uartputs("main_after_cr3: calling exchangeinit\n", 38);
	exchangeinit();
	uartputs("BOOT: exchangeinit complete\n", 29);

	uartputs("main_after_cr3: calling trapinit\n", 35);
	trapinit();
	uartputs("main_after_cr3: calling mathinit\n", 35);
	mathinit();
	if(i8237alloc != nil)
	i8237alloc();
	uartputs("main_after_cr3: calling pcicfginit\n", 37);
	pcicfginit();
	uartputs("main_after_cr3: calling bootscreeninit\n", 41);
	bootscreeninit();
	uartputs("main_after_cr3: calling fbconsoleinit\n", 40);
	fbconsoleinit();  /* Initialize framebuffer console */
	uartputs("main_after_cr3: calling printinit\n", 36);
	printinit();

	/* NOW print() should work - printinit() re-initialized it for new stack */
	print("BOOT: printinit complete - serial console ready\n");
	cpuidprint();

	print("BOOT: getconf(\"*debug\") = %s\n", (getconf("*debug") == nil) ? "NIL" : "SET");
	print("BOOT: cpuserver = %d, conf.monitor = %d\n", cpuserver, conf.monitor);

	/* FIX: Move configuration environment setup earlier to prevent reboot */
	/* FIX: Set only the most critical variable early, defer the rest until devices are ready */
	print("BOOT: setting up minimal configuration environment\n");
	
	/* Note: *debug is now handled by global panic_debug variable, no need for ksetenv here */
	print("BOOT: deferring full environment setup until after chandevreset()\n");
	print("BOOT: after config setup - getconf(\"*debug\") = %s\n", (getconf("*debug") == nil) ? "NIL" : "SET");

	print("BOOT: about to call mmuinit\n");
	mmuinit();
	print("BOOT: mmuinit complete - runtime page tables live\n");
	print("BOOT: arch=%#p arch->intrinit=%#p\n", arch, arch->intrinit);
	if(arch->intrinit) {
	print("BOOT: calling arch->intrinit at %#p\n", arch->intrinit);
	arch->intrinit();
	print("BOOT: arch->intrinit returned successfully\n");
	} else {
	print("WARNING: arch->intrinit is nil\n");
	}
	print("BOOT: calling timersinit\n");
	timersinit();
	print("BOOT: timersinit complete\n");
	print("BOOT: calling arch->clockenable\n");
	if(arch->clockenable)
	arch->clockenable();
	print("BOOT: arch->clockenable complete\n");
	print("BOOT: calling procinit0\n");
	procinit0();
	print("BOOT: procinit0 complete - process table ready\n");
	initseg();
	links();
	iomapinit(0xFFFF);  /* Initialize I/O port allocation after links() */
	chandevreset();
	print("BOOT: device reset sequence finished\n");
	
	/* Set up the full environment - wrap in error handling like working 9/pc64 */
	print("BOOT: setting up full environment with error handling...\n");
	if(!waserror()) {
		char buf[2*KNAMELEN];
		snprint(buf, sizeof(buf), "%s %s", arch->id, conffile);
		ksetenv("terminal", buf, 0);
		ksetenv("cputype", "amd64", 0);
		if(cpuserver)
			ksetenv("service", "cpu", 0);
		else
			ksetenv("service", "terminal", 0);
		poperror();
		print("BOOT: environment setup completed successfully\n");
	} else {
		print("BOOT: WARNING - environment setup failed, continuing anyway\n");
		poperror();
	}
	
	userinit();
	print("BOOT: userinit scheduled *init* kernel process\n");
	print("BOOT: entering scheduler - expecting proc0 hand-off\n");
	schedinit();
}

void
init0(void)
{
	char **sp;

	chandevinit();
	print("BOOT[init0]: chandevinit complete\n");

	/* Register initrd files with devroot - must be after chandevinit */
	extern struct initrd_file *initrd_root;
	extern void initrd_register(void);
	if(initrd_root != nil) {
		print("BOOT[init0]: registering initrd files with devroot\n");
		initrd_register();
		print("BOOT[init0]: initrd files registered\n");
	} else {
		print("BOOT[init0]: WARNING - no initrd files to register\n");
	}

	if(!waserror()){
		/* Configuration environment already set up in main_after_cr3() */
		print("BOOT[init0]: environment already configured\n");
		poperror();
	}
	pebble_sip_issue_test();
	print("BOOT[init0]: starting alarm kproc\n");
	kproc("alarm", alarmkproc, 0);
	print("BOOT[init0]: alarm kproc scheduled\n");

	print("BOOT[init0]: computing user stack frame\n");
	sp = (char**)(USTKTOP - sizeof(Tos) - 8 - sizeof(sp[0])*4);
	print("BOOT[init0]: initial sp computed: %#p\n", sp);
	print("BOOT[init0]: USTKTOP=%#llux sizeof(Tos)=%lld\n", USTKTOP, (long long)sizeof(Tos));
	print("BOOT[init0]: stack frame already seeded in proc0\n");

	print("BOOT[init0]: preparing to disable interrupts\n");
	splhi();
	print("BOOT[init0]: fpukexit(nil) begin\n");
	fpukexit(nil);
	print("BOOT[init0]: fpukexit(nil) done\n");
	print("BOOT[init0]: entering user mode with initcode\n");
	print("BOOT[init0]: calling touser(%#p)\n", sp);
	touser(sp);
}

void
main(void)
{
	char *p;

	mach0init();
	bootargsinit();
	trapinit0();
	ioinit();
	i8250console();
	quotefmtinstall();
	screeninit();
	print("\nLux9\n");
	cpuidentify();
	/* Stash initrd pointers; parsing deferred until proc0 when allocators are ready */
	extern struct limine_module_request *limine_module;
	if(limine_module && limine_module->response && limine_module->response->module_count > 0) {
	struct limine_file *initrd = limine_module->response->modules[0];
	if(initrd && initrd->address){
		uintptr addr = (uintptr)initrd->address;
		if(addr >= saved_limine_hhdm_offset){
			initrd_physaddr = addr - saved_limine_hhdm_offset;
			initrd_base = initrd->address;
		}else{
			initrd_physaddr = addr;
			initrd_base = (void*)(addr + saved_limine_hhdm_offset);
		}
		initrd_size = initrd->size;
		print("initrd: limine reports module (%lld bytes)\n", (uvlong)initrd_size);
	}
	}

	meminit0();

	archinit();
	if(arch->clockinit){
	arch->clockinit();
	}
	meminit();
	ramdiskinit();
	confinit();
	pebbleinit();
	pebble_enabled = 1;
	if((p = getconf("pebble")) != nil)
	pebble_enabled = *p != '0';
	if((p = getconf("pebbledebug")) != nil && *p != '0')
	pebble_debug = 1;
	if(pebble_enabled)
	print("PEBBLE: runtime enabled (default budget %lud bytes)\n", (ulong)PEBBLE_DEFAULT_BUDGET);
	printinit();
	print("BOOT: printinit complete - serial console ready\n");

	/* CRITICAL: Initialize borrow checker BEFORE setuppagetables()
	 * because memory coordination needs it during CR3 switch */
	borrowinit();
	print("BOOT: borrow checker initialized\n");

	/* Initialize memory coordination system for boot handoff */
	boot_memory_coordination_init();
	print("BOOT: memory coordination system initialized\n");

	/* Switch to our own page tables - REQUIRED for user space!
	/* NOTE: This must happen AFTER setuppagetables() to avoid memory map conflicts.
	 * setuppagetables() now uses HHDM and no longer relocates the kernel.
	 * IMPORTANT: setuppagetables() calls main_after_cr3() directly to continue boot. */
	setuppagetables();

	/* UNREACHABLE - setuppagetables() never returns */
	panic("main: setuppagetables returned unexpectedly");
}

static void
rebootjump(uintptr entry, uintptr code, ulong size)
{
	void (*f)(uintptr, uintptr, ulong);
	uintptr *pte;

	arch->introff();

	/*
	 * This allows the reboot code to turn off the page mapping
	 */
	*mmuwalk(m->pml4, 0, 3, 0) = *mmuwalk(m->pml4, KZERO, 3, 0);
	*mmuwalk(m->pml4, 0, 2, 0) = *mmuwalk(m->pml4, KZERO, 2, 0);

	if((pte = mmuwalk(m->pml4, REBOOTADDR, 1, 0)) != nil)
	*pte &= ~PTENOEXEC;
	if((pte = mmuwalk(m->pml4, REBOOTADDR, 0, 0)) != nil)
	*pte &= ~PTENOEXEC;

	mmuflushtlb(PADDR(m->pml4));

	/* setup reboot trampoline function */
	f = (void*)REBOOTADDR;
	memmove(f, rebootcode, sizeof(rebootcode));

	/* off we go - never to return */
	coherence();
	(*f)(entry, code, size);

	for(;;);
}

void
exit(int)
{
	cpushutdown();
	splhi();

	if(m->machno)
	rebootjump(0, 0, 0);

	/* clear secrets */
	zeroprivatepages();
	poolreset(secrmem);

	arch->reset();
}

void
reboot(void *entry, void *code, ulong size)
{
	writeconf();
	vmxshutdown();

	/*
	 * the boot processor is cpu0.  execute this function on it
	 * so that the new kernel has the same cpu0.  this only matters
	 * because the hardware has a notion of which processor was the
	 * boot processor and we look at it at start up.
	 */
	while(m->machno != 0){
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

void
procsetup(Proc *p)
{
	fpuprocsetup(p);

	/* clear debug registers */
	memset(p->dr, 0, sizeof(p->dr));
	if(m->dr7 != 0){
	m->dr7 = 0;
	putdr7(0);
	}
}

void
procfork(Proc *p)
{
	fpuprocfork(p);
}

void
procrestore(Proc *p)
{
	if(p->dr[7] != 0){
	m->dr7 = p->dr[7];
	putdr(p->dr);
	}
	
	if(p->vmx != nil)
	vmxprocrestore(p);

	fpuprocrestore(p);
}

void
procsave(Proc *p)
{
	if(m->dr7 != 0){
	m->dr7 = 0;
	putdr7(0);
	}
	if(p->state == Moribund)
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
	mmuflushtlb(PADDR(m->pml4));
}

int
pcibiosinit(int *, int *)
{
	return -1;
}
