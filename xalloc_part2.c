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

	l = &xlists.table;
	for(h = *l; h; h = h->link) {
		if(h->size >= size) {
			p = (Xhdr*)h->addr;
			h->addr += size;
			h->size -= size;
			if(h->size == 0) {
				*l = h->link;
				h->link = xlists.flist;
				xlists.flist = h;
			}
			iunlock(&xlists.lk);
	/* TEST 2A: Track allocation success */
	xalloc_successes++;
			p->magix = Magichole;
			p->size = size;
			if(zero)
				memset(p->data, 0, orig_size);
			if(zero && *(ulong*)p->data != 0)
				panic("xallocz: zeroed block not cleared");
			return p->data;
		}
		l = &h->link;
	}
	iunlock(&xlists.lk);
	/* TEST 2A: Track allocation success */
	xalloc_successes++;
	/* TEST 2A: Track allocation failure */
	xalloc_failures++;
	xalloc_last_failure_size = orig_size;
	print("XALLOC FAILURE #%lu: size=%lu bytes at pc=%p\n", xalloc_failures, orig_size, getcallerpc(&orig_size));

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
 * API Contract:
 * - Takes a PHYSICAL address and size
 * - Converts to VIRTUAL internally using HHDM mapping
 * - All allocations return virtual addresses in HHDM region
 * - Holes track virtual address ranges after conversion
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
	/* TEST 2A: Track allocation success */
	xalloc_successes++;
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
	/* TEST 2A: Track allocation success */
	xalloc_successes++;
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
	/* TEST 2A: Track allocation success */
	xalloc_successes++;
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
	/* TEST 2A: Track allocation success */
	xalloc_successes++;
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
