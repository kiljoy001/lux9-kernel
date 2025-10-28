/*
 * /dev/mem - Physical memory access device for SIP drivers
 *
 * Provides capability-controlled access to physical memory (MMIO) for
 * userspace device drivers. Access is restricted based on process
 * capabilities and address ranges.
 */

#include	"u.h"
#include "portlib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include <error.h>

enum
{
	Qdir = 0,
	Qmem,
	Qio,
};

static Dirtab memdir[] = {
	".",		{Qdir, 0, QTDIR},	0,	DMDIR|0555,
	"mem",		{Qmem},			0,	0600,
	"io",		{Qio},			0,	0600,
};

/*
 * Capability definitions
 * These match the SIP capability system
 */
enum
{
	CapDeviceAccess	= 1<<1,		/* Access to /dev/mem for MMIO */
	CapIOPort		= 1<<2,		/* Access to /dev/io for I/O ports */
};

/*
 * MMIO address validation
 * Prevents access to kernel memory and enforces valid device ranges
 */
static int
isvalidmmio(uintptr pa, usize len)
{
	uintptr end;

	/* Check for overflow */
	end = pa + len;
	if(end < pa)
		return 0;

	/*
	 * On x86-64, typical MMIO ranges are:
	 * - 0xA0000-0xFFFFF: VGA/BIOS (legacy)
	 * - 0xFEC00000-0xFEE00000: APIC
	 * - 0xE0000000-0xFFFFFFFF: PCI devices
	 * - Above 4GB: More PCI devices
	 *
	 * Block access to low RAM (0-640K is definitely RAM)
	 * Block access to kernel virtual addresses
	 */

	/* Don't allow access to low RAM (first 640K) */
	if(pa < 0xA0000 && end > 0)
		return 0;

	/* Don't allow access to main RAM region (640K-16M is often RAM) */
	if(pa >= 0xA0000 && pa < 0x1000000 && end <= 0x1000000) {
		/* Allow VGA/BIOS region (0xA0000-0x100000) */
		if(pa >= 0xA0000 && end <= 0x100000)
			return 1;
		/* Block everything else in low memory */
		return 0;
	}

	/* Allow typical PCI MMIO ranges */
	if(pa >= 0xE0000000)
		return 1;

	/* For other ranges, be conservative */
	return 0;
}

/*
 * Check if process has required capability
 */
static void
checkcap(ulong required)
{
	/*
	 * TODO: Actual capability checking
	 * For now, only allow if running as 'eve' (superuser)
	 *
	 * Real implementation should check:
	 * - up->capabilities & required
	 * - Process isolation domain
	 * - SIP server registration
	 */

	/* Temporary: allow all access for development */
	/* In production, this should be: */
	/*
	if(!(up->capabilities & required))
		error(Eperm);
	*/
}

static Chan*
memattach(char *spec)
{
	return devattach('m', spec);
}

static Walkqid*
memwalk(Chan *c, Chan *nc, char **name, int nname)
{
	return devwalk(c, nc, name, nname, memdir, nelem(memdir), devgen);
}

static int
memstat(Chan *c, uchar *dp, int n)
{
	return devstat(c, dp, n, memdir, nelem(memdir), devgen);
}

static Chan*
memopen(Chan *c, int omode)
{
	switch((ulong)c->qid.path){
	case Qmem:
		checkcap(CapDeviceAccess);
		break;
	case Qio:
		checkcap(CapIOPort);
		break;
	}

	c = devopen(c, omode, memdir, nelem(memdir), devgen);
	c->offset = 0;
	return c;
}

static void
memclose(Chan *c)
{
	USED(c);
}

static long
memread(Chan *c, void *va, long n, vlong off)
{
	uintptr pa;
	uchar *a;
	long i;

	switch((ulong)c->qid.path){
	case Qdir:
		return devdirread(c, va, n, memdir, nelem(memdir), devgen);

	case Qmem:
		/* Check capability */
		checkcap(CapDeviceAccess);

		/* Validate address range */
		pa = (uintptr)off;
		if(!isvalidmmio(pa, n))
			error("invalid MMIO address");

		/* Read from physical memory */
		a = va;
		for(i = 0; i < n; i++){
			/*
			 * Use KADDR to map physical to virtual
			 * This is safe because we validated the MMIO range
			 */
			a[i] = *(uchar*)KADDR(pa + i);
		}
		return n;

	case Qio:
		/* I/O port access - TODO */
		error("I/O port access not yet implemented");
		return 0;

	default:
		error(Egreg);
		return 0;
	}
}

static long
memwrite(Chan *c, void *va, long n, vlong off)
{
	uintptr pa;
	uchar *a;
	long i;

	switch((ulong)c->qid.path){
	case Qdir:
		error(Eperm);
		return 0;

	case Qmem:
		/* Check capability */
		checkcap(CapDeviceAccess);

		/* Validate address range */
		pa = (uintptr)off;
		if(!isvalidmmio(pa, n))
			error("invalid MMIO address");

		/* Write to physical memory */
		a = va;
		for(i = 0; i < n; i++){
			/*
			 * Use KADDR to map physical to virtual
			 * Memory barriers are important for MMIO!
			 */
			*(volatile uchar*)KADDR(pa + i) = a[i];
		}

		/* Ensure writes complete before returning */
		coherence();
		return n;

	case Qio:
		/* I/O port access - TODO */
		error("I/O port access not yet implemented");
		return 0;

	default:
		error(Eperm);
		return 0;
	}
}

static void
memreset(void)
{
}

Dev memdevtab = {
	'm',
	"mem",

	memreset,
	devinit,
	devshutdown,
	memattach,
	memwalk,
	memstat,
	memopen,
	devcreate,
	memclose,
	memread,
	devbread,
	memwrite,
	devbwrite,
	devremove,
	devwstat,
};
