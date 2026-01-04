#include <u.h>

void *memmove(void *dst, const void *src, ulong n) {
	const char *s = src;
	char *d = dst;
	if (d < s) {
		while (n-- > 0)
			*d++ = *s++;
	} else {
		s += n;
		d += n;
		while (n-- > 0)
			*--d = *--s;
	}
	return dst;
}

void *memset(void *dst, int c, ulong n) {
	char *d = dst;
	while (n-- > 0)
		*d++ = c;
	return dst;
}

ulong strlen(const char *s) {
	const char *p = s;
	while (*p)
		p++;
	return p - s;
}

void *malloc(ulong size) {
    /* Stub - we don't have a heap allocator in liblux yet! */
    /* msgord.c might need it for dynamic buffers if we don't use stack */
    return (void*)0; 
}

int atoi(const char *s) {
	int n = 0;
	int neg = 0;
	if (*s == '-') {
		neg = 1;
		s++;
	}
	while (*s >= '0' && *s <= '9') {
		n = n * 10 + (*s - '0');
		s++;
	}
	return neg ? -n : n;
}

unsigned long long strtoull(const char *s, char **endptr, int base) {
	unsigned long long n = 0;
	if (base == 0) {
		if (*s == '0' && (*(s+1) == 'x' || *(s+1) == 'X')) {
			base = 16;
			s += 2;
		} else {
			base = 10;
		}
	}
	// ... minimal implementation ...
	while (1) {
		int d;
		if (*s >= '0' && *s <= '9') d = *s - '0';
		else if (*s >= 'a' && *s <= 'f') d = *s - 'a' + 10;
		else if (*s >= 'A' && *s <= 'F') d = *s - 'A' + 10;
		else break;
		
		if (d >= base) break;
		n = n * base + d;
		s++;
	}
	if (endptr) *endptr = (char*)s;
	return n;
}
