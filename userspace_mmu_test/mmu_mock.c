#include "kernel_mock.h"

// Mock allocptlock - just a dummy variable for lock/unlock macros
static int allocptlock_mock_dummy;

// Helper: allocate and zero a page table from reserved pool
enum {
	PT_POOL_TABLES = 512, // Arbitrary size for mock
	PT_ENTRIES_PER_TABLE = 512,
};

// Mock physical memory for page tables. Aligned to BY2PG (4KB).
static u64int cpu0pt_pool_mock[PT_POOL_TABLES * PT_ENTRIES_PER_TABLE] __attribute__((aligned(BY2PG)));
static u64int *next_pt_mock = nil;
static int pt_count_mock = 0;

u64int*
alloc_pt(void)
{
	u64int *pt = nil;
	int i;

	lock(&allocptlock_mock_dummy); // Mock lock
	if(next_pt_mock == nil){
		next_pt_mock = cpu0pt_pool_mock;  // Start of PT pool
		uartputs("alloc_pt: initialized mock pool\n", 29);
	}
	if(pt_count_mock < PT_POOL_TABLES){
		pt = next_pt_mock;
		next_pt_mock += PT_ENTRIES_PER_TABLE;  /* Each PT is 512 entries (4KB) */
		pt_count_mock++;
	}
	unlock(&allocptlock_mock_dummy); // Mock unlock

	if(pt == nil){
		pt = (u64int*)rampage(); // Use mock rampage
		if(pt == nil)
			panic("alloc_pt: rampage returned nil");
		uartputs("alloc_pt: mock pool exhausted, using mock rampage\n", 50);
	} else {
		// uartputs("alloc_pt: allocated mock page table from pool\n", 47); // Too verbose for test
	}

	/* Zero the page table */
	for(i = 0; i < PT_ENTRIES_PER_TABLE; i++)
		pt[i] = 0;

	return pt;
}

// From kernel/9front-pc64/mmu.c (simplified)
void*
kaddr(uintptr pa)
{
	if(saved_limine_hhdm_offset == 0)
		panic("kaddr: HHDM not initialized yet!");
	return (void*)hhdm_virt(pa);
}

// From kernel/9front-pc64/mmu.c (simplified)
uintptr
paddr(void *v)
{
	uintptr va;

	va = (uintptr)v;

	// HHDM addresses - direct physical mapping
	if(va >= hhdm_base && va < hhdm_base + (256ULL*GiB)) { // Max 256GB HHDM mapping in mock
		return va - hhdm_base;
	}

	// Kernel addresses at KZERO
	if(va >= KZERO) {
        // Corrected logic from original kernel/9front-pc64/mmu.c
        // Assumes entire kernel is mapped linearly starting from limine_kernel_phys_base
		return (va - KZERO) + limine_kernel_phys_base;
	}

	// VMAP addresses - not used in this simplified mock
	// if(va >= VMAP)
	//	return va - VMAP;

	panic("paddr: va=%#llx", (uvlong)va);
    return 0; // Should not reach here
}

// From kernel/9front-pc64/mmu.c
MMU*
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

	/* Allocate a mock physical page and record its host VA */
	uintptr mock_pa = rampage();
	void *host_va = lookup_mock_host_va(mock_pa);
	if(host_va == nil){
		free(p);
		return nil;
	}

	p->alloc = host_va;
	p->page = (uintptr*)ROUND((uintptr)host_va, BY2PG);
	p->page_mock_pa = mock_pa;
	/* Zero the page table page to ensure clean entries */
	memset(p->page, 0, PTSZ);
	return p;
}


