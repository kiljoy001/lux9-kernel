/* Frama-C Preprocessed Plan 9 Code */
/* Copyright (C) 1991-2024 Free Software Foundation, Inc.
   This file is part of the GNU C Library.
   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.
   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.
   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, see
   <https://www.gnu.org/licenses/>.  */
/* This header is separate from features.h so that the compiler can
   include it implicitly at the start of every compilation.  It must
   not itself include <features.h> or any other header that includes
   <features.h> because the implicit include comes before any feature
   test macros that may be defined in a source file before it first
   explicitly includes a system header.  GCC knows the name of this
   header in order to preinclude it.  */
/* glibc's intent is to support the IEC 559 math functionality, real
   and complex.  If the GCC (4.9 and later) predefined macros
   specifying compiler intent are available, use them to determine
   whether the overall intent is to support these features; otherwise,
   presume an older compiler has intent to support these features and
   define these macros by default.  */
/* wchar_t uses Unicode 10.0.0.  Version 10.0 of the Unicode Standard is
   synchronized with ISO/IEC 10646:2017, fifth edition, plus
   the following additions from Amendment 1 to the fifth edition:
   - 56 emoji characters
   - 285 hentaigana
   - 3 additional Zanabazar Square characters */
/*
 * wasm_dev_srv.c - /srv/wasm service registry device
 *
 * Exposes active WASM sessions as a directory.
 * - Read directory: Lists registered named sessions.
 * - Attach to file: Mounts the WASM 9P file server.
 */

/* Include base types and architecture constants first */
/*
 * Memory and machine-specific definitions.  Used in C and assembler.
 */
/*
 * Sizes
 */
/*
 * Time
 */
/*
 *  Address spaces. User:
 */
/* Per-process exchange page VA region (below the stack). */
/*
 *  Address spaces. Kernel, sorted by address.
 */
/*
 * Fundamental addresses
 */
/*
 * Where configuration info is left for the loaded programme.
 * There are 24064 bytes available at CONFADDR.
 */
/*
 *  known x86 segments (in GDT) and their selectors
 */
/*
 *  fields in segment descriptors
 */
/*
 *  virtual MMU
 */
/*
 *  physical MMU
 */
/*
 * Hierarchical Page Tables.
 * For example, traditional IA-32 paging structures have 2 levels,
 * level 1 is the PD, and level 0 the PT pages; with IA-32e paging,
 * level 3 is the PML4(!), level 2 the PDP, level 1 the PD,
 * and level 0 the PT pages. The PTLX macro gives an index into the
 * page-table page at level 'l' for the virtual address 'v'.
 */
/* PAT entry used for write combining */
/* Include base types */
/*
 * acsl_bounds.h - Machine-realistic bounds for ACSL verification
 *
 * Defines concrete, hardware-realistic limits for ACSL specifications.
 * Nothing is infinite in a real computer - all memory and numbers have bounds.
 */
/*
 * String and Buffer Bounds
 */
/*
 * Format String Bounds
 */
/*
 * Numeric Bounds (matching actual hardware)
 */
/*
 * Memory Bounds
 */
extern char isfrog[256];
/*@ axiomatic AcslBounds {
  @ logic integer ACSL_MAXSTR = 4096;
  @ logic integer ACSL_MAXBUF = 8192;
  @ logic integer ACSL_MAXPATH = 1024;
  @ logic integer ACSL_MAXNAME = 256;
  @
  @ logic integer ACSL_MAX_FMT_ARGS = 32;
  @ logic integer ACSL_MAX_FMT_LEN = 512;
  @
  @ logic integer ACSL_MAX_INT32 = 2147483647;
  @ logic integer ACSL_MIN_INT32 = -2147483648;
  @ logic integer ACSL_MAX_UINT32 = 4294967295;
  @ logic integer ACSL_MAX_INT64 = 9223372036854775807;
  @ logic integer ACSL_MIN_INT64 = -9223372036854775808;
  @ logic integer ACSL_MAX_UINT64 = 18446744073709551615;
  @
  @ logic integer ACSL_MAX_ALLOC = 1099511627776;
  @ logic integer ACSL_PAGE_SIZE = 4096;
  @} */
/*@
  @ predicate valid_string(char *s) =
  @   \exists integer n; 0 <= n < ACSL_MAXSTR &&
  @     \valid_read(s + (0..n)) &&
  @     s[n] == '\0';
  @*/
/*@
  @ predicate valid_string_or_null(char *s) =
  @   s == \null || valid_string(s);
  @
  @ predicate equal_strings(char *s1, char *s2) =
  @   valid_string(s1) && valid_string(s2) &&
  @   \exists integer n; 0 <= n < ACSL_MAXSTR &&
  @     (\forall integer i; 0 <= i <= n ==> s1[i] == s2[i]) &&
  @     s1[n] == '\0';
  @*/
/*@
  @ predicate p9_name_ok_slash(char *s, integer slashok) =
  @   valid_string(s) &&
  @   \exists integer n; 0 <= n < ACSL_MAXSTR &&
  @     s[n] == '\0' &&
  @     (\forall integer i; 0 <= i < n ==>
  @       (isfrog[(unsigned char)s[i]] == 0 ||
  @        (slashok != 0 && s[i] == '/')));
  @*/

/*
 * core_types.h - Minimal Fundamental Types
 *
 * Contains types that are used by value in many headers (like Qid, Dir)
 * and forward declarations for complex structures.
 */
/* Plan 9 universal header */
/*
 * framac_stubs.h - Stubs for GCC builtins and missing functions for Frama-C
 */
/*
 * acsl_bounds.h - Machine-realistic bounds for ACSL verification
 *
 * Defines concrete, hardware-realistic limits for ACSL specifications.
 * Nothing is infinite in a real computer - all memory and numbers have bounds.
 */
/* Minimal Rune definition for Frama-C stubs */
typedef unsigned int Rune;
/* Frama-C requires a definition for __builtin_va_list if used in typedef */
/* GCC Builtins Stubs - Frama-C now provides many of these via
 * __fc_gcc_builtins.h */
static inline unsigned long getcallerpc(void *p) {
  (void)p;
  return 0;
}
/* Attributes */
double __builtin_fabs(double x);
float __builtin_fabsf(float x);
double __builtin_inf(void);
float __builtin_inff(void);
double __builtin_nan(const char *str);
float __builtin_nanf(const char *str);
/* Attributes */
/* Types - provided by native Plan 9 headers during preprocessing */
typedef void *va_list; // Direct definition for Frama-C
/* Frama-C Specific - Must be guarded to prevent GCC pre-pass failure */
/* Frama-C stubs are limited to compiler builtins and basic types. */
/* Add static_assert support for compile-time checks */
typedef unsigned char uchar;
typedef unsigned short ushort;
typedef unsigned int uint;
typedef unsigned long ulong;
typedef unsigned long long uvlong;
typedef long long vlong;
typedef unsigned long usize;
typedef long ssize;
typedef unsigned long uintptr;
typedef long intptr;
/* Plan 9 fixed-width types */
typedef unsigned char u8int;
typedef unsigned short u16int;
typedef unsigned int u32int;
typedef unsigned long long u64int;
typedef signed char s8int;
typedef signed short s16int;
typedef signed int s32int;
typedef signed long long s64int;
typedef u32int Rune; /* UTF-8 code point */
typedef struct {
  u8int data[16];
} uuid_t;
/* UTF-8 constants (standard Plan 9) */
enum {
  UTFmax = 4,         /* maximum bytes per rune */
  Runesync = 0x80,    /* cannot represent part of a UTF sequence */
  Runeself = 0x80,    /* rune and UTF sequences are the same (<) */
  Runeerror = 0xFFFD, /* decoding error in UTF */
  Runemax = 0x10FFFF, /* 21 bit rune */
};
/* USED macro to suppress unused warnings */
/* Compile-time type size assertions */
/* Qid.type bits */
/* Forward declarations */
typedef struct Lock Lock;
typedef struct QLock QLock;
typedef struct RWLock RWLock;
typedef struct Ref Ref;
// typedef struct Qid Qid; // This forward declaration is moved into the
// _QID_DEFINED_ block
typedef struct Dir Dir;
typedef struct Chan Chan;
typedef struct Proc Proc;
typedef struct Mach Mach;
typedef struct Ureg Ureg;
typedef struct Fcall Fcall;
typedef struct Dirtab Dirtab;
typedef struct FPssestate FPssestate;
typedef struct FPavxstate FPavxstate;
typedef struct FPalloc FPalloc;
typedef struct FPsave FPsave;
typedef struct PFPU PFPU;
typedef struct ISAConf ISAConf;
typedef struct Label Label;
typedef struct MMU MMU;
typedef struct PCArch PCArch;
typedef struct Pcidev Pcidev;
typedef struct Physseg Physseg;
typedef struct Watchdog Watchdog;
typedef struct Block Block;
typedef struct Image Image;
typedef struct Page Page;
typedef struct Segment Segment;
typedef struct Schedq Schedq;
typedef struct Egrp Egrp;
typedef struct Fgrp Fgrp;
typedef struct Pgrp Pgrp;
typedef struct Rgrp Rgrp;
typedef struct Cmdbuf Cmdbuf;
typedef struct Note Note;
typedef struct Bpool Bpool;
typedef struct Timer Timer;
typedef struct Rendezq Rendezq;
// typedef struct Rendez Rendez; // Duplicate
// typedef struct Ref Ref; // Duplicate
typedef struct Mhead Mhead;
typedef struct Mntrah Mntrah;
typedef struct Walkqid Walkqid;
typedef struct Dev Dev;
typedef struct DevConf DevConf;
typedef struct Uart Uart;
typedef struct Queue Queue;
/*
 * Universal Plan 9 Types (defined by value)
 */
typedef vlong Tval;
struct Label {
  uintptr sp;  /* offset 0 */
  uintptr pc;  /* offset 8 */
  uintptr rbp; /* offset 16 - frame pointer for local variables */
  uintptr rbx; /* offset 24 - callee-saved */
  uintptr r12; /* offset 32 - callee-saved */
  uintptr r13; /* offset 40 - callee-saved */
  uintptr r14; /* offset 48 - callee-saved */
  uintptr r15; /* offset 56 - callee-saved */
};
typedef struct OWaitmsg OWaitmsg;
struct OWaitmsg {
  char pid[12];      /* of loved one */
  char time[3 * 12]; /* of loved one and descendants */
  char msg[64];      /* compatibility BUG */
};
typedef struct Waitmsg Waitmsg;
struct Waitmsg {
  int pid;       /* of loved one */
  ulong time[3]; /* of loved one and descendants */
  char msg[128]; /* actually variable-size in user mode */
};
/* Qid.type bits defined above */
typedef struct Qid Qid;
struct Qid {
  uvlong path;
  ulong vers;
  uchar type;
};
/* Directory Mode bits */
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
/*
 * Lock types must NOT be defined here as they are architecture-specific.
 * Use forward declarations only.
 */
/* Plan 9 universal header */
/* Copyright (C) 1989-2023 Free Software Foundation, Inc.
This file is part of GCC.
GCC is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 3, or (at your option)
any later version.
GCC is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.
Under Section 7 of GPL version 3, you are granted additional
permissions described in the GCC Runtime Library Exception, version
3.1, as published by the Free Software Foundation.
You should have received a copy of the GNU General Public License and
a copy of the GCC Runtime Library Exception along with this program;
see the files COPYING3 and COPYING.RUNTIME respectively.  If not, see
<http://www.gnu.org/licenses/>.  */
/*
 * ISO C Standard:  7.15  Variable arguments  <stdarg.h>
 */
/* Define __gnuc_va_list.  */
typedef void *__gnuc_va_list;
/* Define the standard macros for the user,
   if this invocation was from the user program.  */
/* Define va_list, if desired, from __gnuc_va_list. */
/* We deliberately do not define va_list when called from
   stdio.h, because ANSI C says that stdio.h is not supposed to define
   va_list.  stdio.h needs to have access to that data type,
   but must not use that name.  It should use the name __gnuc_va_list,
   which is safe because it is reserved for the implementation.  */
/* The macro _VA_LIST_ is the same thing used by this file in Ultrix.
   But on BSD NET2 we must not test or define or undef it.
   (Note that the comments in NET 2's ansi.h
   are incorrect for _VA_LIST_--see stdio.h!)  */
/* The macro _VA_LIST_DEFINED is used in Windows NT 3.5  */
/* The macro _VA_LIST is used in SCO Unix 3.2.  */
/* The macro _VA_LIST_T_H is used in the Bull dpx2  */
/* The macro __va_list__ is used by BeOS.  */
typedef __gnuc_va_list va_list;
typedef unsigned int Rune;
/*
 * mem routines
 */
extern void *memccpy(void *, const void *, int, usize);
/*@
  @ requires \valid(((char*)s) + (0 .. (integer)n - 1));
  @ terminates \true;
  @ assigns ((char*)s)[0 .. (integer)n - 1];
  @ assigns \result \from s;
  @ ensures \result == s;
  @*/
extern void *memset(void *s, int c, usize n);
/*@
  @ requires \valid_read(((char*)s1) + (0 .. (integer)n - 1));
  @ requires \valid_read(((char*)s2) + (0 .. (integer)n - 1));
  @ assigns \nothing;
  @*/
extern int memcmp(const void *s1, const void *s2, usize n);
/*@
  @ requires \valid(((char*)dest) + (0 .. (integer)n - 1));
  @ requires \valid_read(((char*)src) + (0 .. (integer)n - 1));
  @ terminates \true;
  @ exits \false;
  @ assigns ((char*)dest)[0 .. (integer)n - 1];
  @ assigns \result \from dest;
  @ ensures \result == dest;
  @*/
extern void *memmove(void *dest, const void *src, usize n);
extern void *memchr(const void *, int, usize);
/*
 * string routines
 */
extern char *strcat(char *, char *);
extern char *strchr(char *, int);
extern char *strrchr(char *, int);
/*@ requires s1 == \null || valid_string(s1);
  @ requires s2 == \null || valid_string(s2);
  @ terminates \true;
  @ exits \false;
  @ assigns \nothing;
  @*/
extern int strcmp(char *s1, char *s2);
/*@ requires s1 == \null || valid_string(s1);
  @ requires s2 == \null || valid_string(s2);
  @ terminates \true;
  @ exits \false;
  @ assigns s1[0 .. ACSL_MAXSTR-1];
  @ ensures valid_string(s1);
  @*/
extern char *strcpy(char *s1, char *s2);
extern char *strecpy(char *, char *, char *);
extern char *strncat(char *, char *, long);
extern char *strncpy(char *, char *, long);
/*@ requires n >= 0;
  @ requires \valid_read((char*)s1 + (0..n-1)) || valid_string(s1);
  @ requires \valid_read((char*)s2 + (0..n-1)) || valid_string(s2);
  @ terminates \true;
  @ assigns \nothing;
  @*/
extern int strncmp(char *s1, char *s2, long n);
/*@
  @ requires s != \null;
  @ requires valid_string(s);
  @ terminates \true;
  @ exits \false;
  @ assigns \nothing;
  @ ensures \result >= 0;
  @*/
extern long strlen(char *s);
extern char *strstr(char *, char *);
extern int atoi(char *);
extern int fullrune(char *, int);
extern int cistrcmp(char *, char *);
/*@ requires n >= 0;
  @ requires \valid_read((char*)s1 + (0..n-1)) || valid_string(s1);
  @ requires \valid_read((char*)s2 + (0..n-1)) || valid_string(s2);
  @ terminates \true;
  @ assigns \nothing;
  @*/
extern int cistrncmp(char *s1, char *s2, int n);
/* UTF-8 constants moved to u.h */
/*
 * rune routines
 */
extern int runetochar(char *, Rune *);
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
/*@
  @ requires \valid(s1 + (0 .. (es1 - s1)));
  @ requires valid_string(s2);
  @ assigns s1[0 .. (es1 - s1)];
  @ ensures \valid(\result);
  @*/
extern char *utfecpy(char *s1, char *es1, char *s2);
/*@ requires s != \null;
  @ assigns \nothing;
  @*/
extern char *utfrune(char *s, long c);
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
/*@
  @ requires fmt != \null;
  @ terminates \true;
  @ exits \false;
  @ assigns \nothing;
  @*/
extern int print(char *fmt, ...);
/*@
  @ requires \valid(s + (0 .. ACSL_MAXSTR-1));
  @ requires valid_string(fmt);
  @ assigns s[0 .. ACSL_MAXSTR-1];
  @ ensures \valid(\result);
  @*/
extern char *seprint(char *s, char *e, char *fmt, ...);
/*@
  @ requires \valid(s + (0 .. ACSL_MAXSTR-1));
  @ requires valid_string(fmt);
  @ assigns s[0 .. ACSL_MAXSTR-1];
  @ ensures \valid(\result);
  @*/
extern char *vseprint(char *s, char *e, char *fmt, va_list args);
/*@
  @ requires (n > 0 ==> \valid(s + (0 .. (integer)n-1))) || (n == 0);
  @ requires valid_string(fmt);
  @ ensures \result >= 0;
  @ behavior assigned:
  @   assumes n > 0 && s != \null;
  @   assigns s[0 .. (integer)n-1];
  @   ensures \result < n ==> valid_string(s);
  @ behavior nothing:
  @   assumes n == 0 || s == \null;
  @   assigns \nothing;
  @ complete behaviors;
  @ disjoint behaviors;
  @*/
extern int snprint(char *s, int n, char *fmt, ...);
extern void uartputs(char *s, int n);
extern int vsnprint(char *, int, char *, va_list);
extern int sprint(char *, char *, ...);
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
extern double strtod(char *, char **);
/*@
  @ requires valid_string(s);
  @ assigns \result, *endptr;
  @ ensures \valid(endptr) ==> \valid(*endptr);
  @*/
extern long strtol(char *s, char **endptr, int base);
/*@
  @ requires valid_string(s);
  @ assigns \result, *endptr;
  @*/
extern ulong strtoul(char *s, char **endptr, int base);
extern vlong strtoll(char *s, char **endptr, int base);
/*@
  @ requires valid_string(s);
  @ assigns \result, *endptr;
  @*/
extern uvlong strtoull(char *s, char **endptr, int base);
extern char etext[];
extern char edata[];
extern char end[];
extern int getfields(char *, char **, int, int, char *);
/*@
  @ requires valid_string(s);
  @ requires \valid(args + (0..maxargs-1));
  @ assigns args[0..maxargs-1], s[0 .. ACSL_MAXSTR-1];
  @ ensures \result >= 0 && \result <= maxargs;
  @*/
extern int tokenize(char *s, char **args, int maxargs);
extern int dec64(uchar *, int, char *, int);
extern int dec16(uchar *, int, char *, int);
extern int encodefmt(Fmt *);
extern void qsort(void *, usize, usize, int (*)(void *, void *));
/*
 * Syscall data structures
 */
/* Qid and Dir are now in core_types.h */
/* OWaitmsg and Waitmsg are now in core_types.h */
/*
 * Kernel Forward Type Declarations
 *
 * Centralized forward declarations for core kernel types.
 * Include this header when you need type names but not full definitions.
 */
/* Process type */
typedef struct Proc Proc;
/* Machine/CPU type */
typedef struct Mach Mach;
/* Label type (context switch state) */
typedef struct Label Label;
/* Lock type */
typedef struct Lock Lock;
/* Channel type */
typedef struct Chan Chan;
/* 9P message type */
typedef struct Fcall Fcall;
/* User registers type */
typedef struct Ureg Ureg;
/* Rendezvous type */
typedef struct Rendez Rendez;
/* 9P Qid type */
typedef struct Qid Qid;
/* 9P Dir type */
typedef struct Dir Dir;
/* Dirtab type */
typedef struct Dirtab Dirtab;
/* Devgen type */
typedef struct Chan Chan;
typedef struct Dirtab Dirtab;
typedef struct Dir Dir;
typedef int Devgen(Chan *, char *, Dirtab *, int, int, Dir *);
/* QLock type */
typedef struct QLock QLock;
/* RWLock type */
typedef struct RWLock RWLock;
/* Ref record */
typedef struct Ref Ref;
/* Fruity IR forward decls */
typedef struct fruity_module fruity_module_t;
typedef struct fruity_function fruity_function_t;
typedef struct fruity_basic_block fruity_basic_block_t;
typedef struct fruity_instruction fruity_instruction_t;
/* Plan 9 universal header */
typedef struct Conf Conf;
typedef struct Confmem Confmem;
typedef struct FPssestate FPssestate;
typedef struct FPavxstate FPavxstate;
typedef struct FPalloc FPalloc;
typedef struct FPsave FPsave;
typedef struct PFPU PFPU;
typedef struct ISAConf ISAConf;
typedef struct Label Label;
typedef struct MMU MMU;
typedef struct PCArch PCArch;
typedef struct Pcidev Pcidev;
typedef struct PCMmap PCMmap;
typedef struct PCMslot PCMslot;
typedef struct Page Page;
typedef struct PMMU PMMU;
typedef struct Segdesc Segdesc;
typedef vlong Tval;
typedef struct Vctl Vctl;
/*
 *  parameters for sysproc.c
 */
/*
 * Kernel Forward Type Declarations
 *
 * Centralized forward declarations for core kernel types.
 * Include this header when you need type names but not full definitions.
 */
