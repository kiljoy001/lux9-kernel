/*
 * /dev/dma - DMA buffer allocation device for SIP drivers
 *
 * Provides capability-controlled allocation of physically contiguous,
 * DMA-capable memory for userspace device drivers.
 *
 * Filesystem:
 *   /dev/dma/
 *   ├── ctl        - Status and control
 *   └── alloc      - Allocate DMA buffer (write size, read addresses)
 *
 * Usage:
 *   // Allocate 4KB DMA buffer
 *   fd = open("/dev/dma/alloc", ORDWR);
 *   fprint(fd, "size 4096 align 4096");
 *
 *   // Read back addresses
 *   read(fd, buf, sizeof(buf));
 *   // Returns: "vaddr:0x... paddr:0x... size:4096"
 *
 * Architecture Integration:
 *   - Uses xalloc() for physically contiguous allocation
 *   - Tracks allocations per process
 *   - Auto-frees on process exit
 *   - Returns both virtual and physical addresses
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
	Qctl,
	Qalloc,

	MaxAllocs = 256,		/* Max DMA allocations per process */
	MaxDMASize = 16*MiB,		/* Max single allocation */
};

/*
 * DMA allocation tracking
 */
typedef struct DMAAlloc DMAAlloc;
struct DMAAlloc
{
	uintptr	vaddr;		/* Virtual address (HHDM) */
	uintptr	paddr;		/* Physical address */
	usize	size;		/* Allocation size */
	Proc	*owner;		/* Owning process */
	DMAAlloc *next;		/* Free list / allocation list */
};

static struct {
	Lock	lock;
	DMAAlloc *freelist;	/* Free DMAAlloc structures */
	DMAAlloc *alloclist;	/* Active allocations */
	uint	nallocs;	/* Total active allocations */
	uint	nfree;		/* Free structures available */
	usize	totalbytes;	/* Total bytes allocated */
} dmapool;

static Dirtab dmadir[] = {
	".",		{Qdir, 0, QTDIR},	0,	DMDIR|0555,
	"ctl",		{Qctl},			0,	0444,
	"alloc",	{Qalloc},		0,	0600,
};

/*
 * Capability definitions
 */
enum
{
	CapDMA = 1<<5,		/* DMA buffer allocation */
};

static void
checkcap(ulong required)
{
	/*
	 * TODO: Check up->capabilities & required
	 * For now, allow all access for development
	 */
	USED(required);
}

/*
 * Allocate a DMAAlloc structure
 */
static DMAAlloc*
alloc_dmaalloc(void)
{
	DMAAlloc *da;

	lock(&dmapool.lock);
	if(dmapool.freelist != nil) {
		da = dmapool.freelist;
		dmapool.freelist = da->next;
		dmapool.nfree--;
	} else {
		unlock(&dmapool.lock);
		da = smalloc(sizeof(DMAAlloc));
		lock(&dmapool.lock);
	}
	unlock(&dmapool.lock);

	memset(da, 0, sizeof(DMAAlloc));
	return da;
}

/*
 * Free a DMAAlloc structure
 */
static void
free_dmaalloc(DMAAlloc *da)
{
	lock(&dmapool.lock);
	da->next = dmapool.freelist;
	dmapool.freelist = da;
	dmapool.nfree++;
	unlock(&dmapool.lock);
}

/*
 * Track a DMA allocation
 */
static void
track_alloc(DMAAlloc *da)
{
	lock(&dmapool.lock);
	da->next = dmapool.alloclist;
	dmapool.alloclist = da;
	dmapool.nallocs++;
	dmapool.totalbytes += da->size;
	unlock(&dmapool.lock);
}

/*
 * Untrack and free a DMA allocation
 */
static void
untrack_alloc(DMAAlloc *da)
{
	DMAAlloc **pp;

	lock(&dmapool.lock);
	for(pp = &dmapool.alloclist; *pp != nil; pp = &(*pp)->next) {
		if(*pp == da) {
			*pp = da->next;
			dmapool.nallocs--;
			dmapool.totalbytes -= da->size;
			unlock(&dmapool.lock);

			/* Free the actual memory */
			xfree((void*)da->vaddr);

			/* Free the tracking structure */
			free_dmaalloc(da);
			return;
		}
	}
	unlock(&dmapool.lock);
}

/*
 * Free all DMA allocations for a process
 * Called on process exit
 */
void
dma_free_proc(Proc *p)
{
	DMAAlloc *da, *next;
	DMAAlloc **pp;

	lock(&dmapool.lock);
	pp = &dmapool.alloclist;
	while(*pp != nil) {
		da = *pp;
		if(da->owner == p) {
			/* Remove from list */
			*pp = da->next;
			dmapool.nallocs--;
			dmapool.totalbytes -= da->size;

			unlock(&dmapool.lock);

			/* Free memory */
			xfree((void*)da->vaddr);
			free_dmaalloc(da);

			lock(&dmapool.lock);
		} else {
			pp = &(*pp)->next;
		}
	}
	unlock(&dmapool.lock);
}

static void
dmareset(void)
{
	memset(&dmapool, 0, sizeof(dmapool));
}

