#ifndef _LIBC_H_
#define _LIBC_H_

#ifndef __FRAMAC__
#pragma lib "libc.a"
#pragma src "/sys/src/libc"
#endif

#ifdef _PORTLIB_H_
typedef long jmp_buf[16];
#ifndef __FRAMAC__
extern int setjmp(jmp_buf);
extern void longjmp(jmp_buf, int);
#endif
#else
#ifdef __FRAMAC__
typedef long jmp_buf[16];
#else
#include <setjmp.h>
#endif
#endif
#include "u.h"
#include <stdarg.h>

#define nelem(x) (sizeof(x) / sizeof((x)[0]))
#ifndef offsetof
#define offsetof(s, m) (ulong)(&(((s *)0)->m))
#endif
#define assert(x)                                                              \
  if (x) {                                                                     \
  } else {                                                                     \
    print("ASSERT FAILED: %s:%d %s\n", __FILE__, __LINE__, #x);                \
    /* _assert(#x); */                                                         \
  }

/*
 * mem routines
 */
extern void *memccpy(void *, const void *, int, usize);
extern void *memset(void *, int, usize);
extern int memcmp(const void *, const void *, usize);
extern void *memcpy(void *, const void *, usize);
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
#ifndef __FRAMAC_DECL_STRCMP
/*@
  @ requires valid_string(s1);
  @ requires valid_string(s2);
  @ terminates \true;
  @ exits \false;
  @ assigns \nothing;
  @*/
extern int strcmp(const char *s1, const char *s2);
#endif
#ifndef __FRAMAC_DECL_STRCPY
/*@
  @ requires valid_string(src);
  @ requires \valid(dst + (0..ACSL_MAXSTR-1));
  @ terminates \true;
  @ exits \false;
  @ assigns dst[0..ACSL_MAXSTR-1];
  @ ensures \result == dst;
  @*/
extern char *strcpy(char *dst, const char *src);
#endif
extern char *strecpy(char *, char *, const char *);
extern char *strdup(const char *);
extern char *strncat(char *, const char *, ulong);
extern char *strncpy(char *, const char *, ulong);
extern int strncmp(const char *, const char *, ulong);
extern char *strpbrk(const char *, const char *);
extern char *strrchr(const char *, int);
extern char *strtok(char *, char *);
#ifndef __FRAMAC_DECL_STRLEN
/*@
  @ requires valid_string(s);
  @ terminates \true;
  @ exits \false;
  @ assigns \nothing;
  @ ensures \result >= 0;
  @*/
extern ulong strlen(const char *s);
#endif
extern long strspn(char *, char *);
extern long strcspn(char *, char *);
extern char *strstr(const char *, const char *);
extern int cistrncmp(char *, char *, int);
extern int cistrcmp(char *, char *);
extern char *cistrstr(char *, char *);
extern int tokenize(char *, char **, int);

#ifndef _PORTLIB_H_
enum {
  Runemask = 0x1FFFFF, /* bits used by runes (see grep) */
};
#endif

/*
 * rune routines
 */
/*@
  @ requires s == \null || \valid_read(s + (0..UTFmax-1));
  @ assigns \nothing;
  @ ensures \result >= 1 && \result <= UTFmax;
  @*/
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
extern int runelen(long);
extern int runenlen(Rune *, int);
extern int fullrune(char *, int);
extern int utflen(char *);
extern int utfnlen(char *, long);
extern char *utfrune(char *, long);
extern char *utfrrune(char *, long);
extern char *utfutf(char *, char *);
/*@
  @ requires s1 <= es1;
  @ requires \valid(s1 + (0 .. (integer)(es1 - s1) - 1));
  @ requires valid_string(s2);
  @ assigns s1[0 .. (integer)(es1 - s1) - 1];
  @ ensures \valid(\result);
  @*/
extern char *utfecpy(char *s1, char *es1, char *s2);

extern Rune *runestrcat(Rune *, Rune *);
extern Rune *runestrchr(Rune *, Rune);
extern int runestrcmp(Rune *, Rune *);
extern Rune *runestrcpy(Rune *, Rune *);
extern Rune *runestrncpy(Rune *, Rune *, long);
extern Rune *runestrecpy(Rune *, Rune *, Rune *);
extern Rune *runestrdup(Rune *);
extern Rune *runestrncat(Rune *, Rune *, long);
extern int runestrncmp(Rune *, Rune *, long);
extern Rune *runestrrchr(Rune *, Rune);
extern long runestrlen(Rune *);
extern Rune *runestrstr(Rune *, Rune *);

enum { Maxnormctx = 1 + 30 };
typedef struct Norm Norm;
struct Norm {
  long (*getrune)(void *);
  void *ctx;

  struct {
    Rune *e;
    Rune a[Maxnormctx];
  } ibuf, obuf;

  int compose;
};
extern void norminit(Norm *, int, void *, long (*getrune)(void *));
extern long normpull(Norm *, Rune *, long, int);
extern long runecomp(Rune *, long, Rune *, long);
extern long runedecomp(Rune *, long, Rune *, long);
extern long utfcomp(char *, long, char *, long);
extern long utfdecomp(char *, long, char *, long);

extern Rune *runewbreak(Rune *);
extern char *utfwbreak(char *);
extern Rune *runegbreak(Rune *);
extern char *utfgbreak(char *);

extern Rune tolowerrune(Rune);
extern Rune totitlerune(Rune);
extern Rune toupperrune(Rune);
extern int isalpharune(Rune);
extern int islowerrune(Rune);
extern int isspacerune(Rune);
extern int istitlerune(Rune);
extern int isupperrune(Rune);
extern int isdigitrune(Rune);

/*
 * malloc
 */
extern void *malloc(ulong);
extern void *mallocz(ulong, int);
extern void free(void *);
extern ulong msize(void *);
extern void *mallocalign(ulong, ulong, long, ulong);
extern void *calloc(ulong, ulong);
extern void *realloc(void *, ulong);
extern void setmalloctag(void *, uintptr);
extern void setrealloctag(void *, uintptr);
extern uintptr getmalloctag(void *);
extern uintptr getrealloctag(void *);
extern void *malloctopoolblock(void *);

/*
 * print routines
 */
#ifndef _PORTLIB_H_
#ifndef _FMT_TYPEDEF_
#define _FMT_TYPEDEF_
typedef struct Fmt Fmt;
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
  int width;           /* width of format */
  int prec;            /* precision of format */
  ulong flags;
};
#endif
#endif