/* Plan 9 universal header */
struct Lock {
  ulong key;
  ulong sr;
  uintptr pc;
  Proc *p;
  Mach *m;
  ushort isilock;
  long lockcycles;
};
/* For Frama-C: don't alias bprint to print to avoid declaration conflicts */
struct FPssestate {
  u16int fcw;       /* x87 control word */
  u16int fsw;       /* x87 status word */
  u8int ftw;        /* x87 tag word */
  u8int zero;       /* 0 */
  u16int fop;       /* last x87 opcode */
  u64int rip;       /* last x87 instruction pointer */
  u64int rdp;       /* last x87 data pointer */
  u32int mxcsr;     /* MMX control and status */
  u32int mxcsrmask; /* supported MMX feature bits */
  uchar st[128];    /* shared 64-bit media and x87 regs */
  uchar xmm[256];   /* 128-bit media regs */
  uchar ign[96];    /* reserved, ignored */
};
struct FPavxstate {
  FPssestate sse_state;
  uchar header[64]; /* XSAVE header */
  uchar ymm[256];   /* upper 128-bit regs (AVX) */
};
struct FPsave {
  FPavxstate avx_state;
};
struct FPalloc {
  FPsave fp_save;
  FPalloc *link; /* when context nests */
};
enum {
  FPinit,
  FPactive,
  FPprotected,
  FPinactive,       /* fpsave valid when fpstate >= FPincative */
  FPnotify = 0x100, /* fp in note handler */
};
struct PFPU {
  int fpstate;
  int kfpstate;
  FPalloc *fpsave;
  FPalloc *kfpsave;
};
struct Confmem {
  uintptr base;
  ulong npage;
  uintptr kbase;
  uintptr klimit;
};
struct Conf {
  ulong nmach;     /* processors */
  ulong nproc;     /* processes */
  ulong monitor;   /* has monitor? */
  ulong npage;     /* total physical pages of memory */
  ulong upages;    /* user page pool */
  ulong nimage;    /* number of page cache image headers */
  ulong nswap;     /* number of swap pages */
  int nswppo;      /* max # of pageouts per segment pass */
  ulong copymode;  /* 0 is copy on write, 1 is copy on reference */
  ulong ialloc;    /* max interrupt time allocation in bytes */
  ulong pipeqsize; /* size in bytes of pipe queues */
  int nuart;       /* number of uart devices */
  Confmem mem[64]; /* physical memory */
};
struct Segdesc {
  u32int d0;
  u32int d1;
};
/*
 *  MMU structure for PDP, PD, PT pages.
 */
struct MMU {
  MMU *next;
  uintptr *page;
  void *alloc; /* original allocation base for page tables */
  int index;
  int level;
};
/*
 *  MMU stuff in proc
 */
struct PMMU {
  MMU *mmuhead;
  MMU *mmutail;
  MMU *kmaphead;
  MMU *kmaptail;
  ulong kmapcount;
  ulong kmapindex;
  ulong mmucount;
  u64int dr[8];
  void *vmx;
};
/*
 * Kernel Forward Type Declarations
 *
 * Centralized forward declarations for core kernel types.
 * Include this header when you need type names but not full definitions.
 */
/* Plan 9 universal header */
/* UUID Manipulation Functions */
void uuid_clear(uuid_t *u);
int uuid_compare(const uuid_t *a, const uuid_t *b);
void uuid_copy(uuid_t *dst, const uuid_t *src);
int uuid_parse(const char *in, uuid_t *uu);
void uuid_unparse(const uuid_t *uu, char *out);
int uuid_is_null(const uuid_t *uu);
/* UUID Generation (RFC 9562 v8) */
void uuid_new_v8(uuid_t *u);
/* Pack custom data into UUIDv8 payload (122 bits) */
void uuid_pack_v8(uuid_t *u, unsigned long long data_a, unsigned short data_b,
                  unsigned long long data_c);
/* Pebble Token Packing (Token ID + Generation + Index) */
typedef struct {
  unsigned int token;
  unsigned int generation;
  unsigned short index;
} pebble_uuid_data_t;
void uuid_pack_pebble(uuid_t *u, unsigned int token, unsigned int generation,
                      unsigned short index);
int uuid_unpack_pebble(const uuid_t *u, unsigned int *token,
                       unsigned int *generation, unsigned short *index);
/* Capability UUID Packing (PA hash + Type + Perms + Epoch)
 * UUIDv8 Layout for capabilities:
 * Bits 0-47:   PA hash high (48 bits)
 * Bits 48-51:  Version = 8 (0b1000)
 * Bits 52-59:  Type (8 bits: CAP_TYPE_MEMORY, etc.)
 * Bits 60-63:  Permissions (4 bits: R/W/X/T)
 * Bits 64-65:  Variant = 0b10
 * Bits 66-81:  Epoch (16 bits)
 * Bits 82-127: PA hash low (46 bits)
 * Total PA hash: 94 bits from 32-byte BLAKE2b
 */
void uuid_pack_capability(uuid_t *u, const unsigned char *pa_hash,
                          unsigned short epoch, unsigned char type,
                          unsigned char perms);
int uuid_unpack_capability(const uuid_t *u, unsigned short *epoch,
                           unsigned char *type, unsigned char *perms);
/* Extract PA hash bits from capability UUID (94 bits total) */
void uuid_get_pa_hash_bits(const uuid_t *u, unsigned char *pa_hash_out);
/* Lux9 Secure PID2 Packing */
void uuid_pack_pid_lux9(uuid_t *u, const uuid_t *parent_uuid,
                        const u8int *namespace_cid, const u8int *code_hash);
/* Helper to verify PID2 components */
int uuid_verify_pid_lux9(const uuid_t *pid2, const uuid_t *parent_uuid,
                         const u8int *namespace_cid, const u8int *code_hash);
typedef struct Alarms Alarms;
typedef struct Block Block;
typedef struct Bpool Bpool;
typedef struct Cmdbuf Cmdbuf;
typedef struct Cmdtab Cmdtab;
typedef struct Confmem Confmem;
typedef struct Dev Dev;
typedef struct Dirtab Dirtab;
typedef struct Edf Edf;
typedef struct Egrp Egrp;
typedef struct Evalue Evalue;
typedef struct Fgrp Fgrp;
typedef struct DevConf DevConf;
typedef struct Image Image;
typedef struct Log Log;
typedef struct Logflag Logflag;
typedef struct Mntcache Mntcache;
typedef struct Mount Mount;
typedef struct Mntrah Mntrah;
typedef struct Mntrpc Mntrpc;
typedef struct Mntproc Mntproc;
typedef struct Mnt Mnt;
typedef struct Mhead Mhead;
typedef struct Note Note;
typedef struct Page Page;
typedef struct Path Path;
typedef struct Palloc Palloc;
typedef struct Perf Perf;
typedef struct PhysUart PhysUart;
typedef struct Pgrp Pgrp;
typedef struct Physseg Physseg;
typedef struct Pte Pte;
typedef struct PMach PMach;
typedef struct QLock QLock;
typedef struct Queue Queue;
typedef struct Ref Ref;
typedef struct Rendezq Rendezq;
typedef struct Rgrp Rgrp;
typedef struct RWLock RWLock;
typedef struct Sargs Sargs;
typedef struct Schedq Schedq;
typedef struct Segment Segment;
typedef struct Segio Segio;
typedef struct Sema Sema;
typedef struct Timer Timer;
typedef struct Timers Timers;
typedef struct Uart Uart;
typedef struct Waitq Waitq;
typedef struct Walkqid Walkqid;
typedef struct Watchpt Watchpt;
typedef struct Watchdog Watchdog;
typedef struct wasm_cap_table wasm_cap_table_t;
typedef int Devgen(Chan *, char *, Dirtab *, int, int, Dir *);
typedef struct BString {
  char *data;
  int len;
} BString;
/* Qid, Dir, and Waitmsg are defined in portlib.h */
/*
 * Kernel lock DAG metadata and tracing helpers.
 *
 * Each lock can optionally be associated with a LockDagNode. When code
 * acquires/releases that lock, the DAG helpers record the sequence so the
 * kernel can reason about ordering, emit diagnostics, and detect suspicious
 * edges.
 */

struct Proc;
enum {
  LOCKDAG_MAX_NODES = 128,
  LOCKDAG_STACK_DEPTH = 32,
};
typedef struct LockDagNode LockDagNode;
struct LockDagNode {
  const char *name;
  int id; /* Assigned by lockdag_register_node(), -1 if unregistered */
};
struct LockDagEntry {
  LockDagNode *node;
  uintptr key;
};
struct LockDagContext {
  struct LockDagEntry stack[LOCKDAG_STACK_DEPTH];
  int depth;
  int overflow;
};
void lockdag_init(void);
int lockdag_register_node(LockDagNode *node);
int lockdag_allow_edge(LockDagNode *from, LockDagNode *to);
void lockdag_record_acquire(Proc *p, LockDagNode *node, uintptr key);
void lockdag_record_release(Proc *p, LockDagNode *node, uintptr key);

/*
 * Kernel Forward Type Declarations
 *
 * Centralized forward declarations for core kernel types.
 * Include this header when you need type names but not full definitions.
 */
/* Plan 9 universal header */
/*
 * Pebble Primitives - Core Kernel Feature
 *
 * Three capability-style resource types:
 * - Black-only: kernel-managed, non-clonable resources
 * - Black-White: user white token validated to black handle
 * - Red-Blue: copy-on-write shadow (red = safe, blue = speculative)
 */
/* Compile-time configuration */
/* Token economics: 1 token = 8 bytes of memory authorization */
/*
 * Pebble Token Color States (5 mutually exclusive)
 * Used for page-level tracking in fork/COW mechanism.
 * Maps to borrowchecker states: BLACK=EXCLUSIVE, RED=SHARED_OWNED
 */
enum PebbleColor {
  PEBBLE_COLOR_COLORLESS = 0, /* Free pool, not in use */
  PEBBLE_COLOR_WHITE = 1,     /* Unverified/uninitialized (future) */
  PEBBLE_COLOR_BLACK = 2,     /* Exclusive access (one writer) */
  PEBBLE_COLOR_RED = 3,       /* Shared/read-only (multiple readers) */
  PEBBLE_COLOR_BLUE = 4,      /* I/O buffer (future) */
};
/* Syscall costs (in tokens) - Security enforcement */
/*
 * Pebble runtime toggles.
 */
extern int pebble_enabled;
extern int pebble_debug;
/*
 * Global colorless bank - single pool for entire system.
 * Total tokens = system RAM / PEBBLE_BYTES_PER_TOKEN.
 * Processes pull tokens from this pool via PoW.
 * PoW difficulty increases as pool shrinks (scarcity mechanism).
 */
extern Lock pebble_bank_lock;
extern ulong pebble_global_colorless_bank; /* tokens available globally */
extern ulong pebble_total_system_tokens;   /* RAM/8, constant after init */
/* Error handling */
/* Capability flags - Universal CBS model */
/* Helper macro for capability checking */
/* Helper macro for capability checking */
/*
 * 3-Bit Tagged Capabilities (Pointer-Carried Authority)
 * Leveraging the 8-byte alignment gap (Peg).
 */
/* The 8 Holographic Wavelengths (Channels) */
/* Holographic Projection */
/*
 * Blind Ledger - Zero-Knowledge Addressing System
 *
 * Implements the "Identity is Security" paradigm.
 *
 * - UserCapability: The opaque handle held by userspace.
 * - BlindLedgerEntry: The kernel's secret mapping table.
 *
 * The "Tension Resolution":
 * - We do NOT track every 8-byte Peg with a hash.
 * - We track "Spans" (Ranges of Pegs) created during Black Token allocation.
 * - White Tokens are fungible budget; Black Tokens have Identity.
 *
 * Circular Economy:
 * - Banked tokens (Free Pegs) are Colorless.
 * - Issued Tokens (White) are Fungible Budget.
 * - Used Tokens (Black) are Reified Identity (Ledger Entry).
 * - Freed Tokens return to the Bank as Colorless Pegs.
 */
/*
 * Red-Black Tree Implementation
 *
 * Intrusive RB-tree based on Linux kernel design.
 * Provides O(log n) insert, delete, and search operations.
 *
 * Usage:
 *   struct my_node {
 *       struct rb_node rb;
 *       int key;
 *       // ... other data
 *   };
 *
 *   struct rb_root mytree = RB_ROOT;
 *
 *   // Insert: caller provides comparison and linking
 *   // Search: caller walks tree with comparison
 *   // Delete: rb_erase(&node->rb, &mytree)
 */
/* Use kernel's uintptr if available, otherwise define it */
typedef unsigned long uintptr;
/*
 * Red-Black tree node - embed this in your structure
 */
struct rb_node {
  uintptr __rb_parent_color; /* Parent pointer + color bit */
  struct rb_node *rb_right;
  struct rb_node *rb_left;
};
/*
 * Red-Black tree root
 */
struct rb_root {
  struct rb_node *rb_node;
};
/* Color encoding in parent pointer's low bit */
/* Core operations */
void rb_insert_color(struct rb_node *node, struct rb_root *root);
void rb_erase(struct rb_node *node, struct rb_root *root);
/* Link a node into the tree (before calling rb_insert_color) */
static inline void rb_link_node(struct rb_node *node, struct rb_node *parent,
                                struct rb_node **rb_link) {
  node->__rb_parent_color = (uintptr)parent;
  node->rb_left = node->rb_right = ((void *)0);
  *rb_link = node;
}
/* Set parent and color */
static inline void rb_set_parent_color(struct rb_node *rb, struct rb_node *p,
                                       int color) {
  rb->__rb_parent_color = (uintptr)p | (uintptr)color;
}
static inline void rb_set_parent(struct rb_node *rb, struct rb_node *p) {
  rb->__rb_parent_color = (((rb)->__rb_parent_color) & 1) | (uintptr)p;
}
static inline void rb_set_black(struct rb_node *rb) {
  rb->__rb_parent_color |= 1;
}
static inline void rb_set_red(struct rb_node *rb) {
  rb->__rb_parent_color &= ~1UL;
}
/* Tree navigation */
struct rb_node *rb_first(const struct rb_root *root);
struct rb_node *rb_last(const struct rb_root *root);
struct rb_node *rb_next(const struct rb_node *node);
struct rb_node *rb_prev(const struct rb_node *node);
/* Replace a node in the tree (for updates) */
void rb_replace_node(struct rb_node *victim, struct rb_node *new_node,
                     struct rb_root *root);
/* Augmented RB-tree support */
typedef void (*rb_augment_f)(struct rb_node *node, void *data);
void rb_insert_augmented(struct rb_node *node, struct rb_root *root,
                         rb_augment_f augment_rotate, void *data);
void rb_erase_augmented(struct rb_node *node, struct rb_root *root,
                        rb_augment_f augment_rotate, void *data);
/*
 * Kernel Forward Type Declarations
 *
 * Centralized forward declarations for core kernel types.
 * Include this header when you need type names but not full definitions.
 */
/* Plan 9 universal header */
/*
 * The Public Handle (User Space View)
 *
 * Users never see physical addresses. They hold this struct.
 * Access is granted if they can present this struct, and
 * Hash(Entry.Secret | Entry.PhysAddr) == Capability.Hash.
 *
 * HYBRID DESIGN:
 * - uuid: Compact 16-byte UUIDv8 identifier for wire protocol (ring buffers)
 * - hash: Full 32-byte BLAKE2b hash for cryptographic verification
 */
typedef struct UserCapability {
  uuid_t uuid;    /* 16-byte UUIDv8 public identifier */
  u8int hash[32]; /* 32-byte BLAKE2b hash (security anchor) */
  u64int size;    /* Size of the object (Span) in bytes */
  u32int type;    /* Resource Type (Memory, Channel, PCI) */
  u32int perms;   /* Permissions (Read, Write, Execute, Transfer) */
} UserCapability;
/* Capability Resource Types */
enum {
  CAP_TYPE_MEMORY = 1,
  CAP_TYPE_CHANNEL = 2,
  CAP_TYPE_DEVICE = 3,
  CAP_TYPE_IPC = 4,
  CAP_TYPE_SPAWN = 5,
};
/* Capability Permissions */
enum {
  CAP_PERM_READ = 1 << 0,
  CAP_PERM_WRITE = 1 << 1,
  CAP_PERM_EXEC = 1 << 2,
  CAP_PERM_TRANSFER = 1 << 3, /* Can Mint/Send to others */
  CAP_PERM_GRANT = 1 << 4,    /* Can create sub-capabilities */
};
// States for a BlindLedgerEntry
typedef enum BlindLedgerState {
  BLIND_LEDGER_STATE_INACTIVE = 0, // Not yet active or invalid
  BLIND_LEDGER_STATE_ACTIVE = 1,   // Currently active and valid
  BLIND_LEDGER_STATE_BURNED = 2,   // Burned, no longer valid, awaiting cleanup
  BLIND_LEDGER_STATE_COW_RED = 3,  // Copy-on-Write: shared, read-only
  BLIND_LEDGER_STATE_COW_BLUE = 4, // Copy-on-Write: private, writable copy
} BlindLedgerState;
// Type for internal hashes within the ledger (e.g., leaf_hash, process_hash)
typedef u8int BlindLedgerHash[32];
/*
 * The Private Record (Kernel View)
 *
 * Stored in a Kernel-only Hash Map (The Ledger).
 * Lookup key is the UserCapability.hash.
 */
typedef struct BlindLedgerEntry {
  UserCapability capability; // The full capability associated with this entry
  uintptr physical_address;  /* The concrete resource (Start of Span) */
  Proc *owner;               /* The current authoritative owner process */
  u8int secret[32];          /* Cryptographic secret for capability
                                                      derivation */
  u64int epoch;              /* Allocation Cycle (Prevents Use-After-Free) */
  u64int span_len;           /* Length in bytes (must match Capability.size) */
  u32int permissions;        /* Current effective permissions */
  BlindLedgerState state;    /* Current state of the entry */
  BlindLedgerHash leaf_hash; /* Immutable hash of physical properties */
  BlindLedgerHash process_hash; /* Mutable hash of dynamic properties (owner,
                                   perms, state) */
  // Derivation Chain Support (for Red/Blue proofs)
  BlindLedgerHash parent_hash;    /* Parent capability hash (0 if root) */
  BlindLedgerHash derivation_sig; /* HMAC(key, parent || constraints || self) */
} BlindLedgerEntry;
// Error codes for Blind Ledger operations
typedef enum BlindLedgerError {
  BLIND_LEDGER_OK = 0,
  BLIND_LEDGER_EINVAL = 1,    // Invalid arguments
  BLIND_LEDGER_ENOMEM = 2,    // Out of memory
  BLIND_LEDGER_EPERM = 3,     // Permission denied (e.g., not owner)
  BLIND_LEDGER_ENOTFOUND = 4, // Capability not found
  BLIND_LEDGER_EEXPIRED = 5,  // Capability found but not active/expired
  BLIND_LEDGER_EFAULT = 6,    // General internal fault
  BLIND_LEDGER_EBUSY = 7,     // Resource is busy, cannot perform operation
} BlindLedgerError;
// Function prototypes
void blind_ledger_init(void);
BlindLedgerError ledger_mint(UserCapability *out_cap, uintptr pa, ulong len,
                             Proc *owner, u32int permissions,
                             const u8int *vault_secret);
BlindLedgerError ledger_verify(const UserCapability *cap,
                               BlindLedgerEntry *out_entry);
BlindLedgerError ledger_verify_by_uuid(const uuid_t *uuid,
                                       BlindLedgerEntry *out_entry);
BlindLedgerError ledger_transfer(const UserCapability *cap, Proc *from_owner,
                                 Proc *to_owner);
BlindLedgerError ledger_burn(const UserCapability *cap, Proc *owner);
BlindLedgerError ledger_lookup_by_pa_and_owner(uintptr pa, Proc *owner,
                                               UserCapability *out_cap,
                                               BlindLedgerEntry *out_entry);
void blind_ledger_update_merkle_root(void);
const u8int *blind_ledger_get_merkle_root(void);
// Epoch management for preventing use-after-free
u64int ledger_get_current_epoch(void);
void ledger_advance_epoch(void);
// Vault secret generation (TPM-backed)
BlindLedgerError ledger_generate_secret(u8int *secret_out);
BlindLedgerError ledger_destroy_secret(const u8int *secret);
// Atomic rollback support for exchange operations
typedef struct LedgerRollbackToken {
  UserCapability
      original_capability; // Original capability hash before transfer
  Proc *original_owner;    // Original owner before transfer
  BlindLedgerHash
      original_process_hash; // Original process_hash before transfer
  u64int is_valid;           // Magic value to verify token validity
} LedgerRollbackToken;
BlindLedgerError
ledger_transfer_reversible(const UserCapability *cap, Proc *from_owner,
                           Proc *to_owner, LedgerRollbackToken *rollback_token);
// Statistics
typedef struct BlindLedgerStats {
  u64int active_entries;
  u64int burned_entries;
  u64int total_memory_tracked;
  u64int tree_depth;
  u64int epoch;
} BlindLedgerStats;
BlindLedgerError blind_ledger_get_stats(BlindLedgerStats *stats);
// Attestation
BlindLedgerError blind_ledger_attest_root(u8int *out_signature,
                                          u32int *out_len);
// =========================================================================
// Derivation Chain Proofs (Replaces Sibling Merkle)
// =========================================================================
/*
 * DerivationStep - One link in the derivation chain.
 */
typedef struct DerivationStep {
  BlindLedgerHash parent_hash;    // Parent capability hash
  BlindLedgerHash derivation_sig; // HMAC(key, parent || constraints || child)
  u32int constraints;             // Permissions granted (≤ parent)
} DerivationStep;
/*
 * DerivationProof - Proof of capability validity via derivation chain.
 *
 * The proof traces from the target capability up to a trusted root.
 * Each step proves: "child was derived from parent with valid constraints."
 */
