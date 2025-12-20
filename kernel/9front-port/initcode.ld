/* Simple linker script for initcode */
ENTRY(_start)
SECTIONS {
    . = 0x200000;
    .text : {
        *(.text.start)
        *(.text*)
    }
    .rodata : { *(.rodata*) }
    /DISCARD/ : { *(.comment) *(.note*) *(.eh_frame*) }
}
