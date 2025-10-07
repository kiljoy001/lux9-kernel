#include	"u.h"
#include <lib.h>
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"io.h"

/* Limine HHDM offset - defined in boot.c */
extern uintptr limine_hhdm_offset;
/* Saved HHDM offset - defined in globals.c, survives CR3 switch */
extern uintptr saved_limine_hhdm_offset;

/*
 * Simple segment descriptors with no translation.
 */
#define	EXECSEGM(p) 	{ 0, SEGL|SEGP|SEGPL(p)|SEGEXEC }
#define	DATASEGM(p) 	{ 0, SEGB|SEGG|SEGP|SEGPL(p)|SEGDATA|SEGW }
#define	EXEC32SEGM(p) 	{ 0xFFFF, SEGG|SEGD|(0xF<<16)|SEGP|SEGPL(p)|SEGEXEC|SEGR }
#define	DATA32SEGM(p) 	{ 0xFFFF, SEGB|SEGG|(0xF<<16)|SEGP|SEGPL(p)|SEGDATA|SEGW }

Segdesc gdt[NGDT] =
{
[NULLSEG]	{ 0, 0},		/* null descriptor */
[KESEG]		EXECSEGM(0),		/* kernel code */
[KDSEG]		DATASEGM(0),		/* kernel data */
[UE32SEG]	EXEC32SEGM(3),		/* user code 32 bit*/
[UDSEG]		DATA32SEGM(3),		/* user data/stack */
[UESEG]		EXECSEGM(3),		/* user code */
};

static struct {
	Lock;
	MMU	*free;

	ulong	nalloc;
	ulong	nfree;
} mmupool;

enum {
	/* level */
	PML4E	= 2,
	PDPE	= 1,
	PDE	= 0,

	MAPBITS	= 8*sizeof(m->mmumap[0]),
};

static void
loadptr(u16int lim, uintptr off, void (*load)(void*))
{
	u64int b[2], *o;
	u16int *s;

	o = &b[1];
	s = ((u16int*)o)-1;

	*s = lim;
	*o = off;

	(*load)(s);
}

static void
taskswitch(uintptr stack)
{
	Tss *tss;

	__asm__ volatile("outb %0, %1" : : "a"((char)'='), "Nd"((unsigned short)0x3F8));
	tss = m->tss;
	__asm__ volatile("outb %0, %1" : : "a"((char)'+'), "Nd"((unsigned short)0x3F8));
	if(tss != nil){
		__asm__ volatile("outb %0, %1" : : "a"((char)'-'), "Nd"((unsigned short)0x3F8));
		tss->rsp0[0] = (u32int)stack;
		tss->rsp0[1] = stack >> 32;
		tss->rsp1[0] = (u32int)stack;
		tss->rsp1[1] = stack >> 32;
		tss->rsp2[0] = (u32int)stack;
		tss->rsp2[1] = stack >> 32;
		__asm__ volatile("outb %0, %1" : : "a"((char)'_'), "Nd"((unsigned short)0x3F8));
	} else {
		__asm__ volatile("outb %0, %1" : : "a"((char)'N'), "Nd"((unsigned short)0x3F8));  /* tss is nil */
	}
	__asm__ volatile("outb %0, %1" : : "a"((char)'F'), "Nd"((unsigned short)0x3F8));  /* before mmuflushtlb */
	/* For now, skip TLB flush during first process switch - we're using same page tables */
	/* mmuflushtlb(PADDR(m->pml4)); */
	__asm__ volatile("outb %0, %1" : : "a"((char)'~'), "Nd"((unsigned short)0x3F8));
}

static void kernelro(void);

/*
 * Unified Kernel Memory Mapper
 * Single source of truth for all kernel memory mappings
 * Maps kernel ELF segments with correct permissions at 4KB granularity
 */

/* Helper: allocate and zero a page table from reserved pool */
static u64int *next_pt = nil;
static int pt_count = 0;

static u64int*
alloc_pt(void)
{
	extern u64int cpu0pt_pool[];  /* Reserved in linker.ld */
	u64int *pt;
	int i;

	if(next_pt == nil) {
		next_pt = cpu0pt_pool;  /* Start of PT pool */
	}

	pt = next_pt;
	next_pt += 512;  /* Each PT is 512 entries (4KB) */
	pt_count++;

	/* Panic if we run out of preallocated page tables */
	if(pt_count > 512) {
		__asm__ volatile("outb %0, %1" : : "a"((char)'!'), "Nd"((unsigned short)0x3F8));
		__asm__ volatile("outb %0, %1" : : "a"((char)'!'), "Nd"((unsigned short)0x3F8));
		__asm__ volatile("outb %0, %1" : : "a"((char)'!'), "Nd"((unsigned short)0x3F8));
		for(;;);  /* Halt */
	}

	/* Zero the page table */
	for(i = 0; i < 512; i++)
		pt[i] = 0;

	return pt;
}

/* Helper: get physical address */
static u64int
virt2phys(void *virt)
{
	extern uintptr saved_limine_hhdm_offset;
	u64int addr = (u64int)virt;
	extern u64int limine_kernel_phys_base;

	if(addr >= KZERO) {
		/* KZERO region maps to kernel physical base */
		u64int kernel_phys = (limine_kernel_phys_base == 0) ? 0x7f8fa000 : limine_kernel_phys_base;
		return addr - KZERO + kernel_phys;
	} else if(addr >= saved_limine_hhdm_offset) {
		return addr - saved_limine_hhdm_offset;
	} else {
		return addr;  /* Already physical */
	}
}

