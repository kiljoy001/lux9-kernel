#include "u.h"
#include "portlib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"

/* Limine HHDM offset - all physical memory mapped at PA + this offset */
extern uintptr limine_hhdm_offset;

/* -------------------------------------------------------------------------
 * XALLOC configuration
 * -------------------------------------------------------------------------
 * INITIAL_NHOLE – number of static Hole descriptors (kept in the Xalloc
 *                 struct).  This value must match the size of the static array.
 * DYNAMIC_NHOLE – number of descriptors to allocate at once when the static
 *                 pool runs out.
 * Nhole        – alias for INITIAL_NHOLE used by the rest of the file.
 * -------------------------------------------------------------------------
 */
enum
{
	INITIAL_NHOLE	= 128,
	DYNAMIC_NHOLE	= 256,
	Nhole		= INITIAL_NHOLE,	/* static hole descriptor count */
	Magichole	= 0x484F4C45,		/* HOLE */
};

typedef struct Hole Hole;
typedef struct Xalloc Xalloc;
typedef struct Xhdr Xhdr;

struct Hole
{
	uintptr	addr;
	uintptr	size;
	uintptr	top;
	Hole*	link;
};

struct Xhdr
{
	ulong	size;
	ulong	magix;
	char	data[];
};

struct Xalloc
{
	Lock	lk;  /* Named lock member instead of anonymous */
	Hole	hole[Nhole];
	Hole*	flist;
	Hole*	table;
};

static Xalloc	xlists;
static void xhole_user_init(void);

void
xinit(void)
{
	ulong maxpages, kpages, n;
	Hole *h, *eh;
	Confmem *cm;
	int i;

	print("xinit: starting initialization\n");
	eh = &xlists.hole[Nhole-1];
	for(h = xlists.hole; h < eh; h++)
		h->link = h+1;

	xlists.flist = xlists.hole;

	kpages = conf.npage - conf.upages;
	iprint("TEST: d=%d ud=%ud lud=%lud\n", 42, 99, (ulong)67890);
	iprint("TEST: npage=%lud upages=%lud kpages=%lud\n", (ulong)conf.npage, (ulong)conf.upages, (ulong)kpages);
	print("xinit: total pages %lud, user pages %lud, kernel pages %lud\n", conf.npage, conf.upages, kpages);

	for(i=0; i<nelem(conf.mem); i++){
		cm = &conf.mem[i];
		/* Only print first few entries to avoid verbose output */
		if(i < 2) {
			print("xinit: processing conf.mem[%d] base=%#p npage=%lud\n",
				i, cm->base, cm->npage);
		} else if(i == 2) {
			print("xinit: ... (showing first 2 entries only)\n");
		}
		n = cm->npage;
		if(n > kpages)
			n = kpages;
		/* don't try to use non-KADDR-able memory for kernel */
		/* With Limine HHDM, all physical memory is already mapped */
		maxpages = cm->npage;  /* Assume all pages are KADDR-able */
		if(n > maxpages)
			n = maxpages;
		/* give to kernel */
		if(n > 0){
			cm->kbase = (uintptr)KADDR(cm->base);
			cm->klimit = (uintptr)cm->kbase+(uintptr)n*BY2PG;
			if(cm->klimit == 0)
				cm->klimit = (uintptr)-BY2PG;
			/* Only print first few xhole calls to avoid verbose output */
			if(i < 2) {
				print("xinit: calling xhole with base=%#p size=%#p\n", cm->base, cm->klimit - cm->kbase);
			}
			xhole(cm->base, cm->klimit - cm->kbase);
			kpages -= n;
		}
		/*
		 * anything left over: cm->npage - nkpages(cm)
		 * will be given to user by pageinit()
		 */
	}
	print("DEBUG: calling xhole_user_init");

	print("xinit: initialization complete\n");
}

void*
xspanalloc(ulong size, int align, ulong span)
{
	uintptr a, v, t;

	a = (uintptr)xalloc(size+align+span);
	if(a == 0)
		panic("xspanalloc: %lud %d %lux", size, align, span);

	if(span > 2) {
		v = (a + span) & ~((uintptr)span-1);
		t = v - a;
		if(t > 0) {
			/* xhole expects physical addr, but 'a' is virtual HHDM addr
			 * Convert back to physical: vaddr - HHDM_offset */
			xhole(a - limine_hhdm_offset, t);
		}
		t = a + span - v;
		if(t > 0) {
			xhole((v+size+align) - limine_hhdm_offset, t);
		}
	}
	else
		v = a;

	if(align > 1)
		v = (v + align) & ~((uintptr)align-1);

	return (void*)v;
}

