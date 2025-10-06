#include	"u.h"
#include	"tos.h"
#include <lib.h>
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"io.h"
#include	"pci.h"
#include	"ureg.h"
#include	"pool.h"
#include	"rebootcode.i"
#include	"initrd.h"

Conf conf;
int idle_spin;

extern void (*i8237alloc)(void);
extern void bootscreeninit(void);

/* Limine boot structures */
struct limine_file {
	u64int revision;
	void *address;
	u64int size;
	char *path;
	char *cmdline;
	u32int media_type;
	u32int unused;
	u32int tftp_ip;
	u32int tftp_port;
	u32int partition_index;
	u32int mbr_disk_id;
	void *gpt_disk_uuid;
	void *gpt_part_uuid;
	void *part_uuid;
};

struct limine_module_response {
	u64int revision;
	u64int module_count;
	struct limine_file **modules;
};

struct limine_module_request {
	u64int id[4];
	u64int revision;
	struct limine_module_response *response;
};

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

	conf.upages = conf.npage - kpages;
	/* TEMP: Give more memory to kernel for early boot xalloc */
	if(conf.upages > conf.npage/2)
		conf.upages = conf.npage/10;  /* Give 90% to kernel temporarily */
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

	__asm__ volatile("outb %0, %1" : : "a"((char)'0'), "Nd"((unsigned short)0x3F8));
	__asm__ volatile("outb %0, %1" : : "a"((char)'9'), "Nd"((unsigned short)0x3F8));
	__asm__ volatile("outb %0, %1" : : "a"((char)'a'), "Nd"((unsigned short)0x3F8));
	conf.nmach = 1;

	__asm__ volatile("outb %0, %1" : : "a"((char)'0'), "Nd"((unsigned short)0x3F8));
	__asm__ volatile("outb %0, %1" : : "a"((char)'9'), "Nd"((unsigned short)0x3F8));
	__asm__ volatile("outb %0, %1" : : "a"((char)'b'), "Nd"((unsigned short)0x3F8));
	MACHP(0) = (Mach*)CPU0MACH;

	/* Initialize m to point to MACHP(0) */
	__asm__ volatile("outb %0, %1" : : "a"((char)'0'), "Nd"((unsigned short)0x3F8));
	__asm__ volatile("outb %0, %1" : : "a"((char)'9'), "Nd"((unsigned short)0x3F8));
	__asm__ volatile("outb %0, %1" : : "a"((char)'c'), "Nd"((unsigned short)0x3F8));
	m = MACHP(0);
	__asm__ volatile("outb %0, %1" : : "a"((char)'0'), "Nd"((unsigned short)0x3F8));
	__asm__ volatile("outb %0, %1" : : "a"((char)'9'), "Nd"((unsigned short)0x3F8));
	__asm__ volatile("outb %0, %1" : : "a"((char)'d'), "Nd"((unsigned short)0x3F8));

	/* Zero the entire Mach structure to ensure clean state */
	memset(m, 0, sizeof(Mach));

	__asm__ volatile("outb %0, %1" : : "a"((char)'0'), "Nd"((unsigned short)0x3F8));
	__asm__ volatile("outb %0, %1" : : "a"((char)'9'), "Nd"((unsigned short)0x3F8));
	__asm__ volatile("outb %0, %1" : : "a"((char)'e'), "Nd"((unsigned short)0x3F8));
	m->machno = 0;
	__asm__ volatile("outb %0, %1" : : "a"((char)'0'), "Nd"((unsigned short)0x3F8));
	__asm__ volatile("outb %0, %1" : : "a"((char)'9'), "Nd"((unsigned short)0x3F8));
	__asm__ volatile("outb %0, %1" : : "a"((char)'f'), "Nd"((unsigned short)0x3F8));
	m->pml4 = (u64int*)CPU0PML4;
	__asm__ volatile("outb %0, %1" : : "a"((char)'0'), "Nd"((unsigned short)0x3F8));
	__asm__ volatile("outb %0, %1" : : "a"((char)'9'), "Nd"((unsigned short)0x3F8));
	__asm__ volatile("outb %0, %1" : : "a"((char)'g'), "Nd"((unsigned short)0x3F8));
	m->gdt = (Segdesc*)CPU0GDT;
	m->ticks = 0;
	m->ilockdepth = 0;

	__asm__ volatile("outb %0, %1" : : "a"((char)'0'), "Nd"((unsigned short)0x3F8));
	__asm__ volatile("outb %0, %1" : : "a"((char)'9'), "Nd"((unsigned short)0x3F8));
	__asm__ volatile("outb %0, %1" : : "a"((char)'h'), "Nd"((unsigned short)0x3F8));
	machinit();

	__asm__ volatile("outb %0, %1" : : "a"((char)'0'), "Nd"((unsigned short)0x3F8));
	__asm__ volatile("outb %0, %1" : : "a"((char)'9'), "Nd"((unsigned short)0x3F8));
	__asm__ volatile("outb %0, %1" : : "a"((char)'i'), "Nd"((unsigned short)0x3F8));
	active.machs[0] = 1;
	active.exiting = 0;
	__asm__ volatile("outb %0, %1" : : "a"((char)'0'), "Nd"((unsigned short)0x3F8));
	__asm__ volatile("outb %0, %1" : : "a"((char)'9'), "Nd"((unsigned short)0x3F8));
	__asm__ volatile("outb %0, %1" : : "a"((char)'j'), "Nd"((unsigned short)0x3F8));
}

