/* Simple 8250 UART driver for early console */
#include "u.h"
#include "portlib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"

#define COM1 0x3F8

/* UART registers */
#define UART_DATA    0  /* Data register (R/W) */
#define UART_IER     1  /* Interrupt Enable Register */
#define UART_FCR     2  /* FIFO Control Register */
#define UART_LCR     3  /* Line Control Register */
#define UART_MCR     4  /* Modem Control Register */
#define UART_LSR     5  /* Line Status Register */

/* Line Status Register bits */
#define LSR_THRE  0x20  /* Transmit Holding Register Empty */

static int uart_base = COM1;
static int uart_initialized = 0;

static void
uart_putc(int c)
{
	int i;

	if(!uart_initialized)
		return;

	/* Wait for transmit holding register to be empty with proper timeout */
	for(i = 0; i < 100000; i++) {
		if(inb(uart_base + UART_LSR) & LSR_THRE)
			break;
		/* Small delay to avoid busy-waiting too hard */
		for(volatile int j = 0; j < 10; j++)
			;
	}

	outb(uart_base + UART_DATA, c);
}

void
i8250console(void)
{
	/* Disable interrupts */
	outb(uart_base + UART_IER, 0x00);

	/* Enable DLAB (set baud rate divisor) */
	outb(uart_base + UART_LCR, 0x80);

	/* Set divisor to 1 (115200 baud) */
	outb(uart_base + UART_DATA, 0x01);
	outb(uart_base + UART_IER, 0x00);

	/* 8 bits, no parity, one stop bit */
	outb(uart_base + UART_LCR, 0x03);

	/* Enable FIFO, clear them, with 14-byte threshold */
	outb(uart_base + UART_FCR, 0xC7);

	/* Enable IRQs, set RTS/DSR */
	outb(uart_base + UART_MCR, 0x0B);

	uart_initialized = 1;

	/* Hook into screenputs */
	screenputs = uart_screenputs;
}

void
uart_screenputs(char *s, int n)
{
	int i;

	for(i = 0; i < n; i++){
		if(s[i] == '\n')
			uart_putc('\r');
		uart_putc(s[i]);
	}
}

void
uartputs(char *s, int n)
{
	int i;

	if(!uart_initialized)
		return;

	for(i = 0; i < n; i++){
		if(s[i] == '\n')
			uart_putc('\r');
		uart_putc(s[i]);
	}
}
