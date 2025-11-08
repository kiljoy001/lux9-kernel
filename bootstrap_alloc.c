/* BOOTSTRAP ALLOCATOR - Eliminates malloc dependency during early boot */

typedef struct BootHole BootHole;
struct BootHole {
	BootHole* link;
	uintptr addr;
	uintptr size;
	uintptr top;
};

static BootHole bootstrap_pool[64];  // Bootstrap pool (no external deps)
static int bootstrap_next = 0;
static BootHole* bootstrap_flist = nil;

static BootHole*
bootstrap_alloc_hole(void)
{
	// Try bootstrap pool first
	if(bootstrap_next < 64) {
		return &bootstrap_pool[bootstrap_next++];
	}
	
	// Try expanded pool
	if(bootstrap_flist) {
		BootHole* hole = bootstrap_flist;
		bootstrap_flist = hole->link;
		return hole;
	}
	
	// Need to expand pool - use page-based allocation
	uvlong pa = memmapalloc(-1, BY2PG, BY2PG, MemRAM);
	if(pa == -1) return nil;  // Expansion failed
	
	// Convert to virtual HHDM address and link
	BootHole* new_pool = (BootHole*)hhdm_virt(pa);
	new_pool[0].link = bootstrap_flist;
	bootstrap_flist = &new_pool[0];
	
	return &new_pool[0];  // Return first descriptor
}