// From kernel/9front-pc64/mmu.c (simplified)
uintptr*
mmucreate(uintptr *table, uintptr va, int level, int index)
{
	uintptr *page;
    ulong flags = PTEWRITE | PTEVALID;
	MMU *p;

    // For userspace testing, assume user flags unless it's a kernel internal page table
    flags |= PTEUSER;
    
    // Mock m->mmucount and m->mmufree as global for now
    if (m->mmucount >= 0) { // Simulate a free list
        p = mmualloc();
    } else { // Fallback to raw alloc if free list logic not mocked
        p = mallocz(sizeof(MMU), 1);
        if (p == nil) return nil;
        p->alloc = mallocz(PTSZ + BY2PG, 1);
        if (p->alloc == nil) { free(p); return nil; }
        p->page = (uintptr*)ROUND((uintptr)p->alloc, BY2PG);
        memset(p->page, 0, PTSZ);
    }
    
    if (p == nil) return nil;
    
    p->index = index;
    p->level = level;
	page = p->page;

	// Mock mmuhead/mmutail for the current process (up)
	// Assuming 'up' is the process currently having its page tables created
	if(level == PML4E){ // Top level
        if(up->mmuhead == nil)
            up->mmutail = p;
        p->next = (MMU*)up->mmuhead; // Cast to MMU*
        up->mmuhead = p;
    }else{ // Lower levels
        if(up->mmutail != nil)
            ((MMU*)up->mmutail)->next = p; // Cast to MMU*
        up->mmutail = p;
        p->next = nil;
    }
    up->mmucount++;
    
	/* Store mock physical address in the PTE */
	table[index] = p->page_mock_pa | flags;
	
	// uartprintf("mmucreate: va=%#llx level=%d index=%d flags=%#llx entry=%#llx page=%#p\n",
	// 	(uvlong)va, level, index, (uvlong)flags, (uvlong)table[index], page); // Verbose debug print
	return page;
}

// From kernel/9front-pc64/mmu.c
uintptr*
mmuwalk(uintptr *table, uintptr va, int level, int create)
{
	uintptr pte_val;
	int i, x;

    // For loop for levels, highest (PML4E) down to desired 'level'
	for(i = PML4E; i >= level; i--){
		x = PTLX(va, i); // Get index for current level
		uartprintf("mmuwalk: Iteration %d, va=%#llx, level_idx=%d, current_table=%#p\n",
			i, (uvlong)va, x, table);
		pte_val = table[x];

        if(pte_val & PTEVALID){ // Entry is valid
            if((pte_val & PTESIZE) && (i > PTE_LEVEL)) { // If it's a huge page and not at PT level
                // Cannot walk further through huge page to get to a 4KB PTE
                // This means the huge page itself is the target if level matches, or failure if not.
                if (i == level) {
                    return &table[x]; // Found the entry at target level
                }
                return nil; // Cannot resolve 4KB PTE if huge page is mapped at higher level
            }
            pte_val = PPN(pte_val); // Get physical address part
            uartprintf("mmuwalk: Valid PTE, pa_part=%#llx\n", (uvlong)pte_val);
            /* Translate mock physical to host VA to dereference the table */
            table = (uintptr*)get_host_va_from_mock_pa(pte_val);
            uartprintf("mmuwalk: Next_table_addr (host ptr) = %#p\n", table);
        } else { // Entry is not valid
            if(!create)
                return nil; // Don't create, return null
			
			// Create a new page table for the next level
			table = mmucreate(table, va, i, x);
			if(table == nil)
				return nil; // Creation failed
			uartprintf("mmuwalk: Created new table (level %d, idx %d), new_table_addr=%#p\n", i, x, table);

            // After creation, the current 'table' now points to the newly created page table,
            // so we continue the loop, getting the index for the next iteration correctly.
		}
	}
	return &table[x]; // Return pointer to PTE at the desired level
}


// From kernel/9front-pc64/mmu.c
uintptr*
getpte(uintptr va)
{
	uintptr *pte;

	// Simplified: always create missing entries for mock
	if((pte = mmuwalk(m->pml4, va, PTE_LEVEL, 1)) == nil){ // Always create missing entries
		panic("getpte: out of mock MMU pages or cannot create PTE for va=%#llx", (uvlong)va);
	}
	return pte;
}