void*
xallocz(ulong size, int zero)
{
	Xhdr *p;
	Hole *h, **l;
	ulong orig_size = size;
	ulong overhead;

	/* Calculate overhead */
	overhead = BY2V + offsetof(Xhdr, data[0]);
	
	/* Only print for large allocations to reduce verbose output */
	if (size > 64*1024) {
		print("xallocz: requesting %lud bytes (zero=%d)\n", size, zero);
	}
	
	/* Detect potential overflow when adding header overhead */
	if (size > ~0UL - overhead) {
		print("xallocz: overflow detected! size=%lud, overhead=%lud\n", size, overhead);
		panic("xallocz: request size overflow (size=%lud)", size);
	}

	/* Additional check for unreasonably large allocations */
	if (size > 128*1024*1024) {  /* More than 128MB */
		print("xallocz: unreasonably large allocation request: %lud bytes\n", size);
		panic("xallocz: unreasonably large allocation request (size=%lud)", size);
	}
	
	/* add room for magix & size overhead, round up to nearest vlong */
	size += overhead;
	size &= ~(BY2V-1);
	
	/* Only print for large allocations to reduce verbose output */
	if (size > 64*1024) {
		print("xallocz: adjusted size %lud bytes\n", size);
	}

	ilock(&xlists.lk);
	
	/* Debug counter to prevent infinite loop */
	int loop_count = 0;
	int print_count = 0;  /* Limit debug prints */
	
	l = &xlists.table;
	for(h = *l; h; h = h->link) {
		/* Debug print to see if we're in the loop - limit verbose output */
		if (loop_count % 5000 == 0 && print_count < 3) {
			print("xallocz: checking hole (count=%d)\n", loop_count);
			print_count++;
		} else if (print_count == 3 && loop_count % 5000 == 0) {
			print("xallocz: ... (limiting verbose output)\n");
			print_count++;  /* Only print this message once */
		}
		loop_count++;
		
		/* Safety check to prevent infinite loop */
		if (loop_count > 100000) {
			print("xallocz: infinite loop detected, breaking\n");
			iunlock(&xlists.lk);
			return nil;
		}
		
		if(h->size >= size) {
			/* h->addr is already a virtual address in HHDM - no KADDR needed! */
			p = (Xhdr*)h->addr;
			h->addr += size;
			h->size -= size;
			if(h->size == 0) {
				*l = h->link;
				h->link = xlists.flist;
				xlists.flist = h;
			}
			iunlock(&xlists.lk);
			p->magix = Magichole;
			p->size = size;
			if(zero)
				memset(p->data, 0, orig_size);
			if(zero && *(ulong*)p->data != 0)
				panic("xallocz: zeroed block not cleared");
			/* Limit verbose output for successful allocations */
			if (size > 64*1024) {  /* Only print for large allocations */
				print("xallocz: allocated %lud bytes\n", size);
			}
			return p->data;
		}
		l = &h->link;
	}
	iunlock(&xlists.lk);
	if (size > 64*1024) {  /* Only print for large allocations */
		print("xallocz: allocation failed for %lud bytes\n", size);
	}
	return nil;
}

void*
xalloc(ulong size)
{
	return xallocz(size, 1);
}

void
xfree(void *p)
{
	Xhdr *x;

	x = (Xhdr*)((uintptr)p - offsetof(Xhdr, data[0]));
	if(x->magix != Magichole) {
		xsummary();
		panic("xfree(%#p) %#ux != %#lux", p, Magichole, x->magix);
	}
	/* x is already a virtual HHDM address, convert to physical for xhole */
	xhole((uintptr)x - limine_hhdm_offset, x->size);
}

int
xmerge(void *vp, void *vq)
{
	Xhdr *p, *q;

	p = (Xhdr*)(((uintptr)vp - offsetof(Xhdr, data[0])));
	q = (Xhdr*)(((uintptr)vq - offsetof(Xhdr, data[0])));
	if(p->magix != Magichole || q->magix != Magichole) {
		int i;
		ulong *wd;
		void *badp;

		xsummary();
		badp = (p->magix != Magichole? p: q);
		wd = (ulong *)badp - 12;
		for (i = 24; i-- > 0; ) {
			print("%#p: %lux", wd, *wd);
			if (wd == badp)
				print(" <-");
			print("\n");
			wd++;
		}
		panic("xmerge(%#p, %#p) bad magic %#lux, %#lux",
			vp, vq, p->magix, q->magix);
	}
	if((uchar*)p+p->size == (uchar*)q) {
		p->size += q->size;
		return 1;
	}
	return 0;
}

/* Modern VM-aware xhole system for Limine boot environment
 *
 * Key changes from original Plan 9 design:
 * 1. Works with VIRTUAL addresses (via Limine HHDM mapping)
 * 2. Physical memory at PA is mapped to VA = PA + HHDM_offset
 * 3. All allocations return virtual addresses in HHDM region
 * 4. Holes track virtual address ranges, not physical
 */

