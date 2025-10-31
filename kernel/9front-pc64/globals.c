/* Global kernel variables */
#include "u.h"
#include "portlib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "pci.h"
#include "sdhw.h"
#include "io.h"
#include "error.h"

extern uvlong rdtsc(void);

/* Format flags - from libc.h */
enum {
	FmtWidth	= 1,
	FmtLeft		= FmtWidth << 1,
	FmtPrec		= FmtLeft << 1,
	FmtSharp	= FmtPrec << 1,
	FmtSpace	= FmtSharp << 1,
	FmtSign		= FmtSpace << 1,
	FmtZero		= FmtSign << 1,
	FmtUnsigned	= FmtZero << 1,
	FmtShort	= FmtUnsigned << 1,
	FmtLong		= FmtShort << 1,
	FmtVLong	= FmtLong << 1,
	FmtComma	= FmtVLong << 1,
	FmtByte		= FmtComma << 1,
	FmtFlag		= FmtByte << 1
};

/* Format function declarations */
extern int _charfmt(Fmt*);
extern int _runefmt(Fmt*);
extern int _ifmt(Fmt*);
extern int _strfmt(Fmt*);
extern int _runesfmt(Fmt*);
extern int _percentfmt(Fmt*);
extern int _countfmt(Fmt*);
extern int _flagfmt(Fmt*);
extern int _badfmt(Fmt*);

/* Per-CPU and per-process globals */
Mach *m = nil;
Proc *up = nil;

/* HHDM offset - stored in early boot, guaranteed to survive CR3 switch */
uintptr saved_limine_hhdm_offset = 0;

/* Global kernel data structures */
struct Swapalloc swapalloc;
struct Kmesg kmesg;
struct Active active;
Mach* machp[MAXMACH];

/* Global function pointers */
void (*consdebug)(void) = nil;
void (*hwrandbuf)(void*, ulong) = nil;
void (*kproftimer)(uintptr) = nil;
void (*screenputs)(char*, int) = nil;

/* SD hardware function pointers */
int (*sd_inb)(int) = nil;
void (*sd_outb)(int, int) = nil;
ulong (*sd_inl)(int) = nil;
void (*sd_outl)(int, ulong) = nil;
void (*sd_insb)(int, void*, int) = nil;
void (*sd_inss)(int, void*, int) = nil;
void (*sd_outsb)(int, void*, int) = nil;
void (*sd_outss)(int, void*, int) = nil;
Pcidev* (*sd_pcimatch)(Pcidev* prev, int vid, int did) = pcimatch;
void (*sd_microdelay)(int) = nil;

/* libc9 formatting support */
int _fmtFdFlush(Fmt *f)
{
	/* Stub for now - would write buffered format output to FD */
	(void)f;
	return 0;
}

/* Get return address of caller */
uintptr getcallerpc(void *v)
{
	(void)v;
	return (uintptr)__builtin_return_address(1);
}

/* Error strings */
char Etoolong[] = "name too long";

/* Utility stubs */
void srvrenameuser(char *old, char *new)
{
	(void)old; (void)new;
}

void shrrenameuser(char *old, char *new)
{
	(void)old; (void)new;
}

int needpages(void *v)
{
	(void)v;
	return 0;
}

void idlehands(void) {}

char *configfile = "";

/* Device table - array of device drivers */
extern Dev consdevtab;
extern Dev rootdevtab;
extern Dev archdevtab;
extern Dev mntdevtab;
extern Dev procdevtab;
extern Dev sdisabidevtab;
extern Dev exchdevtab;
extern Dev memdevtab;
extern Dev irqdevtab;
extern Dev dmadevtab;
extern Dev pcidevtab;

Dev *devtab[] = {
	&rootdevtab,
	&archdevtab,
	&consdevtab,
	&mntdevtab,
	&procdevtab,
	&sdisabidevtab,
	&exchdevtab,
	&memdevtab,
	&irqdevtab,
	&dmadevtab,
	&pcidevtab,
	nil,
};

/* Additional stubs for console/device support */
char* getconf(char *name) { (void)name; return nil; }
int cpuserver = 0;

#define SEP(x)   ((x)=='/' || (x) == 0)

char*
cleanname(char *name)
{
	char *p, *q, *dotdot;
	int rooted, erasedprefix;

	rooted = name[0] == '/';
	erasedprefix = 0;

	/*
	 * invariants:
	 *	p points at beginning of path element we're considering.
	 *	q points just past the last path element we wrote (no slash).
	 *	dotdot points just past the point where .. cannot backtrack
	 *		any further (no slash).
	 */
	p = q = dotdot = name+rooted;
	while(*p) {
		if(p[0] == '/')	/* null element */
			p++;
		else if(p[0] == '.' && SEP(p[1])) {
			if(p == name)
				erasedprefix = 1;
			p += 1; /* don't count the separator in case it is nul */
		} else if(p[0] == '.' && p[1] == '.' && SEP(p[2])) {
			p += 2;
			if(q > dotdot) {	/* can backtrack */
				while(--q > dotdot && *q != '/')
					;
			} else if(!rooted) {	/* /.. is / but ./../ is .. */
				if(q != name)
					*q++ = '/';
				*q++ = '.';
				*q++ = '.';
				dotdot = q;
			}
			if(q == name)
				erasedprefix = 1; /* erased entire path via dotdot */
		} else {	/* real path element */
			if(q != name+rooted)
				*q++ = '/';
			while((*q = *p) != '/' && *q != 0)
				p++, q++;
		}
	}
	if(q == name) /* empty string is really ``.'' */
		*q++ = '.';
	*q = '\0';
	if(erasedprefix && name[0] == '#'){
		/* this was not a #x device path originally - make it not one now */
		memmove(name+2, name, strlen(name)+1);
		name[0] = '.';
		name[1] = '/';
	}
	return name;
}

/* cycles() - read CPU timestamp counter for timing */
void
cycles(uvlong *t)
{
	*t = rdtsc();
}

void kerndate(long secs) { (void)secs; }

void setupwatchpts(Proc *p, Watchpt *wp, int nwp) { (void)p; (void)wp; (void)nwp; }
extern char end[];  /* End of kernel - defined by linker */

/* Memory address conversion functions provided by mmu.c */

/* Swap system stubs */
Image *swapimage = nil;  /* Global variable, not function */
void putswap(Page *p) { (void)p; }
int swapcount(uintptr pa) { (void)pa; return 0; }
void
kickpager(void)
{
	wakeup(&swapalloc.r);
}

/* Random number - must match portlib.h signature */
int nrand(int n) {
	/* Simple LCG */
	static ulong seed = 1;
	seed = seed * 1103515245 + 12345;
	return (int)((seed / 65536) % (ulong)n);
}

/* Error strings */
char Ecmdargs[] = "invalid command arguments";

/* Platform macro SET */
void SET(void *x) { (void)x; }

/* qsort implementation */
static int (*qsort_cmp)(void*, void*);

static void qsort_swap(char *a, char *b, ulong n) {
	char t;
	while(n--) {
		t = *a;
		*a++ = *b;
		*b++ = t;
	}
}

static void qsort_r(char *a, ulong n, ulong es) {
	char *i, *j;
	if(n < 2)
		return;
	qsort_swap(a, a + n/2 * es, es);
	i = a;
	j = a + n*es;
	for(;;) {
		do i += es; while(i < j && qsort_cmp(i, a) < 0);
		do j -= es; while(j > a && qsort_cmp(j, a) > 0);
		if(i >= j)
			break;
		qsort_swap(i, j, es);
	}
	qsort_swap(a, j, es);
	qsort_r(a, (j-a)/es, es);
	qsort_r(j+es, n - (j-a)/es - 1, es);
}

void qsort(void *va, ulong n, ulong es, int (*cmp)(void*, void*)) {
	qsort_cmp = cmp;
	qsort_r(va, n, es);
}

void dumpmcregs(void) {}

/* Architecture globals */
extern int cpuserver;
char *conffile = "";

/* Memory pool reset */
/* Configuration write */
void writeconf(void) {}

/* VMX (virtualization) stubs */
void vmxshutdown(void) {}
void vmxprocrestore(Proc *p) { (void)p; }

