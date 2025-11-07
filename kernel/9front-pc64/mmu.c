#include	"u.h"
#include <lib.h>
#include	"mem.h"

#include	"fns.h"
#include	"hhdm.h"
#include	"borrowchecker.h"

extern void uartputs(char*, int);

/* CR3 switch memory system functions */
extern int memory_system_ready_before_cr3(void);
extern void transfer_bootloader_to_kernel(void);
extern void pre_init_memory_system(void);

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

/* Unified HHDM implementation moved to include/hhdm.h */




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
[UDSEG]		DATA32SEGM(3),		/* user data/stack 32 bit */
[UESEG]		EXECSEGM(3),		/* user code 64 bit */
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
static Lock allocptlock;

static u64int*
alloc_pt(void)
{
	extern u64int cpu0pt_pool[];  /* Reserved in linker.ld */
	u64int *pt;
	int i;

	lock(&allocptlock);
	if(next_pt == nil){
		next_pt = cpu0pt_pool;  /* Start of PT pool */
		uartputs("alloc_pt: initialized pool\n", 27);
	}
	if(pt_count >= 512)
		panic("alloc_pt: cpu0pt_pool exhausted");

	pt = next_pt;
	next_pt += 512;  /* Each PT is 512 entries (4KB) */
	pt_count++;
	unlock(&allocptlock);

	/* Zero the page table */
	for(i = 0; i < 512; i++)
		pt[i] = 0;

	uartputs("alloc_pt: allocated page table\n", 31);
	return pt;
}

/* Helper: get physical address
 * Option A: Linear mapping with small Limine offset for kernel addresses
 * This is used during page table setup while still on Limine's page tables */
static u64int
virt2phys(void *virt)
{
	extern u64int limine_kernel_phys_base;
	uintptr va = (uintptr)virt;

	/* Kernel addresses at KZERO - need offset since Limine loaded us arbitrarily */
	if(va >= KZERO)
		return (va - KZERO) + limine_kernel_phys_base;

	/* VMAP addresses - simple subtraction like 9front */
	if(va >= VMAP)
		return va - VMAP;

	/* Already physical */
	return va;
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

	dbghex("map_range_2mb: virt_start (before align) = ", virt_start);

	virt_start &= ~0x1FFFFFULL;  /* Align to 2MB */
	phys_start &= ~0x1FFFFFULL;
	size = (size + 0x1FFFFF) & ~0x1FFFFFULL;

	dbghex("map_range_2mb: virt_start (after align) = ", virt_start);

	virt_end = virt_start + size;

	/* Special check: always print for debugging */
	if(virt_start == 0xffffffffffe00000ULL){
		uartputs("!!! EXACT MATCH 0xffffffffffe00000 !!!\n", 40);
		if(virt_end == 0){
			uartputs("!!! virt_end == 0, WRAPAROUND CONFIRMED !!!\n", 45);
		}
	}

	/* Handle address space wrap: if virt_end wrapped to a small value, we're mapping to end of address space */
	int wraps = (virt_end < virt_start);
	if(wraps){
		uartputs("map_range_2mb: *** WRAPAROUND DETECTED! ***\n", 44);
	}

	/* Debug first iteration */
	uartputs("map_range_2mb: about to enter loop\n", 35);
	int first_iter = 1;

	for(virt = virt_start, phys = phys_start; wraps ? (virt >= virt_start) : (virt < virt_end); virt += 2*MiB, phys += 2*MiB) {
		if(first_iter){
			first_iter = 0;
			dbghex("map_range_2mb: virt (loop var) = ", virt);
		}

		/* Calculate indices */
		pml4_idx = (virt >> 39) & 0x1FF;
		pdp_idx = (virt >> 30) & 0x1FF;
		pd_idx = (virt >> 21) & 0x1FF;

		if(first_iter == 0){
			dbghex("map_range_2mb: pml4_idx = ", pml4_idx);
			first_iter = -1;
		}

		/* Get or create PDP */
		if((pml4[pml4_idx] & PTEVALID) == 0) {
			pdp = alloc_pt();
			pdp_phys = virt2phys(pdp);
			pml4[pml4_idx] = pdp_phys | PTEVALID | PTEWRITE | PTEACCESSED;
		} else {
			pdp_phys = pml4[pml4_idx] & ~0xFFF;
			pdp = (u64int*)hhdm_virt(pdp_phys);
		}

		/* Get or create PD */
		if((pdp[pdp_idx] & PTEVALID) == 0) {
			pd = alloc_pt();
			pd_phys = virt2phys(pd);
			pdp[pdp_idx] = pd_phys | PTEVALID | PTEWRITE | PTEACCESSED;
		} else {
			pd_phys = pdp[pdp_idx] & ~0xFFF;
			pd = (u64int*)hhdm_virt(pd_phys);
		}

		/* Map 2MB page directly in PD */
		pd[pd_idx] = phys | perms;  /* perms should include PTESIZE */
	}

	/* Debug: verify PML4 entries were actually set */
	uartputs("map_range_2mb: checking PML4 after mapping\n", 43);
	for(int idx = 0; idx < 512; idx++){
		if(pml4[idx] & PTEVALID){
			uartputs("  PML4 entry is valid\n", 24);
			break;
		}
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
			pml4[pml4_idx] = pdp_phys | PTEVALID | PTEWRITE | PTEACCESSED;
		} else {
			pdp_phys = pml4[pml4_idx] & ~0xFFF;
			/* Our page tables are in KZERO range, existing ones might be anywhere */
			pdp = (u64int*)hhdm_virt(pdp_phys);
		}

		/* Get or create PD */
		if((pdp[pdp_idx] & PTEVALID) == 0) {
			pd = alloc_pt();
			pd_phys = virt2phys(pd);
			pdp[pdp_idx] = pd_phys | PTEVALID | PTEWRITE | PTEACCESSED;
		} else {
			pd_phys = pdp[pdp_idx] & ~0xFFF;
			pd = (u64int*)hhdm_virt(pd_phys);
		}

		/* Get or create PT */
		if((pd[pd_idx] & PTEVALID) == 0) {
			pt = alloc_pt();
			pt_phys = virt2phys(pt);
			pd[pd_idx] = pt_phys | PTEVALID | PTEWRITE | PTEACCESSED;
		} else {
			pt_phys = pd[pd_idx] & ~0xFFF;
			pt = (u64int*)hhdm_virt(pt_phys);
		}

		/* Map 4KB page in PT */
		pt[pt_idx] = phys | perms;
	}
}

