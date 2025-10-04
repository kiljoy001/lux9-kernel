/* Early boot initialization functions */
#include "u.h"
#include "portlib.h"
#include "mem.h"
#include "dat.h"
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

/* External symbols from entry.S */
extern uintptr limine_memmap;
extern struct limine_hhdm_request *limine_hhdm;
extern struct limine_module_request *limine_module;
extern uintptr limine_bootloader_info;

/* Global HHDM offset from Limine - used by kaddr()
 * Limine maps all physical memory starting at HHDM offset
 * Limine typically uses 0xffff800000000000 on x86-64 */
uintptr limine_hhdm_offset = 0xffff800000000000UL;

void
bootargsinit(void)
{
	/* HHDM offset already hardcoded - nothing to do here */
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
	/* Early memory initialization */
	/* TODO: Parse Limine memory map and set up conf.mem[] */

	/* For now, assume we have some memory */
	conf.npage = 64*1024;  /* 256MB at 4KB pages */
	conf.upages = conf.npage / 2;  /* Reserve half for user, half for kernel */
	conf.nproc = 100;
	conf.nimage = 200;
	conf.nswap = 0;
	conf.nswppo = 0;
	conf.ialloc = 0;

	/* Set up first memory region */
	conf.mem[0].base = 1*1024*1024;  /* Start at 1MB */
	conf.mem[0].npage = conf.npage;
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
