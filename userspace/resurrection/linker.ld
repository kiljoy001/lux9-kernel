/* Linker script for resurrection server
 * Loaded at 6MB to avoid TLB aliasing with init (2MB) and wasm_test (4MB)
 */

OUTPUT_FORMAT(elf64-x86-64)
OUTPUT_ARCH(i386:x86-64)
ENTRY(_start)

SECTIONS
{
    /* Userspace loaded at 6MB to avoid conflict with init and other binaries */
    . = 0x600000;

    .text : ALIGN(4K)
    {
        *(.text*)
        *(.rodata*)
    }

    .data : ALIGN(4K)
    {
        *(.data*)
    }

    .bss : ALIGN(4K)
    {
        *(.bss*)
        *(COMMON)
    }

    /* Discard sections we don't need */
    /DISCARD/ :
    {
        *(.note*)
        *(.comment*)
        *(.eh_frame*)
    }
}
