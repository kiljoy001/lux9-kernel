/* Minimal Limine kernel test */
.code64

/* Limine protocol requests - start marker */
.section .limine_requests_start, "aw", @progbits
.global limine_requests_start_marker
limine_requests_start_marker:
    .quad 0xf6b8f4b39de7d1ae
    .quad 0xfab91a6940fcb9cf
    .quad 0x785c6ed015d3e316
    .quad 0x181e920a7852b9d9

/* Limine protocol requests */
.section .limine_requests, "aw", @progbits

/* Base revision - REQUIRED */
.align 8
.global limine_base_revision
limine_base_revision:
    .quad 0xf9562b2d5c95a6c8
    .quad 0x6a7b384944536bdc
    .quad 3

/* Limine protocol requests - end marker */
.section .limine_requests_end, "aw", @progbits
.global limine_requests_end_marker
limine_requests_end_marker:
    .quad 0xadc0e0531bb10d03
    .quad 0x9572709f31764c62

.section .text
.global _start
_start:
    /* Initialize COM1 UART */
    movw $0x3F9, %dx
    xorb %al, %al
    outb %al, %dx          /* Disable interrupts */

    movw $0x3FB, %dx
    movb $0x80, %al
    outb %al, %dx          /* Enable DLAB */

    movw $0x3F8, %dx
    movb $0x01, %al
    outb %al, %dx          /* Divisor low */

    movw $0x3F9, %dx
    xorb %al, %al
    outb %al, %dx          /* Divisor high */

    movw $0x3FB, %dx
    movb $0x03, %al
    outb %al, %dx          /* 8N1 */

    movw $0x3FA, %dx
    movb $0xC7, %al
    outb %al, %dx          /* Enable FIFO */

    movw $0x3FC, %dx
    movb $0x0B, %al
    outb %al, %dx          /* RTS/DSR */

    /* Output "OK\n" to serial */
    movw $0x3F8, %dx
    movb $'O', %al
    outb %al, %dx
    movb $'K', %al
    outb %al, %dx
    movb $'\r', %al
    outb %al, %dx
    movb $'\n', %al
    outb %al, %dx

    /* Halt */
1:
    cli
    hlt
    jmp 1b

.section .bss
.align 16
stack_bottom:
    .skip 4096
stack_top:
