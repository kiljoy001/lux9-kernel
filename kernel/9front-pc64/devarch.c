#include "u.h"
#include "portlib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "io.h"
#include "ureg.h"
#include <error.h>

enum {
	Qdir = 0,
	Qiob,
	Qiow,
	Qiol,
	Qmsr,
	Qbase,

	Qmax = 32,
};

typedef long Rdwrfn(Chan*, void*, long, vlong);

static Rdwrfn *readfn[Qmax];
static Rdwrfn *writefn[Qmax];

static Dirtab archdir[Qmax] = {
	".",		{ Qdir, 0, QTDIR },	0,	0555,
	"iob",		{ Qiob, 0 },		0,	0660,
	"iow",		{ Qiow, 0 },		0,	0660,
	"iol",		{ Qiol, 0 },		0,	0660,
	"msr",		{ Qmsr, 0 },		0,	0660,
};

Lock archwlock;
int narchdir = Qbase;

extern int (*_pcmspecial)(char*, ISAConf*);
extern void (*_pcmspecialclose)(int);

static void checkport(ulong, ulong);
static long rmemread(Chan*, void*, long, vlong);
static long rmemwrite(Chan*, void*, long, vlong);
static long cputyperead(Chan*, void*, long, vlong);

Dirtab*
addarchfile(char *name, int perm, Rdwrfn *rdfn, Rdwrfn *wrfn)
{
	int i;
	Dirtab d;
	Dirtab *dp;

	memset(&d, 0, sizeof d);
	strncpy(d.name, name, sizeof d.name);
	d.name[sizeof d.name - 1] = '\0';
	d.perm = perm;

	lock(&archwlock);
	if(narchdir >= Qmax){
		unlock(&archwlock);
		print("addarchfile: out of entries for %s\n", name);
		return nil;
	}

	for(i=0; i<narchdir; i++)
		if(strcmp(archdir[i].name, name) == 0){
			unlock(&archwlock);
			return nil;
		}

	d.qid.path = narchdir;
	archdir[narchdir] = d;
	readfn[narchdir] = rdfn;
	writefn[narchdir] = wrfn;
	dp = &archdir[narchdir++];
	unlock(&archwlock);

	return dp;
}

void
ioinit(void)
{
	char *excluded;

	iomapinit(0xffff);

	/*
	 * This is necessary to make the IBM X20 boot.
	 * Have not tracked down the reason.
	 * i82557 is at 0x1000, the dummy entry is needed for swappable devs.
	 */
	ioalloc(0x0fff, 1, 0, "dummy");

	if((excluded = getconf("ioexclude")) != nil) {
		char *s;

		s = excluded;
		while (s && *s != '\0' && *s != '\n') {
			char *ends;
			int io_s, io_e;

			io_s = (int)strtol(s, &ends, 0);
			if (ends == nil || ends == s || *ends != '-') {
				print("ioinit: cannot parse option string\n");
				break;
			}
			s = ++ends;

			io_e = (int)strtol(s, &ends, 0);
			if (ends && *ends == ',')
				*ends++ = '\0';
			s = ends;

			ioalloc(io_s, io_e - io_s + 1, 0, "pre-allocated");
		}
	}
}

static void
checkport(ulong start, ulong end)
{
	if(end < start || end > 0x10000)
		error(Ebadarg);

	/* standard vga regs are OK */
	if(start >= 0x2b0 && end <= 0x2df+1)
		return;
	if(start >= 0x3c0 && end <= 0x3da+1)
		return;

	if(iounused(start, end))
		return;
	error(Eperm);
}

static Chan*
archattach(char* spec)
{
	return devattach('P', spec);
}

Walkqid*
archwalk(Chan* c, Chan *nc, char** name, int nname)
{
	return devwalk(c, nc, name, nname, archdir, narchdir, devgen);
}

static int
archstat(Chan* c, uchar* dp, int n)
{
	return devstat(c, dp, n, archdir, narchdir, devgen);
}

static Chan*
archopen(Chan* c, int omode)
{
	return devopen(c, omode, archdir, narchdir, devgen);
}

static void
archclose(Chan*)
{
}

static long
archread(Chan *c, void *a, long n, vlong offset)
{
	ulong port, end;
	uchar *cp;
	ushort *sp;
	ulong *lp;
	vlong *vp;
	Rdwrfn *fn;

	port = offset;
	end = port+n;
	switch((ulong)c->qid.path){
	case Qdir:
		return devdirread(c, a, n, archdir, narchdir, devgen);

	case Qiob:
		checkport(port, end);
		for(cp = a; port < end; port++)
			*cp++ = inb(port);
		return n;

	case Qiow:
		if(n & 1)
			error(Ebadarg);
		checkport(port, end);
		for(sp = a; port < end; port += 2)
			*sp++ = ins(port);
		return n;

	case Qiol:
		if(n & 3)
			error(Ebadarg);
		checkport(port, end);
		for(lp = a; port < end; port += 4)
			*lp++ = inl(port);
		return n;

	case Qmsr:
		if(n & 7)
			error(Ebadarg);
		if((ulong)n/8 > -port)
			error(Ebadarg);
		end = port+(n/8);
		for(vp = a; port != end; port++)
			if(rdmsr(port, vp++) < 0)
				error(Ebadarg);
		return n;

	default:
		if(c->qid.path < narchdir && (fn = readfn[c->qid.path]) != nil)
			return fn(c, a, n, offset);
		error(Eperm);
	}
}

