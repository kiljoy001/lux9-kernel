/* Early boot initialization functions */
#include "u.h"
#include "portlib.h"
#include "mem.h"
#include "dat.h"
#include <stddef.h>
#include "fns.h"
#include "../../limine.h"

/* External symbols from entry.S */
extern struct limine_memmap_request *limine_memmap;
extern struct limine_hhdm_request *limine_hhdm;
extern struct limine_module_request *limine_module;
extern struct limine_kernel_address_request *limine_kernel_address;
extern uintptr limine_bootloader_info;

/* Global HHDM offset from Limine - used by kaddr()
 * Limine maps all physical memory starting at HHDM offset
 * Will be set from Limine response */
uintptr limine_hhdm_offset = 0;

/* Global kernel physical base from Limine */
u64int limine_kernel_phys_base = 0;

void
bootargsinit(void)
{
	/* Parse Limine kernel address response */
	if(limine_kernel_address && limine_kernel_address->response) {
		limine_kernel_phys_base = limine_kernel_address->response->physical_base;
	}

	/* Parse Limine HHDM response */
	if(limine_hhdm && limine_hhdm->response) {
		limine_hhdm_offset = limine_hhdm->response->offset;
	} else {
		/* Fallback if no response */
		limine_hhdm_offset = 0xffff800000000000UL;
	}

	/* Store HHDM offset in early boot memory before CR3 switch */
	extern uintptr saved_limine_hhdm_offset;

	/* Initialize hhdm_base for hhdm.h interface compatibility */
	extern uintptr hhdm_base;
	hhdm_base = limine_hhdm_offset;
	saved_limine_hhdm_offset = limine_hhdm_offset;
}

void
meminit0(void)
{
	struct limine_memmap_response *memmap_response;
	struct limine_memmap_entry *entry;
	u64int i, base, length;
	int nregions = 0;
	ulong totalpages = 0;

	/* Parse Limine memory map and set up conf.mem[] */
	if(limine_memmap && limine_memmap->response) {
		memmap_response = limine_memmap->response;

		/* Iterate through memory map entries */
		for(i = 0; i < memmap_response->entry_count && nregions < nelem(conf.mem); i++) {
			entry = memmap_response->entries[i];

			/* Only use usable memory (type 0) */
			if(entry->type != 0)  /* LIMINE_MEMMAP_USABLE */
				continue;

			base = entry->base;
			length = entry->length;

			/* Skip if too small (less than 1MB) */
			if(length < 1*1024*1024)
				continue;

			/* Set up this memory region */
			conf.mem[nregions].base = base;
			conf.mem[nregions].npage = length / BY2PG;
			conf.mem[nregions].kbase = 0;
			conf.mem[nregions].klimit = 0;

			totalpages += conf.mem[nregions].npage;
			nregions++;
		}
	}

	/* Fallback if no memory map */
	if(nregions == 0) {
		conf.mem[0].base = 1*1024*1024;
		conf.mem[0].npage = 64*1024;  /* 256MB */
		conf.mem[0].kbase = 0;
		conf.mem[0].klimit = 0;
		totalpages = conf.mem[0].npage;
	}

	/* Set global memory configuration */
	conf.npage = totalpages;
	conf.upages = totalpages / 2;  /* Reserve half for user */
	conf.nproc = 100;
	conf.nimage = 200;
	conf.nswap = 0;
	conf.nswppo = 0;
	conf.ialloc = 0;
}