enum {
  FmtWidth = 1,
  FmtLeft = FmtWidth << 1,
  FmtPrec = FmtLeft << 1,
  FmtSharp = FmtPrec << 1,
  FmtSpace = FmtSharp << 1,
  FmtSign = FmtSpace << 1,
  FmtZero = FmtSign << 1,
  FmtUnsigned = FmtZero << 1,
  FmtShort = FmtUnsigned << 1,
  FmtLong = FmtShort << 1,
  FmtVLong = FmtLong << 1,
  FmtComma = FmtVLong << 1,
  FmtByte = FmtComma << 1,

  FmtFlag = FmtByte << 1
};

#ifndef _PORTLIB_H_
extern int print(char *fmt, ...);
extern char *seprint(char *, char *, char *, ...);
extern char *vseprint(char *out, char *eout, const char *fmt, va_list v);
extern int snprint(char *, int, char *, ...);
extern int vsnprint(char *, int, char *, va_list);
extern char *smprint(char *, ...);
extern char *vsmprint(char *, va_list);
extern int sprint(char *, char *, ...);
extern int fprint(int, char *, ...);
extern int vfprint(int, char *, va_list);

extern int runesprint(Rune *, char *, ...);
extern int runesnprint(Rune *, int, char *, ...);
extern int runevsnprint(Rune *, int, char *, va_list);
extern Rune *runeseprint(Rune *, Rune *, char *, ...);
extern Rune *runevseprint(Rune *, Rune *, char *, va_list);
extern Rune *runesmprint(char *, ...);
extern Rune *runevsmprint(char *, va_list);
#endif

extern int fmtfdinit(Fmt *, int, char *, int);
extern int fmtfdflush(Fmt *);
extern int fmtstrinit(Fmt *);
extern char *fmtstrflush(Fmt *);
extern int runefmtstrinit(Fmt *);
extern Rune *runefmtstrflush(Fmt *);

