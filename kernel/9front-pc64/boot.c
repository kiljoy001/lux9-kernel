/* Early boot initialization functions */
#include "u.h"
#include "portlib.h"
#include "mem.h"
#include "dat.h"
#include <stddef.h>
#include "fns.h"

/* Limine structures */
struct limine_hhdm_response {
	u64int revision;
	u64int offset;
};

struct limine_hhdm_request {
	u64int id[4];
	u64int revision;
	struct limine_hhdm_response *response;
};

struct limine_memmap_entry {
	u64int base;
	u64int length;
	u64int type;
	u32int unused;
};

struct limine_memmap_response {
	u64int revision;
	u64int entry_count;
	struct limine_memmap_entry **entries;
};

struct limine_memmap_request {
	u64int id[4];
	u64int revision;
	struct limine_memmap_response *response;
};

struct limine_file {
	u64int revision;
	void *address;
	u64int size;
	char *path;
	char *cmdline;
	u32int media_type;
	u32int unused;
	u32int tftp_ip;
	u32int tftp_port;
	u32int partition_index;
	u32int mbr_disk_id;
	void *gpt_disk_uuid;
	void *gpt_part_uuid;
	void *part_uuid;
};

struct limine_module_response {
	u64int revision;
	u64int module_count;
	struct limine_file **modules;
};

struct limine_module_request {
	u64int id[4];
	u64int revision;
	struct limine_module_response *response;
	u64int internal_module_count;
	struct limine_internal_module **internal_modules;
};

struct limine_kernel_address_response {
	u64int revision;
	u64int physical_base;
	u64int virtual_base;
};

struct limine_kernel_address_request {
	u64int id[4];
	u64int revision;
	struct limine_kernel_address_response *response;
};

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
		__asm__ volatile("outb %0, %1" : : "a"((char)'K'), "Nd"((unsigned short)0x3F8));
		__asm__ volatile("outb %0, %1" : : "a"((char)'A'), "Nd"((unsigned short)0x3F8));
	}

	/* Parse Limine HHDM response */
	if(limine_hhdm && limine_hhdm->response) {
		limine_hhdm_offset = limine_hhdm->response->offset;
		__asm__ volatile("outb %0, %1" : : "a"((char)'H'), "Nd"((unsigned short)0x3F8));
	} else {
		/* Fallback if no response */
		limine_hhdm_offset = 0xffff800000000000UL;
		__asm__ volatile("outb %0, %1" : : "a"((char)'F'), "Nd"((unsigned short)0x3F8));
	}

	/* Store HHDM offset in early boot memory before CR3 switch */
	extern uintptr saved_limine_hhdm_offset;
	saved_limine_hhdm_offset = limine_hhdm_offset;
	__asm__ volatile("outb %0, %1" : : "a"((char)'S'), "Nd"((unsigned short)0x3F8));

	__asm__ volatile("outb %0, %1" : : "a"((char)'B'), "Nd"((unsigned short)0x3F8));
}

void
ioinit(void)
{
	/* I/O port permissions - we're kernel, we have access */
	/* This would set up IOPB in TSS if needed */
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

void
archinit(void)
{
	/* Architecture-specific initialization */
	/* Set up arch structure */
	arch->id = "Lux9 PC64";
	arch->ident = nil;
	arch->reset = nil;
	arch->intrinit = nil;
	arch->clockinit = nil;
	arch->clockenable = nil;
}

