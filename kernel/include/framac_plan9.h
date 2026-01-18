/**
 * Frama-C Plan 9 Compatibility Layer
 *
 * This header provides stub definitions for Plan 9 types and constructs
 * so that Frama-C can parse and verify Plan 9 kernel code.
 */

#ifndef FRAMAC_PLAN9_H
#define FRAMAC_PLAN9_H

#ifdef __FRAMAC__

// Suppress Plan 9 pragmas that Frama-C doesn't understand
#define __plan9_pragma_varargck(...)
#define __plan9_pragma_lib(...)
#define __plan9_pragma_src(...)
#define __plan9_pragma_incomplete(...)

// Redefine pragma statements
#pragma push_macro("varargck")
#pragma push_macro("lib")
#pragma push_macro("src")
#pragma push_macro("incomplete")

#define varargck __plan9_pragma_varargck
#define lib __plan9_pragma_lib
#define src __plan9_pragma_src
#define incomplete __plan9_pragma_incomplete

// Basic Plan 9 integer types
typedef unsigned char uchar;
typedef unsigned short ushort;
typedef unsigned int uint;
typedef unsigned long ulong;
typedef unsigned long long uvlong;
typedef long long vlong;

typedef unsigned char u8int;
typedef unsigned short u16int;
typedef unsigned int u32int;
typedef unsigned long long u64int;

typedef char s8int;
typedef short s16int;
typedef int s32int;
typedef long long s64int;

typedef unsigned long uintptr;
typedef unsigned long usize;
typedef long intptr;
typedef long size;

// UUID type
typedef unsigned char uuid_t[16];

// Plan 9 formatted I/O types
typedef struct Fmt {
  unsigned char runes;
  void *start;
  void *to;
  void *stop;
  int (*flush)(struct Fmt *);
  void *farg;
  int nfmt;
  void *args;
  int r;
  int width;
  int prec;
  unsigned long flags;
} Fmt;

typedef int (*Fmts)(Fmt *);

// Plan 9 locking primitives
typedef struct QLock {
  int locked;
  void *head;
  void *tail;
  int use;
} QLock;

typedef struct Lock {
  unsigned int key;
  int isilock;
  unsigned long pc;
  void *p;
  void *m;
  uvlong lockcycles;
} Lock;

typedef struct RWlock {
  Lock l;
  int readers;
  int writer;
  void *head;
  void *tail;
} RWlock;

typedef struct Rendez {
  Lock l;
  void *p;
} Rendez;

// Plan 9 process and channel types
typedef struct Proc Proc;
typedef struct Chan Chan;
typedef struct Pgrp Pgrp;
typedef struct Segment Segment;
typedef struct Mnt Mnt;
typedef struct Mount Mount;
typedef struct Mhead Mhead;
typedef struct Mntrpc Mntrpc;
typedef struct Mntcache Mntcache;

// Stub definitions for opaque types
struct Proc {
  int pid;
  int state;
  char *text;
  void *user;
  uvlong time[6];
  int insyscall;
  QLock debug;
  void *fpstate;
  // ... other fields stubbed
};

struct Chan {
  Lock l;
  int ref;
  int flag;
  int devno;
  ushort mode;
  ushort qid_type;
  uint qid_vers;
  uvlong qid_path;
  uvlong offset;
  void *aux;
  Chan *next;
  // ... other fields stubbed
};

struct Pgrp {
  int ref;
  RWlock ns;
  QLock debug;
  Chan *slash;
  Chan *dot;
  // ... other fields stubbed
};

struct Segment {
  int ref;
  QLock lk;
  ushort type;
  ushort flags;
  void *base;
  usize size;
  // ... other fields stubbed
};

// Plan 9 device driver types
typedef struct Cmdbuf Cmdbuf;
typedef struct Cmdtab Cmdtab;
typedef struct Dirtab Dirtab;
typedef struct Walkqid Walkqid;
typedef struct Dev Dev;