#ifndef __FRAMAC__
#pragma varargck argpos fmtprint 2
#pragma varargck argpos fprint 2
#pragma varargck argpos print 1
#pragma varargck argpos runeseprint 3
#pragma varargck argpos runesmprint 1
#pragma varargck argpos runesnprint 3
#pragma varargck argpos runesprint 2
#pragma varargck argpos seprint 3
#pragma varargck argpos smprint 1
#pragma varargck argpos snprint 3
#pragma varargck argpos sprint 2

#pragma varargck type "lld" vlong
#pragma varargck type "llo" vlong
#pragma varargck type "llx" vlong
#pragma varargck type "llb" vlong
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
#pragma varargck type "d" int
#pragma varargck type "o" int
#pragma varargck type "x" int
#pragma varargck type "c" int
#pragma varargck type "C" int
#pragma varargck type "b" int
#pragma varargck type "d" uint
#pragma varargck type "x" uint
#pragma varargck type "c" uint
#pragma varargck type "C" uint
#pragma varargck type "b" uint
#pragma varargck type "f" double
#pragma varargck type "e" double
#pragma varargck type "g" double
#pragma varargck type "s" char *
#pragma varargck type "q" char *
#pragma varargck type "S" Rune *
#pragma varargck type "Q" Rune *
#pragma varargck type "r" void
#pragma varargck type "%" void
#pragma varargck type "n" int *
#pragma varargck type "p" uintptr
#pragma varargck type "p" void *
#pragma varargck flag ','
#pragma varargck flag ' '
#pragma varargck flag 'h'
#pragma varargck type "<" void *
#pragma varargck type "[" void *
#pragma varargck type "H" void *
#pragma varargck type "lH" void *
#endif

extern int fmtinstall(int, int (*)(Fmt *));
#include "acsl_bounds.h"

/*@
  @ requires \valid(f);
  @ requires \valid_read(fmt + (0..ACSL_MAX_FMT_LEN-1));
  @ requires \exists integer k; 0 <= k < ACSL_MAX_FMT_LEN && fmt[k] == '\0';
  @ assigns *f;
  @ ensures \result >= -1;
  @*/
extern int dofmt(Fmt *f, char *fmt);
extern int dorfmt(Fmt *, Rune *);
extern int fmtprint(Fmt *, char *, ...);
extern int fmtvprint(Fmt *, char *, va_list);
extern int fmtrune(Fmt *, int);
extern int fmtstrcpy(Fmt *, char *);
extern int fmtrunestrcpy(Fmt *, Rune *);
/*
 * error string for %r
 * supplied on per os basis, not part of fmt library
 */
extern int errfmt(Fmt *f);

/*
 * quoted strings
 */
extern char *unquotestrdup(char *);
extern Rune *unquoterunestrdup(Rune *);
extern char *quotestrdup(char *);
extern Rune *quoterunestrdup(Rune *);
extern int quotestrfmt(Fmt *);
extern int quoterunestrfmt(Fmt *);
extern void quotefmtinstall(void);
extern int (*doquote)(int);
extern int needsrcquote(int);

/*
 * random number
 */
extern void srand(long);
extern int rand(void);
extern int nrand(int);
extern long lrand(void);
extern long lnrand(long);
extern double frand(void);
extern ulong truerand(void);   /* uses /dev/random */
extern ulong ntruerand(ulong); /* uses /dev/random */

/*
 * math
 */
extern ulong getfcr(void);
extern void setfsr(ulong);
extern ulong getfsr(void);
extern void setfcr(ulong);
extern double NaN(void);
extern double Inf(int);
extern int isNaN(double);
extern int isInf(double, int);
extern ulong umuldiv(ulong, ulong, ulong);
extern long muldiv(long, long, long);

extern double pow(double, double);
extern double atan2(double, double);
extern double fabs(double);
extern double atan(double);
extern double log(double);
extern double log10(double);
extern double exp(double);
extern double floor(double);
extern double ceil(double);
extern double hypot(double, double);
extern double sin(double);
extern double cos(double);
extern double tan(double);
extern double asin(double);
extern double acos(double);
extern double sinh(double);
extern double cosh(double);
extern double tanh(double);
extern double sqrt(double);
extern double fmod(double, double);

