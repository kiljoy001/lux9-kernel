#include	"u.h"
#include <lib.h>
#include	"mem.h"

#include	"fns.h"
#include	"io.h"

extern void uartputs(char*, int);

static void
dbghex(char *label, uvlong value)
{
	static char hex[] = "0123456789abcdef";
	char buf[2 + sizeof(uvlong)*2 + 2];
	char *p = buf;

	*p++ = '0';
	*p++ = 'x';
	for(int i = (int)(sizeof(uvlong)*2 - 1); i >= 0; i--)
		*p++ = hex[(value >> (i*4)) & 0xF];
	*p++ = '\n';
	*p = 0;

	if(label != nil)
		uartputs(label, strlen(label));
	uartputs(buf, p - buf);
}

/* Limine HHDM offset - defined in boot.c */
extern uintptr limine_hhdm_offset;
/* Saved HHDM offset - defined in globals.c, survives CR3 switch */
extern uintptr saved_limine_hhdm_offset;
extern char kend[];
extern uintptr initrd_physaddr;
extern usize initrd_size;

static uintptr max_physaddr;

static void
ensure_phys_range(void)
{
	Confmem *cm;
	int i;

	if(max_physaddr != 0)
		return;

	for(i = 0; i < nelem(conf.mem); i++){
		cm = &conf.mem[i];
		if(cm->npage == 0)
			continue;
		if(cm->base + (uintptr)cm->npage * BY2PG > max_physaddr)
			max_physaddr = cm->base + (uintptr)cm->npage * BY2PG;
	}
	if(max_physaddr == 0)
		max_physaddr = (uintptr)1 << 52;	/* 4PB default guard */
}

static inline uintptr
hhdm_virt(uintptr pa)
{
	return pa + saved_limine_hhdm_offset;
}

static inline uintptr
hhdm_phys(uintptr va)
{
	return va - saved_limine_hhdm_offset;
}

static int
is_hhdm_va(uintptr va)
{
	if(saved_limine_hhdm_offset == 0 || va < saved_limine_hhdm_offset)
		return 0;
	ensure_phys_range();
	return hhdm_phys(va) < max_physaddr;
}

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

enum {
	MMUFREELIMIT	= 256,
};

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

	tss = m->tss;
	if(tss != nil){
		tss->rsp0[0] = (u32int)stack;
		tss->rsp0[1] = stack >> 32;
		tss->rsp1[0] = (u32int)stack;
		tss->rsp1[1] = stack >> 32;
		tss->rsp2[0] = (u32int)stack;
		tss->rsp2[1] = stack >> 32;
	}
	/* For now, skip TLB flush during first process switch - we're using same page tables */
	/* mmuflushtlb(PADDR(m->pml4)); */
}

static void kernelro(void);

uintptr
dbg_getpte(uintptr va)
{
	uintptr *pte;

	pte = mmuwalk(m->pml4, va, 0, 0);
	if(pte == nil)
		return 0;
	return *pte;
}

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
	if(pt_count > 512)
		panic("alloc_pt: cpu0pt_pool exhausted");

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

	if(is_hhdm_va(addr))
		return hhdm_phys(addr);
	if(addr >= KZERO) {
		/* KZERO region maps to kernel physical base */
		u64int kernel_phys = (limine_kernel_phys_base == 0) ? 0x7f8fa000 : limine_kernel_phys_base;
		return addr - KZERO + kernel_phys;
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
				pdp = (u64int*)hhdm_virt(pdp_phys);
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
				pd = (u64int*)hhdm_virt(pd_phys);
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
				pdp = (u64int*)hhdm_virt(pdp_phys);
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
				pd = (u64int*)hhdm_virt(pd_phys);
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
				pt = (u64int*)hhdm_virt(pt_phys);
		}

		/* Map the page */
		pt[pt_idx] = phys | perms;
	}
}

static void
map_hhdm_region(u64int *pml4, u64int phys_start, u64int size)
{
	u64int phys = phys_start;
	u64int end = phys_start + size;
	u64int virt = hhdm_virt(phys);
	u64int perms_small = PTEVALID | PTEWRITE | PTEGLOBAL;
	u64int perms_large = perms_small | PTESIZE;

	/* map leading unaligned portion with 4KB pages */
	if((phys & (PGLSZ(1) - 1)) != 0){
		u64int chunk = PGLSZ(1) - (phys & (PGLSZ(1) - 1));
		if(phys + chunk > end)
			chunk = end - phys;
		map_range(pml4, virt, phys, chunk, perms_small);
		phys += chunk;
		virt = hhdm_virt(phys);
	}

	/* map bulk with 2MB pages */
	u64int aligned = (end - phys) & ~(PGLSZ(1) - 1);
	if(aligned != 0){
		map_range_2mb(pml4, virt, phys, aligned, perms_large);
		phys += aligned;
		virt = hhdm_virt(phys);
	}

	/* map trailing remainder */
	if(phys < end)
		map_range(pml4, virt, phys, end - phys, perms_small);
}