typedef struct DerivationProof {
  BlindLedgerHash target_hash; // Capability being proven
  u32int chain_length;         // 0 = root, N = derivation depth
  DerivationStep chain[16];    // Ancestors to root
} DerivationProof;
/*
 * ledger_derive - Create a child capability from a parent.
 *
 * The child inherits a subset of parent's permissions.
 * Returns the new capability hash in out_cap.
 */
BlindLedgerError ledger_derive(const UserCapability *parent_cap, Proc *owner,
                               u32int child_constraints,
                               UserCapability *out_child_cap);
/*
 * ledger_get_derivation_proof - Generate a derivation chain proof.
 *
 * Walks the parent chain from target to root, collecting derivation steps.
 */
BlindLedgerError ledger_get_derivation_proof(const UserCapability *cap,
                                             Proc *owner,
                                             DerivationProof *out_proof);
/*
 * ledger_verify_derivation_proof - Stateless verification of derivation chain.
 *
 * Verifies each derivation step from target to root.
 * Returns BLIND_LEDGER_OK if valid, BLIND_LEDGER_EPERM if any step fails.
 */
BlindLedgerError ledger_verify_derivation_proof(const DerivationProof *proof);
/*
 * Lookup by PA and owner (O(1) average via secondary index)
 * Returns BLIND_LEDGER_OK if found, BLIND_LEDGER_ENOTFOUND otherwise.
 */
BlindLedgerError ledger_lookup_by_pa_and_owner(uintptr pa, Proc *owner,
                                               UserCapability *out_cap,
                                               BlindLedgerEntry *out_entry);
/*
 * Unified borrow checker for kernel primitives
 * Provides Rust-style ownership and borrowing for locks, memory, I/O, etc.
 */

/* Need kernel types before lock.h */
/*
 * Kernel Forward Type Declarations
 *
 * Centralized forward declarations for core kernel types.
 * Include this header when you need type names but not full definitions.
 */
/* Plan 9 universal header */
/* Borrow states - based on Rust borrow semantics */
enum BorrowState {
  BORROW_FREE = 0,     /* Resource is unowned */
  BORROW_EXCLUSIVE,    /* Owned exclusively by one process */
  BORROW_SHARED_OWNED, /* Owner has resource, but lent as shared */
  BORROW_MUT_LENT,     /* Owner lent resource as mutable, blocked */
};
/* Authorization Key */
struct IdentKey {
  u64int gen;   /* Monotonic Generation Counter */
  u64int nonce; /* Hardware RNG Secret */
};
/* System-level owners for boot coordination */
enum BorrowSystemOwner {
  OWNER_BOOTLOADER = 0, /* Limine bootloader owns the resource */
  OWNER_KERNEL,         /* Pebble kernel owns the resource */
  OWNER_TRAMPOLINE,     /* CR3 switch trampoline code */
};
/* Track where ownership bookkeeping structs are allocated */
enum AllocSource {
  ALLOC_BOOTSTRAP,
  ALLOC_XALLOC,
};
/* Memory tracking structures for boot coordination */
struct MemoryRange {
  uintptr start;                /* Start of memory range */
  uintptr end;                  /* End of memory range (exclusive) */
  enum BorrowSystemOwner owner; /* Who owns this range */
  struct MemoryRange *next;     /* Next range in list */
};
/* Per-resource ownership tracking descriptor */
struct BorrowOwner {
  /* Resource identification */
  uintptr key;             /* Unique key for the resource (e.g., address) */
  struct IdentKey key_cap; /* Authorization Capability */
  /* Core ownership */
  Proc *owner;                         /* Original owner process */
  enum BorrowState state;              /* Current ownership state */
  enum BorrowSystemOwner system_owner; /* System owner during boot */
  int is_system_owned;                 /* 1 if owned at system level */
  /* Borrow Checker Interface */
  int shared_count;                   /* Number of shared borrows (&) */
  Proc *mut_borrower;                 /* Exclusive mutable borrower (&mut) */
  struct SharedBorrower *shared_list; /* List of shared borrowers */
  /* Lifetime tracking */
  uvlong acquired_ns;        /* When ownership was acquired */
  uvlong borrow_deadline_ns; /* Automatic return time (0 = never) */
  /* Debugging */
  ulong borrow_count; /* Total times borrowed */
  /* Memory management */
  enum AllocSource alloc_source; /* Where this struct was allocated */
  struct BorrowOwner *next;      /* Next in hash bucket chain */
};
/* Shared borrower tracking */
struct SharedBorrower {
  Proc *proc;                    /* Process that borrowed shared */
  enum AllocSource alloc_source; /* Where this struct was allocated */
  struct SharedBorrower *next;   /* Next shared borrower */
};
/* Hash bucket for borrow pool */
struct BorrowBucket {
  struct BorrowOwner *head; /* Head of owner list for this bucket */
};
/* Borrow pool - hash table of resources */
struct BorrowPool {
  Lock lock;                   /* Protects entire pool */
  struct BorrowBucket *owners; /* Hash table buckets */
  ulong nbuckets;              /* Number of hash buckets */
  ulong nowners;               /* Total number of owned resources */
  ulong nshared;               /* Resources with shared borrows */
  ulong nmut;                  /* Resources with mutable borrows */
  u8int *bloom;                /* Counting bloom filter counters */
  ulong bloom_bits;            /* Number of bloom counters */
  u32int bloom_hashes;         /* Number of hash functions */
};
/* Memory coordination states */
enum MemoryCoordinationState {
  MEMORY_BOOTLOADER = 0, /* Bootloader owns everything */
  MEMORY_COORDINATED,    /* Ownership zones established */
  MEMORY_KERNEL_ACTIVE,  /* Kernel has taken full control */
};
/* Memory coordination structure */
struct MemoryCoordination {
  enum MemoryCoordinationState state;   /* Current coordination state */
  enum BorrowSystemOwner current_owner; /* Current memory owner */
  int coordination_enabled;             /* Enable/disable coordination */
};
/* Error codes for borrow operations */
enum BorrowError {
  BORROW_OK = 0,
  BORROW_EALREADY,      /* Already owned */
  BORROW_ENOTOWNER,     /* Not the owner */
  BORROW_EBORROWED,     /* Can't modify - has borrows */
  BORROW_EMUTBORROW,    /* Can't borrow - already &mut */
  BORROW_ESHAREDBORROW, /* Can't borrow &mut - has & */
  BORROW_ENOTBORROWED,  /* Not borrowed, can't return */
  BORROW_EINVAL,        /* Invalid parameters */
  BORROW_ENOMEM,        /* Out of memory */
  BORROW_ENOTFOUND,     /* Resource not found */
};
/* Global borrow pool */
extern struct BorrowPool borrowpool;
/* Core ownership operations */
void borrowinit(void);
enum BorrowError borrow_acquire(Proc *p, uintptr key);
enum BorrowError borrow_release(Proc *p, uintptr key);
enum BorrowError borrow_transfer(Proc *from, Proc *to, uintptr key);
/* Borrow operations (Rust-style) */
enum BorrowError borrow_borrow_shared(Proc *owner, Proc *borrower, uintptr key);
enum BorrowError borrow_borrow_mut(Proc *owner, Proc *borrower, uintptr key);
enum BorrowError borrow_return_shared(Proc *borrower, uintptr key);
enum BorrowError borrow_return_mut(Proc *borrower, uintptr key);
/* System-level ownership */
enum BorrowError borrow_acquire_system(uintptr key,
                                       enum BorrowSystemOwner owner);
enum BorrowError borrow_release_system(uintptr key,
                                       enum BorrowSystemOwner owner);
enum BorrowError borrow_transfer_system(enum BorrowSystemOwner from,
                                        enum BorrowSystemOwner to, uintptr key);
enum BorrowSystemOwner borrow_get_system_owner(uintptr key);
int borrow_is_owned_by_system(uintptr key, enum BorrowSystemOwner owner);
/* Range-based resource acquisition */
enum BorrowError borrow_acquire_range_phys(uintptr start_pa, usize size,
                                           enum BorrowSystemOwner owner);
int borrow_range_owned_by_system(uintptr start_pa, usize size,
                                 enum BorrowSystemOwner owner);
int borrow_can_access_range_phys(uintptr start_pa, usize size,
                                 enum BorrowSystemOwner requester);
/* Query operations */
int borrow_is_owned(uintptr key);
Proc *borrow_get_owner(uintptr key);
int borrow_get_owner_snapshot(uintptr key, struct BorrowOwner *out);
enum BorrowState borrow_get_state(uintptr key);
int borrow_can_borrow_shared(uintptr key);
int borrow_can_borrow_mut(uintptr key);
/* Process cleanup - called when process dies */
void borrow_cleanup_process(Proc *p);
/* Memory range tracking */
void memory_range_init(void);
void memory_range_add(uintptr start, uintptr end, enum BorrowSystemOwner owner);
void memory_range_add_discovered(uintptr start, uintptr end,
                                 enum BorrowSystemOwner owner);
void memory_range_remove(uintptr start, uintptr end);
void memory_range_dump(void);
int memory_range_capacity(void);
enum BorrowSystemOwner memory_range_get_owner(uintptr addr);
int memory_range_check_access(uintptr addr, enum BorrowSystemOwner requester);
/* Memory coordination */
void boot_memory_coordination_init(void);
void transfer_bootloader_to_kernel(void);
void establish_memory_ownership_zones(void);
void establish_memory_ownership_zones_dynamic(void);
int validate_memory_coordination_ready(void);
int memory_system_ready_before_cr3(void);
int post_cr3_memory_system_operational(void);
/* Statistics and debugging */
void borrow_stats(void);
void borrow_dump_resource(uintptr key);
/* Hash function for keys */
ulong borrow_hash(uintptr key);
/* White token structure - opaque to user */
typedef struct PebbleWhite {
  u32int token;
  u32int generation;
  void *data_ptr;
  ulong size;
} PebbleWhite;
/* Blue object structure - independent colored token for block I/O */
typedef struct PebbleBlue {
  void *blue_data;         /* Physical memory (separate allocation) */
  ulong blue_size;         /* Size of allocation */
  ulong flags;             /* State flags */
  struct PebbleBlue *next; /* List linkage */
} PebbleBlue;
/* Red copy structure - independent colored token for snapshots */
typedef struct PebbleRed {
  void *red_data;         /* Physical memory (separate allocation) */
  ulong red_size;         /* Size of allocation */
  ulong flags;            /* State flags */
  struct PebbleRed *next; /* List linkage */
} PebbleRed;
typedef struct PebbleBlack {
  UserCapability capability; // The UserCapability provided by Blind Ledger
  void *
      physical_addr; // The actual physical memory address managed by this token
  uintptr user_vaddr; // The user-space virtual address mapping (if any)
  ulong size;         // Size of the allocation
  ulong flags;
  struct PebbleBlack *next;
} PebbleBlack;
/* Process Vault structure moved to portdat.h to avoid circular dependency with
 * QLock */
/* Per-process Pebble state */
typedef struct PebbleState {
  ulong colorless_bank; /* remaining bytes for this process (COLORLESS pool) */
  ulong black_inuse;    /* bytes in BLACK state */
  ulong blue_inuse;     /* bytes in BLUE state */
  ulong red_inuse;      /* bytes in RED state */
  ulong white_verified; /* count of active white→black conversions */
  ulong white_pending;  /* bytes authorized by white tokens (WHITE state) */
  ulong red_count;      /* number of live red tokens */
  ulong blue_count;     /* number of live blue tokens */
  ulong total_allocs;   /* total allocations made */
  ulong total_frees;    /* total frees performed */
  uintptr vbase; /* next available user virtual address for Pebble mapping */
  /* Lists for tracking objects */
  PebbleBlack *black_list;
  PebbleBlue *blue_list;
  PebbleRed *red_list;
  void *vault_handle; /* Process-specific Holographic Vault (Wave 6) */
  /* State tracking */
  int in_syscall;    /* set when in Pebble syscalls */
  ulong drop_budget; /* budget that will drop on exit */
  /* White token bookkeeping */
  PebbleWhite whites[4096];
  uchar whites_active[4096];
  ulong white_generation;
  int white_head;
} PebbleState;
/*@
  @ predicate pebble_state_valid(PebbleState *ps) =
  @   \valid(ps) &&
  @   ps->colorless_bank + ps->black_inuse + ps->blue_inuse + ps->red_inuse <=
  pebble_total_system_tokens &&
  @   ps->white_pending <= ps->colorless_bank;
  @*/
/*
 * Arena Branch Bank - Per-Container Resource Management
 *
 * Each WASM container (or other arena) gets its own branch bank
 * for lock-free local allocations. Branches periodically reconcile
 * with the process colorless bank.
 *
 * Token flow: Process colorless_bank → branch local_colorless → BLACK
 * All transitions remain 1:1 (token conservation).
 *
 * See docs/WASM_ARENA_BRANCH_BANKS.md for design details.
 */
typedef struct arena_branch {
  Lock lock;                /* Per-branch lock (no global contention) */
  ulong local_colorless;    /* Tokens available locally in this branch */
  ulong borrowed_from_proc; /* Tokens borrowed from process bank */
  ulong max_tokens;         /* Hard cap on branch tokens */
  ulong low_water;          /* Request refill when below this threshold */
  ulong high_water;         /* Return excess when above this threshold */
  ulong total_allocated;    /* Statistics: total bytes allocated from branch */
  ulong total_freed;        /* Statistics: total bytes freed to branch */
  PebbleState *owner_ps;    /* Back-pointer to owning process PebbleState */
} arena_branch_t;
/* Arena Branch API */
void arena_branch_init(arena_branch_t *branch, PebbleState *ps,
                       ulong initial_budget);
int arena_branch_alloc(arena_branch_t *branch,
                       ulong size); /* Branch tokens → allocation */
void arena_branch_free(arena_branch_t *branch,
                       ulong size); /* Allocation → branch tokens */
int arena_branch_refill(arena_branch_t *branch); /* Process bank → branch */
void arena_branch_drain(arena_branch_t *branch); /* Branch → process bank */
/* Global Pebble lock - one system-wide lock for now */
extern Lock pebble_global_lock;
/* Core API functions */
int pebble_black_alloc(PebbleWhite *white, void *buf, ulong size,
                       UserCapability *out_cap);
void *pebble_get_black_addr(const UserCapability *cap);
int pebble_black_free(const UserCapability *cap);
int pebble_white_verify(PebbleWhite *white_cap, void **black_cap);
int pebble_create_token_uuid(PebbleWhite *white, uuid_t *out_uuid);
int pebble_alloc_with_white(ulong size, UserCapability *out_cap,
                            void **out_addr);
/* Blue/Red API - Independent colored tokens for block I/O transactions */
PebbleBlue *pebble_blue_alloc(ulong size); /* COLORLESS → BLUE */
int pebble_blue_free(PebbleBlue *blue);    /* BLUE → COLORLESS */
PebbleRed *pebble_red_alloc(ulong size);   /* COLORLESS → RED */
int pebble_red_free(PebbleRed *red);       /* RED → COLORLESS */
int pebble_red_snapshot(PebbleBlue *blue,
                        PebbleRed **out_red); /* Copy Blue → Red */
/* Legacy API - DEPRECATED, will be removed */
int pebble_red_copy(PebbleBlue *blue_obj,
                    PebbleRed **red_copy);     /* Use pebble_red_snapshot */
int pebble_blue_discard(PebbleBlue *blue_obj); /* Use pebble_blue_free */
/* Internal helper functions */
PebbleState *pebble_state(void);
int pebble_set_budget(ulong budget);
ulong pebble_get_budget(void);
int pebble_increase_budget(ulong size, u64int nonce);
void pebble_auto_verify(Proc *p, Ureg *ureg);
void pebble_red_blue_exit(void);
/*@ requires ps != \null;
  @ requires \valid(ps);
  @ requires white != \null && \valid(white);
  @ requires white->data_ptr == \null || \valid((uchar*)white->data_ptr +
  (0..(integer)white->size-1));
  @ requires white->size > 0;
  @ terminates \true;
  @ assigns ps->whites[0..4095],
  @         ps->whites_active[0..4095],
  @         ps->white_generation, ps->white_head, ps->white_pending;
  @ ensures \result != 0 ==> \result == 1;
  @ behavior success:
  @   assumes ps->white_pending + white->size <= ps->colorless_bank;
  @   ensures \result == 1;
  @ behavior failure:
  @   assumes ps->white_pending + white->size > ps->colorless_bank;
  @   ensures \result == 0;
  @ complete behaviors;
  @ disjoint behaviors;
  */
int pebble_valid_white_token(PebbleState *ps, PebbleWhite *white);
/*@ requires ps != \null && \valid(ps);
  @ requires data == \null || \valid((uchar*)data + (0..(integer)size-1));
  @ requires size > 0 && size <= ps->colorless_bank;
  @ terminates \true;
  @ assigns ps->whites[0..4095],
  ps->whites_active[0..4095],
  @         ps->white_generation, ps->white_head, ps->white_pending;
  @ ensures \result != \null ==> \valid(\result);
  @ ensures \result != \null ==> \result->data_ptr == data;
  @ ensures \result != \null ==> \result->size == size;
  @ ensures \result == \null || (\result->token < 4096 &&
  ps->whites_active[\result->token] != 0);
  */
PebbleWhite *pebble_issue_white(PebbleState *ps, void *data, ulong size);
/*@ requires ps != \null && \valid(ps);
  @ requires white != \null ==> \valid(white);
  @ requires white != \null ==> white->token < 4096;
  @ terminates \true;
  @ assigns ps->whites_active[0..4095], ps->white_pending;
  */
void pebble_return_white(PebbleState *ps, PebbleWhite *white);
/*@ requires ps != \null && \valid(ps);
  @ requires handle != \null;
  @ terminates \true;
  @ assigns \nothing;
  @ ensures \result == \null || \valid(\result);
  */
PebbleBlack *pebble_lookup_black(PebbleState *ps, void *handle);
/*@ requires ps != \null && \valid(ps);
  @ terminates \true;
  @ assigns \nothing;
  @ ensures \result == \null || \valid(\result);
  */
PebbleBlack *pebble_lookup_black_by_addr(PebbleState *ps, void *addr);
int pebble_blue_exists(PebbleState *ps, PebbleBlue *blue);
int pebble_has_matching_red(PebbleState *ps, PebbleBlue *blue);
PebbleRed *pebble_duplicate_blue(PebbleState *ps, PebbleBlue *blue);
void pebble_mark_red(PebbleState *ps, PebbleBlue *blue, PebbleRed *red);
void pebble_ensure_red_snapshots(PebbleState *ps);
void pebble_cleanup(struct Proc *p);
void pebble_selftest(void);
void pebble_sip_issue_test(void);
/* Debug support */
/* Initialization */
void pebbleinit(void);
void pebbleprocinit(Proc *p);
void *pebble_meta_alloc(ulong size);
void pebble_meta_free(void *v);
/* Constants for validation */
/*
 * B.E.V.I.S. (Byzantine Energy Verification & Isolation Subsystem)
 * B.U.T.T.H.E.A.D. (Bandwidth-Utilizing Thermodynamic Token Hardened Economic
 * Allocation Dispatcher)
 */
