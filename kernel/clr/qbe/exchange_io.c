/* exchange_io.c - Exchange Page I/O Implementation
 *
 * Provides stdio-like interface over exchange pages for zero-copy I/O.
 */

#include "exchange_io.h"
#include <string.h>

/* Kernel function to map physical address to kernel virtual */
extern void *kaddr(uintptr pa);

/* Page size constant */
#ifndef BY2PG
#define BY2PG 4096
#endif

#ifndef EOF
#define EOF (-1)
#endif

/* Helper: Ensure capacity for writing */
static int ensure_capacity(ExchangeFILE *fp, size_t needed)
{
	if (fp->pos + needed > fp->capacity)
		return -1;  /* Out of space */
	return 0;
}

/* Create exchange file from memory buffer */
ExchangeFILE *exchange_fmemopen(void *buf, size_t len, const char *mode)
{
	ExchangeFILE *fp;

	fp = malloc(sizeof(ExchangeFILE));
	if (!fp)
		return NULL;

	memset(fp, 0, sizeof(ExchangeFILE));
	fp->data = (char*)buf;
	fp->capacity = len;
	fp->mode = mode[0];

	if (mode[0] == 'r') {
		fp->len = len;
		fp->pos = 0;
	} else if (mode[0] == 'w') {
		fp->len = 0;
		fp->pos = 0;
	}

	return fp;
}

/* Create exchange file from exchange page handle */
ExchangeFILE *exchange_fmemopen_handle(uintptr handle, const char *mode)
{
	ExchangeFILE *fp;
	char *page_data;

	if (!handle || !mode)
		return NULL;

	/* Map physical address to kernel virtual address */
	page_data = (char*)kaddr(handle);
	if (!page_data)
		return NULL;

	/* Allocate ExchangeFILE structure */
	fp = malloc(sizeof(ExchangeFILE));
	if (!fp)
		return NULL;

	memset(fp, 0, sizeof(ExchangeFILE));
	fp->handle = handle;
	fp->data = page_data;
	fp->capacity = BY2PG;  /* Exchange pages are always 4KB */
	fp->mode = mode[0];

	if (mode[0] == 'r') {
		/* Read mode - assume page is full (caller must set actual length) */
		fp->len = BY2PG;
		fp->pos = 0;
	} else if (mode[0] == 'w') {
		/* Write mode - start empty */
		fp->len = 0;
		fp->pos = 0;
	}

	return fp;
}

/* Open exchange file (placeholder - will integrate with 9P) */
ExchangeFILE *exchange_fopen(const char *path, const char *mode)
{
	/* TODO: Integrate with /dev/clr/compile/ 9P hierarchy */
	(void)path;
	(void)mode;
	return NULL;  /* Not yet implemented */
}

/* Close exchange file */
int exchange_fclose(ExchangeFILE *fp)
{
	if (!fp)
		return EOF;

	/* TODO: Send exchange page back if needed */
	free(fp);
	return 0;
}

/* Check EOF */
int exchange_feof(ExchangeFILE *fp)
{
	return fp ? fp->eof : 1;
}

/* Check error */
int exchange_ferror(ExchangeFILE *fp)
{
	return fp ? fp->error : 1;
}

/* Get character */
int exchange_fgetc(ExchangeFILE *fp)
{
	if (!fp || fp->mode != 'r')
		return EOF;

	if (fp->pos >= fp->len) {
		fp->eof = 1;
		return EOF;
	}

	return (unsigned char)fp->data[fp->pos++];
}

/* Unget character */
int exchange_ungetc(int c, ExchangeFILE *fp)
{
	if (!fp || fp->mode != 'r' || fp->pos == 0)
		return EOF;

	fp->pos--;
	fp->eof = 0;
	return c;
}

/* Put character */
int exchange_fputc(int c, ExchangeFILE *fp)
{
	if (!fp || fp->mode != 'w')
		return EOF;

	if (ensure_capacity(fp, 1) < 0) {
		fp->error = 1;
		return EOF;
	}

	fp->data[fp->pos++] = (char)c;
	if (fp->pos > fp->len)
		fp->len = fp->pos;

	return (unsigned char)c;
}