#define HUGE 3.4028234e38
#define PIO2 1.570796326794896619231e0
#define PI (PIO2 + PIO2)

/*
 * Time-of-day
 */
typedef struct Tzone Tzone;
#ifndef __FRAMAC__
#pragma incomplete Tzone
#endif

typedef struct Tm {
  int nsec;      /* nseconds (range 0...1e9) */
  int sec;       /* seconds (range 0..60) */
  int min;       /* minutes (0..59) */
  int hour;      /* hours (0..23) */
  int mday;      /* day of the month (1..31) */
  int mon;       /* month of the year (0..11) */
  int year;      /* year A.D. */
  int wday;      /* day of week (0..6, Sunday = 0) */
  int yday;      /* day of year (0..365) */
  char zone[16]; /* time zone name */
  int tzoff;     /* time zone delta from GMT */
  Tzone *tz;     /* time zone associated with this date */
} Tm;

typedef struct Tmfmt {
  char *fmt;
  Tm *tm;
} Tmfmt;

#ifndef __FRAMAC__
#pragma varargck type "τ" Tmfmt
#endif

extern Tzone *tzload(char *name);
extern Tm *tmnow(Tm *, Tzone *);
extern Tm *tmtime(Tm *, vlong, Tzone *);
extern Tm *tmtimens(Tm *, vlong, int, Tzone *);
extern Tm *tmparse(Tm *, char *, char *, Tzone *, char **ep);
extern vlong tmnorm(Tm *);
extern Tmfmt tmfmt(Tm *, char *);
extern void tmfmtinstall(void);

extern Tm *gmtime(long);
extern Tm *localtime(long);
extern char *asctime(Tm *);
extern char *ctime(long);
extern double cputime(void);
extern long times(long *);
extern long tm2sec(Tm *);
extern vlong nsec(void);

extern void (*cycles)(uvlong *); /* 64-bit value of the cycle counter if there
                                    is one, 0 if there isn't */

/*
 * one-of-a-kind
 */
enum {
  PNPROC = 1,
  PNGROUP = 2,
};

extern void _assert(char *);
extern int abs(int);
extern int atexit(void (*)(void));
extern void atexitdont(void (*)(void));
extern int atnotify(int (*)(void *, char *), int);
extern double atof(char *);
extern int atoi(const char *);
extern long atol(char *);
extern vlong atoll(char *);
extern double charstod(int (*)(void *), void *);
extern char *cleanname(char *);
extern int decrypt(void *, void *, int);
extern int encrypt(void *, void *, int);

extern int dec64(uchar *, int, char *, int);
extern int enc64(char *, int, uchar *, int);
extern int dec64x(uchar *, int, char *, int, int (*)(int));
extern int enc64x(char *, int, uchar *, int, int (*)(int));
extern int dec32(uchar *, int, char *, int);
extern int enc32(char *, int, uchar *, int);
extern int dec32x(uchar *, int, char *, int, int (*)(int));
extern int enc32x(char *, int, uchar *, int, int (*)(int));
extern int dec16(uchar *, int, char *, int);
extern int enc16(char *, int, uchar *, int);
extern int dec64chr(int);
extern int enc64chr(int);
extern int dec32chr(int);
extern int enc32chr(int);
extern int dec16chr(int);
extern int enc16chr(int);

