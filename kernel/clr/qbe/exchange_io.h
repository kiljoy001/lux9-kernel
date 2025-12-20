/* exchange_io.h - Exchange Page I/O for QBE
 *
 * Replaces stdio FILE* with exchange pages for zero-copy compilation.
 * QBE reads Fruity IR from exchange pages, writes machine code to exchange pages.
 */

#ifndef EXCHANGE_IO_H
#define EXCHANGE_IO_H

#include <stddef.h>
#include <stdarg.h>

/* Forward declaration - actual definition in kernel headers */
typedef struct exchange_page exchange_page_t;
typedef unsigned long uintptr;

/* Exchange-backed FILE replacement */
typedef struct ExchangeFILE {
	exchange_page_t *page;  /* Underlying exchange page */
	uintptr handle;         /* Exchange page handle (physical address) */
	char *data;             /* Direct pointer to page data */
	size_t pos;             /* Current read/write position */
	size_t len;             /* Total data length */
	size_t capacity;        /* Page capacity */
	int eof;                /* EOF flag */
	int mode;               /* 'r' or 'w' */
	int error;              /* Error flag */
} ExchangeFILE;

/* File operations */
ExchangeFILE *exchange_fopen(const char *path, const char *mode);
int exchange_fclose(ExchangeFILE *fp);
int exchange_feof(ExchangeFILE *fp);
int exchange_ferror(ExchangeFILE *fp);

/* Character I/O */
int exchange_fgetc(ExchangeFILE *fp);
int exchange_ungetc(int c, ExchangeFILE *fp);
int exchange_fputc(int c, ExchangeFILE *fp);

/* Formatted I/O */
int exchange_fscanf(ExchangeFILE *fp, const char *fmt, ...);
int exchange_vfscanf(ExchangeFILE *fp, const char *fmt, va_list ap);
int exchange_fprintf(ExchangeFILE *fp, const char *fmt, ...);
int exchange_vfprintf(ExchangeFILE *fp, const char *fmt, va_list ap);

/* String I/O */
char *exchange_fgets(char *s, int size, ExchangeFILE *fp);
int exchange_fputs(const char *s, ExchangeFILE *fp);

/* Block I/O */
size_t exchange_fread(void *ptr, size_t size, size_t nmemb, ExchangeFILE *fp);
size_t exchange_fwrite(const void *ptr, size_t size, size_t nmemb, ExchangeFILE *fp);

/* Position */
long exchange_ftell(ExchangeFILE *fp);
int exchange_fseek(ExchangeFILE *fp, long offset, int whence);
void exchange_rewind(ExchangeFILE *fp);

/* Special: Create exchange file from memory buffer (for kernel use) */
ExchangeFILE *exchange_fmemopen(void *buf, size_t len, const char *mode);

/* Special: Create exchange file from exchange page handle */
ExchangeFILE *exchange_fmemopen_handle(uintptr handle, const char *mode);

/* Special: Get pointer to exchange page data (zero-copy access) */
void *exchange_fdata(ExchangeFILE *fp, size_t *len);

#endif /* EXCHANGE_IO_H */