/* Setup our own page tables with linear RAM mapping at KZERO */

/* Pre-initialize memory system for CR3 switch */
void
pre_init_memory_system(void)
{
	uartputs("pre_init_memory_system: starting\n", 36);
	
	/* Initialize borrow pool if not done yet - needed for memory coordination */
	uartputs("pre_init_memory_system: checking borrow pool\n", 45);
	if(borrowpool.owners == nil) {
		uartputs("pre_init_memory_system: initializing borrow pool\n", 52);
		borrowinit();
		uartputs("pre_init_memory_system: borrow pool initialized\n", 48);
	} else {
		uartputs("pre_init_memory_system: borrow pool already initialized\n", 58);
	}
	
	/* Transfer memory coordination state from bootloader to kernel */
	/* This sets mem_coord.state = MEMORY_KERNEL_ACTIVE */
	uartputs("pre_init_memory_system: transferring memory coordination\n", 56);
	transfer_bootloader_to_kernel();
	uartputs("pre_init_memory_system: memory coordination transferred\n", 57);
	
	uartputs("pre_init_memory_system: complete\n", 33);
}
void
setuppagetables(void)
{
	u64int *pml4;
	u64int pml4_phys;
	int i;
	extern char end[];

	uartputs("setuppagetables: ENTRY\n", 23);

	/* Linker-provided symbols - declared in lib.h as char[] */


	/* Use OUR page tables from linker script - completely independent of Limine */
	extern u64int cpu0pml4[];

	uartputs("setuppagetables: setting pml4 = cpu0pml4\n", 42);
	pml4 = cpu0pml4;
	uartputs("setuppagetables: pml4 set\n", 26);
	uartputs("setuppagetables: about to set next_pt\n", 39);
	next_pt = nil;
	uartputs("setuppagetables: pt_count = 0\n", 31);
	pt_count = 0;
	uartputs("setuppagetables: calling virt2phys(pml4)\n", 41);
	pml4_phys = virt2phys(pml4);
	uartputs("setuppagetables: pml4_phys computed\n", 37);

	uartputs("setuppagetables: about to clear PML4\n", 37);
	/* Clear OUR PML4 to start fresh */
	for(i = 0; i < 512; i++)
		pml4[i] = 0;
	uartputs("setuppagetables: PML4 cleared\n", 30);

	/* Keep kernel at Limine load address - no relocation needed
	 * Limine loads us high (~2GB), and we can work with that
	 * Strategy: Map KZERO directly to the actual physical load address */
	extern u64int limine_kernel_phys_base;
	extern char ttext[], etext[], kend[];  /* Kernel boundaries from linker */

	u64int kernel_phys = limine_kernel_phys_base;  /* Keep at Limine load address */
	uintptr kernel_size = (uintptr)kend - KZERO;  /* Size of kernel in memory */

	uartputs("setuppagetables: using kernel at Limine load address\n", 53);
	dbghex("  kernel physical (Limine): ", kernel_phys);
	dbghex("  kernel size: ", kernel_size);

	/* Map kernel at KZERO to actual Limine load address */
	uartputs("setuppagetables: mapping KZERO to Limine load address\n", 52);

	/* Map kernel image directly */
	map_range(pml4, KZERO, kernel_phys, kernel_size, PTEVALID | PTEWRITE | PTEGLOBAL);
	uartputs("setuppagetables: KZERO mapped to kernel image\n", 47);

	/* Mirror kernel image into HHDM for memory functions */
	uartputs("setuppagetables: mirroring kernel in HHDM\n", 43);
	map_range(pml4, kaddr(kernel_phys), kernel_phys, kernel_size, PTEVALID | PTEWRITE | PTEGLOBAL);
	uartputs("setuppagetables: kernel mapping complete\n", 42);
	uartputs("setuppagetables: KZERO mapped to relocated kernel\n", 50);

	/* Ensure conf.mem is populated */
	uartputs("setuppagetables: calling ensure_phys_range\n", 45);
	ensure_phys_range();
	uartputs("setuppagetables: ensure_phys_range complete\n", 45);

	/* NOTE: We do NOT map high memory (>2GB physical) at KZERO because:
	 * 1. KZERO only has 2GB of virtual address space before wrapping
	 * 2. High memory access should use HHDM instead
	 * 3. Dynamic page mapping uses VMAP, not KZERO */

	/* HHDM provides access to low memory (0-8MB) for firmware/BIOS access
	 * No separate identity mapping needed - HHDM handles all physical memory */
	uartputs("setuppagetables: using HHDM for low memory access\n", 52);

	/* Setup HHDM mapping for all physical memory access */
	uartputs("setuppagetables: setting up HHDM mapping\n", 45);
	
	/* Simple HHDM setup - covers all physical memory */
	u64int hhdm_start = saved_limine_hhdm_offset;
	u64int hhdm_end = hhdm_start + max_physaddr;
	
	/* Map only the actual physical memory we have, not a huge range */
	/* Map in 2MB chunks to avoid excessive page table allocation */
	for(u64int pa = 0; pa < max_physaddr; pa += PGLSZ(2)) {
		u64int va = hhdm_start + pa;
		u64int size = PGLSZ(2);  /* 2MB chunks */
		if(pa + size > max_physaddr) size = max_physaddr - pa;
		
		/* Create 2MB mappings where possible for efficiency */
		map_range_2mb(pml4, va, pa, size, PTEVALID | PTEWRITE | PTEGLOBAL | PTESIZE | PTEACCESSED);
	}
	uartputs("setuppagetables: HHDM mapping complete\n", 42);

	/* Verify our current code location is mapped before switching */
	uintptr current_rip;
	__asm__ volatile("lea (%%rip), %0" : "=r"(current_rip));
	uartputs("setuppagetables: current RIP check\n", 36);

	/* Check if RIP is in KZERO range */
	if(current_rip >= KZERO) {
		uartputs("setuppagetables: RIP is in KZERO range (good)\n", 48);
	} else {
		uartputs("setuppagetables: RIP NOT in KZERO range (bad!)\n", 49);
	}

	/* Debug: Check PML4 entries before switch */
	uartputs("setuppagetables: checking PML4 entries\n", 40);
	if(pml4[0] & PTEVALID)
		uartputs("  PML4[0] (identity 0-8MB) is valid\n", 37);
	if(pml4[511] & PTEVALID)
		uartputs("  PML4[511] (KZERO) is valid\n", 30);
	uartputs("  No HHDM mapping (kernel uses VMAP)\n", 38);

	/* Simplified CR3 switch - HHDM ensures accessibility */
	uartputs("setuppagetables: preparing simple CR3 switch\n", 50);

	/* Check if memory system is ready before CR3 switch */
	if(!memory_system_ready_before_cr3()) {
		/* Transfer memory ownership from bootloader to kernel */
		transfer_bootloader_to_kernel();
		/* Initialize memory system for CR3 switch */
		pre_init_memory_system();
		uartputs("setuppagetables: memory system ready for CR3 switch\n", 50);
	} else {
		uartputs("setuppagetables: memory system already ready\n", 47);
	}

	/* Simple, direct CR3 switch - HHDM ensures both old and new code are accessible */
	uartputs("setuppagetables: switching to kernel page tables\n", 50);
	__asm__ volatile(
		"mov %0, %%cr3" 
		: : "r"(pml4_phys) 
		: "memory"
	);

	/* Update kernel's page table pointer */
	m->pml4 = pml4;
	uartputs("setuppagetables: CR3 switch complete\n", 40);

	/* Direct call to continue boot - no trampoline needed */
	uartputs("setuppagetables: calling main_after_cr3\n", 44);
	main_after_cr3();

	/* Should never reach here */
	panic("setuppagetables: main_after_cr3 returned unexpectedly");
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
	
	// Debug print for EFER
	dbghex("EFER set to: ", v);

	wrmsr(Star, ((uvlong)UESEL << 48) | ((uvlong)KESEL << 32));
	wrmsr(Lstar, (uvlong)syscallentry);
	wrmsr(Sfmask, 0x200);
	
	// Debug print for STAR
	uvlong star_val;
	rdmsr(Star, &star_val);
	dbghex("STAR set to: ", star_val);
}