extern int encodefmt(Fmt *);
extern _Noreturn void exits(char *);
extern double frexp(double, int *);
extern uintptr getcallerpc(void *);
extern char *getenv(char *);
extern int getfields(char *, char **, int, int, char *);
extern int gettokens(char *, char **, int, char *);
extern char *getuser(void);
extern char *getwd(char *, int);
extern int iounit(int);
extern long labs(long);
extern double ldexp(double, int);
#ifndef __FRAMAC__
extern _Noreturn void longjmp(jmp_buf, int);
#endif
extern char *mktemp(char *);
extern double modf(double, double *);
extern int netcrypt(void *, void *);
extern void notejmp(void *, jmp_buf, int);
extern void perror(char *);
extern int postnote(int, int, char *);
extern double pow10(int);
extern int putenv(char *, char *);
extern void qsort(void *, usize, usize, int (*)(const void *, const void *));
#ifndef __FRAMAC__
extern int setjmp(jmp_buf);
#endif
extern double strtod(const char *, char **);
extern long strtol(const char *, char **, int);
extern ulong strtoul(const char *, char **, int);
extern vlong strtoll(const char *, char **, int);
extern uvlong strtoull(const char *, char **, int);
extern _Noreturn void sysfatal(char *, ...);
#ifndef __FRAMAC__
#pragma varargck argpos sysfatal 1
extern void syslog(int, char *, char *, ...);
#pragma varargck argpos syslog 3
#endif
extern long time(long *);
extern int tolower(int);
extern int toupper(int);

/*
 *  profiling
 */
enum {
  Profoff,    /* No profiling */
  Profuser,   /* Measure user time only (default) */
  Profkernel, /* Measure user + kernel time */
  Proftime,   /* Measure total time */
  Profsample, /* Use clock interrupt to sample (default when there is no cycle
                 counter) */
}; /* what */
extern void prof(void (*fn)(void *), void *arg, int entries, int what);

/*
 *  synchronization
 */
typedef struct Lock {
  int val;
} Lock;

extern int _tas(int *);

extern void lock(Lock *);
extern void unlock(Lock *);
extern int canlock(Lock *);

typedef struct QLp QLp;
struct QLp {
  int inuse;
  int state;
  QLp *next;
};

typedef struct QLock {
  Lock lock;
  int locked;
  QLp *head;
  QLp *tail;
} QLock;

extern void qlock(QLock *);
extern void qunlock(QLock *);
extern int canqlock(QLock *);
extern void
_qlockinit(void *(*)(void *, void *)); /* called only by the thread library */

typedef struct RWLock {
  Lock lock;
  int readers; /* number of readers */
  int writer;  /* number of writers */
  QLp *head;   /* list of waiting processes */
  QLp *tail;
} RWLock;

extern void rlock(RWLock *);
extern void runlock(RWLock *);
extern int canrlock(RWLock *);
extern void wlock(RWLock *);
extern void wunlock(RWLock *);
extern int canwlock(RWLock *);

typedef struct Rendez {
  QLock *l;
  QLp *head;
  QLp *tail;
} Rendez;

extern void rsleep(Rendez *); /* unlocks r->l, sleeps, locks r->l again */
extern int rwakeup(Rendez *);
extern int rwakeupall(Rendez *);
extern void **privalloc(void);

extern void procsetname(char *, ...);
#ifndef __FRAMAC__
#pragma varargck argpos procsetname 1
#endif

/*
 * atomic operations
 */

typedef struct Along {
  long v;
} Along;

typedef struct Avlong {
  vlong v;
} Avlong;

typedef struct Aptr {
  void *v;
} Aptr;

extern long agetl(Along *);
extern vlong agetv(Avlong *);
extern void *agetp(Aptr *);

extern long aswapl(Along *, long);
extern vlong aswapv(Avlong *, vlong);
extern void *aswapp(Aptr *, void *);

extern long aincl(Along *, long);
extern vlong aincv(Avlong *, vlong);

extern int acasl(Along *, long, long);
extern int acasv(Avlong *, vlong, vlong);
extern int acasp(Aptr *, void *, void *);

extern void coherence(void);

/*
 *  network dialing
 */
#define NETPATHLEN 40
extern int accept(int, char *);
extern int announce(char *, char *);
extern int dial(char *, char *, char *, int *);
extern void setnetmtpt(char *, int, char *);
extern int hangup(int);
extern int listen(char *, char *);
extern char *netmkaddr(char *, char *, char *);
extern char *netmkaddrbuf(char *, char *, char *, char *, int);
extern int reject(int, char *, char *);

/*
 *  encryption
 */
extern int pushtls(int, char *, char *, int, char *, char *);

/*
 *  network services
 */