/* Map a virtual address range to physical with 2MB pages */
static void
map_range_2mb(u64int *pml4, u64int virt_start, u64int phys_start, u64int size, u64int perms)
{
	extern uintptr saved_limine_hhdm_offset;
	u64int virt, phys, virt_end;
	u64int pml4_idx, pdp_idx, pd_idx;
	u64int *pdp, *pd;
	u64int pdp_phys, pd_phys;

	virt_start &= ~0x1FFFFFULL;  /* Align to 2MB */
	phys_start &= ~0x1FFFFFULL;
	size = (size + 0x1FFFFF) & ~0x1FFFFFULL;

	virt_end = virt_start + size;

	for(virt = virt_start, phys = phys_start; virt < virt_end; virt += 2*MiB, phys += 2*MiB) {
		/* Calculate indices */
		pml4_idx = (virt >> 39) & 0x1FF;
		pdp_idx = (virt >> 30) & 0x1FF;
		pd_idx = (virt >> 21) & 0x1FF;

		/* Get or create PDP */
		if((pml4[pml4_idx] & PTEVALID) == 0) {
			pdp = alloc_pt();
			pdp_phys = virt2phys(pdp);
			pml4[pml4_idx] = pdp_phys | PTEVALID | PTEWRITE;
		} else {
			pdp_phys = pml4[pml4_idx] & ~0xFFF;
			if(pdp_phys < 0x400000)
				pdp = (u64int*)(pdp_phys + KZERO);
			else
				pdp = (u64int*)(pdp_phys + 0xffff800000000000ULL);
		}

		/* Get or create PD */
		if((pdp[pdp_idx] & PTEVALID) == 0) {
			pd = alloc_pt();
			pd_phys = virt2phys(pd);
			pdp[pdp_idx] = pd_phys | PTEVALID | PTEWRITE;
		} else {
			pd_phys = pdp[pdp_idx] & ~0xFFF;
			if(pd_phys < 0x400000)
				pd = (u64int*)(pd_phys + KZERO);
			else
				pd = (u64int*)(pd_phys + 0xffff800000000000ULL);
		}

		/* Map 2MB page directly in PD */
		pd[pd_idx] = phys | perms;  /* perms should include PTESIZE */
	}
}

/* Map a virtual address range to physical with given permissions */
static void
map_range(u64int *pml4, u64int virt_start, u64int phys_start, u64int size, u64int perms)
{
	extern uintptr saved_limine_hhdm_offset;
	u64int virt, phys, virt_end;
	u64int pml4_idx, pdp_idx, pd_idx, pt_idx;
	u64int *pdp, *pd, *pt;
	u64int pdp_phys, pd_phys, pt_phys;

	virt_end = virt_start + size;

	for(virt = virt_start, phys = phys_start; virt < virt_end; virt += 4*KiB, phys += 4*KiB) {
		/* Calculate indices */
		pml4_idx = (virt >> 39) & 0x1FF;
		pdp_idx = (virt >> 30) & 0x1FF;
		pd_idx = (virt >> 21) & 0x1FF;
		pt_idx = (virt >> 12) & 0x1FF;

		/* Get or create PDP */
		if((pml4[pml4_idx] & PTEVALID) == 0) {
			pdp = alloc_pt();
			pdp_phys = virt2phys(pdp);
			pml4[pml4_idx] = pdp_phys | PTEVALID | PTEWRITE;
		} else {
			pdp_phys = pml4[pml4_idx] & ~0xFFF;
			/* Our page tables are in KZERO range, existing ones might be anywhere */
			if(pdp_phys < 0x400000)
				pdp = (u64int*)(pdp_phys + KZERO);
			else
				pdp = (u64int*)(pdp_phys + 0xffff800000000000ULL);
		}

		/* Get or create PD */
		if((pdp[pdp_idx] & PTEVALID) == 0) {
			pd = alloc_pt();
			pd_phys = virt2phys(pd);
			pdp[pdp_idx] = pd_phys | PTEVALID | PTEWRITE;
		} else {
			pd_phys = pdp[pdp_idx] & ~0xFFF;
			if(pd_phys < 0x400000)
				pd = (u64int*)(pd_phys + KZERO);
			else
				pd = (u64int*)(pd_phys + 0xffff800000000000ULL);
		}

		/* Get or create PT */
		if((pd[pd_idx] & PTEVALID) == 0) {
			pt = alloc_pt();
			pt_phys = virt2phys(pt);
			pd[pd_idx] = pt_phys | PTEVALID | PTEWRITE;
		} else {
			pt_phys = pd[pd_idx] & ~0xFFF;
			if(pt_phys < 0x400000)
				pt = (u64int*)(pt_phys + KZERO);
			else
				pt = (u64int*)(pt_phys + 0xffff800000000000ULL);
		}

		/* Map the page */
		pt[pt_idx] = phys | perms;
	}
}