/* Setup our own page tables - SINGLE SOURCE OF TRUTH */
void
setuppagetables(void)
{
	u64int *pml4;
	u64int pml4_phys;
	int i;
	extern char end[];

	/* Linker-provided symbols - declared in lib.h as char[] */


	/* Use OUR page tables from linker script - completely independent of Limine */
	extern u64int cpu0pml4[];

	pml4 = cpu0pml4;
	next_pt = nil;
	pt_count = 0;
	pml4_phys = virt2phys(pml4);

	/* Clear OUR PML4 to start fresh */
	for(i = 0; i < 512; i++)
		pml4[i] = 0;


	/* Map kernel at KZERO to its ACTUAL physical load address from Limine */
	extern u64int limine_kernel_phys_base;
	u64int kernel_phys;

	/* TEMPORARY: Hardcode the physical base we see in QEMU output
	 * TODO: Get this from Limine Kernel Address request */
	if(limine_kernel_phys_base == 0)
		kernel_phys = 0x7f8fa000;  /* From "limine: Physical base: 0x7f8fa000" */
	else
		kernel_phys = limine_kernel_phys_base;

	/* Map the kernel image at KZERO */
	u64int kernel_size = ((uintptr)&kend - KZERO + BY2PG - 1) & ~(BY2PG - 1);
	map_range(pml4, KZERO, kernel_phys, kernel_size, PTEVALID | PTEWRITE | PTEGLOBAL);
	/* Mirror the kernel image into the HHDM so KADDR() stays valid post-switch */
	map_range(pml4, hhdm_virt(kernel_phys), kernel_phys, kernel_size, PTEVALID | PTEWRITE | PTEGLOBAL);

	/* Identity-map the first 2MB for early firmware interactions */
	map_range(pml4, 0, 0, PGLSZ(1), PTEVALID | PTEWRITE | PTEGLOBAL);
	/* Provide HHDM access to the low 2MB (warm-reset vector, AP trampoline, etc.) */
	map_range(pml4, hhdm_virt(0), 0, PGLSZ(1), PTEVALID | PTEWRITE | PTEGLOBAL);

	/* Map physical memory into the HHDM using conf.mem */
	ensure_phys_range();
	for(i = 0; i < nelem(conf.mem); i++){
		Confmem *cm = &conf.mem[i];
		if(cm->npage == 0)
			continue;
		map_hhdm_region(pml4, cm->base, (u64int)cm->npage * BY2PG);
	}
	if(initrd_physaddr != 0 && initrd_size != 0)
		map_hhdm_region(pml4, initrd_physaddr, initrd_size);


	/* Debug: output PT count before switch */

	/* Now switch to OUR page tables */
	__asm__ volatile("mov %0, %%cr3" : : "r"(pml4_phys) : "memory");


	/* Update m->pml4 to point to our page tables */
	m->pml4 = pml4;
}

void
mmuinit(void)
{
	uintptr x;
	vlong v;
	int i;

	/* zap double map done by l.s */
	m->pml4[512] = 0;
	m->pml4[0] = 0;

	if(m->machno == 0)
		kernelro();

	m->tss = mallocz(sizeof(Tss), 1);
	if(m->tss == nil)
		panic("mmuinit: no memory for Tss");
	m->tss->iomap = 0xDFFF;
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
	{
		dbghex("kaddr pa ", pa);
		dbghex("kaddr caller ", (uvlong)getcallerpc(&pa));
		panic("kaddr: pa=%#p pc=%#p", pa, getcallerpc(&pa));
	}

	return (void*)hhdm_virt(pa);
}

uintptr
paddr(void *v)
{
	uintptr va;

	va = (uintptr)v;
	/* Kernel addresses at KZERO (0xffffffff80000000) */
	if(va >= KZERO)
		return virt2phys(v);
	/* HHDM addresses - Limine maps all physical memory here */
	if(is_hhdm_va(va))
		return hhdm_phys(va);
	if(va >= VMAP)
		return va-VMAP;
	panic("paddr: va=%#p pc=%#p", va, getcallerpc(&v));
}

static MMU*
mmualloc(void)
{
	MMU *p;

	p = m->mmufree;
	if(p != nil){
		m->mmufree = p->next;
		m->mmucount--;
		p->next = nil;
		return p;
	}

	p = mallocz(sizeof(MMU), 1);
	if(p == nil)
		return nil;
	p->page = mallocalign(PTSZ, BY2PG, 0, 0);
	if(p->page == nil){
		free(p);
		return nil;
	}
	/* Zero the page table page to ensure clean entries */
	memset(p->page, 0, PTSZ);
	return p;
}