// From kernel/9front-pc64/mmu.c
void
pmap(uintptr pa, uintptr va, vlong size)
{
	uintptr *pte_ptr;
    ulong flags;
    uintptr current_va = va;
    uintptr current_pa = PPN(pa); // Start with aligned physical address
    vlong remaining_size = size;

	if(size <= 0)
		panic("pmap: pa=%#llx va=%#llx size=%lld: size must be positive", (uvlong)pa, (uvlong)va, size);
	
	flags = pa & (BY2PG-1); // Extract flags that were part of 'pa'
	flags |= PTEACCESSED | PTEDIRTY | PTEVALID; // Always set these for pmap
	flags |= PTEUSER; // Assume user mapping for this context

	// Loop through the range, mapping 4KB pages
	while(remaining_size > 0){
		pte_ptr = getpte(current_va); // Get or create PTE for current VA
		if(pte_ptr == nil){
			panic("pmap: failed to get or create PTE for va=%#llx", (uvlong)current_va);
		}
		
        *pte_ptr = current_pa | flags; // Set the PTE with physical address + flags
		uartprintf("pmap: Mapped VA=%#llx to PA=%#llx (PTE=%#llx)\n",
			(uvlong)current_va, (uvlong)current_pa, (uvlong)*pte_ptr);
		
		current_pa += BY2PG;
		current_va += BY2PG;
		remaining_size -= BY2PG;
	}
}

// From kernel/9front-pc64/mmu.c (simplified)
void
punmap(uintptr va, vlong size)
{
	uintptr *pte_ptr;
    uintptr current_va = PPN(va); // Align to page boundary
    vlong remaining_size = size;

	while(remaining_size > 0){
		pte_ptr = mmuwalk(m->pml4, current_va, PTE_LEVEL, 0); // Do not create missing
		if(pte_ptr){
			*pte_ptr = 0; // Clear the PTE
			// invlpg(current_va); // No-op in userspace mock
		}
		current_va += BY2PG;
		remaining_size -= BY2PG;
	}
}

/* Mocked mmuzap/mmuswitch/mmufree to mirror kernel behavior loosely */
void
mmuzap(Proc *proc)
{
	MMU *p;

	if(proc == nil || proc->pml4 == nil)
		return;

	for(p = proc->mmuhead; p != nil; p = p->next){
		if(p->level == PML4E)
			proc->pml4[p->index] = 0;
	}
}

void
mmuswitch(Proc *proc)
{
	MMU *p;

	if(proc == nil || proc->kp)
		return;

	mmuzap(proc);
	for(p = proc->mmuhead; p != nil; p = p->next){
		if(p->level == PML4E)
			proc->pml4[p->index] = p->page_mock_pa | PTEUSER | PTEWRITE | PTEVALID;
	}
	m->pml4 = proc->pml4;
	m->pml4_pa = proc->pml4_pa;
}

void
mmufree(Proc *proc)
{
	MMU *p, *next;

	if(proc == nil)
		return;
	/* Clear root PML4 entries as well */
	mmuzap(proc);
	for(p = proc->mmuhead; p != nil; p = next){
		next = p->next;
		/* Free host backing for this table and drop mock phys map */
		if(p->page_mock_pa)
			free_mock_phys(p->page_mock_pa);
		free(p);
	}
	proc->mmuhead = proc->mmutail = nil;
	proc->mmucount = 0;
}

/* Minimal fault handler: map a user page on demand */
int
fault(uintptr addr, uintptr pc, int read)
{
	(void)pc; (void)read;

	setcr2(addr);
	if(addr >= USTKTOP)
		return -1;

	uintptr *pte = mmuwalk(m->pml4, addr, PTE_LEVEL, 0);
	if(pte != nil && (*pte & PTEVALID))
		return 0;

	Page *p = newpage(addr, nil);
	if(p == nil)
		return -1;
	pmap(p->pa | PTEWRITE | PTEUSER, PPN(addr), BY2PG);
	/* We keep p allocated; caller can free if desired */
	return 0;
}