int pow_calculate_difficulty(int op_class, ulong magnitude);
int pow_verify(u64int nonce, u64int context, int required_diff);
void pow_gate_init(void);
/* Plan 9 universal header */
typedef struct Fcall {
  uchar type;
  u32int fid;
  ushort tag;
  union {
    struct {
      u32int msize;  /* Tversion, Rversion */
      char *version; /* Tversion, Rversion */
    };
    struct {
      ushort oldtag; /* Tflush */
    };
    struct {
      char *ename; /* Rerror */
    };
    struct {
      Qid qid;       /* Rattach, Ropen, Rcreate */
      u32int iounit; /* Ropen, Rcreate */
    };
    struct {
      Qid aqid; /* Rauth */
    };
    struct {
      u32int afid; /* Tauth, Tattach */
      char *uname; /* Tauth, Tattach */
      char *aname; /* Tauth, Tattach */
    };
    struct {
      u32int perm; /* Tcreate */
      char *name;  /* Tcreate */
      uchar mode;  /* Tcreate, Topen */
    };
    struct {
      u32int newfid;   /* Twalk */
      ushort nwname;   /* Twalk */
      char *wname[16]; /* Twalk */
    };
    struct {
      ushort nwqid; /* Rwalk */
      Qid wqid[16]; /* Rwalk */
    };
    struct {
      vlong offset; /* Tread, Twrite */
      u32int count; /* Tread, Twrite, Rread */
      char *data;   /* Twrite, Rread */
    };
    struct {
      ushort nstat; /* Twstat, Rstat */
      uchar *stat;  /* Twstat, Rstat */
    };
    struct {
      u32int scallnr; /* Tsyscall */
      u32int sflags;  /* Tsyscall flags/category */
      uchar *sdata;   /* Tsyscall, Rsyscall */
      u32int scount;  /* Tsyscall, Rsyscall */
      u64int retval;  /* Rsyscall */
    };
    /* Tsys* message fields */
    struct {
      u32int flags; /* Tsysfork (rfork flags), Tsysbind, Tsysmount */
      u32int pid;   /* Rsysfork, Rsyswait */
    };
    struct {
      char *path;     /* Tsysexec - executable path */
      char **argv;    /* Tsysexec - argument array */
      u32int argc;    /* Tsysexec - argument count */
      char *args[16]; /* Tsysexec - workspace for deserialized arguments */
    };
    struct {
      u64int addr; /* Tsysbrk, Rsysbrk - memory address */
    };
    struct {
      char *oldpath; /* Tsysbind, Tsysmount, Tsysunmount - old path */
      u32int fd;     /* Tsysmount - file descriptor */
    };
    struct {
      u32int fid0; /* Rsyspipe - first pipe fid */
      u32int fid1; /* Rsyspipe - second pipe fid */
    };
    struct {
      int whence; /* Tsysseek - seek type (SEEK_SET, etc.) */
    };
    struct {
      u64int handler; /* Tsysnotify - notification handler address */
    };
  };
} Fcall;
/* STATFIXLEN includes leading 16-bit count */
/* The count, however, excludes itself; total size is BIT16SZ+count */
enum {
  Tversion = 100,
  Rversion,
  Tauth = 102,
  Rauth,
  Tattach = 104,
  Rattach,
  Terror = 106, /* illegal */
  Rerror,
  Tflush = 108,
  Rflush,
  Twalk = 110,
  Rwalk,
  Topen = 112,
  Ropen,
  Tcreate = 114,
  Rcreate,
  Tread = 116,
  Rread,
  Twrite = 118,
  Rwrite,
  Tclunk = 120,
  Rclunk,
  Tremove = 122,
  Rremove,
  Tstat = 124,
  Rstat,
  Twstat = 126,
  Rwstat,
  Tmax,
  /* Lux9 custom message types */
  Texec = 128,
  Rexec,
  /* Lux9 syscall message types - for pure 9P message passing */
  /* Generic Syscall Message: Tsyscall (130) - kept for backwards compatibility
   */
  Tsyscall = 130,
  Rsyscall,
  /* Specific syscall wrappers - provide syscall-like semantics over 9P */
  /* I/O Operations */
  Tsysopen = 132, /* open(path, mode) -> fid */
  Rsysopen,
  Tsyscreate = 134, /* create(path, perm, mode) -> fid */
  Rsyscreate,
  Tsysread = 136, /* read(fid, offset, count) -> data */
  Rsysread,
  Tsyswrite = 138, /* write(fid, offset, data) -> count */
  Rsyswrite,
  Tsysclose = 140, /* close(fid) */
  Rsysclose,
  Tsyspread = 142, /* pread(fid, offset, count) -> data */
  Rsyspread,
  Tsyspwrite = 144, /* pwrite(fid, offset, data) -> count */
  Rsyspwrite,
  Tsysremove = 146, /* remove(path) */
  Rsysremove,
  /* File Info Operations */
  Tsysstat = 148, /* stat(path) -> Dir */
  Rsysstat,
  Tsysfstat = 150, /* fstat(fid) -> Dir */
  Rsysfstat,
  Tsyswstat = 152, /* wstat(path, Dir) */
  Rsyswstat,
  Tsysfwstat = 154, /* fwstat(fid, Dir) */
  Rsysfwstat,
  /* Process Control */
  Tsysfork = 160, /* rfork(flags) -> pid */
  Rsysfork,
  Tsysexec = 162, /* exec(path, argv) */
  Rsysexec,
  Tsysexit = 164, /* exits(status) */
  Rsysexit,
  Tsyswait = 166, /* wait() -> Waitmsg */
  Rsyswait,
  Tsysbrk = 168, /* brk(addr) -> addr */
  Rsysbrk,
  Tsyssleep = 170, /* sleep(millisecs) */
  Rsyssleep,
  /* Namespace Operations */
  Tsysbind = 180, /* bind(name, old, flags) */
  Rsysbind,
  Tsysmount = 182, /* mount(fd, afd, old, flags, aname) */
  Rsysmount,
  Tsysunmount = 184, /* unmount(name, old) */
  Rsysunmount,
  Tsyschdir = 186, /* chdir(path) */
  Rsyschdir,
  /* FD Operations */
  Tsysdup = 190, /* dup(oldfd, newfd) -> fid */
  Rsysdup,
  Tsyspipe = 192, /* pipe(fd[2]) -> fid[2] */
  Rsyspipe,
  Tsysfd2path = 194, /* fd2path(fid) -> path */
  Rsysfd2path,
  /* Misc Operations */
  Tsysseek = 200, /* seek(fid, offset, type) -> offset */
  Rsysseek,
  Tsysnotify = 202, /* notify(handler) */
  Rsysnotify,
  Tsysalarm = 204, /* alarm(millisecs) -> previous */
  Rsysalarm,
  Tsysmax,
};
uint convM2S(uchar *, uint, Fcall *);
uint convS2M(Fcall *, uchar *, uint);
uint sizeS2M(Fcall *);
int statcheck(uchar *abuf, uint nbuf);
uint convM2D(uchar *, uint, Dir *, char *);
uint convD2M(Dir *, uchar *, uint);
uint sizeD2M(Dir *);
int fcallfmt(Fmt *);
int dirfmt(Fmt *);
int dirmodefmt(Fmt *);
int read9pmsg(int, void *, uint);
/* Syscall Numbers */
enum {
  SYS_OPEN = 1,
  SYS_CLOSE,
  SYS_READ,
  SYS_WRITE,
  SYS_PREAD,
  SYS_PWRITE,
  SYS_CREATE,
  SYS_REMOVE = 25,
  SYS_EXIT,
  SYS_FORK,
  SYS_STAT,
  SYS_WSTAT,
  SYS_RFORK = 19,
  SYS_PIPE = 21,
  SYS_SEEK = 39,
  SYS_MOUNT = 46,
  SYS_NSEC = 53,
  SYS_BRK = 55,
  SYS_PEBBLE_ALLOC = 59,
  SYS_PEBBLE_FREE = 60,
  SYS_PEBBLE_INCREASE_BUDGET = 61,
  SYS_WASM_COMPILE = 160,
  SYS_WASM_EXECUTE = 161,
  SYS_WASM_DESTROY = 162,
  SYS_GETPID2 = 66,
  SYS_EXCHANGE_ALLOC = 67,
  SYS_EXCHANGE_FREE = 68,
  SYS_EXCHANGE_PUBLISH = 69,
  SYS_EXCHANGE_SUBSCRIBE = 70,
  SYS_EXCHANGE_UNSUBSCRIBE = 71,
  SYS_EXCHANGE_RECEIVE = 72,
  SYS_WAIT = 166,
  /* Process Vault syscalls (210-219) */
  SYS_VAULT_CREATE = 210,
  SYS_VAULT_LOCK = 211,
  SYS_VAULT_UNLOCK = 212,
  SYS_VAULT_READ = 213,
  SYS_VAULT_WRITE = 214,
  SYS_VAULT_WIPE = 215,
  SYS_VAULT_EXPORT = 216,
  SYS_VAULT_IMPORT = 217,
  SYS_VAULT_STATUS = 218
};
struct Ref {
  long ref;
};
struct Rendez {
  Lock lock;
  Proc *p;
};
struct QLock {
  Lock use;   /* to access Qlock structure */
  Proc *head; /* next process waiting for object */
  Proc *tail; /* last process waiting for object */
  uintptr pc; /* pc of owner */
  int locked; /* flag */
};
/* Process Vault Structure */
typedef struct ProcessVault ProcessVault;
struct ProcessVault {
  ProcessVault *next;
  int id;                  /* Unique ID */
  int pid;                 /* Owner PID */
  QLock lock;              /* Protect concurrent access */
  uchar *data;             /* Vault data (Pebble Black allocated) */
  ulong size;              /* Vault size in bytes */
  int locked;              /* 1 = locked (encrypted), 0 = unlocked */
  int refcount;            /* Number of open channels */
  uchar ephemeral_key[32]; /* Only present when unlocked. Wiped on lock. */
  int has_key;             /* 1 = ephemeral_key is valid */
  /* Holographic Header (Stateless):
   * When locked:
   *   header[0..15]  = Salt
   *   header[16..47] = Elligator Rep (R)
   *   header[48..71] = Nonce
   *   header[72..87] = MAC (XChaCha20-Poly1305)
   */
  UserCapability capability; /* Pebble Black capability */
  int initialized;           /* 1 = data allocated */
  int dead;                  /* 1 = unlinked/zombie, waiting for refcount=0 */
};
/*@
  @ type invariant refcount_inv(struct ProcessVault rd) =
  @   rd.refcount >= 0;
  @
  @ type invariant nonce_storage_inv(struct ProcessVault rd) =
  @   (rd.locked == 1 && rd.size >= 24 && \valid(rd.data)) ==>
  @     \valid(rd.data + (0..23));
  @*/
struct Rendezq {
  QLock qlock;
  Rendez rendez;
};
struct RWLock {
  Lock use;
  Proc *head; /* list of waiting processes */
  Proc *tail;
  uintptr wpc; /* pc of writer */
  int writer;  /* number of writers */
  int readers; /* number of readers */
};
struct Alarms {
  QLock lock;
  Proc *head;
};
struct Sargs {
  uchar args[5 * sizeof(ulong)];
};
/*
 * Access types in namec & channel flags
 */
enum {
  Aaccess,          /* as in stat, wstat */
  Abind,            /* for left-hand-side of bind */
  Atodir,           /* as in chdir */
  Aopen,            /* for i/o */
  Amount,           /* to be mounted or mounted upon */
  Acreate,          /* is to be created */
  Aremove,          /* will be removed by caller */
  Aunmount,         /* unmount arg[0] */
  COPEN = 0x0001,   /* for i/o */
  CMSG = 0x0002,    /* the message channel for a mount */
                    /* rsc CCREATE = 0x0004, permits creation if c->mnt */
  CCEXEC = 0x0008,  /* close on exec (per file descriptor) */
  CFREE = 0x0010,   /* not in use */
  CRCLOSE = 0x0020, /* remove on close */
  CCACHE = 0x0080,  /* client cache */
};
/* flag values */
enum {
  BINTR = (1 << 0),
  BFREE = (1 << 1),
  Bipck = (1 << 2),  /* ip checksum */
  Budpck = (1 << 3), /* udp checksum */
  Btcpck = (1 << 4), /* tcp checksum */
  Bpktck = (1 << 5), /* packet checksum */
};
struct Block {
  Block *next;
  Block *list;
  uchar *rp;   /* first unconsumed byte */
  uchar *wp;   /* first empty byte */
  uchar *lim;  /* 1 past the end of the buffer */
  uchar *base; /* start of the buffer */
  Bpool *pool;
  ushort flag;
  ushort checksum; /* IP checksum of complete packet (minus media header) */
};
struct Bpool {
  ulong size;  /* block size */
  ulong align; /* block alignment */
  Lock lock;
  Block *head; /* freelist head */
};
struct Chan {
  long ref;
  Lock lock;
  Chan *next; /* allocation */
  Chan *link;
  vlong offset;    /* in fd */
  vlong devoffset; /* in underlying device; see read */
  ushort type;
  ulong dev;
  ushort mode; /* read/write */
  ushort flag;
  Qid qid;
  int fid;        /* for devmnt */
  ulong iounit;   /* chunk size for i/o; 0==default */
  Mhead *umh;     /* mount point that derived Chan; used in unionread */
  Chan *umc;      /* channel in union; held for union read */
  QLock umqlock;  /* serialize unionreads */
  int uri;        /* union read index */
  int dri;        /* devdirread index */
  uchar *dirrock; /* directory entry rock for translations */
  int nrock;
  int mrock;
  QLock rockqlock;
  int ismtpt;
  Mntcache *mcp; /* Mount cache pointer */
  Mnt *mux;      /* Mnt for clients using me for messages */
  union {
    void *aux;
    ulong mid; /* for ns in devproc */
  };
  Chan *mchan; /* channel to mounted server */
  Qid mqid;    /* qid of root of mount point */
  Path *path;
  char *srvname; /* /srv/name when posted */
};
struct Path {
  long ref;
  char *s;
  Chan **mtpt; /* mtpt history */
  int len;     /* strlen(s) */
  int alen;    /* allocated length of s */
  int mlen;    /* number of path elements */
  int malen;   /* allocated length of mtpt */
};
struct Dev {
  int dc;
  char *name;
  void (*reset)(void);
  void (*init)(void);
  void (*shutdown)(void);
  Chan *(*attach)(char *);
  Walkqid *(*walk)(Chan *, Chan *, char **, int);
  int (*stat)(Chan *, uchar *, int);
  Chan *(*open)(Chan *, int);
  Chan *(*create)(Chan *, char *, int, ulong);
  void (*close)(Chan *);
  long (*read)(Chan *, void *, long, vlong);
  Block *(*bread)(Chan *, long, ulong);
  long (*write)(Chan *, void *, long, vlong);
  long (*bwrite)(Chan *, Block *, ulong);
  void (*remove)(Chan *);
  int (*wstat)(Chan *, uchar *, int);
  void (*power)(int); /* power mgt: power(1) => on, power (0) => off */
  int (*config)(int, char *, DevConf *); /* returns nil on error */
};
struct Dirtab {
  char name[28];
  Qid qid;
  vlong length;
  long perm;
};
struct Walkqid {
  Chan *clone;
  int nqid;
  Qid qid[1];
};
struct Mount {
  uvlong mountid;
  int mflag;
  Mount *next;
  Mount *order;
  Chan *to; /* channel replacing channel */
  char spec[128];
};
struct Mhead {
  long ref;
  RWLock lock;
  Chan *from;   /* channel mounted upon */
  Mount *mount; /* what's mounted upon it */
  Mhead *hash;  /* Hash chain */
};
struct Mntrah {
  Rendez rendez;
  ulong vers;
  vlong off;
  vlong seq;
  uint i;
  Mntrpc *r[8];
};
struct Mntproc {
  Rendez rendez;
  Mnt *m;
  Mntrpc *r;
  void *a;
  void (*f)(Mntrpc *, void *);
};
struct Mnt {
  Lock lock;
  /* references are counted using c->ref; channels on this mount point
   * incref(c->mchan) == Mnt.c */
  Chan *c;            /* Channel to file service */
  Proc *rip;          /* Reader in progress */
  Mntrpc *queue;      /* Queue of pending requests on this channel */
  Mntproc defered[8]; /* Worker processes for defered RPCs (read ahead) */
  ulong id;           /* Multiplexer id for channel check */
  Mnt *list;          /* Free list */
  int flags;          /* cache */
  int msize;          /* data + IOHDRSZ */
  char *version;      /* 9P version */
  Queue *q;           /* input queue */
};
enum {
  NUser,  /* note provided externally */
  NExit,  /* deliver note quietly */
  NDebug, /* print debug message */
};
struct Note {
  char msg[128];
  int flag; /* whether system posted it */
  long ref;
};
enum {
  PG_MOD = 0x01,  /* software modified bit */
  PG_REF = 0x02,  /* software referenced bit */
  PG_PRIV = 0x04, /* private page */
};
struct Page {
  long ref;
  Page *next;       /* Free list or Hash chains */
  uintptr pa;       /* Physical address in memory */
  uintptr va;       /* Virtual address for user */
  uintptr daddr;    /* Disc address on swap */
  Image *image;     /* Associated text or swap image */
  ushort refage;    /* Swap reference age */
  char modref;      /* Simulated modify/reference bits */
  char color;       /* Cache coloring */
  char token_color; /* Pebble token color (enum PebbleColor) */
};
struct Swapalloc {
  Lock lock;       /* Free map lock */
  int free;        /* currently free swap pages */
  uchar *swmap;    /* Base of swap map in memory */
  uchar *alloc;    /* Round robin allocator */
  uchar *last;     /* Speed swap allocation */
  uchar *top;      /* Top of swap map */
  Rendez r;        /* Pager kproc idle sleep */
  ulong highwater; /* Pager start threshold */
  ulong headroom;  /* Space pager frees under highwater */
  ulong xref;      /* Ref count for all map refs >= 255 */
};
extern struct Swapalloc swapalloc;
struct Pte {
  Page *pages[((1ull * 1048576u) /
               (0x1000ull))]; /* Page map for this chunk of pte */
  Page **first;               /* First used entry */
  Page **last;                /* Last used entry */
};
/* Segment types */
enum {
  SG_TYPE = 07, /* Mask type of segment */
  SG_TEXT = 00,
  SG_DATA = 01,
  SG_BSS = 02,
  SG_STACK = 03,
  SG_SHARED = 04,
  SG_PHYSICAL = 05,
  SG_FIXED = 06,
  SG_STICKY = 07,
  SG_RONLY = 0040,   /* Segment is read only */
  SG_CEXEC = 0100,   /* Detach at exec */
  SG_FAULT = 0200,   /* Fault on access */
  SG_CACHED = 0400,  /* Normal cached memory */
  SG_DEVICE = 01000, /* Memory mapped device */
  SG_NOEXEC = 02000, /* No execute */
  SG_WASM = 04000,   /* WASM-isolated segment */
};
struct Physseg {
  int attr;      /* Segment attributes */
  char *name;    /* Attach name */
  uintptr pa;    /* Physical address */
  uintptr size;  /* Maximum segment size in bytes */
  Physseg *next; /* Next in linked list */
  Physseg *prev; /* Previous in linked list */
};
struct Sema {
  Rendez rendez;
  long *addr;
  int waiting;
  Sema *next;
  Sema *prev;
};
struct Segment {
  QLock qlock;
  long ref;
  int type;       /* segment type */
  ulong size;     /* size in pages */
  uintptr base;   /* virtual base */
  uintptr top;    /* virtual top */
  uintptr fstart; /* start address in file for demand load */
  uintptr flen;   /* length of segment in file */
  int flushme;    /* maintain icache for this segment */
  Image *image;   /* text in file attached to this segment */
  Physseg *pseg;
  ulong *profile; /* Tick profile area */
  Pte **map;
  int mapsize;
  Pte *ssegmap[16];
  ulong used;    /* pages used (swapped or not) */
  ulong swapped; /* pages swapped */
  Sema sema;
};
struct Segio {
  QLock lock;
  Rendez cmdwait;
  Rendez replywait;
  Proc *p; /* segmentio kproc */
  Segment *s;
  char *data;
  char *addr;
  int dlen;
  int cmd;
  char *err;
};
enum {
  RENDLOG = 5,
  RENDHASH = 1 << RENDLOG, /* Hash to lookup rendezvous tags */
  MNTLOG = 5,
  MNTHASH = 1 << MNTLOG, /* Hash to walk mount table */
  NFD = 100,             /* per process file descriptors */
  ENVLOG = 5,
  ENVHASH = 1 << ENVLOG, /* Egrp hash for variable lookup */
};
struct Image {
  Lock lock;
  long ref;
  long pgref;    /* number of cached pages (pgref <= ref) */
  ulong nattach; /* usage frequency */
  Image **link;  /* idle list */
  Image *next;   /* idle list */
  Image *hash;   /* Qid hash chains */
  Segment *s;    /* TEXT segment for image if running */
  Chan *c;       /* channel to text file, nil when not used */
  Qid qid;       /* Qid for page cache coherence */
  ulong dev;     /* Device id of owning channel */
  ushort type;   /* Device type of owning channel */
  char notext;   /* no file associated */
  ulong pghsize;
  Page *pghash[]; /* page cache */
};
struct Pgrp {
  long ref;
  RWLock ns;            /* Namespace n read/one write lock */
  u64int notallowed[4]; /* Room for 256 devices */
  Mhead *mnthash[MNTHASH];
  /* Namespace spawn limits - cryptographically bound via identity_hash */
  u8int identity_hash[16]; /* Blake2b hash of Pgrp for spawn cap binding */
  u8int namespace_cid[32]; /* Full BLAKE2b hash of Namespace Config
                              (Mounts+Caps) */
  Lock spawn_lock;         /* Protect spawn counts */
  u32int spawn_limit;      /* Max procs allowed in this namespace */
  u32int spawn_count;      /* Current proc count in namespace */
};
struct Rgrp {
  long ref;
  Lock lock;
  Proc *rendhash[RENDHASH]; /* Rendezvous tag hash */
};
struct Evalue {
  char *value;
  int len;
  ulong vers;
  uvlong path; /* qid.path: Egrp.path << 32 | index (of Egrp.ent[]) */
  Evalue *hash;
  char name[];
};
struct Egrp {
  Ref ref;
  RWLock rwlock;
  Evalue **ent;
  int nent;              /* numer of slots in ent[] */
  int low;               /* lowest free index in ent[] */
  int alloc;             /* bytes allocated for env */
  ulong path;            /* generator for qid path */
  ulong vers;            /* of Egrp */
  Evalue *hash[ENVHASH]; /* hashtable for name lookup */
};
struct Fgrp {
  Lock lock;
  Ref ref;
  Chan **fd;
  uchar *flag; /* per file-descriptor flags (CCEXEC) */
  int nfd;     /* number allocated */
  int maxfd;   /* highest fd in use */
  int exceed;  /* debugging */
};
enum {
  DELTAFD = 20 /* incremental increase in Fgrp.fd's */
};
struct Palloc {
  Lock lock;
  Page *head;       /* freelist head */
  ulong freecount;  /* how many pages on free list now */
  Page *pages;      /* array of all pages */
  ulong user;       /* how many user pages */
  Rendezq pwait[2]; /* Queues of procs waiting for memory */
};
struct Waitq {
  Waitmsg w;
  Waitq *next;
};
/*
 * fasttick timer interrupts
 */