/* Setup our own page tables - SINGLE SOURCE OF TRUTH */
void
setuppagetables(void)
{
	u64int *pml4;
	u64int pml4_phys;
	int i;

	/* Linker-provided symbols - declared in lib.h as char[] */

	__asm__ volatile("outb %0, %1" : : "a"((char)'M'), "Nd"((unsigned short)0x3F8));
	__asm__ volatile("outb %0, %1" : : "a"((char)'1'), "Nd"((unsigned short)0x3F8));

	/* Use OUR page tables from linker script - completely independent of Limine */
	extern u64int cpu0pml4[];

	pml4 = cpu0pml4;
	pml4_phys = virt2phys(pml4);

	/* Clear OUR PML4 to start fresh */
	for(i = 0; i < 512; i++)
		pml4[i] = 0;

	__asm__ volatile("outb %0, %1" : : "a"((char)'M'), "Nd"((unsigned short)0x3F8));
	__asm__ volatile("outb %0, %1" : : "a"((char)'2'), "Nd"((unsigned short)0x3F8));

	/* Map kernel at KZERO to its ACTUAL physical load address from Limine */
	extern u64int limine_kernel_phys_base;
	u64int kernel_phys;

	/* TEMPORARY: Hardcode the physical base we see in QEMU output
	 * TODO: Get this from Limine Kernel Address request */
	if(limine_kernel_phys_base == 0)
		kernel_phys = 0x7f8fa000;  /* From "limine: Physical base: 0x7f8fa000" */
	else
		kernel_phys = limine_kernel_phys_base;

	__asm__ volatile("outb %0, %1" : : "a"((char)'['), "Nd"((unsigned short)0x3F8));
	/* Map 8MB starting from the kernel's physical base address */
	map_range(pml4, KZERO, kernel_phys, 0x800000, PTEVALID | PTEWRITE);
	__asm__ volatile("outb %0, %1" : : "a"((char)']'), "Nd"((unsigned short)0x3F8));

	/* Map HHDM (Higher Half Direct Map) - identity map first 4GB of physical memory
	 * HHDM base is at 0xffff800000000000 and maps to physical 0x0
	 * Use 2MB pages for HHDM to avoid consuming too many page tables
	 * PURE HHDM MODEL: This is our ONLY mapping for physical memory access */
	__asm__ volatile("outb %0, %1" : : "a"((char)'H'), "Nd"((unsigned short)0x3F8));
	map_range_2mb(pml4, 0xffff800000000000ULL, 0, 0x100000000ULL, PTEVALID | PTEWRITE | PTESIZE);  /* 4GB with 2MB pages */
	__asm__ volatile("outb %0, %1" : : "a"((char)'D'), "Nd"((unsigned short)0x3F8));

	__asm__ volatile("outb %0, %1" : : "a"((char)'M'), "Nd"((unsigned short)0x3F8));
	__asm__ volatile("outb %0, %1" : : "a"((char)'6'), "Nd"((unsigned short)0x3F8));

	/* Debug: output PT count before switch */
	__asm__ volatile("outb %0, %1" : : "a"((char)'P'), "Nd"((unsigned short)0x3F8));
	__asm__ volatile("outb %0, %1" : : "a"((char)'T'), "Nd"((unsigned short)0x3F8));
	__asm__ volatile("outb %0, %1" : : "a"((char)('0' + (pt_count / 100))), "Nd"((unsigned short)0x3F8));
	__asm__ volatile("outb %0, %1" : : "a"((char)('0' + ((pt_count / 10) % 10))), "Nd"((unsigned short)0x3F8));
	__asm__ volatile("outb %0, %1" : : "a"((char)('0' + (pt_count % 10))), "Nd"((unsigned short)0x3F8));

	/* Now switch to OUR page tables */
	__asm__ volatile("outb %0, %1" : : "a"((char)'C'), "Nd"((unsigned short)0x3F8));
	__asm__ volatile("outb %0, %1" : : "a"((char)'R'), "Nd"((unsigned short)0x3F8));
	__asm__ volatile("outb %0, %1" : : "a"((char)'3'), "Nd"((unsigned short)0x3F8));
	__asm__ volatile("mov %0, %%cr3" : : "r"(pml4_phys) : "memory");
	__asm__ volatile("outb %0, %1" : : "a"((char)'O'), "Nd"((unsigned short)0x3F8));
	__asm__ volatile("outb %0, %1" : : "a"((char)'K'), "Nd"((unsigned short)0x3F8));

	__asm__ volatile("outb %0, %1" : : "a"((char)'M'), "Nd"((unsigned short)0x3F8));
	__asm__ volatile("outb %0, %1" : : "a"((char)'9'), "Nd"((unsigned short)0x3F8));

	/* Update m->pml4 to point to our page tables */
	m->pml4 = pml4;
}