static Chan*
dmaattach(char *spec)
{
	return devattach('D', spec);
}

static Walkqid*
dmawalk(Chan *c, Chan *nc, char **name, int nname)
{
	return devwalk(c, nc, name, nname, dmadir, nelem(dmadir), devgen);
}

static int
dmastat(Chan *c, uchar *dp, int n)
{
	return devstat(c, dp, n, dmadir, nelem(dmadir), devgen);
}

static Chan*
dmaopen(Chan *c, int omode)
{
	checkcap(CapDMA);

	c = devopen(c, omode, dmadir, nelem(dmadir), devgen);
	c->offset = 0;
	return c;
}

static void
dmaclose(Chan *c)
{
	USED(c);
}

static long
dmaread(Chan *c, void *va, long n, vlong off)
{
	char buf[256];
	int len;
	DMAAlloc *da;
	int count;

	switch((int)c->qid.path){
	case Qdir:
		return devdirread(c, va, n, dmadir, nelem(dmadir), devgen);

	case Qctl:
		/* Return DMA pool status */
		len = 0;
		lock(&dmapool.lock);
		len += snprint(buf + len, sizeof(buf) - len,
			"allocations: %ud\n", dmapool.nallocs);
		len += snprint(buf + len, sizeof(buf) - len,
			"total bytes: %lud\n", dmapool.totalbytes);
		len += snprint(buf + len, sizeof(buf) - len,
			"free structs: %ud\n", dmapool.nfree);

		/* Count allocations for this process */
		count = 0;
		for(da = dmapool.alloclist; da != nil; da = da->next) {
			if(da->owner == up)
				count++;
		}
		len += snprint(buf + len, sizeof(buf) - len,
			"process allocations: %d\n", count);
		unlock(&dmapool.lock);

		if(off >= len)
			return 0;
		if(off + n > len)
			n = len - off;
		memmove(va, buf + off, n);
		return n;

	case Qalloc:
		/* Read returns allocated addresses */
		da = c->aux;
		if(da == nil)
			error("no allocation");

		len = snprint(buf, sizeof(buf),
			"vaddr:0x%p paddr:0x%p size:%lud\n",
			da->vaddr, da->paddr, da->size);

		if(off >= len)
			return 0;
		if(off + n > len)
			n = len - off;
		memmove(va, buf + off, n);
		return n;

	default:
		error(Egreg);
		return 0;
	}
}

static long
dmawrite(Chan *c, void *va, long n, vlong off)
{
	char *buf, *fields[8];
	int nf;
	usize size, align;
	void *mem;
	DMAAlloc *da;

	USED(off);

	switch((int)c->qid.path){
	case Qdir:
	case Qctl:
		error(Eperm);
		return 0;

	case Qalloc:
		/* Parse allocation request: size <bytes> align <bytes> */
		buf = smalloc(n + 1);
		memmove(buf, va, n);
		buf[n] = '\0';

		nf = tokenize(buf, fields, nelem(fields));
		if(nf < 2) {
			free(buf);
			error("usage: size <bytes> [align <bytes>]");
		}

		/* Parse size */
		if(strcmp(fields[0], "size") != 0) {
			free(buf);
			error("expected: size <bytes>");
		}
		size = strtoul(fields[1], nil, 0);

		/* Parse optional alignment */
		align = BY2PG;	/* Default: page aligned */
		if(nf >= 4 && strcmp(fields[2], "align") == 0) {
			align = strtoul(fields[3], nil, 0);
		}

		free(buf);

		/* Validate parameters */
		if(size == 0 || size > MaxDMASize)
			error("invalid size");
		if(align == 0 || (align & (align-1)) != 0)
			error("alignment must be power of 2");
		if(align < BY2PG)
			align = BY2PG;

		/* Check process allocation limit */
		lock(&dmapool.lock);
		int count = 0;
		for(da = dmapool.alloclist; da != nil; da = da->next) {
			if(da->owner == up)
				count++;
		}
		unlock(&dmapool.lock);

		if(count >= MaxAllocs)
			error("too many DMA allocations");

		/* Allocate physically contiguous memory */
		mem = xspanalloc(size, (int)align, 0);
		if(mem == nil)
			error("DMA allocation failed");

		/* Create tracking structure */
		da = alloc_dmaalloc();
		da->vaddr = (uintptr)mem;
		da->paddr = PADDR(mem);
		da->size = size;
		da->owner = up;

		/* Track it */
		track_alloc(da);

		/* Store in channel for read */
		c->aux = da;

		print("devdma: allocated %lud bytes at vaddr=0x%p paddr=0x%p (pid %d)\n",
			size, da->vaddr, da->paddr, up->pid);

		return n;

	default:
		error(Eperm);
		return 0;
	}
}

Dev dmadevtab = {
	'D',
	"dma",

	dmareset,
	devinit,
	devshutdown,
	dmaattach,
	dmawalk,
	dmastat,
	dmaopen,
	devcreate,
	dmaclose,
	dmaread,
	devbread,
	dmawrite,
	devbwrite,
	devremove,
	devwstat,
};
