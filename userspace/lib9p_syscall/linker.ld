/* Linker script for userspace init
 * Clean userspace using shared kernel libc9
 */

OUTPUT_FORMAT(elf64-x86-64)
OUTPUT_ARCH(i386:x86-64)
ENTRY(_start)

SECTIONS
{
    /* Userspace loaded at 2MB (UTZERO) to match kernel expectations */
    . = 0x200000;

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