/* RAM page allocation - uses xallocz which returns HHDM virtual addresses */
void* rampage(void) {
	return xallocz(BY2PG, 1);  /* 1 = zero the memory */
}

/* CPU identification */
int cpuidentify(void) {
	ulong regs[4];

	/* Debug: entering function */

	if(m == nil)
		return 0;

	/* Debug: about to call cpuid */

	cpuid(0, 0, regs);

	/* Debug output after cpuid */

	m->cpuidax = regs[0];

	/* Debug output after cpuidax write */

	/* Build vendor string - cpuid returns it in EBX, EDX, ECX order */
	m->cpuidid[0] = (regs[1] >> 0) & 0xFF;
	m->cpuidid[1] = (regs[1] >> 8) & 0xFF;
	m->cpuidid[2] = (regs[1] >> 16) & 0xFF;
	m->cpuidid[3] = (regs[1] >> 24) & 0xFF;
	m->cpuidid[4] = (regs[3] >> 0) & 0xFF;
	m->cpuidid[5] = (regs[3] >> 8) & 0xFF;
	m->cpuidid[6] = (regs[3] >> 16) & 0xFF;
	m->cpuidid[7] = (regs[3] >> 24) & 0xFF;
	m->cpuidid[8] = (regs[2] >> 0) & 0xFF;
	m->cpuidid[9] = (regs[2] >> 8) & 0xFF;
	m->cpuidid[10] = (regs[2] >> 16) & 0xFF;
	m->cpuidid[11] = (regs[2] >> 24) & 0xFF;
	m->cpuidid[12] = 0;

	cpuid(1, 0, regs);
	m->cpuidax = regs[0];
	m->cpuidcx = regs[2];
	m->cpuiddx = regs[3];

	m->cpuidfamily = (regs[0] >> 8) & 0xF;
	if(m->cpuidfamily == 15){
		m->cpuidfamily += (regs[0] >> 20) & 0xFF;
	}
	m->cpuidmodel = (regs[0] >> 4) & 0xF;
	if(m->cpuidfamily >= 6)
		m->cpuidmodel |= (regs[0] >> 12) & 0xF0;
	m->cpuidstepping = regs[0] & 0xF;

	/* Check vendor string byte by byte */
	if(m->cpuidid[0] == 'A' && m->cpuidid[1] == 'u' && m->cpuidid[2] == 't' &&
	   m->cpuidid[3] == 'h' && m->cpuidid[4] == 'e' && m->cpuidid[5] == 'n' &&
	   m->cpuidid[6] == 't' && m->cpuidid[7] == 'i' && m->cpuidid[8] == 'c' &&
	   m->cpuidid[9] == 'A' && m->cpuidid[10] == 'M' && m->cpuidid[11] == 'D')
		m->cpuidtype = "amd64";
	else if(m->cpuidid[0] == 'G' && m->cpuidid[1] == 'e' && m->cpuidid[2] == 'n' &&
	        m->cpuidid[3] == 'u' && m->cpuidid[4] == 'i' && m->cpuidid[5] == 'n' &&
	        m->cpuidid[6] == 'e' && m->cpuidid[7] == 'I' && m->cpuidid[8] == 'n' &&
	        m->cpuidid[9] == 't' && m->cpuidid[10] == 'e' && m->cpuidid[11] == 'l')
		m->cpuidtype = "intel";
	else
		m->cpuidtype = "unknown";

	return 1;
}
void cpuidprint(void) {}

/* Clock synchronization */
void syncclock(void) {}

/* NVRAM access */
uchar nvramread(int addr) { (void)addr; return 0; }
void nvramwrite(int addr, uchar val) { (void)addr; (void)val; }

void i8042reset(void) {}
void nmienable(void) {}

/* DMA controller - function pointer (nil = not available) */
void (*i8237alloc)(void) = nil;

/* Boot screen */
void bootscreeninit(void) {}

/* Links function - defined by bootlinks */
void links(void) {}

/* Environment */
void ksetenv(char *name, char *val, int conf) { (void)name; (void)val; (void)conf; }
void setconfenv(void) {}

