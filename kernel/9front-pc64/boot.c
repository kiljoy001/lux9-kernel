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
	struct limine_memmap_response *memmap_response;
	struct limine_memmap_entry *entry;
	u64int i, max_addr;
	extern u64int MemMin;

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

	/* Initialize MemMin from Limine memory map
	 * MemMin indicates the end of the initially mapped physical memory
	 * With Limine HHDM, all physical memory is mapped, so we set MemMin
	 * to the highest usable physical address (limited to 4GB for cankaddr) */
	extern void uartputs(char*, int);
	max_addr = 0;
	uartputs("bootargsinit: checking limine_memmap\n", 38);
	if(limine_memmap && limine_memmap->response) {
		memmap_response = limine_memmap->response;
		uartputs("bootargsinit: memmap response valid\n", 37);
		for(i = 0; i < memmap_response->entry_count; i++) {
			entry = memmap_response->entries[i];
			/* HHDM maps ALL physical memory - check all entry types
			 * This includes usable RAM, modules (initrd), MMIO, reserved regions, etc. */
			u64int end = entry->base + entry->length;
			if(end > max_addr)
				max_addr = end;
		}
	} else {
		uartputs("bootargsinit: ERROR - memmap is NULL!\n", 39);
	}

	/* Set MemMin - use the actual max RAM address we found
	 * Now supports full 64-bit address range for modern systems */
	if(max_addr == 0) {
		uartputs("bootargsinit: WARNING - max_addr is 0, using fallback\n", 56);
		max_addr = 4ULL*1024*1024*1024;  /* 4GB fallback */
	}

	MemMin = max_addr;
	uartputs("bootargsinit: MemMin set\n", 26);
}

/* meminit0() provided by memory_9front.c - it handles memory discovery */
