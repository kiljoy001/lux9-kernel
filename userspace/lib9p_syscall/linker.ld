/* linker.ld - Linker script for pure 9P init
 *
 * Simple ELF layout placed at 0x200000 to match kernel expectations
 */

ENTRY(_start)

SECTIONS
{
    . = 0x200000;
    
    .text : {
        *(.text)
        *(.text.*)
    }
    
    .rodata : {
        *(.rodata)
        *(.rodata.*)
    }
    
    .data : {
        *(.data)
        *(.data.*)
    }
    
    .bss : {
        *(.bss)
        *(.bss.*)
        *(COMMON)
    }
    
    /DISCARD/ : {
        *(.note*)
        *(.comment*)
        *(.eh_frame*)
    }
}
