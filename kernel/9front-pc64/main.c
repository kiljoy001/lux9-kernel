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
	conf.nmach = 1;

	__asm__ volatile("outb %0, %1" : : "a"((char)'1'), "Nd"((unsigned short)0x3F8));
	MACHP(0) = (Mach*)CPU0MACH;

	/* Initialize m to point to MACHP(0) */
	__asm__ volatile("outb %0, %1" : : "a"((char)'A'), "Nd"((unsigned short)0x3F8));
	m = MACHP(0);
	__asm__ volatile("outb %0, %1" : : "a"((char)'B'), "Nd"((unsigned short)0x3F8));

	/* Zero the entire Mach structure to ensure clean state */
	memset(m, 0, sizeof(Mach));

	__asm__ volatile("outb %0, %1" : : "a"((char)'2'), "Nd"((unsigned short)0x3F8));
	m->machno = 0;
	__asm__ volatile("outb %0, %1" : : "a"((char)'3'), "Nd"((unsigned short)0x3F8));
	m->pml4 = (u64int*)CPU0PML4;
	__asm__ volatile("outb %0, %1" : : "a"((char)'4'), "Nd"((unsigned short)0x3F8));
	m->gdt = (Segdesc*)CPU0GDT;
	m->ticks = 0;
	m->ilockdepth = 0;

	__asm__ volatile("outb %0, %1" : : "a"((char)'5'), "Nd"((unsigned short)0x3F8));
	machinit();

	__asm__ volatile("outb %0, %1" : : "a"((char)'6'), "Nd"((unsigned short)0x3F8));
	active.machs[0] = 1;
	active.exiting = 0;
	__asm__ volatile("outb %0, %1" : : "a"((char)'7'), "Nd"((unsigned short)0x3F8));
}

void
init0(void)
{
	char buf[2*KNAMELEN], **sp;

	chandevinit();

	if(!waserror()){
		snprint(buf, sizeof(buf), "%s %s", arch->id, conffile);
		ksetenv("terminal", buf, 0);
		ksetenv("cputype", "amd64", 0);
		if(cpuserver)
			ksetenv("service", "cpu", 0);
		else
			ksetenv("service", "terminal", 0);
		setconfenv();
		poperror();
	}
	kproc("alarm", alarmkproc, 0);

	sp = (char**)(USTKTOP - sizeof(Tos) - 8 - sizeof(sp[0])*4);
	sp[3] = sp[2] = nil;
	strcpy(sp[1] = (char*)&sp[4], "boot");
	sp[0] = nil;

	splhi();
	fpukexit(nil);
	touser(sp);
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
	__asm__ volatile("outb %0, %1" : : "a"((char)'M'), "Nd"((unsigned short)0x3F8));
	mach0init();
	__asm__ volatile("outb %0, %1" : : "a"((char)'m'), "Nd"((unsigned short)0x3F8));
	bootargsinit();
	__asm__ volatile("outb %0, %1" : : "a"((char)'b'), "Nd"((unsigned short)0x3F8));
	trapinit0();
	debugchar('t');  /* trapinit0 done */
	ioinit();
	debugchar('i');  /* ioinit done */
	i8250console();
	debugchar('c');  /* i8250console done */
	quotefmtinstall();
	debugchar('q');  /* quotefmtinstall done */
	screeninit();
	debugchar('s');  /* screeninit done */
	/* Skip print for now - it crashes */
	/* print("\nPlan 9\n"); */
	debugchar('C');  /* about to cpuidentify */
	cpuidentify();
	debugchar('c');  /* cpuidentify done */
	debugchar('M');  /* about to meminit0 */
	meminit0();
	debugchar('m');  /* meminit0 done */

	/* Initialize initrd from Limine module */
	debugchar('I');  /* about to initrd_init */
	extern struct limine_module_request *limine_module;
	if(limine_module && limine_module->response && limine_module->response->module_count > 0) {
		struct limine_file *initrd = limine_module->response->modules[0];
		initrd_init(initrd->address, initrd->size);
	}
	debugchar('i');  /* initrd_init done */

	debugchar('A');  /* about to archinit */
	archinit();
	debugchar('!');  /* archinit done */
	debugchar('@');  /* returned from archinit */
	debugchar('#');  /* about to check clockinit */
	if(arch->clockinit){
		debugchar('$');  /* clockinit exists */
		arch->clockinit();
		debugchar('%');  /* clockinit done */
	}
	debugchar('^');  /* past clockinit */
	debugchar('&');  /* about to meminit */
	meminit();
	debugchar('*');  /* meminit done */
	debugchar('(');  /* about to ramdiskinit */
	ramdiskinit();
	debugchar(')');  /* ramdiskinit done */
	debugchar('_');  /* about to confinit */
	confinit();
	debugchar('+');  /* confinit done */
	debugchar('X');  /* about to xinit */
	xinit();
	debugchar('x');  /* xinit done */
	debugchar('T');  /* about to trapinit */
	trapinit();
	debugchar('t');  /* trapinit done */
	debugchar('M');  /* about to mathinit */
	mathinit();
	debugchar('m');  /* mathinit done */
	if(i8237alloc != nil)
		i8237alloc();
	debugchar('P');  /* about to pcicfginit */
	pcicfginit();
	debugchar('p');  /* pcicfginit done */
	debugchar('B');  /* about to bootscreeninit */
	bootscreeninit();
	debugchar('b');  /* bootscreeninit done */
	debugchar('R');  /* about to printinit */
	printinit();
	debugchar('r');  /* printinit done */
	debugchar('U');  /* about to cpuidprint */
	cpuidprint();
	debugchar('u');  /* cpuidprint done */
	debugchar('N');  /* about to mmuinit */
	/* Skip mmuinit - needs xinit's memory pools */
	/* mmuinit(); */
	debugchar('n');  /* mmuinit skipped */
	debugchar('I');  /* about to arch->intrinit */
	if(arch->intrinit)
		arch->intrinit();
	debugchar('i');  /* intrinit done */
	debugchar('T');  /* about to timersinit */
	/* Skip timersinit - needs xinit's memory pools */
	/* timersinit(); */
	debugchar('t');  /* timersinit skipped */
	debugchar('C');  /* about to arch->clockenable */
	if(arch->clockenable)
		arch->clockenable();
	debugchar('c');  /* clockenable done */
	debugchar('P');  /* about to procinit0 */
	procinit0();
	debugchar('p');  /* procinit0 done */
	debugchar('S');  /* about to initseg */
	initseg();
	debugchar('s');  /* initseg done */
	debugchar('L');  /* about to links */
	links();
	debugchar('l');  /* links done */
	debugchar('D');  /* about to chandevreset */
	chandevreset();
	debugchar('d');  /* chandevreset done */
	debugchar('A');  /* about to preallocpages */
	/* Skip preallocpages for now - crashes accessing conf.mem */
	/* preallocpages(); */
	debugchar('a');  /* preallocpages skipped */
	debugchar('G');  /* about to pageinit */
	pageinit();
	debugchar('g');  /* pageinit done */

	/* Initialize run queues to ensure clean state */
	extern Schedq runq[];
	extern int Nrq;
	for(int i = 0; i < 22; i++){  /* Nrq = 22 */
		runq[i].head = nil;
		runq[i].tail = nil;
		runq[i].n = 0;
	}
	debugchar('r');  /* runq initialized */

	debugchar('U');  /* about to userinit */
	userinit();
	debugchar('u');  /* userinit done */
	debugchar('Z');  /* about to schedinit */
	schedinit();
	debugchar('z');  /* schedinit done - shouldn't reach here */
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

