/* Minimal support routines for init */
#include <stdarg.h>
#include <stddef.h>
#include "../lib/syscall.h"

static size_t
cstringlen(const char *s)
{
	size_t n = 0;
	if(s == NULL)
		return 0;
	while(s[n] != '\0')
		n++;
	return n;
}

int
strcmp(const char *s1, const char *s2)
{
	while(*s1 && (*s1 == *s2)) {
		s1++;
		s2++;
	}
	return *(const unsigned char*)s1 - *(const unsigned char*)s2;
}

char*
strcpy(char *dest, const char *src)
{
	char *ret = dest;
	while((*dest++ = *src++))
		;
	return ret;
}

static void
write_full(const char *buf, size_t len)
{
	while(len > 0) {
		ssize_t n = write(1, buf, len);
		if(n <= 0)
			return;
		buf += n;
		len -= n;
	}
}

static void
write_str(const char *s)
{
	if(s == NULL)
		s = "(null)";
	write_full(s, cstringlen(s));
}

int
putchar(int c)
{
	char ch = (char)c;
	write_full(&ch, 1);
	return c;
}

int
puts(const char *s)
{
	write_str(s);
	write_full("\n", 1);
	return 0;
}

static void
print_unsigned(unsigned long val, unsigned base, int prefix_hex)
{
	char buf[32];
	const char digits[] = "0123456789abcdef";
	int i = 0;

	if(prefix_hex)
		write_full("0x", 2);

	if(val == 0) {
		write_full("0", 1);
		return;
	}

	while(val > 0 && i < (int)sizeof(buf)) {
		buf[i++] = digits[val % base];
		val /= base;
	}
	while(i-- > 0)
		write_full(&buf[i], 1);
}

static void
print_signed(long val)
{
	if(val < 0) {
		write_full("-", 1);
		val = -val;
	}
	print_unsigned((unsigned long)val, 10, 0);
}

static int
vprintf_internal(const char *fmt, va_list ap)
{
	while(*fmt) {
		if(*fmt == '%') {
			fmt++;
			switch(*fmt) {
			case 's':
				write_str(va_arg(ap, char*));
				break;
			case 'd':
			case 'i':
				print_signed(va_arg(ap, int));
				break;
			case 'u':
				print_unsigned(va_arg(ap, unsigned int), 10, 0);
				break;
			case 'x':
				print_unsigned(va_arg(ap, unsigned int), 16, 0);
				break;
			case 'p':
				print_unsigned(va_arg(ap, unsigned long), 16, 1);
				break;
			case '%':
				write_full("%", 1);
				break;
			default:
				write_full("%", 1);
				if(*fmt)
					write_full(fmt, 1);
				else
					fmt--;
				break;
			}
			if(*fmt)
				fmt++;
			continue;
		}
		write_full(fmt, 1);
		fmt++;
	}
	return 0;
}

int
printf(const char *fmt, ...)
{
	va_list ap;
	int rv;

	va_start(ap, fmt);
	rv = vprintf_internal(fmt, ap);
	va_end(ap);
	return rv;
}

int
__printf_chk(int flag, const char *fmt, ...)
{
	va_list ap;
	int rv;

	(void)flag;
	va_start(ap, fmt);
	rv = vprintf_internal(fmt, ap);
	va_end(ap);
	return rv;
}

void
sleep_ms(int ms)
{
	if(ms <= 0)
		sleep(0);
	else
		sleep((unsigned long)ms);
}
