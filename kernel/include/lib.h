#ifndef _LIB_H_
#define _LIB_H_

#include "acsl_bounds.h"

/* If portlib.h was already included (via GNUmakefile -include), skip lib.h */
#ifdef _PORTLIB_H_
/* portlib.h provides all the same definitions - no need to duplicate */
#else
#include <stdarg.h>
/*
 * functions (possibly) linked in, complete, from libc.
 */
#define nelem(x) (sizeof(x) / sizeof((x)[0]))
#define offsetof(s, m) (ulong)(&(((s *)0)->m))
#define assert(x)                                                              \
  if (x) {                                                                     \
  } else                                                                       \
    _assert("x")

/*
 * mem routines
 */
extern void *memccpy(void *, const void *, int, usize);
/*@
  @ requires (n > 0 ==> \valid((char*)s + (0 .. (integer)n-1))) || (n == 0);
  @ terminates \true;
  @ assigns ((char*)s)[0 .. (integer)n-1] \if (n > 0);
  @ ensures \result == s;
  @*/
extern void *memset(void *s, int c, usize n);
extern int memcmp(const void *, const void *, usize);
#ifndef __FRAMAC_DECL_MEMMOVE
/*@
  @ requires \valid((char *)dest + (0..n-1));
  @ requires \valid_read((char *)src + (0..n-1));
  @ terminates \true;
  @ exits \false;
  @ assigns ((char *)dest)[0..n-1];
  @ ensures \result == dest;
  @*/
extern void *memmove(void *dest, const void *src, usize n);
#endif
extern void *memchr(const void *, int, usize);

/*
 * string routines
 */
extern char *strcat(char *, const char *);
extern char *strchr(const char *, int);
extern char *strrchr(const char *, int);
#ifndef __FRAMAC_DECL_STRCMP
/*@
  @ requires valid_string((char *)s1);
  @ requires valid_string((char *)s2);
  @ terminates \true;
  @ exits \false;
  @ assigns \nothing;
  @*/
extern int strcmp(const char *s1, const char *s2);
#endif
/*@
  @ requires valid_string((char *)src);
  @ requires \valid(dst + (0..ACSL_MAXSTR-1));
  @ terminates \true;
  @ exits \false;
  @ assigns dst[0..ACSL_MAXSTR-1];
  @ ensures \result == dst;
  @*/
#ifndef __FRAMAC_DECL_STRCPY
extern char *strcpy(char *dst, const char *src);
#endif
extern char *strecpy(char *, char *, const char *);
extern char *strncat(char *, const char *, long);
extern char *strncpy(char *, const char *, long);
extern int strncmp(const char *s1, const char *s2, long);
#ifndef __FRAMAC_DECL_STRLEN
/*@
  @ requires valid_string((char *)s);
  @ terminates \true;
  @ exits \false;
  @ assigns \nothing;
  @ ensures \result >= 0;
  @*/
extern ulong strlen(const char *s);
#endif
extern char *strstr(const char *, const char *);
extern int atoi(const char *);
extern int fullrune(char *, int);
extern int cistrcmp(char *, char *);
extern int cistrncmp(char *, char *, int);

/* UTF-8 constants moved to u.h */

/*
 * rune routines
 */
extern int runetochar(char *, Rune *);
#ifndef __FRAMAC_DECL_CHARTORUNE
/*@
  @ requires r != \null;
  @ requires s != \null;
  @ requires valid_string(s);
  @ terminates \true;
  @ exits \false;
  @ assigns *r \from s[0 .. ACSL_MAXSTR-1];
  @ ensures 1 <= \result <= 4;
  @*/
extern int chartorune(Rune *r, char *s);
#endif
/*@
  @ requires \valid(s1 + (0 .. (es1 - s1)));
  @ requires valid_string(s2);
  @ assigns s1[0 .. (es1 - s1)];
  @ ensures \valid(\result);
  @*/
extern char *utfecpy(char *s1, char *es1, char *s2);
extern char *utfrune(char *, long);
extern int utflen(char *);
extern int utfnlen(char *, long);
extern int runelen(long);

/*
 * random number
 */
extern int rand(void);
extern int nrand(int);
extern long lrand(void);
extern long lnrand(long);

extern int abs(int);

/*
 * print routines
 */
#ifndef __FRAMAC__
typedef struct Fmt Fmt;
typedef int (*Fmts)(Fmt *);
struct Fmt {
  uchar runes;         /* output buffer is runes or chars? */
  void *start;         /* of buffer */
  void *to;            /* current place in the buffer */
  void *stop;          /* end of the buffer; overwritten if flush fails */
  int (*flush)(Fmt *); /* called when to == stop */
  void *farg;          /* to make flush a closure */
  int nfmt;            /* num chars formatted so far */
  va_list args;        /* args passed to dofmt */
  int r;               /* % format Rune */
  int width;
  int prec;
  ulong flags;
};
#endif
#ifndef __FRAMAC_DECL_PRINT
/*@
  @ requires fmt != \null;
  @ terminates \true;
  @ exits \false;
  @ assigns \nothing;
  @*/