void
init0(void)
{
	char buf[2*KNAMELEN], **sp;

	__asm__ volatile("outb %0, %1" : : "a"((char)'I'), "Nd"((unsigned short)0x3F8));  /* init0: entry */
	__asm__ volatile("outb %0, %1" : : "a"((char)'0'), "Nd"((unsigned short)0x3F8));
	chandevinit();
	__asm__ volatile("outb %0, %1" : : "a"((char)'I'), "Nd"((unsigned short)0x3F8));  /* init0: chandevinit done */
	__asm__ volatile("outb %0, %1" : : "a"((char)'1'), "Nd"((unsigned short)0x3F8));

	if(!waserror()){
		__asm__ volatile("outb %0, %1" : : "a"((char)'I'), "Nd"((unsigned short)0x3F8));  /* init0: in waserror */
		__asm__ volatile("outb %0, %1" : : "a"((char)'2'), "Nd"((unsigned short)0x3F8));
		snprint(buf, sizeof(buf), "%s %s", arch->id, conffile);
		__asm__ volatile("outb %0, %1" : : "a"((char)'I'), "Nd"((unsigned short)0x3F8));  /* init0: snprint done */
		__asm__ volatile("outb %0, %1" : : "a"((char)'3'), "Nd"((unsigned short)0x3F8));
		ksetenv("terminal", buf, 0);
		__asm__ volatile("outb %0, %1" : : "a"((char)'I'), "Nd"((unsigned short)0x3F8));  /* init0: ksetenv 1 done */
		__asm__ volatile("outb %0, %1" : : "a"((char)'4'), "Nd"((unsigned short)0x3F8));
		ksetenv("cputype", "amd64", 0);
		__asm__ volatile("outb %0, %1" : : "a"((char)'I'), "Nd"((unsigned short)0x3F8));  /* init0: ksetenv 2 done */
		__asm__ volatile("outb %0, %1" : : "a"((char)'5'), "Nd"((unsigned short)0x3F8));
		if(cpuserver)
			ksetenv("service", "cpu", 0);
		else
			ksetenv("service", "terminal", 0);
		__asm__ volatile("outb %0, %1" : : "a"((char)'I'), "Nd"((unsigned short)0x3F8));  /* init0: ksetenv 3 done */
		__asm__ volatile("outb %0, %1" : : "a"((char)'6'), "Nd"((unsigned short)0x3F8));
		setconfenv();
		__asm__ volatile("outb %0, %1" : : "a"((char)'I'), "Nd"((unsigned short)0x3F8));  /* init0: setconfenv done */
		__asm__ volatile("outb %0, %1" : : "a"((char)'7'), "Nd"((unsigned short)0x3F8));
		poperror();
		__asm__ volatile("outb %0, %1" : : "a"((char)'I'), "Nd"((unsigned short)0x3F8));  /* init0: poperror done */
		__asm__ volatile("outb %0, %1" : : "a"((char)'8'), "Nd"((unsigned short)0x3F8));
	}
	__asm__ volatile("outb %0, %1" : : "a"((char)'I'), "Nd"((unsigned short)0x3F8));  /* Before alarm kproc */
	__asm__ volatile("outb %0, %1" : : "a"((char)'9'), "Nd"((unsigned short)0x3F8));
	kproc("alarm", alarmkproc, 0);
	__asm__ volatile("outb %0, %1" : : "a"((char)'J'), "Nd"((unsigned short)0x3F8));  /* After alarm kproc */
	__asm__ volatile("outb %0, %1" : : "a"((char)'0'), "Nd"((unsigned short)0x3F8));

	__asm__ volatile("outb %0, %1" : : "a"((char)'s'), "Nd"((unsigned short)0x3F8));  /* Before sp calculation */
	__asm__ volatile("outb %0, %1" : : "a"((char)'p'), "Nd"((unsigned short)0x3F8));
	sp = (char**)(USTKTOP - sizeof(Tos) - 8 - sizeof(sp[0])*4);
	__asm__ volatile("outb %0, %1" : : "a"((char)'S'), "Nd"((unsigned short)0x3F8));  /* After sp calculation */
	__asm__ volatile("outb %0, %1" : : "a"((char)'P'), "Nd"((unsigned short)0x3F8));
	sp[3] = sp[2] = nil;
	strcpy(sp[1] = (char*)&sp[4], "boot");
	sp[0] = nil;

	__asm__ volatile("outb %0, %1" : : "a"((char)'J'), "Nd"((unsigned short)0x3F8));  /* Before splhi */
	__asm__ volatile("outb %0, %1" : : "a"((char)'1'), "Nd"((unsigned short)0x3F8));
	splhi();
	__asm__ volatile("outb %0, %1" : : "a"((char)'J'), "Nd"((unsigned short)0x3F8));  /* Before fpukexit */
	__asm__ volatile("outb %0, %1" : : "a"((char)'2'), "Nd"((unsigned short)0x3F8));
	fpukexit(nil);
	__asm__ volatile("outb %0, %1" : : "a"((char)'J'), "Nd"((unsigned short)0x3F8));  /* Before touser */
	__asm__ volatile("outb %0, %1" : : "a"((char)'3'), "Nd"((unsigned short)0x3F8));
	touser(sp);
	__asm__ volatile("outb %0, %1" : : "a"((char)'J'), "Nd"((unsigned short)0x3F8));  /* After touser - should never reach */
	__asm__ volatile("outb %0, %1" : : "a"((char)'4'), "Nd"((unsigned short)0x3F8));
}