void
mmuinit(void)
{
	uintptr x;
	vlong v;
	int i;

	__asm__ volatile("outb %0, %1" : : "a"((char)'1'), "Nd"((unsigned short)0x3F8));
	/* zap double map done by l.s */
	m->pml4[512] = 0;
	__asm__ volatile("outb %0, %1" : : "a"((char)'2'), "Nd"((unsigned short)0x3F8));
	m->pml4[0] = 0;

	__asm__ volatile("outb %0, %1" : : "a"((char)'3'), "Nd"((unsigned short)0x3F8));
	if(m->machno == 0)
		kernelro();

	__asm__ volatile("outb %0, %1" : : "a"((char)'4'), "Nd"((unsigned short)0x3F8));
	m->tss = mallocz(sizeof(Tss), 1);
	__asm__ volatile("outb %0, %1" : : "a"((char)'5'), "Nd"((unsigned short)0x3F8));
	if(m->tss == nil)
		panic("mmuinit: no memory for Tss");
	__asm__ volatile("outb %0, %1" : : "a"((char)'6'), "Nd"((unsigned short)0x3F8));
	m->tss->iomap = 0xDFFF;
	__asm__ volatile("outb %0, %1" : : "a"((char)'7'), "Nd"((unsigned short)0x3F8));
	for(i=0; i<14; i+=2){
		x = (uintptr)m + MACHSIZE;
		m->tss->ist[i] = x;
		m->tss->ist[i+1] = x>>32;
	}

	/*
	 * We used to keep the GDT in the Mach structure, but it
	 * turns out that that slows down access to the rest of the
	 * page.  Since the Mach structure is accessed quite often,
	 * it pays off anywhere from a factor of 1.25 to 2 on real
	 * hardware to separate them (the AMDs are more sensitive
	 * than Intels in this regard).  Under VMware it pays off
	 * a factor of about 10 to 100.
	 */
	memmove(m->gdt, gdt, sizeof gdt);

	x = (uintptr)m->tss;
	m->gdt[TSSSEG+0].d0 = (x<<16)|(sizeof(Tss)-1);
	m->gdt[TSSSEG+0].d1 = (x&0xFF000000)|((x>>16)&0xFF)|SEGTSS|SEGPL(0)|SEGP;
	m->gdt[TSSSEG+1].d0 = x>>32;
	m->gdt[TSSSEG+1].d1 = 0;

	loadptr(sizeof(gdt)-1, (uintptr)m->gdt, lgdt);
	loadptr(sizeof(Segdesc)*512-1, (uintptr)IDTADDR, lidt);
	taskswitch((uintptr)m + MACHSIZE);
	ltr(TSSSEL);

	wrmsr(FSbase, 0ull);
	wrmsr(GSbase, (uvlong)&machp[m->machno]);
	wrmsr(KernelGSbase, 0ull);

	/* enable syscall extension */
	rdmsr(Efer, &v);
	v |= 1ull;
	wrmsr(Efer, v);

	wrmsr(Star, ((uvlong)UE32SEL << 48) | ((uvlong)KESEL << 32));
	wrmsr(Lstar, (uvlong)syscallentry);
	wrmsr(Sfmask, 0x200);
}

/*
 * These could go back to being macros once the kernel is debugged,
 * but the extra checking is nice to have.
 */

void*
kaddr(uintptr pa)
{
	if(pa >= (uintptr)-KZERO)
		panic("kaddr: pa=%#p pc=%#p", pa, getcallerpc(&pa));

	/* Use Limine's HHDM offset instead of KZERO */
	return (void*)(pa + saved_limine_hhdm_offset);
}

uintptr
paddr(void *v)
{
	uintptr va;

	va = (uintptr)v;
	/* Kernel addresses at KZERO (0xffffffff80000000) */
	if(va >= KZERO)
		return va - KZERO;
	/* HHDM addresses - Limine maps all physical memory here */
	if(va >= saved_limine_hhdm_offset)
		return va - saved_limine_hhdm_offset;
	if(va >= VMAP)
		return va-VMAP;
	panic("paddr: va=%#p pc=%#p", va, getcallerpc(&v));
}

static MMU*
mmualloc(void)
{
	MMU *p;
	int i, n;

	p = m->mmufree;
	if(p != nil){
		m->mmufree = p->next;
		m->mmucount--;
	} else {
		lock(&mmupool);
		p = mmupool.free;
		if(p != nil){
			mmupool.free = p->next;
			mmupool.nfree--;
		} else {
			unlock(&mmupool);

			n = 256;
			p = malloc(n * sizeof(MMU));
			if(p == nil)
				return nil;
			p->page = mallocalign(n * PTSZ, BY2PG, 0, 0);
			if(p->page == nil){
				free(p);
				return nil;
			}
			for(i=1; i<n; i++){
				p[i].page = p[i-1].page + (1<<PTSHIFT);
				p[i-1].next = &p[i];
			}

			lock(&mmupool);
			p[n-1].next = mmupool.free;
			mmupool.free = p->next;
			mmupool.nalloc += n;
			mmupool.nfree += n-1;
		}
		unlock(&mmupool);
	}
	p->next = nil;
	return p;
}

static uintptr*
mmucreate(uintptr *table, uintptr va, int level, int index)
{
	uintptr *page, flags;
	MMU *p;
	
	flags = PTEWRITE|PTEVALID;
	if(va < VMAP){
		assert(up != nil);
		assert((va < USTKTOP) || (va >= KMAP && va < KMAP+KMAPSIZE));
		if((p = mmualloc()) == nil)
			return nil;
		p->index = index;
		p->level = level;
		if(va < USTKTOP){
			flags |= PTEUSER;
			if(level == PML4E){
				if((p->next = up->mmuhead) == nil)
					up->mmutail = p;
				up->mmuhead = p;
				m->mmumap[index/MAPBITS] |= 1ull<<(index%MAPBITS);
			} else {
				up->mmutail->next = p;
				up->mmutail = p;
			}
			up->mmucount++;
		} else {
			if(level == PML4E){
				up->kmaptail = p;
				up->kmaphead = p;
			} else {
				up->kmaptail->next = p;
				up->kmaptail = p;
			}
			up->kmapcount++;
		}
		page = p->page;
	} else {
		page = rampage();
	}
	memset(page, 0, PTSZ);
	table[index] = PADDR(page) | flags;
	return page;
}