extern int print(char *fmt, ...);
#endif
extern char *seprint(char *, char *, char *, ...);
extern char *vseprint(char *, char *, const char *, va_list);
/*@
  @ requires (n > 0 ==> \valid(s + (0 .. (integer)n-1))) || (n == 0);
  @ requires valid_string(fmt);
  @ assigns s[0 .. (integer)n-1] \if (s != \null && n > 0);
  @ ensures \result >= 0;
  @*/
extern int snprint(char *s, int n, char *fmt, ...);
extern int vsnprint(char *, int, char *, va_list);
extern int sprint(char *, char *, ...);

#ifndef __FRAMAC__
#pragma varargck argpos fmtprint 2
#pragma varargck argpos print 1
#pragma varargck argpos seprint 3
#pragma varargck argpos snprint 3
#pragma varargck argpos sprint 2

#pragma varargck type "llb" vlong
#pragma varargck type "lld" vlong
#pragma varargck type "llx" vlong
#pragma varargck type "llb" uvlong
#pragma varargck type "lld" uvlong
#pragma varargck type "llo" uvlong
#pragma varargck type "llx" uvlong
#pragma varargck type "llb" uvlong
#pragma varargck type "ld" long
#pragma varargck type "lo" long
#pragma varargck type "lx" long
#pragma varargck type "lb" long
#pragma varargck type "ld" ulong
#pragma varargck type "lo" ulong
#pragma varargck type "lx" ulong
#pragma varargck type "lb" ulong
#pragma varargck type "zd" intptr
#pragma varargck type "zo" intptr
#pragma varargck type "zx" intptr
#pragma varargck type "zb" intptr
#pragma varargck type "zd" uintptr
#pragma varargck type "zo" uintptr
#pragma varargck type "zx" uintptr
#pragma varargck type "zb" uintptr
#pragma varargck type "b" int
#pragma varargck type "d" int
#pragma varargck type "x" int
#pragma varargck type "c" int
#pragma varargck type "C" int
#pragma varargck type "b" uint
#pragma varargck type "d" uint
#pragma varargck type "x" uint
#pragma varargck type "c" uint
#pragma varargck type "C" uint
#pragma varargck type "s" char *
#pragma varargck type "q" char *
#pragma varargck type "S" Rune *
#pragma varargck type "%" void
#pragma varargck type "p" uintptr
#pragma varargck type "p" void *
#pragma varargck flag ','
#endif

extern int fmtstrinit(Fmt *);
extern int fmtinstall(int, int (*)(Fmt *));
extern void quotefmtinstall(void);
extern int fmtprint(Fmt *, char *, ...);
extern int fmtstrcpy(Fmt *, char *);
extern char *fmtstrflush(Fmt *);

/*
 * one-of-a-kind
 */
extern char *cleanname(char *);
extern uintptr getcallerpc(void *);

extern long strtol(const char *, char **, int);
extern ulong strtoul(const char *, char **, int);
extern vlong strtoll(const char *, char **, int);
extern uvlong strtoull(const char *, char **, int);
extern char etext[];
extern char edata[];
extern char end[];
extern int getfields(char *, char **, int, int, char *);
extern int tokenize(char *, char **, int);
extern int dec64(uchar *, int, char *, int);
extern int dec16(uchar *, int, char *, int);
extern int encodefmt(Fmt *);
extern void qsort(void *, usize, usize, int (*)(void *, void *));

/*
 * Syscall data structures
 */
#define MORDER 0x0003  /* mask for bits defining order of mounting */
#define MREPL 0x0000   /* mount replaces object */
#define MBEFORE 0x0001 /* mount goes before others in union directory */
#define MAFTER 0x0002  /* mount goes after others in union directory */
#define MCREATE 0x0004 /* permit creation in mounted directory */
#define MCACHE 0x0010  /* cache some data */
#define MMASK 0x0017   /* all bits on */

#define OREAD 0      /* open for read */
#define OWRITE 1     /* write */
#define ORDWR 2      /* read and write */
#define OEXEC 3      /* execute, == read but check execute permission */
#define OTRUNC 16    /* or'ed in (except for exec), truncate file first */
#define OCEXEC 32    /* or'ed in (per file descriptor), close on exec */
#define ORCLOSE 64   /* or'ed in, remove on close */
#define OEXCL 0x1000 /* or'ed in, exclusive create */

#define NCONT 0 /* continue after note */
#define NDFLT 1 /* terminate after note */
#define NSAVE 2 /* clear note but hold state */
#define NRSTR 3 /* restore saved state */

#ifndef __FRAMAC__
typedef struct Qid Qid;
typedef struct Dir Dir;
typedef struct OWaitmsg OWaitmsg;
typedef struct Waitmsg Waitmsg;

#define ERRMAX 128  /* max length of error string */
#define KNAMELEN 28 /* max length of name held in kernel */

/* bits in Qid.type */
/* QT and DM bits moved to core_types.h */

/* Dir, OWaitmsg, and Waitmsg moved to core_types.h */

#endif

#endif /* _PORTLIB_H_ not defined - end of lib.h definitions */

#endif /* _LIB_H_ */