/*
 * These could go back to being macros once the kernel is debugged,
 * but the extra checking is nice to have.
 */

void*
kaddr(uintptr pa)
{
	if(saved_limine_hhdm_offset == 0)
		panic("kaddr: HHDM not initialized yet!");
	return (void*)hhdm_virt(pa);
}

uintptr
paddr(void *v)
{
	/* Handle multiple address spaces:
	 * HHDM, KZERO (kernel), and VMAP (virtual mappings) */
	extern u64int limine_kernel_phys_base;
	extern uintptr hhdm_base;
	extern char end[];
	uintptr va;

	va = (uintptr)v;

	/* HHDM addresses - direct physical mapping */
	if(va >= hhdm_base && va < hhdm_base + (256ULL*GiB)) {
		return va - hhdm_base;
	}

	/* Kernel addresses at KZERO - kernel was relocated to physical 2MB */
	if(va >= KZERO && va < (uintptr)end) {
		/* Inside kernel image: kernel is now at physical 2MB (relocated from Limine's location) */
		return (va - KZERO) + (2*MiB);
	} else if(va >= KZERO) {
		/* Outside kernel but >= KZERO: simple linear mapping */
		return va - KZERO;
	}

	/* VMAP addresses - exact 9front style */
	if(va >= VMAP)
		return va - VMAP;

	/* Neither HHDM, KZERO, nor VMAP - panic like 9front does */
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
	p->alloc = mallocz(PTSZ + BY2PG, 1);
	if(p->alloc == nil){
		free(p);
		return nil;
	}
	p->page = (uintptr*)ROUND((uintptr)p->alloc, BY2PG);
	/* Zero the page table page to ensure clean entries */
	memset(p->page, 0, PTSZ);
	return p;
}