static uintptr*
mmucreate(uintptr *table, uintptr va, int level, int index)
{
	uintptr *page, flags;
	MMU *p;

	flags = PTEWRITE|PTEVALID;
	if(va < VMAP){
		if(up == nil){
			page = rampage();
			goto make_entry;
		}
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
make_entry:
	/* Zero the page table page first */
	memset(page, 0, PTSZ);
	
	/* For intermediate page tables, pre-initialize the target entry */
	if(flags & PTEUSER && va < USTKTOP && level > 0) {
		int target_index = 0;
		switch(level) {
		case PDPE: /* PDPT level - pre-init entry for PD */
			target_index = PTLX(va, 0);
			break;
		case PDE:  /* PD level - pre-init entry for PT */  
			target_index = PTLX(va, 0);
			break;
		default:
			target_index = -1;
			break;
		}
		if(target_index >= 0 && target_index < 512) {
			/* Pre-initialize entry as present but zero - will be filled by caller */
			((uintptr*)page)[target_index] = PTEVALID;  /* Mark present but zero data */
		}
	}
	
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
			table = (void*)hhdm_virt(pte);
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

	/* Get current PML4 from CR3 */
	__asm__ volatile("mov %%cr3, %0" : "=r"(cr3));
	oldpml4 = kaddr(PPN(cr3));

	/* Allocate a single writable page for our PML4 */
	newpml4 = rampage();
	if(newpml4 == nil)
		panic("copypagetables: out of memory");

	/* Copy all PML4 entries - these point to Limine's lower level tables */
	memmove(newpml4, oldpml4, PTSZ);

	/* Update m->pml4 to point to new writable PML4 */
	m->pml4 = newpml4;

	/* DON'T switch CR3 - just use Limine's page tables for now */
	/* Switching CR3 causes triple fault - needs more investigation */
	/* cr3 = paddr(newpml4); */
	/* __asm__ volatile("mov %0, %%cr3" : : "r"(cr3)); */
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
	if(size <= 0)
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
	MMU *next, *p;

	for(p = proc->mmuhead; p != nil; p = next){
		next = p->next;
		if(m->mmucount < MMUFREELIMIT){
			p->next = m->mmufree;
			m->mmufree = p;
			m->mmucount++;
			continue;
		}
		free(p->page);
		free(p);
	}
	proc->mmuhead = proc->mmutail = nil;
	proc->mmucount = 0;
}

void
flushmmu(void)
{
	int x;

	x = splhi();
	up->newtlb = 1;
	mmuswitch(up);
	splx(x);
}

void
mmuswitch(Proc *proc)
{
	MMU *p;


	mmuzap();
	if(proc->newtlb){
		mmufree(proc);
		proc->newtlb = 0;
	}

	/* For kernel processes, there are no user MMU structures */
	if(proc->kp){
		taskswitch((uintptr)proc);
		return;
	}

	if((p = proc->kmaphead) != nil){
		m->pml4[PTLX(KMAP, 3)] = PADDR(p->page) | PTEWRITE|PTEVALID;
	}
	
	/* Process all PML4E entries in the linked list */
	for(p = proc->mmuhead; p != nil && p->level == PML4E; p = p->next){
		m->mmumap[p->index/MAPBITS] |= 1ull<<(p->index%MAPBITS);
		m->pml4[p->index] = PADDR(p->page) | PTEUSER|PTEWRITE|PTEVALID;
	}
	
	taskswitch((uintptr)proc);
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
	uintptr pa, va;

	/* Pure HHDM model: all physical memory already mapped via HHDM */
	pa = page->pa;
	va = hhdm_virt(pa);
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
	va = hhdm_virt(pa);

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

void
preallocpages(void)
{
	Confmem *cm;
	uintptr base, top;
	vlong psize;
	ulong np;
	int i;

	np = 0;
	for(i = 0; i < nelem(conf.mem); i++){
		cm = &conf.mem[i];
		if(cm->npage == 0)
			continue;
		np += cm->npage - nkpages(cm);
	}
	if(np == 0)
		return;

	psize = (uvlong)np * sizeof(Page);
	psize = ROUND(psize, PGLSZ(1));

	for(i = 0; i < nelem(conf.mem); i++){
		cm = &conf.mem[i];
		if(cm->npage == 0)
			continue;
		base = cm->base;
		top = base + (uvlong)cm->npage * BY2PG;
		base += (uvlong)nkpages(cm) * BY2PG;
		top &= -PGLSZ(1);
		if(top <= VMAPSIZE && (vlong)(top - base) >= psize){
			top -= psize;
			cm->npage = (top - cm->base) / BY2PG;
			palloc.pages = (Page*)hhdm_virt(top);
			break;
		}
	}
	if(palloc.pages == nil)
		panic("preallocpages: insufficient memory for page array");
}