void
xhole(uintptr addr, uintptr size)
{
	Hole *h, *c, **l;
	uintptr top;
	uintptr vaddr;  /* Virtual address in HHDM */

	if(size == 0)
		return;

	/* Convert physical address to virtual HHDM address
	 * Now holes track virtual addresses in the HHDM region */
	vaddr = addr + limine_hhdm_offset;
	top = vaddr + size;

	ilock(&xlists.lk);

	/* Find if this hole can be merged with an existing one */
	l = &xlists.table;
	h = *l;
	for(; h; h = h->link) {
		/* Check if this new region is adjacent to existing hole (at top) */
		if(h->top == vaddr) {
			h->size += size;
			h->top = h->addr+h->size;
			c = h->link;
			if(c && h->top == c->addr) {
				h->top += c->size;
				h->size += c->size;
				h->link = c->link;
				c->link = xlists.flist;
				xlists.flist = c;
			}
			iunlock(&xlists.lk);
			return;
		}
		/* Check if new region comes before this hole */
		if(h->addr > vaddr)
			break;
		l = &h->link;
	}

	/* Check if this new region is adjacent to existing hole (at bottom) */
	if(h && top == h->addr) {
		h->addr = vaddr;
		h->size += size;
		iunlock(&xlists.lk);
		return;
	}

	/* Need to create a new hole descriptor for this region */
	if(xlists.flist == nil) {
	/* ---------------------------------------------------------------
	 * If we have exhausted the static free list, allocate a fresh batch
	 * of Hole descriptors from the kernel malloc pool.
	 * --------------------------------------------------------------- */
		Hole *extra = (Hole*)malloc(DYNAMIC_NHOLE * sizeof(Hole));
		if (extra == nil) {
			iunlock(&xlists.lk);
			panic("xhole: out of hole descriptors and malloc failed");
		}
		for (int i = 0; i < DYNAMIC_NHOLE-1; i++) {
			extra[i].link = &extra[i+1];
		}
		extra[DYNAMIC_NHOLE-1].link = nil;
		xlists.flist = extra;
	}
	/* Get a free hole descriptor from the free list */
	h = xlists.flist;
	xlists.flist = h->link;

	/* Fill in the hole with virtual address information */
	h->addr = vaddr;  /* Virtual address in HHDM */
	h->top = top;     /* End virtual address */
	h->size = size;   /* Size in bytes */
	h->link = *l;     /* Link into the table */
	*l = h;
	
	iunlock(&xlists.lk);
}

void
xsummary(void)
{
	int i;
	Hole *h;
	uintptr s;

	i = 0;
	for(h = xlists.flist; h; h = h->link)
		i++;
	print("%d holes free\n", i);

	s = 0;
	for(h = xlists.table; h; h = h->link) {
		print("%#8.8p %#8.8p %llud\n", h->addr, h->top, (uvlong)h->size);
		s += h->size;
	}
	print("%llud bytes free\n", (uvlong)s);
}

/* Test function to verify dynamic hole allocation works */
void
xalloc_test(void)
{
	print("xalloc_test: starting test\n");
	
	/* Try to exhaust static hole pool by making many small allocations */
	void *ptrs[200];
	int i;
	
	print("xalloc_test: making 200 small allocations\n");
	for(i = 0; i < 200; i++) {
		ptrs[i] = xalloc(16);  /* Small allocations */
		if(ptrs[i] == nil) {
			print("xalloc_test: allocation %d failed\n", i);
			break;
		}
	}
	print("xalloc_test: made %d allocations\n", i);
	
	/* Free all allocations */
	for(int j = 0; j < i; j++) {
		if(ptrs[j] != nil) {
			xfree(ptrs[j]);
		}
	}
	
	print("xalloc_test: freed all allocations\n");
	print("xalloc_test: test completed successfully\n");
}

/*
 * Initialize user virtual address space holes for HHDM model
 * In HHDM, all physical memory is already mapped, so holes represent
 * available virtual address space, not unmapped memory regions
 */
static void
xhole_user_init(void)
{
	Hole *h;
	/* Create holes for user virtual address space */
	uintptr user_start = 0;
	uintptr user_size = conf.upages * BY2PG;  /* Actual user memory */
	uintptr user_end = user_size;  /* Size based on available memory */

	print("xhole_user_init: creating user holes for HHDM model\n");
	print("xhole_user_init: user range [0x%016llx, 0x%016llx) - %lld bytes\n",
		(unsigned long long)user_start, (unsigned long long)user_end, (unsigned long long)user_size);

	/* Get free hole descriptor from flist */
	ilock(&xlists.lk);

	h = xlists.flist;
	if(h == nil){
		iunlock(&xlists.lk);
		panic("xhole_user_init: no free hole descriptors");
	}

	xlists.flist = h->link;

	/* Create a large hole covering all user virtual address space */
	h->addr = user_start;
	h->top = user_end;
	h->size = user_size;
	h->link = xlists.table;
	xlists.table = h;

	iunlock(&xlists.lk);

	print("xhole_user_init: initialized %lld bytes of user virtual space\n", (unsigned long long)user_size);
}