enum {
  /* Mode */
  Trelative, /* timer programmed in ns from now */
  Tperiodic, /* periodic timer, period in ns */
};
struct Timer {
  /* Internal - Lock MUST be first for ilock(timer) to work */
  Lock lock;
  /* Public interface */
  int tmode; /* See above */
  vlong tns; /* meaning defined by mode */
  void (*tf)(Ureg *, Timer *);
  void *ta;
  /* Internal */
  Mach *tactive; /* The cpu that tf is active on */
  Timers *tt;    /* Timers queue this timer runs on */
  Tval tticks;   /* tns converted to ticks */
  Tval twhen;    /* ns represented in fastticks */
  Timer *tnext;
};
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
  RFNOMNT = (1 << 14),
};
/*
 *  process memory segments - NSEG always last !
 */
enum {
  SSEG,
  TSEG,
  DSEG,
  BSEG,
  ESEG,
  P9SEG,
  LSEG,
  SEG1,
  SEG2,
  SEG3,
  SEG4,
  NSEG
};
enum {
  Dead = 0, /* Process states */
  Moribund,
  New,
  Ready,
  Scheding,
  Running,
  Queueing,
  QueueingR,
  QueueingW,
  Wakeme,
  Broken,
  Stopped,
  Rendezvous,
  Waitrelease,
  Proc_stopme = 1, /* devproc requests */
  Proc_exitme,
  Proc_traceme,
  Proc_exitbig,
  Proc_tracesyscall,
  TUser = 0, /* Proc.time */
  TSys,
  TReal,
  TCUser,
  TCSys,
  TCReal,
  NERR = 32,
  NNOTE = 5,
  Npriq = 20,           /* number of scheduler priority levels */
  Nrq = Npriq + 2,      /* number of priority levels including real time */
  PriRelease = Npriq,   /* released edf processes */
  PriEdf = Npriq + 1,   /* active edf processes */
  PriNormal = 10,       /* base priority for normal processes */
  PriExtra = Npriq - 1, /* edf processes at high best-effort pri */
  PriKproc = 13,        /* Magic guard values for detecting corruption */
  PriRoot = 13,         /* base priority for root processes */
};
struct Schedq {
  Lock lock;
  Proc *head;
  Proc *tail;
  int n;
};
struct Proc {
  Label sched; /* known to l.s - MUST BE FIRST for asm */
  union {
    Timer timer; /* Timer state for tsleep/realtime */
    struct {
      Lock tlock;
      int tmode;
      vlong tns;
      void (*tf)(Ureg *, Timer *);
      void *ta;
      Mach *tactive;
      Timers *tt;
      Tval tticks;
      Tval twhen;
      Timer *tnext;
    };
  };
  Mach *mach; /* machine running this proc */
  char *text;
  char *user;
  uintptr entry_point; /* Entry point for process (ELF e_entry or UTZERO for
                          a.out) */
  char *args;
  int nargs;     /* number of bytes of args */
  int setargs;   /* process changed its args */
  Proc *rnext;   /* next process in run queue */
  Proc *qnext;   /* next process on queue for a QLock */
  char *psstate; /* What /proc/#/status reports */
  int state;
  ushort state_trace;  /* FSM: last 4 states (4 bits each) */
  ushort hdr_checksum; /* CRC-16 of critical fields */
  /* 9P Exchange Page for pure 9P architecture */
  void *p9page;           /* DEPRECATED: Fixed exchange page (legacy).
                           * New code should use exchange_channel via #X device.
                           * Kept for backwards compatibility with existing doorbell code.
                           */
  uvlong p9page_phys;     /* physical address of p9page */
  uintptr p9uaddr;        /* user VA for exchange page (per-process slot) */
  void *exchange_channel; /* ExchangeChannel from #X device (devexchange.c)
                           * Provides: ring buffer, page pool, capabilities */
  /* 9P FID tracking for syscall translation layer */
  u32int fid_counter;     /* Next FID to allocate for this process */
  u32int dot_fid;         /* FID for current working directory */
  vlong fid_offsets[256]; /* Offset per FID for read/write/seek tracking */
  ulong pid;
  uuid_t pid2;  /* Lux9 Secure ID */
  ulong noteid; /* Equivalent of note group */
  ulong parentpid;
  ulong index;
  Proc *parent; /* Process to send wait record on exit */
  Lock exl;     /* Lock count and waitq */
  Waitq *waitq; /* Exited processes wait children */
  int nchild;   /* Number of living children */
  int nwait;    /* Number of uncollected wait records */
  QLock qwaitr;
  Rendez waitr;  /* Place to hang out in wait */
  QLock seglock; /* locked whenever seg[] changes */
  Segment *seg[NSEG];
  Pgrp *pgrp;        /* Process group for namespace */
  Egrp *egrp;        /* Environment group */
  Fgrp *fgrp;        /* File descriptor group */
  Rgrp *rgrp;        /* Rendez group */
  Fgrp *closingfgrp; /* used during teardown */
  int insyscall;
  ulong time[6]; /* User, Sys, Real; child U, S, R */
  /* Borrow checker / lock DAG */
  uintptr waiting_for_key;
  struct LockDagContext lockdag;
  uvlong kentry; /* Kernel entry time stamp (for profiling) */
  /*
   * pcycles: cycles spent in this process (updated on procswitch)
   * when this is the current proc and we're in the kernel
   * (procrestores outnumber procsaves by one)
   * the number of cycles spent in the proc is pcycles + cycles()
   * when this is not the current process or we're in user mode
   * (procrestores and procsaves balance), it is pcycles.
   */
  vlong pcycles;
  QLock debug;     /* to access debugging elements of User */
  Proc *pdbg;      /* the debugging process */
  ulong procmode;  /* proc device default file mode */
  int privatemem;  /* proc does not let anyone read mem */
  int noswap;      /* process is not swappable */
  int hang;        /* hang at next exec for debug */
  int procctl;     /* Control for /proc debugging */
  Lock rlock;      /* sync sleep/wakeup with procinterrupt */
  Rendez *r;       /* rendezvous point slept on */
  Rendez sleep;    /* place for syssleep/debug */
  int notepending; /* note issued but not acted on */
  int kp;          /* true if a kernel process */
  Proc *palarm;    /* Next alarm time */
  ulong alarm;     /* Time of call */
  int newtlb;      /* Pager has changed my pte's, I must flush */
  Proc *vforkp;    /* vfork parent to unblock on exec/exit */
  uintptr rendtag; /* Tag for rendezvous */
  uintptr rendval; /* Value for rendezvous */
  Proc *rendhash;  /* Hash list for tag values */
  Rendez *trend;
  int (*tfn)(void *);
  void (*kpfun)(void *);
  void *kparg;
  Sargs s;     /* syscall arguments */
  int scallnr; /* sys call number */
  int nerrlab;
  Label errlab[NERR];
  char *syserrstr; /* last error from a system call, errbuf0 or 1 */
  char *errstr;    /* reason we're unwinding the error stack, errbuf1 or 0 */
  char errbuf0[128];
  char errbuf1[128];
  char genbuf[4096]; /* buffer used e.g. for last name element from namec */
  Chan *slash;
  Chan *dot;
  Note *lastnote;
  Note *note[NNOTE];
  short nnote;
  short notified; /* sysnoted is due */
  int (*notify)(void *, char *);
  Lock *lastlock;  /* debugging */
  Lock *lastilock; /* debugging */
  int nlocks;      /* number of locks held by proc */
  ulong delaysched;
  ulong priority; /* priority level */
  ulong basepri;  /* base priority level */
  uchar fixedpri; /* priority level doesn't change */
  uchar wired;
  int affinity; /* machno this process last ran on */
  ulong cpu;    /* cpu average */
  ulong lastupdate;
  uchar *kstack; /* base of kernel stack allocation */
  Edf *edf;    /* if non-null, real-time proc, edf contains scheduling params */
  int trace;   /* process being traced? */
  uintptr qpc; /* pc calling last blocking qlock */
  uintptr pc;  /* program counter for profiling */
  QLock *eql;  /* interruptable eqlock */
  void *noteureg; /* User registers for notes */
  void *dbgreg;   /* User registers for devproc */
  /* PFPU fields */
  int fpstate;
  int kfpstate;
  FPalloc *fpsave;
  FPalloc *kfpsave;
  /* PMMU fields */
  MMU *mmuhead;
  MMU *mmutail;
  MMU *kmaphead;
  MMU *kmaptail;
  ulong kmapcount;
  ulong kmapindex;
  ulong mmucount;
  u64int dr[8];
  void *vmx;
  char *syscalltrace; /* syscall trace */
  Watchpt *watchpt;   /* watchpoints */
  int nwatchpt;
  /* SIP/HIP capabilities - see docs/SIP_DEV_PLAN.md */
  ulong capabilities; /* Capability bitmap for hardware access */
  /* Temporary storage for devwalk unwind */
  Walkqid *walkq;
  Chan *walkclone;
  int walkalloc;
  /* Pebble resource tracking */
  PebbleState pebble;
  /* BEVIS: Proof-of-Work Nonce for resource acquisition */
  u64int pow_nonce;
  /* Security: Hash of the running binary (Blake2b-512) */
  uchar text_hash[64];
  /* CLR Thread-Local Storage (for managed code LocalDataStore) */
  void *clr_tls[64];
  int clr_tls_next_slot;
  /* WASM execution context (only populated if this is a WASM process)
   * See ADR_WASM_AS_PROCESSES.md for architecture rationale.
   * WASM programs run as first-class processes, not separate instances.
   */
  struct {
    int initialized;       /* 1 if this is a WASM process, 0 for native */
    void *runtime;         /* IM3Runtime - wasm3 runtime for this process */
    void *module;          /* IM3Module - loaded WASM module */
    void *env;             /* IM3Environment - per-process wasm3 environment */
    u8int *linear_memory;  /* WASM linear memory (mapped to seg[LSEG]) */
    u32int memory_size;    /* Size of linear memory in bytes */
    u32int memory_pages;   /* Number of 64KB WASM pages */
    u32int linear_charged; /* Pebble-charged linear memory bytes */
    u8int *heap_base;      /* WASM runtime heap base (userspace addr) */
    u32int heap_size;      /* WASM runtime heap size in bytes */
    u32int heap_used;      /* WASM runtime heap used bytes */
    void *heap_head;       /* WASM heap block list head */
    u32int heap_live;      /* WASM heap live bytes (token-backed) */
    arena_branch_t branch; /* Local Pebble branch bank for this container */
    void *wasi_ctx;        /* WASI Context (wasi_lux9_shim.h wasi_context_t) */
    void *module_bytes;    /* Persistent WASM module bytecode */
    u32int module_bytes_len;
    u32int permissions; /* Active WASM capability permissions bitmask */
    wasm_cap_table_t *cap_table; /* Capability handle table */
  } wasm;
  /* Spawn Capability - UUIDv8-based process creation control.
   * Uses CAP_TYPE_SPAWN capability token with child limit. */
  uuid_t spawn_cap; /* UUIDv8 spawn capability (null = no spawn rights) */
  u32int spawn_max_children; /* Maximum children this process can spawn */
  u32int spawn_children;     /* Current number of children spawned */
  /* Init Hardening: Binary binding for spawn.
   * If non-zero, this process can ONLY exec binaries matching this hash.
   * Used to ensure init can only spawn resurrection server. */
  u8int spawn_bound_binary[64]; /* Blake2b-512 of allowed binary (0 = any) */
};
enum {
  PRINTSIZE = 256,
  NUMSIZE = 12, /* size of formatted number */
  MB = (1024 * 1024),
  /* READSTR was 1000, which is way too small for usb's ctl file */
  READSTR = 8000, /* temporary buffer size for device reads */
};
extern Conf conf;
extern char *conffile;
extern int cpuserver;
extern Dev *devtab[];
extern char *eve;
extern char hostdomain[];
extern uchar initcode[];
extern Queue *kprintoq;
extern int nsyscall;
extern Palloc palloc;
extern int panicking;
extern Queue *serialoq;
extern char *statename[];
extern Image *swapimage;
extern Image *fscache;
extern char *sysname;
extern uint qiomaxatomic;
extern char *sysctab[];
enum {
  LRESPROF = 3,
};
/*
 *  action log
 */
struct Log {
  Lock lock;
  int opens;
  char *buf;
  char *end;
  char *rptr;
  int len;
  int nlog;
  int minread;
  int logmask; /* mask of things to debug */
  QLock readq;
  Rendez readr;
};
struct Logflag {
  char *name;
  int mask;
};
enum { NCMDFIELD = 128 };
struct Cmdbuf {
  char *buf;
  char **f;
  int nf;
};
struct Cmdtab {
  int index; /* used by client to switch on result */
  char *cmd; /* command name */
  int narg;  /* expected #args; 0 ==> variadic */
};
/*
 *  routines to access UART hardware
 */
struct PhysUart {
  char *name;
  Uart *(*pnp)(void);
  void (*enable)(Uart *, int);
  void (*disable)(Uart *);
  void (*kick)(Uart *);
  void (*dobreak)(Uart *, int);
  int (*baud)(Uart *, int);
  int (*bits)(Uart *, int);
  int (*stop)(Uart *, int);
  int (*parity)(Uart *, int);
  void (*modemctl)(Uart *, int);
  void (*rts)(Uart *, int);
  void (*dtr)(Uart *, int);
  char *(*status)(Uart *, char *, char *);
  void (*fifo)(Uart *, int);
  void (*power)(Uart *, int);
  int (*getc)(Uart *); /* polling versions, for iprint, rdb */
  void (*putc)(Uart *, int);
};
enum { Stagesize = 2048 };
/*
 *  software UART
 */
struct Uart {
  void *regs;     /* hardware stuff */
  void *saveregs; /* place to put registers on power down */
  char *name;     /* internal name */
  ulong freq;     /* clock frequency */
  int bits;       /* bits per character */
  int stop;       /* stop bits */
  int parity;     /* even, odd or no parity */
  int baud;       /* baud rate */
  PhysUart *phys;
  int console; /* used as a serial console */
  int special; /* internal kernel device */
  Uart *next;  /* list of allocated uarts */
  QLock lock;
  int type; /* ?? */
  int dev;
  int opens;
  int enabled;
  Uart *elist; /* next enabled interface */
  int perr;    /* parity errors */
  int ferr;    /* framing errors */
  int oerr;    /* rcvr overruns */
  int berr;    /* no input buffers */
  int serr;    /* input queue overflow */
  /* buffers */
  int (*putc)(Queue *, int);
  Queue *iq;
  Queue *oq;
  Lock rlock;
  uchar istage[Stagesize];
  uchar *iw;
  uchar *ir;
  uchar *ie;
  Lock tlock; /* transmit */
  uchar ostage[Stagesize];
  uchar *op;
  uchar *oe;
  int drain;
  int modem;  /* hardware flow control on */
  int xonoff; /* software flow control on */
  int blocked;
  int cts, dsr, dcd; /* keep track of modem status */
  int ctsbackoff;
  int hup_dsr, hup_dcd; /* send hangup upstream? */
  int dohup;
  Rendez r;
};
extern Uart *consuart;
/*
 *  performance timers, all units in perfticks
 */
struct Perf {
  ulong intrts;     /* time of last interrupt */
  ulong inintr;     /* time since last clock tick in interrupt handlers */
  ulong avg_inintr; /* avg time per clock tick in interrupt handlers */
  ulong inidle;     /* time since last clock tick in idle loop */
  ulong avg_inidle; /* avg time per clock tick in idle loop */
  ulong last;       /* value of perfticks() at last clock tick */
  ulong period;     /* perfticks() per clock tick */
};
struct Watchdog {
  void (*enable)(void);         /* watchdog enable */
  void (*disable)(void);        /* watchdog disable */
  void (*restart)(void);        /* watchdog restart */
  void (*stat)(char *, char *); /* watchdog statistics */
};
struct Watchpt {
  enum {
    WATCHRD = 1,
    WATCHWR = 2,
    WATCHEX = 4,
  } type;
  uintptr addr, len;
};
struct PMach {
  Proc *readied;    /* for runproc */
  Label sched;      /* scheduler wakeup */
  ulong ticks;      /* of the clock since boot time */
  ulong schedticks; /* next forced context switch */
  int pfault;
  int cs;
  int syscall;
  int load;
  int intr;
  int ilockdepth;
  int flushmmu; /* make current proc flush it's mmu state */
  int tlbfault;
  int tlbpurge;
  Perf perf;        /* performance counters */
  uvlong cyclefreq; /* Frequency of user readable cycle counter */
};
/* queue state bits,  Qmsg, Qcoalesce, and Qkick can be set in qopen */
enum {
  /* Queue.state */
  Qstarve = (1 << 0),   /* consumer starved */
  Qmsg = (1 << 1),      /* message stream */
  Qclosed = (1 << 2),   /* queue has been closed/hungup */
  Qflow = (1 << 3),     /* producer flow controlled */
  Qcoalesce = (1 << 4), /* coallesce packets on read */
  Qkick = (1 << 5),     /* always call the kick routine after qwrite */
};
/*
 * Log console output so it can be retrieved via /dev/kmesg.
 * This is good for catching boot-time messages after the fact.
 */
struct Kmesg {
  Lock lk;
  uint n;
  char buf[16384];
};
extern struct Kmesg kmesg;
typedef struct {
  u32int _0_;
  u32int rsp0[2];
  u32int rsp1[2];
  u32int rsp2[2];
  u32int _28_[2];
  u32int ist[14];
  u16int _92_[5];
  u16int iomap;
} Tss;
struct Mach {
  int machno;    /* physical id of processor */
  uintptr splpc; /* pc of last caller to splhi */
  Proc *proc;    /* current process on this processor */
  /* PMach fields */
  uintptr rbx_restore; /* scratch for saving user RBX during syscallentry */
  Proc *readied;       /* for runproc */
  Label sched;         /* scheduler wakeup */
  ulong ticks;         /* of the clock since boot time */
  ulong schedticks;    /* next forced context switch */
  int pfault;
  int cs;
  int syscall;
  int load;
  int intr;
  int ilockdepth;
  int flushmmu; /* make current proc flush it's mmu state */
  int tlbfault;
  int tlbpurge;
  Perf perf;        /* performance counters */
  uvlong cyclefreq; /* Frequency of user readable cycle counter */
  uvlong tscticks;
  ulong spuriousintr;
  int lastintr;
  int loopconst;
  int delaylcycles;
  int cpumhz;
  uvlong cpuhz;
  int cpuidax;
  int cpuidcx;
  int cpuiddx;
  char cpuidid[16];
  char *cpuidtype;
  uchar cpuidfamily;
  uchar cpuidmodel;
  uchar cpuidstepping;
  char havetsc;
  char havepge;
  char havewatchpt8;
  char havenx;
  char haveaes;    /* AES-NI instructions available */
  char havesha;    /* SHA extensions available */
  char havepclmul; /* PCLMULQDQ instruction available */
  char haverdrand; /* RDRAND instruction available */
  int fpstate;     /* FPU state for interrupts */
  FPalloc *fpsave;
  uintptr *pml4; /* pml4 base for this processor (va) */
  Tss *tss;      /* tss for this processor */
  Segdesc *gdt;  /* gdt for this processor */
  u64int dr7;    /* shadow copy of dr7 */
  u64int xcr0;
  void *vmx;
  MMU *mmufree;     /* freelist for MMU structures */
  ulong mmucount;   /* number of MMU structures in freelist */
  u64int mmumap[4]; /* bitmap of pml4 entries for zapping */
  uintptr stack[1];
};
/*
 * KMap the structure
 */
typedef void KMap;
extern u64int MemMin;
struct Active {
  char machs[128]; /* bitmap of active CPUs */
  int exiting;     /* shutdown */
};
extern struct Active active;
/*
 *  routines for things outside the PC model, like power management
 */
struct PCArch {
  char *id;
  int (*ident)(void);  /* this should be in the model */
  void (*reset)(void); /* this should be in the model */
  void (*intrinit)(void);
  int (*intrassign)(Vctl *);
  int (*intrirqno)(int, int);
  int (*intrvecno)(int);
  int (*intrspurious)(int);
  void (*introff)(void);
  void (*intron)(void);
  void (*clockinit)(void);
  void (*clockenable)(void);
  uvlong (*fastclock)(uvlong *);
  void (*timerset)(uvlong);
};
/* cpuid instruction result register bits */
enum {
  /* ax */
  Xsaveopt = 1 << 0,
  Xsaves = 1 << 3,
  /* cx */
  Pclmulqdq = 1 << 1, /* PCLMULQDQ instruction */
  Monitor = 1 << 3,
  Aes = 1 << 25, /* AES-NI instructions */
  Xsave = 1 << 26,
  Avx = 1 << 28,
  Rdrnd = 1 << 30, /* RDRAND instruction */
  /* dx */
  Fpuonchip = 1 << 0,
  Vmex = 1 << 1,   /* virtual-mode extensions */
  Pse = 1 << 3,    /* page size extensions */
  Tsc = 1 << 4,    /* time-stamp counter */
  Cpumsr = 1 << 5, /* model-specific registers, rdmsr/wrmsr */
  Pae = 1 << 6,    /* physical-addr extensions */
  Mce = 1 << 7,    /* machine-check exception */
  Cmpxchg8b = 1 << 8,
  Cpuapic = 1 << 9,
  Mtrr = 1 << 12, /* memory-type range regs.  */
  Pge = 1 << 13,  /* page global extension */
  Mca = 1 << 14,  /* machine-check architecture */
  Pat = 1 << 16,  /* page attribute table */
  Pse2 = 1 << 17, /* more page size extensions */
  Clflush = 1 << 19,
  Acpif = 1 << 22, /* therm control msr */
  Mmx = 1 << 23,
  Fxsr = 1 << 24, /* have SSE FXSAVE/FXRSTOR */
  Sse = 1 << 25,  /* thus sfence instr. */
  Sse2 = 1 << 26, /* thus mfence & lfence instr.s */
};
enum {                            /* MSRs */
       PerfEvtbase = 0xc0010000,  /* Performance Event Select */
       PerfCtrbase = 0xc0010004,  /* Performance Counters */
       Efer = 0xc0000080,         /* Extended Feature Enable */
       Star = 0xc0000081,         /* Legacy Target IP and [CS]S */
       Lstar = 0xc0000082,        /* Long Mode Target IP */
       Cstar = 0xc0000083,        /* Compatibility Target IP */
       Sfmask = 0xc0000084,       /* SYSCALL Flags Mask */
       FSbase = 0xc0000100,       /* 64-bit FS Base Address */
       GSbase = 0xc0000101,       /* 64-bit GS Base Address */
       KernelGSbase = 0xc0000102, /* SWAPGS instruction */
};
/*
 *  a parsed plan9.ini line
 */