static long
archwrite(Chan *c, void *a, long n, vlong offset)
{
	ulong port, end;
	uchar *cp;
	ushort *sp;
	ulong *lp;
	vlong *vp;
	Rdwrfn *fn;

	port = offset;
	end = port+n;
	switch((ulong)c->qid.path){
	case Qiob:
		checkport(port, end);
		for(cp = a; port < end; port++)
			outb(port, *cp++);
		return n;

	case Qiow:
		if(n & 1)
			error(Ebadarg);
		checkport(port, end);
		for(sp = a; port < end; port += 2)
			outs(port, *sp++);
		return n;

	case Qiol:
		if(n & 3)
			error(Ebadarg);
		checkport(port, end);
		for(lp = a; port < end; port += 4)
			outl(port, *lp++);
		return n;

	case Qmsr:
		if(n & 7)
			error(Ebadarg);
		if((ulong)n/8 > -port)
			error(Ebadarg);
		end = port+(n/8);
		for(vp = a; port != end; port++)
			if(wrmsr(port, *vp++) < 0)
				error(Ebadarg);
		return n;

	default:
		if(c->qid.path < narchdir && (fn = writefn[c->qid.path]) != nil)
			return fn(c, a, n, offset);
		error(Eperm);
	}
}

Dev archdevtab = {
	'P',
	"arch",

	devreset,
	devinit,
	devshutdown,
	archattach,
	archwalk,
	archstat,
	archopen,
	devcreate,
	archclose,
	archread,
	devbread,
	archwrite,
	devbwrite,
	devremove,
	devwstat,
};

static long
rmemrw(int isr, void *a, long n, vlong off)
{
	uintptr addr = off;

	if(off < 0 || n < 0)
		error("bad offset/count");
	if(isr){
		if(addr >= MB)
			return 0;
		if(addr+n > MB)
			n = MB - addr;
		memmove(a, KADDR(addr), n);
	}else{
		/* allow vga framebuf's write access */
		if(addr >= MB || addr+n > MB ||
		    (addr < 0xA0000 || addr+n > 0xB0000+0x10000))
			error("bad offset/count in write");
		memmove(KADDR(addr), a, n);
	}
	return n;
}

static long
rmemread(Chan*, void *a, long n, vlong off)
{
	return rmemrw(1, a, n, off);
}

static long
rmemwrite(Chan*, void *a, long n, vlong off)
{
	return rmemrw(0, a, n, off);
}

extern int cpuserver;
extern PCArch archgeneric;
extern PCArch archmp;

PCArch *arch = &archgeneric;
static PCArch* knownarch[] = {
	&archmp,
	&archgeneric,
	nil,
};

extern int (*cmpswap)(long*, long, long);
extern int cmpswap486(long*, long, long);
extern void (*coherence)(void);
extern void mb386(void);
extern void mb586(void);
extern void mfence(void);

static long
cputyperead(Chan*, void *a, long n, vlong off)
{
	char buf[128];
	char *p, *ep;

	p = buf;
	ep = buf + sizeof buf;

	p = seprint(p, ep, "arch %s\n", arch->id);
	p = seprint(p, ep, "cpuid %s %s\n", m->cpuidid, m->cpuidtype);
	p = seprint(p, ep, "family %d\n", m->cpuidfamily);
	p = seprint(p, ep, "model %d\n", m->cpuidmodel);
	p = seprint(p, ep, "stepping %d\n", m->cpuidstepping);

	return readstr(off, a, n, buf);
}

void
archinit(void)
{
	PCArch **p;

	arch = knownarch[0];
	for(p = knownarch; *p != nil; p++){
		if((*p)->ident != nil && (*p)->ident() == 0){
			arch = *p;
			break;
		}
	}
	if(arch != knownarch[0]){
		if(arch->id == nil)
			arch->id = knownarch[0]->id;
		if(arch->reset == nil)
			arch->reset = knownarch[0]->reset;
		if(arch->intrinit == nil)
			arch->intrinit = knownarch[0]->intrinit;
		if(arch->intrassign == nil)
			arch->intrassign = knownarch[0]->intrassign;
		if(arch->clockinit == nil)
			arch->clockinit = knownarch[0]->clockinit;
		if(arch->timerset == nil)
			arch->timerset = knownarch[0]->timerset;
		if(arch->fastclock == nil)
			arch->fastclock = knownarch[0]->fastclock;
	}

	if(m->cpuidfamily == 3)
		conf.copymode = 1;

	if(m->cpuidfamily >= 4)
		cmpswap = cmpswap486;

	if(m->cpuidfamily >= 5)
		coherence = mb586;

	if(m->cpuiddx & Sse2)
		coherence = mfence;

	addarchfile("cputype", 0444, cputyperead, nil);
	addarchfile("realmodemem", 0660, rmemread, rmemwrite);
}