struct Dirtab {
  char *name;
  uvlong qid;
  vlong length;
  long perm;
};

struct Walkqid {
  Chan *clone;
  int nqid;
  void *qid[16];
};

// Plan 9 directory and file info
typedef struct Dir Dir;
typedef struct Qid Qid;

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

// Plan 9 network and 9P types
typedef struct Fcall Fcall;
struct Fcall {
  uchar type;
  u32int fid;
  ushort tag;
  union {
    struct {
      u32int msize;
      char *version;
    };
    struct {
      u32int afid;
      char *uname;
      char *aname;
    };
    // ... other message types stubbed
  };
};

// Plan 9 authentication
typedef struct Authinfo Authinfo;
struct Authinfo {
  char *cuid;
  char *suid;
  char *cap;
  int ncap;
  uchar *secret;
  int nsecret;
};

// Plan 9 string functions - declare as extern to avoid conflicts
/*@ assigns \nothing; */
extern int utflen(char *s);

/*@ assigns \nothing; */
extern int utfnlen(char *s, long m);

/*@ assigns \result; */
extern char *utfrune(char *s, long c);

// varargs support
// varargs support
typedef __builtin_va_list va_list;

#undef va_start
#undef va_end
#undef va_arg
#undef va_copy

#define va_start(ap, last) __builtin_va_start(ap, last)
#define va_end(ap) __builtin_va_end(ap)
#define va_arg(ap, type) __builtin_va_arg(ap, type)
#define va_copy(dest, src) __builtin_va_copy(dest, src)

// Plan 9 formatted I/O functions (stubs)
/*@ assigns \result \from fmt; */
extern int print(char *fmt, ...);
extern int sprint(char *buf, char *fmt, ...);
extern int snprint(char *buf, int len, char *fmt, ...);
extern int vsnprint(char *buf, int len, char *fmt, va_list args);
extern int fmtprint(Fmt *f, char *fmt, ...);
extern int fmtstrcpy(Fmt *f, char *s);
extern int fmtinstall(int c, int (*f)(Fmt *));
extern int fmtstrinit(Fmt *f);

// Standard C memory/string functions are already declared
// Don't redeclare them to avoid conflicts

// Plan 9 error handling
extern void error(char *msg);
extern void panic(char *fmt, ...);

// Plan 9 wait/sleep
extern void sleep(Rendez *r, int (*f)(void *), void *arg);
extern int wakeup(Rendez *r);
extern void tsleep(Rendez *r, int (*f)(void *), void *arg, ulong ms);

// Plan 9 process management
extern Proc *up; /* current process (global) */
extern void yield(void);
extern void sched(void);
extern Proc *newproc(void);

// Locking functions
/*@ assigns l->locked; */
extern void qlock(QLock *l);

/*@ assigns l->locked; */
extern void qunlock(QLock *l);

/*@ assigns \nothing; */
extern int canqlock(QLock *l);

/*@ assigns lk->key; */
extern void lock(Lock *lk);

/*@ assigns lk->key; */
extern void unlock(Lock *lk);

// Channel operations
/*@ assigns c->ref; */
extern void cclose(Chan *c);

/*@ assigns c->ref; */
extern Chan *cclone(Chan *c);

/*@ assigns \result; */
extern Chan *namec(char *name, int amode, int omode, ulong perm);

// Error constants
// Error constants
#ifndef ERRMAX
enum { ERRMAX = 128, KNAMELEN = 28 };
#endif

// Architecture-specific but needed
typedef struct Ureg Ureg;
struct Ureg {
  u64int ax;
  u64int bx;
  u64int cx;
  u64int dx;
  u64int si;
  u64int di;
  u64int bp;
  u64int r8;
  u64int r9;
  u64int r10;
  u64int r11;
  u64int r12;
  u64int r13;
  u64int r14;
  u64int r15;
  // ... other registers stubbed
};

#endif /* __FRAMAC__ */
#endif /* FRAMAC_PLAN9_H */