struct ISAConf {
  char *type;
  uvlong port;
  int irq;
  ulong dma;
  ulong mem;
  ulong size;
  ulong freq;
  int nopt;
  char *opt[8];
};
extern PCArch *arch; /* PC architecture */
extern Mach *machp[128];
extern Mach *m;  /* R15 */
extern Proc *up; /* R14 */
/*
 *  hardware info about a device
 */
typedef struct {
  ulong port;
  int size;
} Devport;
struct DevConf {
  ulong intnum;   /* interrupt number */
  char *type;     /* card type, malloced */
  int nports;     /* Number of ports */
  Devport *ports; /* The ports themselves */
};
extern char Enoerror[128];    /* no error */
extern char Emount[128];      /* inconsistent mount */
extern char Eunmount[128];    /* not mounted */
extern char Eismtpt[128];     /* is a mount point */
extern char Eunion[128];      /* not in union */
extern char Emountrpc[128];   /* mount rpc error */
extern char Eshutdown[128];   /* device shut down */
extern char Enocreate[128];   /* mounted directory forbids creation */
extern char Enonexist[128];   /* file does not exist */
extern char Eexist[128];      /* file already exists */
extern char Ebadsharp[128];   /* unknown device in # filename */
extern char Enotdir[128];     /* not a directory */
extern char Eisdir[128];      /* file is a directory */
extern char Ebadchar[128];    /* bad character in file name */
extern char Efilename[128];   /* file name syntax */
extern char Eperm[128];       /* permission denied */
extern char Ebadusefd[128];   /* inappropriate use of fd */
extern char Ebadarg[128];     /* bad arg in system call */
extern char Einuse[128];      /* device or object already in use */
extern char Eio[128];         /* i/o error */
extern char Etoobig[128];     /* read or write too large */
extern char Etoosmall[128];   /* read or write too small */
extern char Enoport[128];     /* network port not available */
extern char Ehungup[128];     /* i/o on hungup channel */
extern char Ebadctl[128];     /* bad process or channel control request */
extern char Enodev[128];      /* no free devices */
extern char Eprocdied[128];   /* process exited */
extern char Enochild[128];    /* no living children */
extern char Eioload[128];     /* i/o error in demand load */
extern char Enovmem[128];     /* virtual memory allocation failed */
extern char Ebadfd[128];      /* fd out of range or not open */
extern char Enofd[128];       /* no free file descriptors */
extern char Eisstream[128];   /* seek on a stream */
extern char Ebadexec[128];    /* exec header invalid */
extern char Etimedout[128];   /* connection timed out */
extern char Econrefused[128]; /* connection refused */
extern char Econinuse[128];   /* connection in use */
extern char Eintr[128];       /* interrupted */
extern char Enomem[128];      /* kernel allocate failed */
extern char Esoverlap[128];   /* segments overlap */
extern char Emouseset[128];   /* mouse type already set */
extern char Eshort[128];      /* i/o count too small */
extern char Egreg[128];       /* the front fell off */
extern char Ebadspec[128];    /* bad attach specifier */
extern char Enoreg[128];      /* process has no saved registers */
extern char Enoattach[128];   /* mount/attach disallowed */
extern char Eshortstat[128];  /* stat buffer too small */
extern char Ebadstat[128];    /* malformed stat buffer */
extern char Enegoff[128];     /* negative i/o offset */
extern char Ecmdargs[128];    /* wrong #args in control message */
extern char Ebadip[128];      /* bad ip address syntax */
extern char Edirseek[128];    /* seek in directory */
extern char Etoolong[128];    /* name too long */
extern char Echange[128];     /* media or partition has changed */

/* Copyright (C) 1989-2023 Free Software Foundation, Inc.
This file is part of GCC.
GCC is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 3, or (at your option)
any later version.
GCC is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.
Under Section 7 of GPL version 3, you are granted additional
permissions described in the GCC Runtime Library Exception, version
3.1, as published by the Free Software Foundation.
You should have received a copy of the GNU General Public License and
a copy of the GCC Runtime Library Exception along with this program;
see the files COPYING3 and COPYING.RUNTIME respectively.  If not, see
<http://www.gnu.org/licenses/>.  */
/*
 * ISO C Standard:  7.15  Variable arguments  <stdarg.h>
 */
void _assert(char *);
void accounttime(void);
Timer *addclock0link(void (*)(void), int);
Physseg *addphysseg(Physseg *);
void addbootfile(char *, uchar *, ulong);
void addwatchdog(Watchdog *);
Block *adjustblock(Block *, int);
void alarmkproc(void *);
Block *allocb(int);
int anyhigher(void);
int anyready(void);
Image *attachimage(Chan *, ulong size);
ulong beswal(ulong);
uvlong beswav(uvlong);
int blocklen(Block *);
void bootlinks(void);
void cachedel(Image *, uintptr);
void cachepage(Page *, Image *);
void callwithureg(void (*)(Ureg *));
char *chanpath(Chan *);
/*@ requires \valid(l);
  @ terminates \true;
  @ assigns \nothing;
  @*/
int canlock(Lock *l);
int canpage(Proc *);
/*@ requires \valid(q);
  @ terminates \true;
  @ assigns \nothing;
  @*/
int canqlock(QLock *q);
int cmpswap486(long *, long, long);
int canrlock(RWLock *);
void chandevinit(void);
void chandevreset(void);
void chandevshutdown(void);
void chanfree(Chan *);
void checkalarms(void);
void checkpages(void);
void checkb(Block *, char *);
void cinit(void);
Chan *cclone(Chan *);
void cclose(Chan *);
void ccloseq(Chan *);
void closeegrp(Egrp *);
void closefgrp(Fgrp *);
void closepgrp(Pgrp *);
void closergrp(Rgrp *);
long clrfpintr(void);
_Noreturn void cmderror(Cmdbuf *, char *);
int cmount(Chan *, Chan *, int, char *);
void confinit(void);
int consactive(void);
extern void (*consdebug)(void);
void cpushutdown(void);
int copen(Chan *);
void cclunk(Chan *);
Block *concatblock(Block *);
Block *copyblock(Block *, int);
void copypage(Page *, Page *);
void countpagerefs(ulong *, int);
int cread(Chan *, uchar *, int, vlong);
void ctrunc(Chan *);
void cunmount(Chan *, Chan *);
void cupdate(Chan *, uchar *, int, vlong);
void cwrite(Chan *, uchar *, int, vlong);
uintptr dbgpc(Proc *);
Page *deadpage(Page *);
/*@ requires \valid(r);
  @ terminates \true;
  @ assigns r->ref;
  @ ensures r->ref < \old(r->ref);
  @*/
long decref(Ref *r);
int decrypt(void *, void *, int);
void delay(int);
Proc *dequeueproc(Schedq *, Proc *);
/*@ assigns \nothing; */
Chan *devattach(int, char *spec);
Block *devbread(Chan *, long, ulong);
long devbwrite(Chan *, Block *, ulong);
Chan *devclone(Chan *);
int devconfig(int, char *, DevConf *);
Chan *devcreate(Chan *, char *, int, ulong);
/*@ requires c != \null;
  @ requires name == \null || \valid(name);
  @ requires user == \null || \valid(user);
  @ requires dp != \null;
  @ terminates \true;
  @ assigns *dp;
  */
void devdir(Chan *c, Qid qid, char *name, vlong length, char *user, long perm,
            Dir *dp);
/*@ requires c != \null;
  @ terminates \true;
  @ assigns \nothing;
  */
long devdirread(Chan *c, char *va, long n, Dirtab *tab, int ntab, Devgen *gen);
Devgen devgen;
void devinit(void);
int devno(int, int);
Chan *devopen(Chan *, int, Dirtab *, int, Devgen *);
void devpermcheck(char *, ulong, int);
void devpower(int);
void devremove(Chan *);
void devreset(void);
void devshutdown(void);
/*@ requires c != \null;
  @ terminates \true;
  @ assigns \nothing;
  */
int devstat(Chan *c, uchar *dp, int n, Dirtab *tab, int ntab, Devgen *gen);
/*@ requires c != \null;
  @ terminates \true;
  @ assigns \result \from \nothing;
  */
Walkqid *devwalk(Chan *c, Chan *nc, char **name, int nname, Dirtab *tab,
                 int ntab, Devgen *gen);
int devwstat(Chan *, uchar *, int);
Dir *dirchanstat(Chan *);
int donotify(Ureg *);
void syscall_to_9p(Ureg *); /* Phase 6: Pure 9P dispatch replaces dosyscall */
void drawactive(int);
void drawcmap(void);
void dtracytick(Ureg *);
void dumpaproc(Proc *);
void dumpregs(Ureg *);
void dumpstack(void);
Fgrp *dupfgrp(Fgrp *);
void dupswap(Page *);
void edfinit(Proc *);
char *edfadmit(Proc *);
int edfready(Proc *);
void edfrecord(Proc *);
void edfrun(Proc *, int);
void edfstop(Proc *);
void edfyield(void);
int emptystr(char *);
int encrypt(void *, void *, int);
void envcpy(Egrp *, Egrp *);
int eqchan(Chan *, Chan *, int);
int eqchantdqid(Chan *, int, int, Qid, int);
int eqqid(Qid, Qid);
/* Frama-C compatible version without attributes */
/*@
  @ requires \valid_read(e);
  @ terminates \true;
  @ assigns \nothing;
  @ ensures \false;
  @*/
void lux9_error(char *e);
void eqlock(QLock *);
uintptr execregs(uintptr, ulong, ulong);
void exhausted(char *);
void exit(int);
uvlong fastticks(uvlong *);
uvlong fastticks2ns(uvlong);
uvlong fastticks2us(uvlong);
int fault(uintptr, uintptr, int);
int fixfault(Segment *, uintptr, int);
void faultnote(char *, char *, uintptr);
void fdclose(int, int);
Chan *fdtochan(int, int, int, int);
int findmount(Chan **, Mhead **, int, int, Qid);
void flushmmu(void);
void forceclosefgrp(void);
void forkchild(Proc *, Ureg *);
void forkret(void);
void fpunotify(Proc *);
void fpunoted(Proc *);
/*@ terminates \true;
  @ assigns \nothing;
  @ behavior null:
  @   assumes p == \null;
  @   assigns \nothing;
  @ behavior valid:
  @   assumes p != \null;
  @   requires \freeable(p);
  @   assigns \nothing;
  @ complete behaviors;
  @ disjoint behaviors;
  @*/
void free(void *p);
void freeb(Block *);
void freeblist(Block *);
int freebroken(void);
void freenote(Note *);
void freenotes(Proc *);
void freepages(Page *, Page *, ulong);
void getcolor(ulong, ulong *, ulong *, ulong *);
uintptr getmalloctag(void *);
uintptr getrealloctag(void *);
_Noreturn void gotolabel(Label *);
/*@ requires name == \null || valid_string(name);
  @ assigns \result \from name[0 .. ACSL_MAXSTR-1];
  @ ensures \result == \null || valid_string(\result);
  @*/
char *getconf(char *name);
char *getconfenv(void);
void growbp(Bpool *, int);
long hostdomainwrite(char *, int);
long hostownerwrite(char *, int);
extern void (*hwrandbuf)(void *, ulong);
void hzsched(void);
Block *iallocb(int);
Block *iallocbp(Bpool *);
uintptr ibrk(uintptr, int);
/*@ requires l != \null;
  @ terminates \true;
  @ assigns *l;
  @*/
void ilock(Lock *l);
_Noreturn void interrupted(void);
/*@ requires l != \null;
  @ terminates \true;
  @ assigns *l;
  @*/
void iunlock(Lock *l);
ulong imagecached(void);
ulong imagereclaim(ulong);
/*@ requires \valid(r);
  @ terminates \true;
  @ assigns r->ref;
  @ ensures r->ref > \old(r->ref);
  @*/
long incref(Ref *r);
void init0(void);
void initseg(void);
/*@
  @ requires name == \null || valid_string(name);
  @ assigns \nothing;
  @*/
int ioalloc(ulong addr, ulong size, ulong align, char *name);
void iofree(ulong);
void iomapinit(ulong);
/*@
  @ requires name == \null || valid_string(name);
  @ assigns \nothing;
  @*/
int ioreserve(ulong addr, ulong size, ulong align, char *name);
/*@
  @ requires name == \null || valid_string(name);
  @ assigns \nothing;
  @*/
int ioreservewin(ulong addr, ulong size, ulong align, ulong win, char *name);
int iounused(ulong, ulong);
/*@
  @ requires valid_string(fmt);
  @ assigns \nothing;
  @*/
int iprint(char *fmt, ...);
/*@
  @ requires valid_string(fmt);
  @ assigns \nothing;
  @*/
int iprint_intr(char *fmt, ...);
void isdir(Chan *);
int iseve(void);
int islo(void);
Segment *isoverlap(uintptr, uintptr);
Physseg *findphysseg(char *);
int kenter(Ureg *);
void kexit(Ureg *);
void kickpager(void);
void killbig(void);
void killproc(Proc *, int);
/*@
  @ requires valid_string(name);
  @ assigns \result;
  @*/
int kproc(char *name, void (*fn)(void *), void *arg);
void kprocchild(Proc *, void (*)(void));
void linkproc(void);
extern void (*kproftimer)(uintptr);
void ksetenv(char *, char *, int);
int kopen(char *, int);
void kstrcpy(char *, char *, int);
void kstrdup(char **, char *);
/*@ requires \valid(l);
  @ terminates \true;
  @ assigns *l;
  @*/
void lock(Lock *l);
void logopen(Log *);
void logclose(Log *);
char *logctl(Log *, int, char **, Logflag *);
void logn(Log *, int, void *, int);
long logread(Log *, void *, ulong, long);
void log(Log *, int, char *, ...);
Cmdtab *lookupcmd(Cmdbuf *, Cmdtab *, int);
Page *lookpage(Image *, uintptr);
void machinit(void);
/*@ terminates \true;
  @ assigns \result \from size;
  @ behavior success:
  @   assumes size > 0;
  @   assigns \result \from size;
  @   ensures \result != \null && \valid((char *)\result + (0 .. size - 1));
  @ behavior failure:
  @   assumes size > 0;
  @   assigns \result \from size;
  @   ensures \result == \null;
  @ behavior zero:
  @   assumes size == 0;
  @   assigns \result \from size;
  @   ensures \result == \null;
  @ complete behaviors;
  @ disjoint behaviors;
  @*/
void *malloc(ulong size);
/*@ terminates \true;
  @ assigns \result \from size;
  @ behavior success:
  @   assumes size > 0;
  @   assigns \result \from size;
  @   ensures \result != \null && \valid((char *)\result + (0 .. size - 1));
  @   ensures \forall integer i; 0 <= i < size ==> ((char *)\result)[i] == 0;
  @*/
void *mallocz(ulong size, int clr);
/*@ terminates \true;
  @ behavior zero:
  @   assumes size == 0;
  @   assigns \result \from \nothing;
  @   ensures \result == \null || \valid((char *)\result);
  @ behavior nonzero:
  @   assumes size > 0;
  @   assigns \result \from \nothing;
  @   ensures \result == \null || \valid(((char *)\result) + (0 .. (integer)size
  - 1));
  @ complete behaviors;
  @ disjoint behaviors;
  */
void *mallocalign(ulong size, ulong align, long offset, ulong span);
void mallocsummary(void);
void memmapdump(void);
uvlong memmapnext(uvlong, ulong);
uvlong memmapsize(uvlong, uvlong);
void memmapadd(uvlong, uvlong, ulong);
uvlong memmapalloc(uvlong, uvlong, uvlong, ulong);
void memmapfree(uvlong, uvlong, ulong);
void mfreeseg(Segment *, uintptr, ulong);
void microdelay(int);
uvlong mk64fract(uvlong, uvlong);
void mkqid(Qid *, vlong, ulong, int);
void mmurelease(Proc *);
void mmuswitch(Proc *);
Chan *mntattach(Chan *, Chan *, char *, int);
Chan *mntauth(Chan *, char *);
int mntversion(Chan *, char *, int, int);
void mouseresize(void);
void mountfree(Mount *);
ulong ms2tk(ulong);
ulong msize(void *);
ulong ms2tk(ulong);
uvlong ms2fastticks(ulong);
void mul64fract(uvlong *, uvlong, uvlong);
void muxclose(Mnt *);
Chan *namec(char *, int, int, ulong);
_Noreturn void namelenerror(char *, int, char *);
int needpages(void *);
Chan *newchan(void);
Egrp *newegrp(void);
int growfd(Fgrp *, int);
void unlockfgrp(Fgrp *);
int newfd(Chan *, int);
Mhead *newmhead(Chan *);
Mhead *newmhead(Chan *);
/*@ requires spec != \null;
  @ terminates \true;
  @ assigns \nothing;
  @ ensures \result == \null || \valid(\result);
  @*/
Mount *newmount(Chan *, int, char *spec);
Image *newimage(ulong);
Page *newpage(uintptr, Segment *);
/*@
  @ requires s.len >= 0;
  @ requires s.data == \null || \valid(s.data + (0..s.len-1));
  @ assigns \nothing;
  @ ensures \valid(\result);
  @*/
Path *newpath(BString s);
Pgrp *newpgrp(void);
Rgrp *newrgrp(void);
Proc *newproc(void);
_Noreturn void nexterror(void);
Ureg *notify(Ureg *, char *);
int noted(Ureg *, Ureg *, int);
FPsave *notefpsave(Proc *);
ulong nkpages(Confmem *);
uvlong ns2fastticks(uvlong);
int okaddr(uintptr, ulong, int);
int openmode(ulong);
Block *packblock(Block *);
Block *padblock(Block *, int);
void pageinit(void);
ulong pagereclaim(Image *);
_Noreturn void panic(const char *fmt, ...);
Cmdbuf *parsecmd(char *a, int n);
void pathclose(Path *);
ulong perfticks(void);
_Noreturn void pexit(char *, int);
void pgrpcpy(Pgrp *, Pgrp *);
void namespace_cid_update(Pgrp *);
void namespace_cid_update_locked(Pgrp *);
ulong pidalloc(Proc *);
void portcountpagerefs(ulong *, int);
char *popnote(Ureg *);
/*@ requires s == \null || valid_string(s); */
int postnote(Proc *, int, char *s, int);
void postnotepg(ulong, char *, int);
/*@ requires valid_string(fmt);
  @ assigns \nothing;
  @*/
int pprint(char *fmt, ...);
void preempted(int);
void prflush(void);
void printinit(void);
void setkprintqsize(char *);
void prbuf_init(void);
int prbuf_print(char *, int);
void prbuf_start_consumer(void);
int prbuf_has_data(void);
int prbuf_ready(void);
void prbuf_kprint_open(void);
void prbuf_kprint_close(void);
long prbuf_kprint_read(void *, long);
ulong procalarm(ulong);
void procctl(void);
int procfdprint(Chan *, int, char *, int);
void procflushseg(Segment *);
void procflushpseg(Physseg *);
void procflushothers(void);
int procindex(ulong);
void procinit0(void);
void procinterrupt(Proc *);
ulong procpagecount(Proc *);
void procpriority(Proc *, int, int);
void procsetuser(char *);
Proc *proctab(int);
extern void (*proctrace)(Proc *, int, vlong);
void procwired(Proc *, int);
Pte *ptealloc(void);
int pullblock(Block **, int);
Block *pullupblock(Block *, int);
Block *pullupqueue(Queue *, int);
int pushnote(Proc *, Note *);
void putimage(Image *);
void putmhead(Mhead *);
void putmmu(uintptr, uintptr, Page *);
void putpage(Page *);
void putseg(Segment *);
void putstrn(char *, int);
void putswap(Page *);
ulong pwait(Waitmsg *);
int qaddlist(Queue *, Block *);
Block *qbread(Queue *, int);
long qbwrite(Queue *, Block *);
Queue *qbypass(void (*)(void *, Block *), void *);
int qcanread(Queue *);
/*@ requires q != \null;
  @ terminates \true;
  @ assigns \nothing;
  */
void qclose(Queue *q);
int qconsume(Queue *, void *, int);
Block *qcopy(Queue *, int, ulong);
int qdiscard(Queue *, int);
void qflush(Queue *);
/*@ requires q != \null;
  @ terminates \true;
  @ assigns \nothing;
  */
void qfree(Queue *q);
int qfull(Queue *);
Block *qget(Queue *);
void qhangup(Queue *, char *);
int qisclosed(Queue *);
int qiwrite(Queue *, void *, int);
/*@ requires q != \null;
  @ terminates \true;
  @ assigns \nothing;
  */
