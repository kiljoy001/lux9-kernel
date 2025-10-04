OUTPUT_FORMAT(elf64-x86-64)
OUTPUT_ARCH(i386:x86-64)

ENTRY(_start)

PHDRS
{
    requests PT_LOAD    FLAGS(6);   /* Write + Read - for Limine */
    text     PT_LOAD    FLAGS(5);   /* Execute + Read */
    rodata   PT_LOAD    FLAGS(4);   /* Read only */
    data     PT_LOAD    FLAGS(6);   /* Write + Read */
}

SECTIONS
{
    /* Kernel starts in higher half */
    . = 0xffffffff80000000;

    /* Limine requests at start */
    .limine_requests : {
        KEEP(*(.limine_base_revision))
        KEEP(*(.limine_requests_start))
        KEEP(*(.limine_requests))
        KEEP(*(.limine_requests_end))
    } :requests

    . = ALIGN(CONSTANT(MAXPAGESIZE));

    /* Reserve space for ALL bootstrap structures at fixed offsets from KZERO */
    . = 0xffffffff80001000;  /* Start after first page */
    .cpu0_data : {
        . = 0x200;   /* CONFADDR at KZERO+0x1200 */
        confaddr = .;
        . += 0x1000;

        . = 0x6000;  /* APBOOTSTRAP at KZERO+0x7000 */
        apbootstrap = .;
        . += 0x1000;

        . = 0xF000;  /* IDTADDR at KZERO+0x10000 */
        idtaddr = .;
        . += 0x3000;  /* IDT is 512 entries * 16 bytes = 8KB, round to 12KB */

        . = 0x12000;  /* CPU0PML4 at KZERO+0x13000 */
        cpu0pml4 = .;
        . += 0x1000;
        cpu0pdp = .;
        . += 0x1000;
        cpu0pd0 = .;
        . += 0x1000;
        cpu0pd1 = .;
        . += 0x1000;
        cpu0gdt = .;
        . += 0x1000;
        cpu0mach = .;
        . += 0x8000;  /* MACHSIZE */

        *(.data .data.*)
        *(.bss .bss.*)
        *(COMMON)

        /* Kernel stack - 128KB */
        . = ALIGN(16);
        stack_bottom = .;
        . += 131072;  /* 128KB stack */
        stack_top = .;

        end = .;
    } :data

    . = ALIGN(CONSTANT(MAXPAGESIZE));

    .text : {
        KEEP(*(.text))
        *(.text.*)
        etext = .;
    } :text

    . = ALIGN(CONSTANT(MAXPAGESIZE));

    .rodata : {
        *(.rodata .rodata.*)
    } :rodata

    /DISCARD/ : {
        *(.eh_frame)
        *(.note .note.*)
    }
}