/* Memory initialization - meminit stub, meminit0 in boot.c */
void meminit(void) {}

/* Ramdisk */
void ramdiskinit(void) {}

/* Coherence function pointer - implementation in l.S */
extern void coherence_impl(void);
void (*coherence)(void) = coherence_impl;

/* Additional global function pointers and buffers */
int (*_pcmspecial)(char*, ISAConf*) = nil;
void (*_pcmspecialclose)(int) = nil;
void (*fprestore)(FPsave*) = nil;
void (*fpsave)(FPsave*) = nil;

/* cmpswap - compare and swap function pointer */
extern int cmpswap486(long*, long, long);
int (*cmpswap)(long*, long, long) = cmpswap486;

/* Architecture reset */
/* String functions */
char* strrchr(char *s, int c) {
	char *last = nil;
	while(*s) {
		if(*s == c) last = s;
		s++;
	}
	if(c == '\0') return s;
	return last;
}

/* Error strings */
char Edirseek[] = "directory seek";
char Eismtpt[] = "is a mount point";
char Enegoff[] = "negative offset";

/* Environment */
void envcpy(Egrp *to, Egrp *from) { (void)to; (void)from; }
void closeegrp(Egrp *eg) { (void)eg; }
Egrp* newegrp(void) { return nil; }

/* Swap */
void dupswap(Page *p) { (void)p; }

/* Signal search */
void* sigsearch(char *name, int len) { (void)name; (void)len; return nil; }

/* Timer */
void timerset(Tval tv) { (void)tv; }

/* System call table - global array of syscall name strings */
int nosyscall(Sargs *args) { (void)args; return -1; }
char *sysctab[] = { nil };
void sysexit(Sargs *args, uintptr *ret) { (void)args; (void)ret; }

/* DTrace */
void dtracytick(Ureg *u) { (void)u; }

/* Multiprocessor tables */
void mpinit(void) {}
void mpshutdown(void) {}
void mpintrinit(int irq) { (void)irq; }
void mpintrassign(void) {}
int mpisabus = -1;
int mpeisabus = -1;
int mpbuslast = 0;
void *mpbus = nil;
void *mpapic = nil;
void *mpioapic = nil;

/* UART console - global pointer */
Uart *consuart = nil;

int uartgetc(void)
{
	if(consuart == nil || consuart->phys == nil || consuart->phys->getc == nil)
		return -1;
	return consuart->phys->getc(consuart);
}

void uartputc(int c)
{
	if(consuart == nil || consuart->phys == nil || consuart->phys->putc == nil)
		return;
	consuart->phys->putc(consuart, c);
}

/* Checksum */
int checksum(void *addr, int len) {
	uchar *p = addr;
	int sum = 0;
	while(len-- > 0)
		sum += *p++;
	return sum & 0xFF;
}

/* Delay loop */
void delayloop(int ms) { (void)ms; }

/* MTRR */
void mtrrclock(void) {}

/* Memory reserve */
void memreserve(uintptr pa, uintptr len) { (void)pa; (void)len; }

/* Format functions */
/* Crypto */
void sha2_512(uchar *data, ulong len, uchar *digest) { (void)data; (void)len; (void)digest; }
void setupChachastate(void *state, uchar *key, ulong keylen, uchar *iv, int ivlen) {
	(void)state; (void)key; (void)keylen; (void)iv; (void)ivlen;
}
void chacha_encrypt(uchar *data, ulong len, void *state) { (void)data; (void)len; (void)state; }

/* Math */
void mul64fract(uvlong *result, uvlong a, uvlong b) {
	*result = (a * b) >> 32;
}

/* UTF-8 */
char* utfecpy(char *to, char *e, char *from) {
	if(to >= e) return to;
	while(*from && to < e-1)
		*to++ = *from++;
	*to = '\0';
	return to;
}

/* UPA (user programmable arrays) - not implemented */
uvlong upaalloc(uvlong, uvlong, uvlong) {
	return 0;
}

uvlong upaallocwin(uvlong, uvlong, uvlong, uvlong) {
	return 0;
}
