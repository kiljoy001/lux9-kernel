#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"

/* Direct UART hardware access for early boot */
#define COM1_PORT 0x3F8

#define UART_DATA 0
#define UART_IER 1
#define UART_FCR 2
#define UART_LCR 3
#define UART_MCR 4
#define UART_LSR 5

static int early_uart_initialized = 0;

/* Direct port I/O (borrowed from existing implementation) */
static inline void early_outb(int port, uchar data)
{
	asm volatile("outb %0, %1" : : "a"(data), "d"(port));
}

static inline uchar early_inb(int port)
{
	uchar data;
	asm volatile("inb %1, %0" : "=a"(data) : "d"(port));
	return data;
}

/* Early UART character output */
static void early_uart_putc(int c)
{
	/* Wait for transmitter to be ready */
	while((early_inb(COM1_PORT + UART_LSR) & 0x20) == 0)
		;

	early_outb(COM1_PORT + UART_DATA, c);
}

/* Initialize early UART console */
void early_i8250console(void)
{
	/* Disable interrupts */
	early_outb(COM1_PORT + UART_IER, 0x00);

	/* Enable DLAB (set baud rate divisor) */
	early_outb(COM1_PORT + UART_LCR, 0x80);

	/* Set divisor to 1 (115200 baud) */
	early_outb(COM1_PORT + UART_DATA, 0x01);
	early_outb(COM1_PORT + UART_IER, 0x00);

	/* 8 bits, no parity, one stop bit */
	early_outb(COM1_PORT + UART_LCR, 0x03);

	/* Enable FIFO, clear them, with 14-byte threshold */
	early_outb(COM1_PORT + UART_FCR, 0xC7);

	/* Enable IRQs, set RTS/DSR */
	early_outb(COM1_PORT + UART_MCR, 0x0B);

	early_uart_initialized = 1;
}

/* Early print function - direct hardware access, no allocation */
void early_iprint(char *fmt, ...)
{
	char buf[256];
	va_list args;
	int i, n;

	if(!early_uart_initialized)
		return;

	va_start(args, fmt);
	n = vseprint(buf, buf+sizeof(buf), fmt, args) - buf;
	va_end(args);

	for(i = 0; i < n; i++){
		if(buf[i] == '\n')
			early_uart_putc('\r');
		early_uart_putc(buf[i]);
	}
}