uintptr*
mmuwalk(uintptr *table, uintptr va, int level, int create)
{
	uintptr pte;
	int i, x;

	x = PTLX(va, 3);
	for(i = 2; i >= level; i--){
		pte = table[x];
		if(pte & PTEVALID){
			if(pte & PTESIZE)
				return nil;
			pte = PPN(pte);
			/* Pure HHDM model: ALL physical addresses mapped via HHDM */
			table = (void*)(pte + saved_limine_hhdm_offset);
		} else {
			if(!create)
				return nil;
			table = mmucreate(table, va, i, x);
			if(table == nil)
				return nil;
		}
		x = PTLX(va, i);
	}
	return &table[x];
}

static uintptr*
getpte(uintptr va)
{
	uintptr *pte;

	if((pte = mmuwalk(m->pml4, va, 0, 1)) == nil){
		flushmmu();
		while((pte = mmuwalk(m->pml4, va, 0, 1)) == nil){
			int x = spllo();
			resrcwait("out of MMU pages");
			splx(x);
		}
	}
	return pte;
}

static int
ptecount(uintptr va, int level)
{
	return (1<<PTSHIFT) - (va & PGLSZ(level+1)-1) / PGLSZ(level);
}

static void
ptesplit(uintptr* table, uintptr va)
{
	uintptr *pte, pa, off;

	pte = mmuwalk(table, va, 1, 0);
	if(pte == nil || (*pte & PTESIZE) == 0 || (va & PGLSZ(1)-1) == 0)
		return;
	table = rampage();
	va &= -PGLSZ(1);
	pa = *pte & ~PTESIZE;
	for(off = 0; off < PGLSZ(1); off += PGLSZ(0))
		table[PTLX(va + off, 0)] = pa + off;
	*pte = PADDR(table) | PTEVALID|PTEWRITE;
	invlpg(va);
}

/*
 * map kernel text segment readonly
 * and everything else no-execute.
 */
/*
 * Simple approach: Copy only the PML4 page to writable memory.
 * Lower level page tables remain Limine's (they don't need to be modified).
 */
void
copypagetables(void)
{
	uintptr *oldpml4, *newpml4;
	uintptr cr3;

	__asm__ volatile("outb %0, %1" : : "a"((char)'1'), "Nd"((unsigned short)0x3F8));
	/* Get current PML4 from CR3 */
	__asm__ volatile("mov %%cr3, %0" : "=r"(cr3));
	__asm__ volatile("outb %0, %1" : : "a"((char)'2'), "Nd"((unsigned short)0x3F8));
	oldpml4 = kaddr(PPN(cr3));
	__asm__ volatile("outb %0, %1" : : "a"((char)'3'), "Nd"((unsigned short)0x3F8));

	/* Allocate a single writable page for our PML4 */
	newpml4 = rampage();
	__asm__ volatile("outb %0, %1" : : "a"((char)'4'), "Nd"((unsigned short)0x3F8));
	if(newpml4 == nil)
		panic("copypagetables: out of memory");
	__asm__ volatile("outb %0, %1" : : "a"((char)'5'), "Nd"((unsigned short)0x3F8));

	/* Copy all PML4 entries - these point to Limine's lower level tables */
	memmove(newpml4, oldpml4, PTSZ);
	__asm__ volatile("outb %0, %1" : : "a"((char)'6'), "Nd"((unsigned short)0x3F8));

	/* Update m->pml4 to point to new writable PML4 */
	m->pml4 = newpml4;
	__asm__ volatile("outb %0, %1" : : "a"((char)'7'), "Nd"((unsigned short)0x3F8));

	/* DON'T switch CR3 - just use Limine's page tables for now */
	/* Switching CR3 causes triple fault - needs more investigation */
	/* cr3 = paddr(newpml4); */
	/* __asm__ volatile("outb %0, %1" : : "a"((char)'8'), "Nd"((unsigned short)0x3F8)); */
	/* __asm__ volatile("mov %0, %%cr3" : : "r"(cr3)); */
	__asm__ volatile("outb %0, %1" : : "a"((char)'9'), "Nd"((unsigned short)0x3F8));
}

static void
kernelro(void)
{
	extern char ttext[], etext[];
	uintptr *pte, psz, va;

	ptesplit(m->pml4, APBOOTSTRAP);
	ptesplit(m->pml4, KTZERO);
	ptesplit(m->pml4, (uintptr)ttext);
	ptesplit(m->pml4, (uintptr)etext-1);

	/* Now we can modify PTEs - our page tables are writable! */
	for(va = KZERO; va != 0; va += psz){
		psz = PGLSZ(0);
		pte = mmuwalk(m->pml4, va, 0, 0);
		if(pte == nil){
			if(va & PGLSZ(1)-1)
				continue;
			pte = mmuwalk(m->pml4, va, 1, 0);
			if(pte == nil)
				continue;
			psz = PGLSZ(1);
		}
		if((*pte & PTEVALID) == 0)
			continue;
		if(va >= (uintptr)ttext && va < (uintptr)etext)
			*pte &= ~PTEWRITE;  /* Make text section read-only */
		else if(va != (APBOOTSTRAP & -BY2PG))
			*pte |= PTENOEXEC;  /* Make non-text sections non-executable */
		invlpg(va);
	}
}

void
pmap(uintptr pa, uintptr va, vlong size)
{
	uintptr *pte, *ptee, flags;
	int z, l;

	/* Pure HHDM model: accept addresses in HHDM range, not VMAP */
	if(size <= 0 || va < saved_limine_hhdm_offset)
		panic("pmap: pa=%#p va=%#p size=%lld", pa, va, size);
	flags = pa;
	pa = PPN(pa);
	flags -= pa;
	flags |= PTEACCESSED|PTEDIRTY;
	if(va >= KZERO)
		flags |= PTEGLOBAL;
	while(size > 0){
	if(size >= PGLSZ(1) && size < PGLSZ(2) && (va % PGLSZ(1)) == 0)
			flags |= PTESIZE;
		l = (flags & PTESIZE) != 0;
		z = PGLSZ(l);
		pte = mmuwalk(m->pml4, va, l, 1);
		if(pte == nil){
			pte = mmuwalk(m->pml4, va, ++l, 0);
			if(pte && (*pte & PTESIZE)){
				flags |= PTESIZE;
				z = va & (PGLSZ(l)-1);
				va -= z;
				pa -= z;
				size += z;
				continue;
			}
			panic("pmap: pa=%#p va=%#p size=%lld", pa, va, size);
		}
		ptee = pte + ptecount(va, l);
		while(size > 0 && pte < ptee){
			*pte++ = pa | flags;
			pa += z;
			va += z;
			size -= z;
		}
	}
}

void
punmap(uintptr va, vlong size)
{
	uintptr *pte;
	int l;

	va = PPN(va);
	while(size > 0){
		if((va % PGLSZ(1)) != 0 || size < PGLSZ(1))
			ptesplit(m->pml4, va);
		l = 0;
		pte = mmuwalk(m->pml4, va, l, 0);
		if(pte == nil && (va % PGLSZ(1)) == 0 && size >= PGLSZ(1))
			pte = mmuwalk(m->pml4, va, ++l, 0);
		if(pte){
			*pte = 0;
			invlpg(va);
		}
		va += PGLSZ(l);
		size -= PGLSZ(l);
	}
}

static void
mmuzap(void)
{
	uintptr *pte;
	u64int w;
	int i, x;

	pte = m->pml4;
	pte[PTLX(KMAP, 3)] = 0;

	/* common case */
	pte[PTLX(UTZERO, 3)] = 0;
	pte[PTLX(USTKTOP-1, 3)] = 0;
	m->mmumap[PTLX(UTZERO, 3)/MAPBITS] &= ~(1ull<<(PTLX(UTZERO, 3)%MAPBITS));
	m->mmumap[PTLX(USTKTOP-1, 3)/MAPBITS] &= ~(1ull<<(PTLX(USTKTOP-1, 3)%MAPBITS));

	for(i = 0; i < nelem(m->mmumap); pte += MAPBITS, i++){
		if((w = m->mmumap[i]) == 0)
			continue;
		m->mmumap[i] = 0;
		for(x = 0; w != 0; w >>= 1, x++){
			if(w & 1)
				pte[x] = 0;
		}
	}
}

static void
mmufree(Proc *proc)
{
	MMU *p;

	p = proc->mmutail;
	if(p == nil)
		return;
	if(m->mmucount+proc->mmucount < 256){
		p->next = m->mmufree;
		m->mmufree = proc->mmuhead;
		m->mmucount += proc->mmucount;
	} else {
		lock(&mmupool);
		p->next = mmupool.free;
		mmupool.free = proc->mmuhead;
		mmupool.nfree += proc->mmucount;
		unlock(&mmupool);
	}
	proc->mmuhead = proc->mmutail = nil;
	proc->mmucount = 0;
}

void
flushmmu(void)
{
	int x;

	__asm__ volatile("outb %0, %1" : : "a"((char)'F'), "Nd"((unsigned short)0x3F8));
	x = splhi();
	__asm__ volatile("outb %0, %1" : : "a"((char)'S'), "Nd"((unsigned short)0x3F8));
	up->newtlb = 1;
	__asm__ volatile("outb %0, %1" : : "a"((char)'M'), "Nd"((unsigned short)0x3F8));
	mmuswitch(up);
	__asm__ volatile("outb %0, %1" : : "a"((char)'X'), "Nd"((unsigned short)0x3F8));
	splx(x);
	__asm__ volatile("outb %0, %1" : : "a"((char)'!'), "Nd"((unsigned short)0x3F8));
}