int qlen(Queue *q);
/*@ requires l != \null;
  @ terminates \true;
  @ assigns *l;
  */
void qlock(QLock *l);
/*@ terminates \true;
  @ assigns \result \from \nothing;
  @ ensures \result == \null || \valid(\result);
  */
Queue *qopen(int, int, void (*)(void *), void *);
int qpass(Queue *, Block *);
int qpassnolim(Queue *, Block *);
int qproduce(Queue *, void *, int);
void qputback(Queue *, Block *);
/*@ requires q != \null;
  @ requires buf == \null || (n >= 0 && \valid(((char *)buf) + (0..n-1)));
  @ terminates \true;
  @ assigns ((char *)buf)[0..n-1];
  */
long qread(Queue *q, void *buf, int n);
Block *qremove(Queue *);
void qreopen(Queue *);
void qsetlimit(Queue *, int);
/*@ requires l != \null;
  @ terminates \true;
  @ assigns *l;
  */
void qunlock(QLock *l);
/*@ requires q != \null;
  @ requires buf == \null || (n >= 0 && \valid(((char *)buf) + (0..n-1)));
  @ terminates \true;
  @ assigns \nothing;
  */
int qwrite(Queue *q, void *buf, int n);
void qnoblock(Queue *, int);
void qsetnoblock_early(Queue *, int);
void randominit(void);
ulong randomread(void *, ulong);
void rdb(void);
long readblist(Block *, uchar *, long, ulong);
int readnum(ulong, char *, ulong, ulong, int);
int readstr(ulong, char *, ulong, char *);
void ready(Proc *);
void *realloc(void *v, ulong size);
void rebootcmd(int, char **);
void reboot(void *, void *, ulong);
void relocateseg(Segment *, uintptr);
void renameuser(char *, char *);
void resched(char *);
void resrcwait(char *);
int return0(void *);
void rlock(RWLock *);
long rtctime(void);
void runlock(RWLock *);
Proc *runproc(void);
void sched(void);
_Noreturn void schedinit(void);
extern void (*screenputs)(char *, int);
void *secalloc(ulong);
void secfree(void *);
long seconds(void);
uintptr segattach(int, char *, uintptr, uintptr);
void segclock(uintptr);
long segio(Segio *, Segment *, void *, long, vlong, int);
void segpage(Segment *, Page *);
int setcolor(ulong, ulong, ulong, ulong);
void setkernur(Ureg *, Proc *);
int setlabel(Label *);
void setmalloctag(void *, uintptr);
ulong setnoteid(Proc *, ulong);
void setrealloctag(void *, uintptr);
void setregisters(Ureg *, char *, char *, int);
void setupwatchpts(Proc *, Watchpt *, int);
char *skipslash(char *);
void sleep(Rendez *, int (*)(void *), void *);
/*@
  @ requires size > 0;
  @ assigns \nothing;
  @ ensures \valid((char*)\result + (0 .. size-1));
  @ ensures \fresh(\result, size);
  @*/
void *smalloc(ulong size);
void *pebble_meta_alloc(ulong);
void pebble_meta_free(void *);
int splhi(void);
int spllo(void);
void splx(int);
void splxpc(int);
char *srvname(Chan *);
void srvrenameuser(char *, char *);
void shrrenameuser(char *, char *);
int swapcount(uintptr);
int swapfull(void);
void syscallfmt(ulong syscallno, uintptr pc, va_list list);
void sysretfmt(ulong syscallno, va_list list, uintptr ret, uvlong start,
               uvlong stop);
void timeradd(Timer *);
void timerdel(Timer *);
void timersinit(void);
void timerintr(Ureg *, Tval);
void timerset(Tval);
ulong tk2ms(ulong);
uvlong tod2fastticks(vlong);
vlong todget(vlong *, vlong *);
void todsetfreq(vlong);
void todinit(void);
void todset(vlong, vlong, int);
Block *trimblock(Block *, int, int);
/*@
  @ requires \valid(r);
  @ terminates \true;
  @ assigns \nothing;
  @*/
void tsleep(Rendez *r, int (*fn)(void *), void *arg, ulong ms);
/*@
  @ requires \valid(r);
  @ terminates \true;
  @ assigns \nothing;
  @*/
void sleep(Rendez *r, int (*fn)(void *), void *arg);
void twakeup(Ureg *, Timer *);
int uartctl(Uart *, char *);
int uartgetc(void);
void uartkick(void *);
void uartmouse(char *, int (*)(Queue *, int), int);
void uartsetmouseputc(char *, int (*)(Queue *, int));
void uartputc(int);
void uartputs(char *, int);
void uartrecv(Uart *, char);
int uartstageoutput(Uart *);
void unbreak(Proc *);
void uncachepage(Page *);
long unionread(Chan *, void *, long);
/*@ requires \valid(l);
  @ terminates \true;
  @ assigns \nothing;
  @*/
void unlock(Lock *l);
uvlong us2fastticks(uvlong);
void userinit(void);
uintptr userpc(void);
long userwrite(char *, int);
void validaddr(uintptr, ulong, int);
/*@
  @ requires aname != \null;
  @ requires valid_string(aname);
  @ assigns \nothing;
  @ ensures p9_name_ok_slash(aname, slashok);
  @*/
void validname(char *aname, int slashok);
/*@
  @ requires aname != \null;
  @ requires valid_string(aname);
  @ assigns \result \from aname[0..];
  @ ensures \result != \null ==> valid_string(\result);
  @ ensures \result != \null ==> p9_name_ok_slash(\result, slashok);
  @*/
char *validnamedup(char *aname, int slashok);
void validstat(uchar *, int);
void *vmemchr(void *, int, ulong);
Proc *wakeup(Rendez *);
int walk(Chan **, char **, int, int, int *);
void wlock(RWLock *);
void wunlock(RWLock *);
/*@ terminates \true;
    exits \false;
    allocates \result;
    assigns \result \from size;
    ensures \result == \null || \valid((char*)\result + (0..size-1));
*/
void *xalloc(ulong size);
/*@ terminates \true;
    exits \false;
    allocates \result;
    assigns \result \from size;
    ensures \result == \null || \valid((char*)\result + (0..size-1));
*/
void *xalloc_raw(ulong size);
/*@ terminates \true;
    exits \false;
    allocates \result;
    assigns \result \from size;
    ensures \result == \null || \valid((char*)\result + (0..size-1));
*/
void *xallocz(ulong size, int zero);
/*@ terminates \true;
    exits \false;
    allocates \result;
    assigns \result \from size;
    ensures \result == \null || \valid((char*)\result + (0..size-1));
*/
void *xallocz_raw(ulong size, int zero);
/*@ terminates \true;
    allocates \result;
    assigns \result \from size;
    ensures \result == \null || \valid((char*)\result + (0..size-1));
*/
void *xalloc_driver(ulong size);
/*@ terminates \true;
    exits \false;
    allocates \result;
    assigns \result \from size, zero;
    ensures \result == \null || \valid((char*)\result + (0..size-1));
*/
void *xallocz_driver(ulong size, int zero);
/*@ terminates \true;
    exits \false;
    allocates \result;
    assigns \result \from size;
    ensures \result == \null || \valid((char*)\result + (0..size-1));
*/
void *smalloc_driver(ulong size);
/*@ terminates \true;
  @ exits \false;
  @ assigns \nothing;
*/
void xfree_driver(void *p);
/*@ terminates \true;
  @ exits \false;
  @ assigns \nothing;
*/
void xfree(void *p);
void xhole(uintptr, uintptr);
void xinit(void);
int xmerge(void *, void *);
void *xspanalloc(ulong, int, ulong);
void xsummary(void);
void *bootstrap_alloc(ulong size);
void *bootstrap_alloc_aligned(ulong size, ulong alignment);
uintptr get_hhdm_offset(void);
void yield(void);
Page *fillpage(Page *, int);
void zeroprivatepages(void);
Segment *data2txt(Segment *);
Segment *dupseg(Segment **, int, int);
Segment *newseg(int, uintptr, ulong);
Segment *seg(Proc *, uintptr, int);
Segment *txt2data(Segment *);
void hnputv(void *, uvlong);
void hnputl(void *, uint);
void hnputs(void *, ushort);
uvlong nhgetv(void *);
uint nhgetl(void *);
ushort nhgets(void *);
/* Frama-C struggles with the UTF-8 symbol here; provide an ASCII alias. */
ulong us(void);
long lcycles(void);
extern void (*cycles)(uvlong *);
void devmask(Pgrp *, int, char *);
int devallowed(Pgrp *, int);
int canmount(Pgrp *);
/* PCI configuration space access function pointers */
extern int (*pcicfgrw8)(int, int, int, int);
extern int (*pcicfgrw16)(int, int, int, int);
extern int (*pcicfgrw32)(int, int, int, int);
/* Platform-specific address macros - must be provided by arch */
extern void *kaddr(uintptr);
int userureg(Ureg *);
KMap *kmap(Page *);
void kunmap(KMap *);
void setuppagetables(void); /* Setup kernel page tables */
/* Pebble primitives */
void pebbleinit(void);
void pebbleprocinit(Proc *);
void pebble_cleanup(Proc *);
/* Interrupt handling functions */
void intrdisable(int, void (*)(Ureg *, void *), void *, int, char *);
void intrenable(int, void (*)(Ureg *, void *), void *, int, char *);
/* Additional low-level functions */
void idlehands(void);
int tas(ulong *); /* Updated to match architecture-specific declaration */
/* coherence() is declared as function pointer in arch-specific fns.h */
extern void (*coherence)(void);
void SET(void *);
/* addarchfile() is declared in arch-specific fns.h with proper signature */
Dirtab *addarchfile(char *, int, long (*)(Chan *, void *, long, vlong),
                    long (*)(Chan *, void *, long, vlong));
/* I/O port access functions */
ushort ins(int port);              /* Input from I/O port */
void outs(int port, ushort value); /* Output to I/O port */
/* Memory and page ownership functions */
uintptr cankaddr(uintptr); /* Check if address in kernel address space - matches
                              arch signature */
int pageown_acquire(Proc *, uintptr, u64int); /* Acquire page ownership */
int pageown_release(Proc *, uintptr);         /* Release page ownership */
void procfork(Proc *);                        /* Fork process state */
int proc_setup_p9page(Proc *);     /* Setup 9P exchange page (deprecated) */
int proc_setup_p9seg_stub(Proc *); /* Setup stub P9SEG for lazy allocation */
uintptr p9_pick_uaddr(Proc *, const UserCapability *);
void *kernel_setup_init_exchange(
    Proc *); /* Kernel boot: setup #X exchange channel for init */
/* MMU and page table functions */
uintptr *mmuwalk(uintptr *, uintptr, int, int); /* Walk page table */
u64int getcr3(void); /* Get CR3 register (page directory base) */
void putcr3(u64int); /* Set CR3 register (page directory base) */
/* Device registry and PCI framework functions */
void devregistry_init(void);
void pci_framework_init(void);
int pci_framework_enumerate(void);
/* Phase 4b: Benchmarking functions */
void benchmark_init(void);
void benchmark_enable(void);
void benchmark_disable(void);
void benchmark_boot_start(void);
void benchmark_boot_stage(int);
void benchmark_boot_end(void);
void benchmark_print_summary(void);
int validate_all(void);
uvlong rdtsc(void);
/* MMU virtual mapping (architecture-specific but commonly used) */
void *vmap(uvlong, vlong);
void vunmap(void *, vlong);
long kread(int, void *, long);
long kwrite(int, void *, long);
vlong kseek(int, vlong, int);
/* WASM runtime functions */
void wasm_runtime_init(void);
void wasm_arena_test(void);
struct Chan;       /* Forward declaration */
struct M3Function; /* Forward declaration for WASM3 */
int wasm_exec_compile(struct Chan *, struct M3Function **);
void wasm_exec_run(struct M3Function *);
void wasm_runtime_cleanup_process(Proc *);
extern int boot_verbose;
/* Bounded print for formal verification */
/*@ requires \valid_read(fmt);
  @ terminates \true;
  @ assigns \nothing;
  @*/
int bprint(const char *fmt, ...);
/*@ requires \valid_read(fmt);
  @ terminates \false;
  @*/
void bpanic(const char *fmt, ...);
/*
 * Lux9 9P Router
 *
 * Central routing of 9P messages from Exchange Pages to kernel services.
 */
/* Include base types */
/*
 * Kernel Forward Type Declarations
 *
 * Centralized forward declarations for core kernel types.
 * Include this header when you need type names but not full definitions.
 */
/* Plan 9 universal header */
/*
 * 9P Exchange Page Layout - PER-PROCESS VA MODEL
 * ==============================================
 * Each process has ONE 4KB exchange page allocated from the pool and mapped
 * at a per-process virtual address (p->p9uaddr). Ownership flips between
 * process and kernel via borrow checker.
 *
 * Layout:
 *   0x000 - 0xEFF: Message area (3840 bytes) - request OR reply
 *   0xF00 - 0xFFF: Control block (256 bytes)
 */
/* Exchange page ring layout for small messages */
/* Legacy aliases (for transition) */
/* Legacy fixed user VA (deprecated). */
uintptr p9_user_base(Proc *p);
/*
 * Lux9 Kernel Atomics Shim
 * Wraps GCC/Clang built-ins for safe memory ordering.
 */
/*
 * Memory Order Definitions
 * (Matching C11 standard for documentation purposes)
 */
/* Frama-C doesn't support GCC atomic builtins; provide functional shims */
/*
 * Atomic Load
 * usage: int val = atomic_load(&var, ORDER_ACQUIRE);
 */
/*
 * Atomic Store
 * usage: atomic_store(&var, val, ORDER_RELEASE);
 */
/*
 * Atomic Exchange (useful for locks)
 */
/*
 * Compare and Swap
 * usage: if (atomic_cas(&var, &expected, desired)) ...
 */
/*
 * Memory Fences
 */
/* Control Block (at offset 0xF00) */
typedef struct P9Control {
  uint doorbell; /* Access via atomic_load/store */
  uint status;   /* Access via atomic_load/store */
  volatile uint req_head;
  volatile uint req_tail;
  volatile uint rep_head;
  volatile uint rep_tail;
  volatile uint req_seq;
  volatile uint rep_seq;
  uchar session_pebble[64];
  uchar reserved[160];
} P9Control;
/* Status codes */
/* Pebble Token (embedded in Tattach data) */
typedef struct PebbleToken {
  uvlong ledger_id;
  uvlong expires;
  uchar permissions;
  uchar reserved[7];
  uchar signature[16];
} PebbleToken;
/* Pebble permission flags */
/* Holographic Channels (Bits 4-6) - Visibility Masks */
/* Custom 9P message types for Lux9 - defined in fcall.h enum */
/* Texec = 128, Rexec = 129 */
/* Router API */
void p9_router_init(void);
int p9_alloc_page(Proc *p);
void p9_free_page(Proc *p);
int p9_handle_doorbell(Proc *p, Ureg *ureg);
int p9_route(Proc *p, Fcall *t, Fcall *r);
int p9_dispatch(Proc *p, Fcall *t, Fcall *r);
int p9_handle_ring(Proc *p, P9Control *ctl, uchar *msg_buf);
/* Pebble validation */
int p9_extract_pebble(uchar *data, ulong len, PebbleToken *out);
int p9_validate_pebble(PebbleToken *tok, char *path, int access);
/* 9P Service handlers */
int proc_9p_handle(Proc *caller, Fcall *t, Fcall *r);
int dev_9p_handle(Proc *caller, Fcall *t, Fcall *r);
int env_9p_handle(Proc *caller, Fcall *t, Fcall *r);
int srv_9p_handle(Proc *caller, Fcall *t, Fcall *r);
int mnt_9p_handle(Proc *caller, Fcall *t, Fcall *r);
/* /srv registry helpers */
void srv_init(void);
int srv_post_fd(Proc *caller, const char *name, int fd);
int srv_create_entry(Proc *caller, const char *name);
int srv_remove_entry(Proc *caller, const char *name);
int srv_get_by_index(int index, char *name, int namelen);
int srv_index_of(const char *name);
int srv_get_by_index_for_proc(Proc *caller, int index, char *name, int namelen);
int srv_index_of_for_proc(Proc *caller, const char *name);
Chan *srv_clone_chan(const char *name);
/*
 * Async 9P Operations (Phase 3)
 */
/* Completion callback type */
typedef void (*P9CompletionCallback)(Fcall *reply, void *arg, int status);
/* Async operation tracking */
typedef struct AsyncP9Op {
  uint op_id;                    /* MSGORD message ID */
  Fcall request;                 /* Original request (copied) */
  Fcall reply;                   /* Reply when ready */
  P9CompletionCallback callback; /* Completion callback */
  void *callback_arg;            /* Callback argument */
  uvlong submit_time;            /* When submitted */
  int status;                    /* P9_STATUS_* */
  struct AsyncP9Op *next;        /* Linked list for per-process tracking */
} AsyncP9Op;
/* Async completion status codes */
/* Async Router API */
int p9_handle_doorbell_async(Proc *p);
uint p9_submit_async(Proc *p, Fcall *t, char *path, P9CompletionCallback cb,
                     void *arg);
int p9_check_async(Proc *p, uint op_id, Fcall *reply_out);
void p9_cancel_async(Proc *p, uint op_id);
/* Fire all ready async completions for a process */
int p9_fire_completions(Proc *p);
/*
 * Red-Black Tree Implementation
 *
 * Intrusive RB-tree based on Linux kernel design.
 * Provides O(log n) insert, delete, and search operations.
 *
 * Usage:
 *   struct my_node {
 *       struct rb_node rb;
 *       int key;
 *       // ... other data
 *   };
 *
 *   struct rb_root mytree = RB_ROOT;
 *
 *   // Insert: caller provides comparison and linking
 *   // Search: caller walks tree with comparison
 *   // Delete: rb_erase(&node->rb, &mytree)
 */
/* Plan 9 universal header */
/* wasm_9p_integration.h - 9P Protocol + Capability Integration for WASM
 *
 * Integrates Plan 9's 9P protocol with the capability system for WASM servers.
 * WASM modules implement 9P file servers, and capabilities are validated on
 * each 9P operation.
 *
 * Architecture:
 *   1. Client sends 9P message with embedded capability UUID
 *   2. Kernel validates UUID → unpacks Pebble (token, gen, index)
 *   3. Kernel checks capability permissions
 *   4. Kernel routes valid message to WASM server
 *   5. WASM server processes request, returns 9P response
 *
 * UUID Encoding in 9P:
 *   - Tattach: aname field or initial auth data contains UUID
 *   - Topen/Tcreate: Special "capid=" prefix in file name
 *   - Control: Write UUID to /mnt/ctl before operations
 */
/* lux_capability.h - Capability-Based Security for Lux9 Runtime
 *
 * Implements monotonic capability derivation with per-module and per-class
 * granularity. Properties proven in proofs/capability/*.v:
 *   - PermsBitmask.v: Permission bitmask operations
 *   - CapabilityModel.v: Core capability structure and derivation
 *   - DerivationChain.v: Transitive chain properties
 *   - LedgerInvariants.v: Uniqueness and preservation invariants
 *
 * Security model: Capabilities can only be derived with FEWER permissions
 * (monotonic decrease). This is enforced at derivation time and proven correct
 * in the Coq formalization.
 */
/*
 * Kernel Forward Type Declarations
 *
 * Centralized forward declarations for core kernel types.
 * Include this header when you need type names but not full definitions.
 */
/* Plan 9 types if not in kernel context */
/* ========== Permission Bits (matching proofs/capability/PermsBitmask.v)
 * ========== */
/*
 * @coq_proof: proofs/capability/PermsBitmask.v
 * @definition: perm_read, perm_write, perm_exec, perm_transfer, perm_grant
 *
 * Permission bits form a lattice under subset ordering.
 * @theorem: perms_subset_refl, perms_subset_trans
 *
 * In kernel mode, we use the enum values from blind_ledger.h.
 * In userspace test mode, we define our own macros.
 */
/* Kernel mode: use values from blind_ledger.h (included via pebble.h) */
/*
 * Blind Ledger - Zero-Knowledge Addressing System
 *
 * Implements the "Identity is Security" paradigm.
 *
 * - UserCapability: The opaque handle held by userspace.
 * - BlindLedgerEntry: The kernel's secret mapping table.
 *
 * The "Tension Resolution":
 * - We do NOT track every 8-byte Peg with a hash.
 * - We track "Spans" (Ranges of Pegs) created during Black Token allocation.
 * - White Tokens are fungible budget; Black Tokens have Identity.
 *
 * Circular Economy:
 * - Banked tokens (Free Pegs) are Colorless.
 * - Issued Tokens (White) are Fungible Budget.
 * - Used Tokens (Black) are Reified Identity (Ledger Entry).
 * - Freed Tokens return to the Bank as Colorless Pegs.
 */
/* Mapping blind_ledger names to LUX_CAP names for consistency */
/*@ axiomatic LuxCapPerms {
  @ logic integer LUX_CAP_PERM_ALL;
  @ logic integer lux_cap_perms_subset_logic(u32int a, u32int b);
  @} */
