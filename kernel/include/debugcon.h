#pragma once

static inline void debugcon_putc(char c)
{
	asm volatile("outb %0, %1" :: "a"(c), "Nd"(0xE9));
}

static inline void debugcon_print(const char *s)
{
	while(*s)
		debugcon_putc(*s++);
}

static inline void debugcon_hex(ulong v)
{
	static const char hex[] = "0123456789abcdef";
	for(int i = (int)(sizeof(v)*2 - 1); i >= 0; i--)
		debugcon_putc(hex[(v >> (i*4)) & 0xF]);
}