/*
 * pt_page - Allocate a page for page tables from the palloc pool
 * Returns HHDM virtual address of the page, or nil if none available
 */
static uintptr*
mmucreate(uintptr *table, uintptr va, int level, int index)
{
	uintptr *page, flags;
	MMU *p;
	extern uintptr hhdm_base;

	flags = PTEWRITE|PTEVALID;
	/* Check if this is a user/kmap address that needs process tracking
	 * Exclude HHDM addresses which are kernel direct-map and don't need up */
	if(va < VMAP && (va < hhdm_base || va >= hhdm_base + (256ULL*GiB))){
		if(up == nil)
			panic("mmucreate: up nil va=%#p", va);
		if((p = mmualloc()) == nil)
			return nil;
		p->index = index;
		p->level = level;
		page = p->page;
		memset(page, 0, PTSZ);
		if(va < USTKTOP){
			flags |= PTEUSER;
			if(level == PML4E){
				if((p->next = up->mmuhead) == nil)
					up->mmutail = p;
				up->mmuhead = p;
				m->mmumap[index/MAPBITS] |= 1ull<<(index%MAPBITS);
			}else{
				if(up->mmutail != nil)
					up->mmutail->next = p;
				up->mmutail = p;
			}
			up->mmucount++;
		}else{
			if(level == PML4E){
				up->kmaphead = p;
				up->kmaptail = p;
			}else{
				if(up->kmaptail != nil)
					up->kmaptail->next = p;
				up->kmaptail = p;
			}
			up->kmapcount++;
		}
	}else{
		page = rampage();
		if(page == nil)
			return nil;
		memset(page, 0, PTSZ);
	}
	table[index] = PADDR(page) | flags;
	print("mmucreate: va=%#p level=%d index=%d flags=%#llux entry=%#llux page=%#p\n",
		va, level, index, (uvlong)flags, (uvlong)table[index], page);
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
	extern char ttext[], etext[], kend[];
	uintptr *pte, psz, va;
	uintptr kernel_end;
	uintptr *active_pml4;

	/* CRITICAL FIX: Only process kernel image, not entire address space!
	 * Original bug: loop continued until va wrapped to 0, corrupting HHDM and other regions
	 * Use kend (end of .rodata) not end (end of .data which comes before .text!) */
	kernel_end = PGROUND((uintptr)kend) + 16*MiB;  /* Kernel + reasonable margin */

	/* Get the ACTIVE page tables from CR3, not m->pml4 which is unused */
	active_pml4 = (uintptr*)getcr3();
	active_pml4 = (uintptr*)kaddr((uintptr)active_pml4);  /* Convert physical to virtual */

	print("kernelro: ttext=%#p etext=%#p kend=%#p kernel_end=%#p\n",
	      ttext, etext, kend, kernel_end);
	print("kernelro: active CR3=%#p (using Limine's page tables)\n", active_pml4);
	print("kernelro: m->havenx=%d PTENOEXEC=%#llux\n", m->havenx, PTENOEXEC);

	ptesplit(active_pml4, APBOOTSTRAP);
	ptesplit(active_pml4, KTZERO);
	ptesplit(active_pml4, (uintptr)ttext);
	ptesplit(active_pml4, (uintptr)etext-1);

	/* Now we can modify PTEs - our page tables are writable! */
	uintptr text_pages = 0, noexec_pages = 0;
	for(va = KZERO; va < kernel_end; va += psz){
		psz = PGLSZ(0);
		pte = mmuwalk(active_pml4, va, 0, 0);
		if(pte == nil){
			if(va & PGLSZ(1)-1)
				continue;
			pte = mmuwalk(active_pml4, va, 1, 0);
			if(pte == nil)
				continue;
			psz = PGLSZ(1);
		}
		if((*pte & PTEVALID) == 0)
			continue;
		if(va >= (uintptr)ttext && va < (uintptr)etext){
			uvlong old_pte = *pte;
			*pte &= ~PTEWRITE;  /* Make text section read-only */
			if(text_pages < 3 || va == 0xffffffff802e8000){  /* Debug first few and todinit page */
				print("  text page va=%#p: pte %#llux -> %#llux\n", va, old_pte, *pte);
			}
			text_pages++;
			invlpg(va);
		}
		/* TEMPORARY: Skip PTENOEXEC to isolate the issue */
		/*
		else if(va != (APBOOTSTRAP & -BY2PG)){
			*pte |= PTENOEXEC;
			noexec_pages++;
			invlpg(va);
		}
		*/
	}
	print("kernelro: marked %lud text pages R-X, %lud data pages RW-\n",
	      text_pages, noexec_pages);
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
		free(p->alloc);
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
	va = (uintptr)hhdm_virt(pa);
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

	/* Validate size */
	if(size <= 0){
		print("vmap pa=%llux size=%lld pc=%#p\n", pa, size, getcallerpc(&pa));
		return nil;
	}

	/*
	 * All physical addresses map through HHDM, including PCI MMIO regions.
	 * The HHDM maps the entire physical address space.
	 */
	if(pa < BY2PG){
		print("vmap: invalid low pa=%llux size=%lld pc=%#p\n", pa, size, getcallerpc(&pa));
		return nil;
	}
	va = (uintptr)hhdm_virt(pa);

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
