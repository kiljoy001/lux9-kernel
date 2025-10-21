/* Limine boot entry point for 9front kernel */
.code64

/* Limine protocol requests - start marker */
.section .limine_requests_start, "aw", @progbits
.global limine_requests_start_marker
limine_requests_start_marker:
    .quad 0xf6b8f4b39de7d1ae
    .quad 0xfab91a6940fcb9cf
    .quad 0x785c6ed015d3e316
    .quad 0x181e920a7852b9d9

/* Base revision - REQUIRED (must be BEFORE requests section) */
.section .limine_base_revision, "aw", @progbits
.align 8
.global limine_base_revision
limine_base_revision:
    .quad 0xf9562b2d5c95a6c8  /* ID[0] */
    .quad 0x6a7b384944536bdc  /* ID[1] */
    .quad 2                   /* Revision 2 - for Kernel Address request support */

/* Limine protocol requests - pointers to actual requests */
.section .limine_requests, "aw", @progbits
    .quad hhdm_request
    .quad memmap_request
    .quad framebuffer_request
    .quad bootloader_info_request
    .quad module_request
    .quad kernel_address_request

/* HHDM (Higher half direct map) request - IN SAME SECTION */
.align 8
hhdm_request:
    .quad 0xc7b1dd30df4c8b88  /* Limine common magic [0] */
    .quad 0x0a82e883a194f07b  /* Limine common magic [1] */
    .quad 0x48dcf1cb8ad2b852  /* HHDM request ID [2] */
    .quad 0x63984e959a98244b  /* HHDM request ID [3] */
    .quad 0                   /* revision */
    .quad 0                   /* response pointer */

/* Memory map request */
.align 8
memmap_request:
    .quad 0xc7b1dd30df4c8b88  /* Limine common magic [0] */
    .quad 0x0a82e883a194f07b  /* Limine common magic [1] */
    .quad 0x67cf3d9d378a806f  /* Memory map request ID [2] */
    .quad 0xe304acdfc50c3c62  /* Memory map request ID [3] */
    .quad 0                   /* revision */
    .quad 0                   /* response pointer */

/* Framebuffer request */
.align 8
framebuffer_request:
    .quad 0xc7b1dd30df4c8b88  /* Limine common magic [0] */
    .quad 0x0a82e883a194f07b  /* Limine common magic [1] */
    .quad 0x9d5827dcd881dd75  /* Framebuffer request ID [2] */
    .quad 0xa3148604f6fab11b  /* Framebuffer request ID [3] */
    .quad 0                   /* revision */
    .quad 0                   /* response pointer */
    .quad 0                   /* framebuffer count */
    .quad 0                   /* framebuffers */

/* Bootloader info request */
.align 8
bootloader_info_request:
    .quad 0xc7b1dd30df4c8b88  /* Limine common magic [0] */
    .quad 0x0a82e883a194f07b  /* Limine common magic [1] */
    .quad 0xf55038d8e2a1202f  /* Bootloader info request ID [2] */
    .quad 0x279426fcf5f59740  /* Bootloader info request ID [3] */
    .quad 0                   /* revision */
    .quad 0                   /* response pointer (filled by bootloader) */

/* Module request */
.align 8
module_request:
    .quad 0xc7b1dd30df4c8b88  /* Limine common magic [0] */
    .quad 0x0a82e883a194f07b  /* Limine common magic [1] */
    .quad 0x3e7e279702be32af  /* Module request ID [2] */
    .quad 0xca1c4f3bd1280cee  /* Module request ID [3] */
    .quad 0                   /* revision */
    .quad 0                   /* response pointer (filled by bootloader) */
    .quad 0                   /* internal module count */
    .quad 0                   /* internal modules */

/* Kernel Address request */
.align 8
.global kernel_address_request
kernel_address_request:
    .quad 0xc7b1dd30df4c8b88  /* Limine common magic [0] */
    .quad 0x0a82e883a194f07b  /* Limine common magic [1] */
    .quad 0x71ba76863cc55f63  /* Kernel Address request ID [2] */
    .quad 0xb2644a48c516a487  /* Kernel Address request ID [3] */
    .quad 0                   /* revision */
    .quad 0                   /* response pointer (filled by bootloader) */