void
mmuswitch(Proc *proc)
{
	MMU *p;

	__asm__ volatile("outb %0, %1" : : "a"((char)'&'), "Nd"((unsigned short)0x3F8));

	mmuzap();
	__asm__ volatile("outb %0, %1" : : "a"((char)'*'), "Nd"((unsigned short)0x3F8));
	if(proc->newtlb){
		mmufree(proc);
		proc->newtlb = 0;
	}
	__asm__ volatile("outb %0, %1" : : "a"((char)'^'), "Nd"((unsigned short)0x3F8));

	/* For kernel processes, there are no user MMU structures */
	__asm__ volatile("outb %0, %1" : : "a"((char)'K'), "Nd"((unsigned short)0x3F8));  /* Before kp check */
	if(proc->kp){
		__asm__ volatile("outb %0, %1" : : "a"((char)'P'), "Nd"((unsigned short)0x3F8));  /* Kernel process detected, skip MMU setup */
		taskswitch((uintptr)proc);
		__asm__ volatile("outb %0, %1" : : "a"((char)';'), "Nd"((unsigned short)0x3F8));  /* After taskswitch */
		return;
	}
	__asm__ volatile("outb %0, %1" : : "a"((char)'U'), "Nd"((unsigned short)0x3F8));  /* User process, do MMU setup */

	__asm__ volatile("outb %0, %1" : : "a"((char)'1'), "Nd"((unsigned short)0x3F8));  /* Before kmaphead check */
	if((p = proc->kmaphead) != nil){
		__asm__ volatile("outb %0, %1" : : "a"((char)'2'), "Nd"((unsigned short)0x3F8));  /* kmaphead not nil */
		m->pml4[PTLX(KMAP, 3)] = PADDR(p->page) | PTEWRITE|PTEVALID;
		__asm__ volatile("outb %0, %1" : : "a"((char)'3'), "Nd"((unsigned short)0x3F8));  /* After pml4 write */
	}
	__asm__ volatile("outb %0, %1" : : "a"((char)'%'), "Nd"((unsigned short)0x3F8));
	if(proc->mmuhead == nil){
		__asm__ volatile("outb %0, %1" : : "a"((char)'N'), "Nd"((unsigned short)0x3F8));
		__asm__ volatile("outb %0, %1" : : "a"((char)'U'), "Nd"((unsigned short)0x3F8));
		__asm__ volatile("outb %0, %1" : : "a"((char)'L'), "Nd"((unsigned short)0x3F8));
		__asm__ volatile("outb %0, %1" : : "a"((char)'L'), "Nd"((unsigned short)0x3F8));
	}
	for(p = proc->mmuhead; p != nil && p->level == PML4E; p = p->next){
		__asm__ volatile("outb %0, %1" : : "a"((char)'+'), "Nd"((unsigned short)0x3F8));
		m->mmumap[p->index/MAPBITS] |= 1ull<<(p->index%MAPBITS);
		m->pml4[p->index] = PADDR(p->page) | PTEUSER|PTEWRITE|PTEVALID;
	}
	__asm__ volatile("outb %0, %1" : : "a"((char)'$'), "Nd"((unsigned short)0x3F8));
	taskswitch((uintptr)proc);
	__asm__ volatile("outb %0, %1" : : "a"((char)';'), "Nd"((unsigned short)0x3F8));
}

void
mmurelease(Proc *proc)
{
	MMU *p;

	mmuzap();
	if((p = proc->kmaptail) != nil){
		if((p->next = proc->mmuhead) == nil)
			proc->mmutail = p;
		proc->mmuhead = proc->kmaphead;
		proc->mmucount += proc->kmapcount;

		proc->kmaphead = proc->kmaptail = nil;
		proc->kmapcount = proc->kmapindex = 0;
	}
	mmufree(proc);
	taskswitch((uintptr)m+MACHSIZE);
}

void
putmmu(uintptr va, uintptr pa, Page *)
{
	uintptr *pte, old;
	int x;

	x = splhi();
	pte = getpte(va);
	old = *pte;
	*pte = pa | PTEACCESSED|PTEDIRTY|PTEUSER;
	splx(x);
	if(old & PTEVALID)
		invlpg(va);
}

/*
 * Double-check the user MMU.
 * Error checking only.
 */
void
checkmmu(uintptr va, uintptr pa)
{
	uintptr *pte, old;
	int x;

	x = splhi();
	pte = mmuwalk(m->pml4, va, 0, 0);
	if(pte == nil || ((old = *pte) & PTEVALID) == 0 || PPN(old) == pa){
		splx(x);
		return;
	}
	splx(x);
	print("%ld %s: va=%#p pa=%#p pte=%#p\n", up->pid, up->text, va, pa, old);
}

uintptr
cankaddr(uintptr pa)
{
	/* With Limine HHDM, all physical memory is already mapped
	 * Limine maps up to 4GB+ of memory typically
	 * For now, assume we can map anything under 4GB */
	if(pa >= 4ULL*1024*1024*1024)  /* 4GB limit */
		return 0;
	return (4ULL*1024*1024*1024) - pa;
}

KMap*
kmap(Page *page)
{
	extern uintptr saved_limine_hhdm_offset;
	uintptr pa, va;

	/* Pure HHDM model: all physical memory already mapped via HHDM */
	__asm__ volatile("outb %0, %1" : : "a"((char)'K'), "Nd"((unsigned short)0x3F8));
	pa = page->pa;
	va = pa + saved_limine_hhdm_offset;
	__asm__ volatile("outb %0, %1" : : "a"((char)'M'), "Nd"((unsigned short)0x3F8));
	return (KMap*)va;
}

void
kunmap(KMap *k)
{
	/* Pure HHDM model: no-op, memory stays mapped via HHDM */
	USED(k);
}

/*
 * Add a device mapping to the vmap range.
 * note that the VMAP and KZERO PDPs are shared
 * between processors (see mpstartap) so no
 * synchronization is being done.
 */
void*
vmap(uvlong pa, vlong size)
{
	uintptr va;
	int o;

	/* Pure HHDM model: no size limit check needed, physical memory is already mapped */
	if(pa < BY2PG || size <= 0 || -pa < size){
		print("vmap pa=%llux size=%lld pc=%#p\n", pa, size, getcallerpc(&pa));
		return nil;
	}
	va = pa+limine_hhdm_offset;

	/*
	 * might be asking for less than a page.
	 */
	o = pa & (BY2PG-1);
	pa -= o;
	va -= o;
	size += o;
	pmap(pa | PTEUNCACHED|PTEWRITE|PTENOEXEC|PTEVALID, va, size);
	return (void*)(va+o);
}

void
vunmap(void *v, vlong)
{
	paddr(v);	/* will panic on error */
}

/*
 * mark pages as write combining (used for framebuffer)
 */
