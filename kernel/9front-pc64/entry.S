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
    .quad 3                   /* Revision 3 */

/* Limine protocol requests - pointers to actual requests */
.section .limine_requests, "aw", @progbits
    .quad hhdm_request
    .quad memmap_request
    .quad framebuffer_request
    .quad bootloader_info_request
    .quad module_request

/* Actual request structures in .data section */
.section .data

/* HHDM (Higher half direct map) request */
.align 8
hhdm_request:
    .quad 0x48dcf1cb8ad2b852  /* Limine HHDM magic */
    .quad 0x63984e959a98244b
    .quad 0                   /* revision */
    .quad 0                   /* response pointer */

/* Memory map request */
.align 8
memmap_request:
    .quad 0x67cf3d9d378a806f  /* Limine memory map magic */
    .quad 0xe304acdfc50c3c62
    .quad 0                   /* revision */
    .quad 0                   /* response pointer */

/* Framebuffer request */
.align 8
framebuffer_request:
    .quad 0x9d5827dcd881dd75  /* Limine framebuffer magic */
    .quad 0xa3148604f6fab11b
    .quad 0                   /* revision */
    .quad 0                   /* response pointer */
    .quad 0                   /* framebuffer count */
    .quad 0                   /* framebuffers */

/* Bootloader info request */
.align 8
bootloader_info_request:
    .quad 0xf55038d8e2a1202f  /* Limine bootloader info magic */
    .quad 0x279426fcf5f59740
    .quad 0                   /* revision */
    .quad 0                   /* response pointer (filled by bootloader) */

/* Module request */
.align 8
module_request:
    .quad 0x3e7e279702be32af  /* Limine module magic */
    .quad 0xca1c4f3bd1280cee
    .quad 0                   /* revision */
    .quad 0                   /* response pointer (filled by bootloader) */
    .quad 0                   /* internal module count */
    .quad 0                   /* internal modules */

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

    /* Zero BSS section - CRITICAL for uninitialized globals! */
    lea stack_bottom(%rip), %rdi    /* BSS starts after stack_bottom */
    lea end(%rip), %rcx              /* End of BSS */
    sub %rdi, %rcx                   /* BSS size */
    xor %rax, %rax                   /* Zero value */
    rep stosb                        /* Zero BSS */

    /* Now set up proper kernel stack */
    lea stack_top(%rip), %rsp
    /* Ensure 16-byte alignment: RSP must be 16-byte aligned, but call pushes 8 bytes,
       so we need RSP % 16 == 0 before call, which becomes (RSP-8) % 16 == 8 after call */
    and $~0xF, %rsp           /* Align to 16-byte boundary */
    xor %rbp, %rbp            /* Clear frame pointer */

    /* Initialize UART before any output */
    movw $0x3F8, %dx          /* COM1 base port */
    movb $'Z', %al            /* Signal BSS zeroing complete */
    outb %al, %dx
    movw $0x3F8, %dx          /* COM1 base port */

    /* Disable interrupts on UART */
    movw $0x3F9, %dx          /* IER */
    movb $0x00, %al
    outb %al, %dx

    /* Enable DLAB (set baud rate) */
    movw $0x3FB, %dx          /* LCR */
    movb $0x80, %al
    outb %al, %dx

    /* Set divisor to 1 (115200 baud) */
    movw $0x3F8, %dx          /* DLL */
    movb $0x01, %al
    outb %al, %dx
    movw $0x3F9, %dx          /* DLM */
    movb $0x00, %al
    outb %al, %dx

    /* 8 bits, no parity, one stop bit */
    movw $0x3FB, %dx          /* LCR */
    movb $0x03, %al
    outb %al, %dx

    /* Enable FIFO */
    movw $0x3FA, %dx          /* FCR */
    movb $0xC7, %al
    outb %al, %dx

    /* Enable IRQs, set RTS/DSR */
    movw $0x3FC, %dx          /* MCR */
    movb $0x0B, %al
    outb %al, %dx

    /* Early debug: output '1' to show entry point reached */
    movw $0x3F8, %dx          /* COM1 data port */
    movb $'1', %al
    outb %al, %dx

    /* Output '2' to show about to call main */
    movb $'2', %al
    outb %al, %dx

    /* Set up GS base to point to CPU0MACH for per-CPU data (m pointer) */
    /* GS base MSR is 0xC0000101 */
    movl $0xC0000101, %ecx
    movl $0x80018000, %eax     /* CPU0MACH low 32 bits */
    movl $0xffffffff, %edx     /* CPU0MACH high 32 bits */
    wrmsr

    /* Save Limine boot info (TODO: pass to kernel init) */
    /* For now, 9front's main() doesn't take boot info */

    /* Call 9front kernel main */
    call main

    /* Output '3' if main returns */
    movw $0x3F8, %dx
    movb $'3', %al
    outb %al, %dx

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

/* Stack is now defined in linker.ld, not here */