/* Get string */
char *exchange_fgets(char *s, int size, ExchangeFILE *fp)
{
	int i;
	int c;

	if (!fp || !s || size <= 0)
		return NULL;

	for (i = 0; i < size - 1; i++) {
		c = exchange_fgetc(fp);
		if (c == EOF) {
			if (i == 0)
				return NULL;
			break;
		}
		s[i] = (char)c;
		if (c == '\n') {
			i++;
			break;
		}
	}

	s[i] = '\0';
	return s;
}

/* Put string */
int exchange_fputs(const char *s, ExchangeFILE *fp)
{
	size_t len;

	if (!fp || !s)
		return EOF;

	len = strlen(s);
	if (ensure_capacity(fp, len) < 0) {
		fp->error = 1;
		return EOF;
	}

	memcpy(fp->data + fp->pos, s, len);
	fp->pos += len;
	if (fp->pos > fp->len)
		fp->len = fp->pos;

	return 0;
}

/* Block read */
size_t exchange_fread(void *ptr, size_t size, size_t nmemb, ExchangeFILE *fp)
{
	size_t total;
	size_t avail;
	size_t to_read;

	if (!fp || !ptr || fp->mode != 'r')
		return 0;

	total = size * nmemb;
	avail = fp->len - fp->pos;
	to_read = (total < avail) ? total : avail;

	memcpy(ptr, fp->data + fp->pos, to_read);
	fp->pos += to_read;

	if (fp->pos >= fp->len)
		fp->eof = 1;

	return to_read / size;
}

/* Block write */
size_t exchange_fwrite(const void *ptr, size_t size, size_t nmemb, ExchangeFILE *fp)
{
	size_t total;

	if (!fp || !ptr || fp->mode != 'w')
		return 0;

	total = size * nmemb;
	if (ensure_capacity(fp, total) < 0) {
		fp->error = 1;
		return 0;
	}

	memcpy(fp->data + fp->pos, ptr, total);
	fp->pos += total;
	if (fp->pos > fp->len)
		fp->len = fp->pos;

	return nmemb;
}

/* Tell position */
long exchange_ftell(ExchangeFILE *fp)
{
	return fp ? (long)fp->pos : -1L;
}

/* Seek */
int exchange_fseek(ExchangeFILE *fp, long offset, int whence)
{
	long new_pos;

	if (!fp)
		return -1;

	switch (whence) {
	case 0: /* SEEK_SET */
		new_pos = offset;
		break;
	case 1: /* SEEK_CUR */
		new_pos = (long)fp->pos + offset;
		break;
	case 2: /* SEEK_END */
		new_pos = (long)fp->len + offset;
		break;
	default:
		return -1;
	}

	if (new_pos < 0 || (size_t)new_pos > fp->len)
		return -1;

	fp->pos = (size_t)new_pos;
	fp->eof = 0;
	return 0;
}

/* Rewind */
void exchange_rewind(ExchangeFILE *fp)
{
	if (fp) {
		fp->pos = 0;
		fp->eof = 0;
		fp->error = 0;
	}
}

/* Get direct pointer to data (zero-copy) */
void *exchange_fdata(ExchangeFILE *fp, size_t *len)
{
	if (!fp)
		return NULL;
	if (len)
		*len = fp->len;
	return fp->data;
}

/* Helper functions for scanf */
extern int atoi(const char *s);
#ifndef _KERNEL_QBE
extern double strtod(const char *s, char **endptr);
#endif