typedef struct NetConnInfo NetConnInfo;
struct NetConnInfo {
  char *dir;   /* connection directory */
  char *root;  /* network root */
  char *spec;  /* binding spec */
  char *lsys;  /* local system */
  char *lserv; /* local service */
  char *rsys;  /* remote system */
  char *rserv; /* remote service */
  char *laddr; /* local address */
  char *raddr; /* remote address */
};
extern NetConnInfo *getnetconninfo(char *, int);
extern void freenetconninfo(NetConnInfo *);

extern int idn2utf(char *, char *, int);
extern int utf2idn(char *, char *, int);

/*
 * system calls
 *
 */
#define IOUNIT 32768U  /* default buffer size for 9p io */
#define STATMAX 65535U /* max length of machine-independent stat structure */
#define DIRMAX (sizeof(Dir) + STATMAX) /* max length of Dir structure */
#define ERRMAX 128                     /* max length of error string */

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
#define OCEXEC 32    /* or'ed in, close on exec */
#define ORCLOSE 64   /* or'ed in, remove on close */
#define OEXCL 0x1000 /* or'ed in, exclusive use (create only) */

#define AEXIST 0 /* accessible: exists */
#define AEXEC 1  /* execute access */
#define AWRITE 2 /* write access */
#define AREAD 4  /* read access */

/* Segattch */
#define SG_RONLY 0040 /* read only */
#define SG_CEXEC 0100 /* detach on exec */

#define NCONT 0 /* continue after note */
#define NDFLT 1 /* terminate after note */
#define NSAVE 2 /* clear note but hold state */
#define NRSTR 3 /* restore saved state */

/* bits in Qid.type */
#define QTDIR 0x80    /* type bit for directories */
#define QTAPPEND 0x40 /* type bit for append only files */
#define QTEXCL 0x20   /* type bit for exclusive use files */
#define QTMOUNT 0x10  /* type bit for mounted channel */
#define QTAUTH 0x08   /* type bit for authentication file */
#define QTTMP 0x04    /* type bit for not-backed-up file */
#define QTFILE 0x00   /* plain file */

/* bits in Dir.mode */
#define DMDIR 0x80000000    /* mode bit for directories */
#define DMAPPEND 0x40000000 /* mode bit for append only files */
#define DMEXCL 0x20000000   /* mode bit for exclusive use files */
#define DMMOUNT 0x10000000  /* mode bit for mounted channel */
#define DMAUTH 0x08000000   /* mode bit for authentication file */
#define DMTMP 0x04000000    /* mode bit for non-backed-up files */
#define DMREAD 0x4          /* mode bit for read permission */
#define DMWRITE 0x2         /* mode bit for write permission */
#define DMEXEC 0x1          /* mode bit for execute permission */

/* rfork */
enum {
  RFNAMEG = (1 << 0),
  RFENVG = (1 << 1),
  RFFDG = (1 << 2),
  RFNOTEG = (1 << 3),
  RFPROC = (1 << 4),
  RFMEM = (1 << 5),
  RFNOWAIT = (1 << 6),
  RFCNAMEG = (1 << 10),
  RFCENVG = (1 << 11),
  RFCFDG = (1 << 12),
  RFREND = (1 << 13),
  RFNOMNT = (1 << 14)
};

#ifndef _QID_DEFINED_
#define _QID_DEFINED_
typedef struct Qid {
  uvlong path;
  ulong vers;
  uchar type;
} Qid;
#endif

#ifndef _DIR_DEFINED_
#define _DIR_DEFINED_
typedef struct Dir {
  /* system-modified data */
  ushort type; /* server type */
  uint dev;    /* server subtype */
  /* file data */
  Qid qid;      /* unique id from server */
  ulong mode;   /* permissions */
  ulong atime;  /* last read time */
  ulong mtime;  /* last write time */
  vlong length; /* file length */
  char *name;   /* last element of path */
  char *uid;    /* owner name */
  char *gid;    /* group name */
  char *muid;   /* last modifier name */
} Dir;
#endif

#ifndef _WAITMSG_DEFINED_
#define _WAITMSG_DEFINED_
typedef struct Waitmsg {
  int pid;       /* of loved one */
  ulong time[3]; /* of loved one & descendants */
  char *msg;
} Waitmsg;
#endif

