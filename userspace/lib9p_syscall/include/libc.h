#ifndef _LIBC_H_
#define _LIBC_H_

#include <u.h>

#define OREAD 0
#define OWRITE 1
#define ORDWR 2
#define OEXEC 3
#define OTRUNC 16

typedef struct Qid {
  u64int path;
  u32int vers;
  u8int type;
} Qid;

typedef struct Dir Dir;
typedef struct Fmt Fmt;

void *memmove(void *dst, const void *src, ulong n);
void *memset(void *s, int c, ulong n);
char *strcpy(char *dst, const char *src);
int strlen(const char *s);

#endif
