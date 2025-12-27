#ifndef _STDIO_H_
#define _STDIO_H_

#include <stddef.h>
#include <stdarg.h>

typedef void FILE;
#define stderr ((FILE*)2)
#define stdout ((FILE*)1)
#define stdin  ((FILE*)0)

/* Provided by kernel/lib.h or fns.h */
int print(char *, ...);
int snprint(char *, int, char *, ...);
int vsnprint(char *, int, char *, va_list);

/* Mappings */
#define printf print
#define fprintf(stream, ...) print(__VA_ARGS__)
#define snprintf(str, size, ...) snprint(str, size, __VA_ARGS__)
#define vsnprintf(str, size, fmt, ap) vsnprint(str, size, fmt, ap)
#define sprintf(str, ...) sprint(str, __VA_ARGS__)

/* Putc for debug */
#define putc(c, stream) print("%c", c)
#define fputc(c, stream) print("%c", c)
#define fputs(s, stream) print("%s", s)
#define fwrite(ptr, size, nmemb, stream) print("fwrite not implemented\n")
#define fflush(stream)

#endif
