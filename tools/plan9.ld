ENTRY(_start)

SECTIONS
{
    /* Plan 9 Text Base is 0x200028 (Header size 0x28).
       But ELF headers take space.
       Linux defaults to 0x400000.
       We want VAddr 0x200000 range.
       
       We start at 0x200000 + 0x1000 (0x201000) to allow ELF headers at 0x200000?
       Or we pack tight?
       elf2plan9-cs expects p_vaddr - minText.
       
       Let's stick to bflat's offset 0x200028 for consistency with a.out header.
       We will use -n to pack headers if needed, but we want alignment.
       
       Simplest: Standard Page Alignment.
       Text at 0x200000 + SIZEOF_HEADERS (aligned to 0x1000 + offset?)
       
       Let's force Text to 0x201000.
       And tell Plan9 Header to use Entry 0x201000?
       No, Plan 9 a.out header has fixed size 0x20 (or 0x28).
       Text segment follows immediately.
       So Text VAddr MUST be (Base + HeaderSize).
       Base = 0x200000. Header = 0x28.
       Text VAddr = 0x200028.
    */
    
    . = 0x200028;

    .text : {
        *(.text)
        *(.text.*)
        *(.init)
        *(.fini)
        /* Keep Code in Text */
    }

    /* Page Align Data to ensure separate segment permissions */
    . = ALIGN(0x1000);

    .data : {
        *(.data)
        *(.data.*)
        *(.rodata)        /* MOVE RODATA HERE (RW) */
        *(.rodata.*)
        *(__NONGCSTATICS) /* MOVE STATICS HERE (RW) */
        *(__managedcode)  /* MOVE MANAGED CODE HERE (Just in case it is patched) */
        *(.got)
        *(.got.plt)
    }

    .bss : {
        *(.bss)
        *(.bss.*)
        *(COMMON)
    }

    /DISCARD/ : {
        *(.eh_frame)
        *(.eh_frame_hdr)
        *(.dotnet_eh_table)
        *(.note.*)
        *(.interp)
        *(.dynamic)
        *(.dynsym)
        *(.dynstr)
        *(.gnu.hash)
    }
}