void
patwc(void *a, int n)
{
	uintptr *pte, mask, attr, va;
	int z, l;
	vlong v;

	/* check if pat is usable */
	if((MACHP(0)->cpuiddx & Pat) == 0
	|| rdmsr(0x277, &v) == -1
	|| ((v >> PATWC*8) & 7) != 1)
		return;

	/* set the bits for all pages in range */
	for(va = (uintptr)a; n > 0; n -= z, va += z){
		l = 0;
		pte = mmuwalk(m->pml4, va, l, 0);
		if(pte == nil)
			pte = mmuwalk(m->pml4, va, ++l, 0);
		if(pte == nil || (*pte & PTEVALID) == 0)
			panic("patwc: va=%#p", va);
		z = PGLSZ(l);
		z -= va & (z-1);
		mask = l == 0 ? 3<<3 | 1<<7 : 3<<3 | 1<<12;
		attr = (((PATWC&3)<<3) | ((PATWC&4)<<5) | ((PATWC&4)<<10));
		*pte = (*pte & ~mask) | (attr & mask);
	}
}

/*
 * The palloc.pages array and mmupool can be a large chunk
 * out of the 2GB window above KZERO, so we allocate from
 * upages and map in the VMAP window before pageinit()
 */
void
preallocpages(void)
{
	Confmem *cm;
	uintptr va, base, top;
	vlong tsize, psize;
	ulong np, nt;
	int i;

	__asm__ volatile("outb %0, %1" : : "a"((char)'1'), "Nd"((unsigned short)0x3F8));
	np = 0;
	__asm__ volatile("outb %0, %1" : : "a"((char)'i'), "Nd"((unsigned short)0x3F8));
	i = 0;
	__asm__ volatile("outb %0, %1" : : "a"((char)'L'), "Nd"((unsigned short)0x3F8));
	for(; i<nelem(conf.mem); i++){
		__asm__ volatile("outb %0, %1" : : "a"((char)'C'), "Nd"((unsigned short)0x3F8));
		cm = &conf.mem[i];
		__asm__ volatile("outb %0, %1" : : "a"((char)'c'), "Nd"((unsigned short)0x3F8));
		if(cm->npage == 0)
			continue;
		np += cm->npage - nkpages(cm);
	}
	__asm__ volatile("outb %0, %1" : : "a"((char)'2'), "Nd"((unsigned short)0x3F8));
	nt = np / 50;	/* 2% for mmupool */
	np -= nt;
	__asm__ volatile("outb %0, %1" : : "a"((char)'3'), "Nd"((unsigned short)0x3F8));

	nt = (uvlong)nt*BY2PG / (sizeof(MMU)+PTSZ);
	tsize = (uvlong)nt * (sizeof(MMU)+PTSZ);

	psize = (uvlong)np * BY2PG;
	psize += sizeof(Page) + BY2PG;
	psize = (psize / (sizeof(Page)+BY2PG)) * sizeof(Page);

	psize += tsize;
	psize = ROUND(psize, PGLSZ(1));

	__asm__ volatile("outb %0, %1" : : "a"((char)'4'), "Nd"((unsigned short)0x3F8));
	for(i=0; i<nelem(conf.mem); i++){
		cm = &conf.mem[i];
		if(cm->npage == 0)
			continue;
		__asm__ volatile("outb %0, %1" : : "a"((char)'5'), "Nd"((unsigned short)0x3F8));
		base = cm->base;
		top = base + (uvlong)cm->npage * BY2PG;
		base += (uvlong)nkpages(cm) * BY2PG;
		top &= -PGLSZ(1);
		__asm__ volatile("outb %0, %1" : : "a"((char)'6'), "Nd"((unsigned short)0x3F8));
		if(top <= VMAPSIZE && (vlong)(top - base) >= psize){
			__asm__ volatile("outb %0, %1" : : "a"((char)'7'), "Nd"((unsigned short)0x3F8));
			/* steal memory from the end of the bank */
			__asm__ volatile("outb %0, %1" : : "a"((char)'8'), "Nd"((unsigned short)0x3F8));
			top -= psize;
			__asm__ volatile("outb %0, %1" : : "a"((char)'N'), "Nd"((unsigned short)0x3F8));
			cm->npage = (top - cm->base) / BY2PG;
			__asm__ volatile("outb %0, %1" : : "a"((char)'n'), "Nd"((unsigned short)0x3F8));

			__asm__ volatile("outb %0, %1" : : "a"((char)'9'), "Nd"((unsigned short)0x3F8));
			__asm__ volatile("outb %0, %1" : : "a"((char)'V'), "Nd"((unsigned short)0x3F8));
			/* Pure HHDM model: map physical memory via HHDM, not VMAP */
			va = top + limine_hhdm_offset;
			__asm__ volatile("outb %0, %1" : : "a"((char)'P'), "Nd"((unsigned short)0x3F8));
			pmap(top | PTEGLOBAL|PTEWRITE|PTENOEXEC|PTEVALID, va, psize);
			__asm__ volatile("outb %0, %1" : : "a"((char)'p'), "Nd"((unsigned short)0x3F8));

			palloc.pages = (void*)(va + tsize);

			mmupool.nfree = mmupool.nalloc = nt;
			mmupool.free = (void*)(va + (uvlong)nt*PTSZ);
			for(i=0; i<nt; i++){
				mmupool.free[i].page = (uintptr*)va;
				mmupool.free[i].next = &mmupool.free[i+1];
				va += PTSZ;
			}
			mmupool.free[i-1].next = nil;

			break;
		}
	}
}
