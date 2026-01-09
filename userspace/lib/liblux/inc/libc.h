#ifndef _LIBC_H_
#define _LIBC_H_

#include <u.h>

#define MAXWELEM 16

typedef struct Lock {
  int val;
} Lock;

typedef struct Qid Qid;
typedef struct Dir Dir;

struct Qid {
  uvlong path;
  ulong vers;
  uchar type;
};

struct Dir {
  ushort type;
  uint dev;
  Qid qid;
  ulong mode;
  ulong atime;
  ulong mtime;
  vlong length;
  char *name;
  char *uid;
  char *gid;
  char *muid;
};

typedef struct Fmt Fmt;
struct Fmt {
  unsigned char runes;
  void *start;
  void *to;
  void *stop;
  int (*flush)(Fmt *);
  void *farg;
  int nfmt;
  void *args;
  int r;
  int width;
  int prec;
  unsigned long flags;
};

/* Libc functions */
void *memmove(void *dst, const void *src, ulong n);
void *memset(void *dst, int c, ulong n);
int snprint(char *buf, int n, char *fmt, ...);
int atoi(const char *s);
unsigned long long strtoull(const char *s, char **endptr, int base);
ulong strlen(const char *s);

#endif