/* ========== Capability Scope ========== */
typedef enum {
  LUX_CAP_SCOPE_MODULE, /* Assembly-level capability (root for assembly) */
  LUX_CAP_SCOPE_CLASS,  /* Class-level capability (derived from module) */
  LUX_CAP_SCOPE_METHOD  /* Method-level capability (derived from class) */
} lux_capability_scope_t;
typedef struct {
  fruity_module_t *fruity_module;
  fruity_function_t *fruity_function;
  fruity_basic_block_t *fruity_block;
  fruity_instruction_t *fruity_instruction;
  void *raw;
} lux_cap_aux_t;
/* ========== Monotonic Capability Structure ========== */
/*
 * @coq_proof: proofs/capability/CapabilityModel.v
 * @record: Capability (cap_id, cap_perms, cap_parent)
 *
 * ACSL contracts derived from:
 *   @theorem: derived_perm_monotonic
 *     derived_from ct c p -> perms_subset (cap_perms c) (cap_perms p)
 */
typedef struct lux_capability {
  /* Identity */
  uuid_t uuid;   /* Unique capability identifier (UUIDv8) */
  u32int cap_id; /* Numeric ID for fast lookup */
  /* Derivation chain */
  uuid_t parent_uuid;      /* Parent capability UUID (null if root) */
  u32int parent_id;        /* Parent numeric ID (0 if root) */
  u32int derivation_depth; /* Chain depth (0 = root, increases on derive) */
  /* Permissions (monotonic: can only decrease) */
  u32int permissions;     /* Current active permissions */
  u32int max_permissions; /* Original immutable max (set at creation) */
  /* Timing (monotonic counters) */
  u64int creation_time;   /* Monotonic counter at creation */
  u64int expiration_time; /* Optional expiration (0 = no expiry) */
  /* Scope and metadata */
  lux_capability_scope_t scope; /* MODULE, CLASS, or METHOD */
  char *bound_metadata;         /* Immutable binding: "assembly:class:method" */
  u8int is_validated;           /* Has chain been validated? */
  u8int is_revoked;             /* Revocation flag */
  lux_cap_aux_t aux;            /* Auxiliary data (e.g., Fruity IR object) */
} lux_capability_t;
/* ========== Capability Manager ========== */
/*
 * @coq_proof: proofs/capability/LedgerInvariants.v
 * @definition: cap_table
 * @theorem: find_cap_unique
 *
 * The manager maintains a ledger of all capabilities with invariants:
 *   - All cap_id values are unique
 *   - Parent references point to existing capabilities
 *   - Permission chains are monotonically decreasing
 */
typedef struct lux_capability_manager {
  /* Capability storage */
  lux_capability_t *capabilities; /* Dynamic array */
  u32int count;                   /* Number of capabilities */
  u32int capacity;                /* Allocated slots */
  /* Monotonic counters */
  u64int monotonic_time; /* Global monotonic timestamp */
  u32int next_cap_id;    /* Next capability ID to assign */
  /* Statistics */
  u32int derivations; /* Total derivations performed */
  u32int validations; /* Total chain validations */
  u32int rejections;  /* Failed derivations (permission violation) */
} lux_capability_manager_t;
/* ========== Core API ========== */
/*@
  @ requires \true;
  @ assigns \result \from \nothing;
  @ ensures \result == \null || \result->count == 0;
  @*/
lux_capability_manager_t *lux_cap_manager_create(void);
/*@
  @ requires manager != \null;
  @ assigns manager->capabilities, manager->count \from manager->capabilities,
  @         manager->count;
  @ ensures \true;
  @*/
void lux_cap_manager_destroy(lux_capability_manager_t *manager);
/*@
  @ requires manager != \null;
  @ requires assembly_name != \null && \valid_read(assembly_name);
  @ terminates \true;
  @ exits \false;
  @ assigns manager->capabilities, manager->count \from manager->capabilities,
  @         manager->count, assembly_name;
  @ ensures \result != \null ==> (
  @   \result->scope == LUX_CAP_SCOPE_MODULE &&
  @   \result->parent_id == 0 &&
  @   \result->permissions == LUX_CAP_PERM_ALL
  @ );
  @*/
lux_capability_t *lux_cap_create_module(lux_capability_manager_t *manager,
                                        const char *assembly_name);
/*@
  @ requires manager != \null;
  @ requires parent != \null && \valid(parent);
  @ requires class_name != \null && \valid_read(class_name);
  @ requires (permission_mask & parent->permissions) == permission_mask;
  @ terminates \true;
  @ exits \false;
  @ assigns manager->capabilities, manager->count \from manager->capabilities,
  @         manager->count, parent, class_name, permission_mask;
  @ ensures \result != \null ==> (
  @   \result->scope == LUX_CAP_SCOPE_CLASS &&
  @   \result->parent_id == parent->cap_id &&
  @   \result->permissions == permission_mask &&
  @   \result->derivation_depth == parent->derivation_depth + 1
  @ );
  @*/
lux_capability_t *lux_cap_derive_class(lux_capability_manager_t *manager,
                                       lux_capability_t *parent,
                                       const char *class_name,
                                       u32int permission_mask);
/*@
  @ requires manager != \null;
  @ requires child != \null && \valid(child);
  @ terminates \true;
  @ exits \false;
  @ assigns \result \from manager->capabilities[0..manager->count-1], child;
  @ ensures \result == 1 ==> (
  @   \forall integer i; 0 <= i < child->derivation_depth ==>
  @     lux_cap_perms_subset_logic(child->permissions,
  @       manager->capabilities[i].permissions) != 0
  @ );
  @*/
int lux_cap_validate_chain(lux_capability_manager_t *manager,
                           lux_capability_t *child);
/*@
  @ requires cap != \null && \valid(cap);
  @ terminates \true;
  @ exits \false;
  @ assigns \result \from cap->permissions, required;
  @ ensures \result == ((cap->permissions & required) == required);
  @*/
int lux_cap_check_permission(lux_capability_t *cap, u32int required);
/*@
  @ requires manager != \null && \valid(manager);
  @ requires uuid != \null && \valid_read(uuid);
  @ assigns \result \from manager->capabilities[0..manager->count-1], uuid;
  @ ensures \result != \null ==> (
  @   \result->uuid.data[0] == uuid->data[0] &&
  @   \result->uuid.data[15] == uuid->data[15]
  @ );
  @*/
lux_capability_t *lux_cap_find_by_uuid(lux_capability_manager_t *manager,
                                       const uuid_t *uuid);
/*
 * Find capability by numeric ID.
 *
 * @requires: manager != NULL
 * @ensures: \result != NULL ==> \result->cap_id == cap_id
 */
lux_capability_t *lux_cap_find_by_id(lux_capability_manager_t *manager,
                                     u32int cap_id);
/* ========== Permission Utilities ========== */
/*@
  @ terminates \true;
  @ exits \false;
  @ assigns \nothing;
  @ ensures \result == lux_cap_perms_subset_logic(a, b);
  @*/
int lux_cap_perms_subset(u32int a, u32int b);
/*
 * Format permissions as human-readable string.
 * @ensures: writes at most buflen-1 chars to buf
 */
void lux_cap_perms_to_string(u32int perms, char *buf, u32int buflen);
/* ========== Debug/Diagnostics ========== */
/*
 * Dump capability details to kernel console.
 */
void lux_cap_dump(lux_capability_t *cap);
/*
 * Dump manager statistics to kernel console.
 */
void lux_cap_manager_dump_stats(lux_capability_manager_t *manager);
/*
 * Blind Ledger - Zero-Knowledge Addressing System
 *
 * Implements the "Identity is Security" paradigm.
 *
 * - UserCapability: The opaque handle held by userspace.
 * - BlindLedgerEntry: The kernel's secret mapping table.
 *
 * The "Tension Resolution":
 * - We do NOT track every 8-byte Peg with a hash.
 * - We track "Spans" (Ranges of Pegs) created during Black Token allocation.
 * - White Tokens are fungible budget; Black Tokens have Identity.
 *
 * Circular Economy:
 * - Banked tokens (Free Pegs) are Colorless.
 * - Issued Tokens (White) are Fungible Budget.
 * - Used Tokens (Black) are Reified Identity (Ledger Entry).
 * - Freed Tokens return to the Bank as Colorless Pegs.
 */
/* Forward declarations for 9P types (from Plan 9 headers) */
typedef struct Fcall Fcall;
typedef struct Chan Chan;
/* ========== 9P Capability Message Format ========== */
/*
 * Capabilities are embedded in 9P messages using a standard encoding:
 *
 * Option 1: In aname field (Tattach)
 *   Format: "capid=<uuid-hex>" or just the 16-byte UUID binary
 *
 * Option 2: In file paths (Twalk, Topen)
 *   Format: "/mnt/fs#<uuid-hex>/path/to/file"
 *
 * Option 3: Control file (Write to /mnt/ctl)
 *   Format: Write "cap <uuid-hex>" to set capability for session
 *
 * We use Option 1 for simplicity: UUID in aname during Tattach
 */
/* Maximum UUID string length: "capid=" + 32 hex chars + null */
/* ========== 9P Session with Capability ========== */
/*
 * Each 9P session (attach) is associated with a validated capability.
 * The session carries the Pebble capability that grants access to resources.
 */
/*
 * Red-Black Tree Implementation
 *
 * Intrusive RB-tree based on Linux kernel design.
 * Provides O(log n) insert, delete, and search operations.
 *
 * Usage:
 *   struct my_node {
 *       struct rb_node rb;
 *       int key;
 *       // ... other data
 *   };
 *
 *   struct rb_root mytree = RB_ROOT;
 *
 *   // Insert: caller provides comparison and linking
 *   // Search: caller walks tree with comparison
 *   // Delete: rb_erase(&node->rb, &mytree)
 */
/* ========== 9P Session with Capability ========== */
/*
 * Each 9P session (attach) is associated with a validated capability.
 * The session carries the Pebble capability that grants access to resources.
 */
typedef struct wasm_9p_session {
  struct rb_node rb;         /* RB-Tree node for registry */
  uuid_t cap_uuid;           /* Capability UUID from attach */
  UserCapability pebble_cap; /* Validated Pebble capability */
  u32int permissions;        /* Effective permissions */
  u32int session_id;         /* Unique session identifier */
  int ref;                   /* Reference count */
  char *name;                /* Optional session name (e.g. "echo") */
  void *wasm_server;         /* WASM server handling this session */
} wasm_9p_session_t;
/* ========== 9P Message Validation ========== */
/*
 * Extract capability UUID from 9P Tattach message.
 * Looks for "capid=<uuid>" in aname field or binary UUID.
 *
 * @param aname: The aname field from Tattach
 * @param uuid_out: Output UUID
 * @returns: 0 on success, -1 on error
 */
int wasm_9p_extract_cap_uuid(const char *aname, uuid_t *uuid_out);
/*
 * Validate capability for 9P operation.
 * Unpacks UUID to Pebble (token, gen, index), validates against ledger.
 *
 * @param uuid: Capability UUID from message
 * @param required_perms: Required permissions for operation (CAP_PERM_READ,
 * etc.)
 * @param pebble_cap_out: Output validated Pebble capability
 * @returns: 1 if valid, 0 if invalid/insufficient permissions
 */
int wasm_9p_validate_capability(const uuid_t *uuid, u32int required_perms,
                                UserCapability *pebble_cap_out);
/*
 * Create 9P session with validated capability.
 *
 * @param uuid: Capability UUID
 * @param wasm_server: WASM server instance
 * @returns: Session pointer, or NULL on error
 */
wasm_9p_session_t *wasm_9p_create_session(const uuid_t *uuid,
                                          void *wasm_server);
/*
 * Destroy 9P session and clean up resources.
 *
 * @param session: Session to destroy
 */
void wasm_9p_destroy_session(wasm_9p_session_t *session);
/* ========== 9P Operation Permission Requirements ========== */
/*
 * Map 9P operations to required capability permissions.
 * These are checked on every operation before routing to WASM.
 */
typedef enum {
  WASM_9P_OP_ATTACH, /* Requires: CAP_PERM_READ (base access) */
  WASM_9P_OP_WALK,   /* Requires: CAP_PERM_READ */
  WASM_9P_OP_OPEN,   /* Requires: CAP_PERM_READ or WRITE depending on mode */
  WASM_9P_OP_CREATE, /* Requires: CAP_PERM_WRITE */
  WASM_9P_OP_READ,   /* Requires: CAP_PERM_READ */
  WASM_9P_OP_WRITE,  /* Requires: CAP_PERM_WRITE */
  WASM_9P_OP_CLUNK,  /* Requires: (none, cleanup) */
  WASM_9P_OP_REMOVE, /* Requires: CAP_PERM_WRITE */
  WASM_9P_OP_STAT,   /* Requires: CAP_PERM_READ */
  WASM_9P_OP_WSTAT,  /* Requires: CAP_PERM_WRITE */
} wasm_9p_operation_t;
/*
 * Get required permissions for a 9P operation.
 *
 * @param op: 9P operation type
 * @param mode: Open mode (for OPEN operation, e.g., OREAD, OWRITE)
 * @returns: Required permission bitmask
 */
u32int wasm_9p_required_perms(wasm_9p_operation_t op, u32int mode);
/* MAX_WASM_SESSIONS removed - using dynamic RB-Tree */
wasm_9p_session_t *wasm_9p_get_session(u32int session_id);
void wasm_9p_session_ref(u32int session_id);
void wasm_9p_session_unref(u32int session_id);
/* ========== 9P → WASM Routing ========== */
/*
 * Route validated 9P message to WASM server.
 * This is the main entry point: validates capability, then dispatches to WASM.
 *
 * @param t: 9P request
 * @param r: 9P reply
 * @param session: Active session with capability
 * @returns: 0 on success, error code otherwise
 */
int wasm_9p_route_to_wasm(Fcall *t, Fcall *r, wasm_9p_session_t *session);
/* Exposed for wasm_dev_srv.c */
extern struct rb_root session_tree;
extern Lock session_lock;
/* Register a named alias for a session */
int wasm_router_register(u32int session_id, char *name);
enum {
  Qdir = 0,
};
typedef struct WasmSrvChanState {
  wasm_9p_session_t *session;
  uchar *resp_buf;
  uint resp_len;
} WasmSrvChanState;
static WasmSrvChanState *wasm_srv_state(Chan *c) {
  return (WasmSrvChanState *)c->aux;
}
static long wasm_9p_chan_read(Chan *c, void *va, long n, vlong offset);
static void wasmsrvinit(void) {
  /* Integration init is handled elsewhere or lazy */
}
static Chan *wasmsrvattach(char *spec) { return devattach('W', spec); }
/*
 * Iterator for RB-tree to find nth named session
 * Returns: session pointer or nil
 */
static wasm_9p_session_t *wasm_get_nth_named_session(int n) {
  /* This is O(N) linear scan of the tree.
     For a high-performance registry, we would maintain a secondary list or
     index. For <1000 sessions, this is acceptable. */
  extern struct rb_root session_tree; /* Access global tree */
  extern Lock session_lock;
  lock(&session_lock);
  struct rb_node *node = rb_first(&session_tree);
  int count = 0;
  while (node) {
    wasm_9p_session_t *s =
        ((wasm_9p_session_t *)((char *)(node) -
                               (uintptr)(&((wasm_9p_session_t *)0)->rb)));
    if (s->name != ((void *)0)) {
      if (count == n) {
        unlock(&session_lock);
        return s;
      }
      count++;
    }
    node = rb_next(node);
  }
  unlock(&session_lock);
  return ((void *)0);
}
static wasm_9p_session_t *wasm_find_session_by_name(char *name) {
  extern struct rb_root session_tree;
  extern Lock session_lock;
  lock(&session_lock);
  struct rb_node *node = rb_first(&session_tree);
  while (node) {
    wasm_9p_session_t *s =
        ((wasm_9p_session_t *)((char *)(node) -
                               (uintptr)(&((wasm_9p_session_t *)0)->rb)));
    if (s->name != ((void *)0) && strcmp(s->name, name) == 0) {
      unlock(&session_lock);
      return s;
    }
    node = rb_next(node);
  }
  unlock(&session_lock);
  return ((void *)0);
}
static int wasmgen(Chan *c, char *name, Dirtab *, int, int s, Dir *dp) {
  Qid qid;
  if (s == -1) {
    mkqid(&qid, Qdir, 0, 0x80);
    devdir(c, qid, "#W", 0, eve, 0555, dp);
    return 1;
  }
  if (c->qid.path != Qdir) {
    /* If path is not root, we are inside a session file (or directory if we
     * supported hierarchy) */
    /* But we only expose files for sessions to attach to */
    return -1;
  }
  if (name != ((void *)0)) {
    /* Lookup by name */
    wasm_9p_session_t *session = wasm_find_session_by_name(name);
    if (!session)
      return -1;
    /* Qid path = session_id (which is > 0) */
    mkqid(&qid, session->session_id, 0, 0x00);
    devdir(c, qid, session->name, 0, eve, 0666, dp);
    return 1;
  }
  /* List by index */
  wasm_9p_session_t *session = wasm_get_nth_named_session(s);
  if (!session)
    return -1;
  mkqid(&qid, session->session_id, 0, 0x00);
  devdir(c, qid, session->name, 0, eve, 0666, dp);
  return 1;
}
static Walkqid *wasmsrvwalk(Chan *c, Chan *nc, char **name, int nname) {
  return devwalk(c, nc, name, nname, ((void *)0), 0, wasmgen);
}
static int wasmsrvstat(Chan *c, uchar *db, int n) {
  return devstat(c, db, n, ((void *)0), 0, wasmgen);
}
static Chan *wasmsrvopen(Chan *c, int omode) {
  /* If opening root, standard directory open */
  if (c->qid.path == Qdir) {
    if ((omode & 7) != 0)
      lux9_error(Eperm);
  } else {
    u32int session_id = (u32int)c->qid.path;
    wasm_9p_session_t *session = wasm_9p_get_session(session_id);
    if (!session)
      lux9_error(Eio);
    WasmSrvChanState *state = xallocz(sizeof(*state), 1);
    if (!state)
      lux9_error(Eio);
    state->resp_buf = xallocz(0xF00, 0);
    if (!state->resp_buf) {
      xfree(state);
      lux9_error(Eio);
    }
    state->session = session;
    wasm_9p_session_ref(session_id);
    c->aux = state;
  }
  c->mode = openmode(omode);
  c->flag |= COPEN;
  c->offset = 0;
  return c;
}
static void wasmsrvclose(Chan *c) {
  WasmSrvChanState *state = wasm_srv_state(c);
  if (!state)
    return;
  if (state->session)
    wasm_9p_session_unref(state->session->session_id);
  if (state->resp_buf)
    xfree(state->resp_buf);
  xfree(state);
  c->aux = ((void *)0);
}
static long wasmsrvread(Chan *c, void *va, long n, vlong offset) {
  if (c->qid.path == Qdir)
    return devdirread(c, va, n, ((void *)0), 0, wasmgen);
  return wasm_9p_chan_read(c, va, n, offset);
}
static long wasmsrvwrite(Chan *c, void *va, long n, vlong offset) {
  if (c->qid.path == Qdir)
    lux9_error(Eperm);
  WasmSrvChanState *state = wasm_srv_state(c);
  if (!state || !state->session || !state->resp_buf)
    lux9_error(Eio);
  if (offset != 0)
    lux9_error(Eio);
  if (n <= 0)
    return 0;
  if (n > 0xF00)
    lux9_error(Eio);
  uchar buf[0xF00];
  Fcall t;
  Fcall r;
  memmove(buf, va, n);
  t = (Fcall){0};
  r = (Fcall){0};
  if (convM2S(buf, n, &t) == 0)
    lux9_error(Eio);
  if (wasm_9p_route_to_wasm(&t, &r, state->session) < 0 && r.type != Rerror) {
    r.type = Rerror;
    r.ename = "wasm 9p route failed";
  }
  uint rep_size = convS2M(&r, state->resp_buf, 0xF00);
  if (rep_size == 0)
    lux9_error(Eio);
  state->resp_len = rep_size;
  return n;
}
/*
 * Attach to a WASM session (via mount)
 * When a user mounts /srv/wasm/my_server /n/local
 * The kernel calls attach on the device with the spec.
 * But here we are PROVIDING the /srv/wasm.
 * The standard 'mount' involves opening the file descriptor to the server and
 * pushing it. But since WASM is in-kernel, we can support direct attach via
 * something else?
 *
 * Plan 9 approach:
 * fd = open("/srv/wasm/mysrv", ORDWR);
 * mount(fd, "/n/local", MREPL, ...);
 *
 * When 'mount' writes to the file descriptor (if it's a pipe) or calls Tattach.
 * But devwasm exposes files. If we open one, we get a Chan.
 * If we treat this Chan as a 9P connection, we need to handle 9P messages on
 * read/write.
 *
 * IMPLEMENTATION CHOICE:
 * When we open "/srv/wasm/mysrv", we return a Chan that is "connected" to the
 * WASM server. Reads/Writes on this Chan should be 9P messages handled by the
 * WASM server.
 */
static long wasm_9p_chan_read(Chan *c, void *va, long n, vlong offset) {
  if (c->qid.path == Qdir)
    return devdirread(c, va, n, ((void *)0), 0, wasmgen);
  WasmSrvChanState *state = wasm_srv_state(c);
  if (!state || !state->resp_buf)
    lux9_error(Eio);
  if (n <= 0 || offset < 0)
    return 0;
  if ((ulong)offset >= state->resp_len)
    return 0;
  ulong avail = state->resp_len - (ulong)offset;
  if ((ulong)n > avail)
    n = avail;
  memmove(va, state->resp_buf + offset, n);
  if ((ulong)offset + n >= state->resp_len)
    state->resp_len = 0;
  return n;
}
/* Re-using srvread/write for now which are empty for files */
/* Dev wasmsrvdevtab moved to wasm_dev_srv.c
Dev wasmsrvdevtab = {
    'W',
    "wasm",
    devreset,
};
*/