/* Limine protocol requests - end marker */
.section .limine_requests_end, "aw", @progbits
.global limine_requests_end_marker
limine_requests_end_marker:
    .quad 0xadc0e0531bb10d03
    .quad 0x9572709f31764c62

.section .text
.global _start
_start:
    /* Limine bootloader entry - we're in 64-bit mode, paging enabled */
    cli                       /* Disable interrupts */

    /* Clear direction flag for string operations */
    cld

    /* Step 01: Zero BSS section */
    movw $0x3F8, %dx

    lea bss_start(%rip), %rdi
    lea bss_end(%rip), %rcx
    sub %rdi, %rcx
    xor %rax, %rax
    rep stosb

    /* Step 02: Set up kernel stack */
    movw $0x3F8, %dx

    lea stack_top(%rip), %rsp
    and $~0xF, %rsp
    xor %rbp, %rbp

    /* Step 03: Initialize UART */
    movw $0x3F8, %dx

    /* Disable interrupts on UART */
    movw $0x3F9, %dx
    movb $0x00, %al
    outb %al, %dx

    /* Enable DLAB */
    movw $0x3FB, %dx
    movb $0x80, %al
    outb %al, %dx

    /* Set divisor to 12 (9600 baud) */
    movw $0x3F8, %dx
    movb $0x0C, %al
    outb %al, %dx
    movw $0x3F9, %dx
    movb $0x00, %al
    outb %al, %dx

    /* 8 bits, no parity, one stop bit */
    movw $0x3FB, %dx
    movb $0x03, %al
    outb %al, %dx

    /* Enable FIFO */
    movw $0x3FA, %dx
    movb $0xC7, %al
    outb %al, %dx

    /* Enable IRQs, set RTS/DSR */
    movw $0x3FC, %dx
    movb $0x0B, %al
    outb %al, %dx

    /* Step 04: UART initialized */
    movw $0x3F8, %dx

    /* Step 05: Map .cpu0_data section in page tables */
    movw $0x3F8, %dx

    /* Get current CR3 (Limine's PML4) */
    mov %cr3, %rax

    /* Map 0xffffffff80001000 to physical address (identity mapped from kernel base)
     * We need to add page table entries for the .cpu0_data section
     * For now, we'll map it as a 2MB page at 0xffffffff80000000 which covers
     * the data section at 0xffffffff80001000
     *
     * The kernel ELF is loaded at a physical address by Limine, and we need
     * to ensure the data section is also mapped.
     *
     * Since Limine already mapped the kernel text, we just need to ensure
     * the data section (which comes BEFORE text in memory due to linker script)
     * is also mapped. Let's verify this is already done by Limine.
     */

    /* Step 06: Set up GS base for CPU-local data */
    movw $0x3F8, %dx

    movl $0xC0000101, %ecx
    movl $0x80018000, %eax
    movl $0xffffffff, %edx
    wrmsr

    /* Step 07: About to call main */
    movw $0x3F8, %dx

    /* Call 9front kernel main */
    call main

    /* Step 08: main returned (should not happen) */
    movw $0x3F8, %dx

    /* If main returns, halt */
.Lhang:
    cli
    hlt
    jmp .Lhang

/* Export Limine request responses for kernel access */
.section .data
.global limine_bootloader_info
.global limine_memmap
.global limine_framebuffer
.global limine_hhdm
.global limine_module
.global limine_kernel_address

limine_bootloader_info:
    .quad bootloader_info_request
limine_memmap:
    .quad memmap_request
limine_framebuffer:
    .quad framebuffer_request
limine_hhdm:
    .quad hhdm_request
limine_module:
    .quad module_request
limine_kernel_address:
    .quad kernel_address_request

/* Stack is now defined in linker.ld, not here */

/* Page table structures - aligned to 4KB */
.section .bss
.align 4096
pml4:
    .skip 4096
pdpt_low:
    .skip 4096
pdpt_high:
    .skip 4096
pd_kernel:
    .skip 4096