/* Formatted input - simplified version for QBE's needs */
int exchange_vfscanf(ExchangeFILE *fp, const char *fmt, va_list ap)
{
	char buf[256];
	char *p;
	int count = 0;
	const char *f;

	if (!fp || !fmt)
		return EOF;

	f = fmt;
	while (*f) {
		/* Skip whitespace in format */
		while (*f == ' ' || *f == '\t' || *f == '\n')
			f++;

		/* Check for conversion specifier */
		if (*f == '%') {
			f++;
			if (*f == 'd' || *f == 'i') {
				/* Integer */
				int *ip = va_arg(ap, int *);
				p = buf;
				/* Skip whitespace in input */
				while (fp->pos < fp->len &&
				       (fp->data[fp->pos] == ' ' || fp->data[fp->pos] == '\t' || fp->data[fp->pos] == '\n'))
					fp->pos++;
				/* Read number */
				if (fp->pos >= fp->len)
					break;
				if (fp->data[fp->pos] == '-' || fp->data[fp->pos] == '+')
					*p++ = fp->data[fp->pos++];
				while (fp->pos < fp->len && fp->data[fp->pos] >= '0' && fp->data[fp->pos] <= '9')
					*p++ = fp->data[fp->pos++];
				*p = '\0';
				*ip = atoi(buf);
				count++;
			}
#ifndef _KERNEL_QBE
			/* Float/double scanning not supported in kernel (no FPU) */
			else if (*f == 'f' || *f == 'l') {
				/* Float/double */
				double *dp = va_arg(ap, double *);
				p = buf;
				/* Skip whitespace */
				while (fp->pos < fp->len &&
				       (fp->data[fp->pos] == ' ' || fp->data[fp->pos] == '\t' || fp->data[fp->pos] == '\n'))
					fp->pos++;
				if (fp->pos >= fp->len)
					break;
				/* Read number (including sign, decimal, exponent) */
				if (fp->data[fp->pos] == '-' || fp->data[fp->pos] == '+')
					*p++ = fp->data[fp->pos++];
				while (fp->pos < fp->len &&
				       ((fp->data[fp->pos] >= '0' && fp->data[fp->pos] <= '9') ||
				        fp->data[fp->pos] == '.' || fp->data[fp->pos] == 'e' ||
				        fp->data[fp->pos] == 'E' || fp->data[fp->pos] == '-' ||
				        fp->data[fp->pos] == '+'))
					*p++ = fp->data[fp->pos++];
				*p = '\0';
				*dp = strtod(buf, NULL);
				count++;
			}
#endif
			else if (*f == 's') {
				/* String */
				char *sp = va_arg(ap, char *);
				/* Skip whitespace */
				while (fp->pos < fp->len &&
				       (fp->data[fp->pos] == ' ' || fp->data[fp->pos] == '\t' || fp->data[fp->pos] == '\n'))
					fp->pos++;
				/* Read until whitespace */
				while (fp->pos < fp->len && fp->data[fp->pos] != ' ' &&
				       fp->data[fp->pos] != '\t' && fp->data[fp->pos] != '\n')
					*sp++ = fp->data[fp->pos++];
				*sp = '\0';
				count++;
			}
			f++;
		} else {
			/* Literal character - must match */
			if (fp->pos >= fp->len || fp->data[fp->pos] != *f)
				break;
			fp->pos++;
			f++;
		}
	}

	return count;
}

int exchange_fscanf(ExchangeFILE *fp, const char *fmt, ...)
{
	va_list ap;
	int ret;

	va_start(ap, fmt);
	ret = exchange_vfscanf(fp, fmt, ap);
	va_end(ap);

	return ret;
}

/* Formatted output */
int exchange_vfprintf(ExchangeFILE *fp, const char *fmt, va_list ap)
{
	char buf[4096];
	int len;

	if (!fp)
		return -1;

	len = vsnprintf(buf, sizeof(buf), fmt, ap);
	if (len < 0)
		return len;

	if (ensure_capacity(fp, (size_t)len) < 0) {
		fp->error = 1;
		return -1;
	}

	memcpy(fp->data + fp->pos, buf, (size_t)len);
	fp->pos += (size_t)len;
	if (fp->pos > fp->len)
		fp->len = fp->pos;

	return len;
}

int exchange_fprintf(ExchangeFILE *fp, const char *fmt, ...)
{
	va_list ap;
	int ret;

	va_start(ap, fmt);
	ret = exchange_vfprintf(fp, fmt, ap);
	va_end(ap);

	return ret;
}