static void
debugchar(char c)
{
	/* Direct serial output for early debugging */
	__asm__ volatile("outb %0, %1" : : "a"(c), "Nd"((unsigned short)0x3F8));
}

void
main(void)
{
	debugchar('0'); debugchar('9');  /* Step 09: mach0init */
	mach0init();
	debugchar('1'); debugchar('0');  /* Step 10: bootargsinit */
	bootargsinit();
	debugchar('1'); debugchar('1');  /* Step 11: trapinit0 */
	trapinit0();
	debugchar('1'); debugchar('2');  /* Step 12: ioinit */
	ioinit();
	debugchar('1'); debugchar('3');  /* Step 13: i8250console */
	i8250console();
	debugchar('1'); debugchar('4');  /* Step 14: quotefmtinstall */
	quotefmtinstall();
	debugchar('1'); debugchar('5');  /* Step 15: screeninit */
	screeninit();
	debugchar('1'); debugchar('6');  /* Step 16: print */
	print("\nPlan 9\n");
	debugchar('1'); debugchar('7');  /* Step 17: cpuidentify */
	cpuidentify();
	/* Initialize initrd from Limine module - BEFORE meminit0 */
	debugchar('1'); debugchar('8');  /* Step 18: about to initrd_init */
	extern struct limine_module_request *limine_module;
	if(limine_module) {
		debugchar('1'); debugchar('9');  /* Step 19: limine_module exists */
		if(limine_module->response) {
			debugchar('2'); debugchar('0');  /* Step 20: response exists */
			if(limine_module->response->module_count > 0) {
				debugchar('2'); debugchar('1');  /* Step 21: module_count > 0 */
				struct limine_file *initrd = limine_module->response->modules[0];
				if(initrd && initrd->address) {
					debugchar('2'); debugchar('2');  /* Step 22: initrd address valid */
					initrd_init(initrd->address, initrd->size);
					debugchar('2'); debugchar('3');  /* Step 23: initrd_init done */
				}
			}
		}
	}
	debugchar('2'); debugchar('4');  /* Step 24: initrd check complete */

	debugchar('2'); debugchar('5');  /* Step 25: about to meminit0 */
	meminit0();
	debugchar('2'); debugchar('6');  /* Step 26: meminit0 done */

	debugchar('2'); debugchar('7');  /* Step 27: about to archinit */
	archinit();
	debugchar('2'); debugchar('8');  /* Step 28: archinit done */
	debugchar('2'); debugchar('9');  /* Step 29: returned from archinit */
	debugchar('3'); debugchar('0');  /* Step 30: about to check clockinit */
	if(arch->clockinit){
		debugchar('3'); debugchar('1');  /* Step 31: clockinit exists */
		arch->clockinit();
		debugchar('3'); debugchar('2');  /* Step 32: clockinit done */
	}
	debugchar('3'); debugchar('3');  /* Step 33: past clockinit */
	debugchar('3'); debugchar('4');  /* Step 34: about to meminit */
	meminit();
	debugchar('3'); debugchar('5');  /* Step 35: meminit done */
	debugchar('P'); debugchar('O');  /* Page ownership init */
	pageowninit();
	debugchar('P'); debugchar('D');  /* Page ownership done */
	debugchar('3'); debugchar('6');  /* Step 36: about to ramdiskinit */
	ramdiskinit();
	debugchar('3'); debugchar('7');  /* Step 37: ramdiskinit done */
	debugchar('3'); debugchar('8');  /* Step 38: about to confinit */
	confinit();
	debugchar('3'); debugchar('9');  /* Step 39: confinit done */
	debugchar('4'); debugchar('0');  /* Step 40: about to xinit */
	xinit();
	debugchar('4'); debugchar('1');  /* Step 41: xinit done */

	/* Setup OUR page tables - independent of Limine */
	debugchar('S'); debugchar('P');  /* About to setup page tables */
	setuppagetables();
	debugchar('S'); debugchar('D');  /* Setup page tables done */

	debugchar('4'); debugchar('2');  /* Step 42: about to trapinit */
	trapinit();
	debugchar('4'); debugchar('3');  /* Step 43: trapinit done */
	debugchar('4'); debugchar('4');  /* Step 44: about to mathinit */
	mathinit();
	debugchar('4'); debugchar('5');  /* Step 45: mathinit done */
	if(i8237alloc != nil)
		i8237alloc();
	debugchar('4'); debugchar('6');  /* Step 46: about to pcicfginit */
	pcicfginit();
	debugchar('4'); debugchar('7');  /* Step 47: pcicfginit done */
	debugchar('4'); debugchar('8');  /* Step 48: about to bootscreeninit */
	bootscreeninit();
	debugchar('4'); debugchar('9');  /* Step 49: bootscreeninit done */
	debugchar('5'); debugchar('0');  /* Step 50: about to printinit */
	printinit();
	debugchar('5'); debugchar('1');  /* Step 51: printinit done */
	debugchar('5'); debugchar('2');  /* Step 52: about to cpuidprint */
	cpuidprint();
	debugchar('5'); debugchar('3');  /* Step 53: cpuidprint done */
	debugchar('5'); debugchar('4');  /* Step 54: about to mmuinit */
	mmuinit();
	debugchar('5'); debugchar('5');  /* Step 55: mmuinit done */
	debugchar('5'); debugchar('6');  /* Step 56: about to arch->intrinit */
	if(arch->intrinit)
		arch->intrinit();
	debugchar('5'); debugchar('7');  /* Step 57: intrinit done */
	debugchar('5'); debugchar('8');  /* Step 58: about to timersinit */
	timersinit();
	debugchar('5'); debugchar('9');  /* Step 59: timersinit done */
	debugchar('6'); debugchar('0');  /* Step 60: about to arch->clockenable */
	if(arch->clockenable)
		arch->clockenable();
	debugchar('6'); debugchar('1');  /* Step 61: clockenable done */
	debugchar('6'); debugchar('2');  /* Step 62: about to procinit0 */
	procinit0();
	debugchar('6'); debugchar('3');  /* Step 63: procinit0 done */
	debugchar('6'); debugchar('4');  /* Step 64: about to initseg */
	initseg();
	debugchar('6'); debugchar('5');  /* Step 65: initseg done */
	debugchar('6'); debugchar('6');  /* Step 66: about to links */
	links();
	debugchar('6'); debugchar('7');  /* Step 67: links done */
	debugchar('6'); debugchar('8');  /* Step 68: about to chandevreset */
	chandevreset();
	debugchar('6'); debugchar('9');  /* Step 69: chandevreset done */

	/* Register initrd files with devroot - must be after chandevreset */
	if(initrd_root != nil) {
		debugchar('7'); debugchar('0');  /* Step 70: about to initrd_register */
		initrd_register();
		debugchar('7'); debugchar('1');  /* Step 71: initrd_register done */
	}

	debugchar('7'); debugchar('2');  /* Step 72: about to preallocpages */
	preallocpages();
	debugchar('7'); debugchar('3');  /* Step 73: preallocpages done */
	debugchar('7'); debugchar('4');  /* Step 74: about to pageinit */
	pageinit();
	debugchar('7'); debugchar('5');  /* Step 75: pageinit done */

	/* Initialize run queues to ensure clean state */
	extern Schedq runq[];
	extern int Nrq;
	for(int i = 0; i < 22; i++){  /* Nrq = 22 */
		runq[i].head = nil;
		runq[i].tail = nil;
		runq[i].n = 0;
	}
	debugchar('7'); debugchar('6');  /* Step 76: runq initialized */

	debugchar('7'); debugchar('7');  /* Step 77: about to userinit */
	userinit();
	debugchar('7'); debugchar('8');  /* Step 78: userinit done */
	debugchar('7'); debugchar('9');  /* Step 79: about to schedinit */
	schedinit();
	debugchar('8'); debugchar('0');  /* Step 80: schedinit done - shouldn't reach here */
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