typedef struct IOchunk {
  void *addr;
  ulong len;
} IOchunk;

extern _Noreturn void _exits(char *);

extern _Noreturn void abort(void);
extern int access(char *, int);
extern long alarm(ulong);
extern int await(char *, int);
extern int bind(char *, char *, int);
extern int brk(void *);
extern int chdir(char *);
extern int close(int);
extern int create(char *, int, ulong);
extern int dup(int, int);
extern int errstr(char *, uint);
extern int exec(char *, char *[]);
extern int execl(char *, ...);
extern int fork(void);
extern int rfork(int);
extern int fauth(int, char *);
extern int fstat(int, uchar *, int);
extern int fwstat(int, uchar *, int);
extern int fversion(int, int, char *, int);
extern int mount(int, int, char *, int, char *);
extern int unmount(char *, char *);
extern int noted(int);
extern int notify(void (*)(void *, char *));
extern int open(char *, int);
extern int fd2path(int, char *, int);
extern int pipe(int *);
extern long pread(int, void *, long, vlong);
extern long preadv(int, IOchunk *, int, vlong);
extern long pwrite(int, void *, long, vlong);
extern long pwritev(int, IOchunk *, int, vlong);
extern long read(int, void *, long);
extern long readn(int, void *, long);
extern long readv(int, IOchunk *, int);
extern int remove(char *);
extern void *sbrk(usize);
extern long oseek(int, long, int);
extern vlong seek(int, vlong, int);
extern void *segattach(int, char *, void *, ulong);
extern void *segbrk(void *, void *);
extern int segdetach(void *);
extern int segflush(void *, ulong);
extern int segfree(void *, ulong);
extern int semacquire(long *, int);
extern long semrelease(long *, long);
extern int sleep(long);
extern int stat(char *, uchar *, int);
extern int tsemacquire(long *, ulong);
extern Waitmsg *wait(void);
extern int waitpid(void);
extern long write(int, void *, long);
extern long writev(int, IOchunk *, int);
extern int wstat(char *, uchar *, int);
extern void *rendezvous(void *, void *);

extern Dir *dirstat(char *);
extern Dir *dirfstat(int);
extern int dirwstat(char *, Dir *);
extern int dirfwstat(int, Dir *);
extern long dirread(int, Dir **);
extern void nulldir(Dir *);
extern long dirreadall(int, Dir **);
extern int getpid(void);
extern int getppid(void);
extern void rerrstr(char *, uint);
extern char *sysname(void);
extern void werrstr(char *, ...);
#pragma varargck argpos werrstr 1

extern long ainc(long *);
extern long adec(long *);

extern char *argv0;
#define ARGBEGIN                                                               \
  for ((argv0 || (argv0 = *argv)), argv++, argc--;                             \
       argv[0] && argv[0][0] == '-' && argv[0][1]; argc--, argv++) {           \
    char *_args, *_argt;                                                       \
    Rune _argc;                                                                \
    _args = &argv[0][1];                                                       \
    if (_args[0] == '-' && _args[1] == 0) {                                    \
      argc--;                                                                  \
      argv++;                                                                  \
      break;                                                                   \
    }                                                                          \
    _argc = 0;                                                                 \
    while (*_args && (_args += chartorune(&_argc, _args)))                     \
      switch (_argc)
#define ARGEND                                                                 \
  SET(_argt);                                                                  \
  USED(_argt, _argc, _args);                                                   \
  }                                                                            \
  USED(argv, argc);
#define ARGF()                                                                 \
  (_argt = _args, _args = "",                                                  \
   (*_argt    ? _argt                                                          \
    : argv[1] ? (argc--, *++argv)                                              \
              : 0))
#define EARGF(x)                                                               \
  (_argt = _args, _args = "",                                                  \
   (*_argt    ? _argt                                                          \
    : argv[1] ? (argc--, *++argv)                                              \
              : ((x), abort(), (char *)0)))

#define ARGC() _argc

/* this is used by sbrk and brk,  it's a really bad idea to redefine it */
extern char end[];
#endif /* _LIBC_H_ */
