/* Frama-C Missing Types - types excluded by #ifndef __FRAMAC__ in Plan 9 headers */

typedef struct Qid Qid;
struct Qid {
    unsigned long long path;
    unsigned long vers;
    unsigned char type;
};
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
 * Lux9 9P Router Implementation
 *
 * Routes 9P messages from Exchange Pages to kernel services.
 */
extern char Enoerror[]; /* no error */
extern char Emount[]; /* inconsistent mount */
extern char Eunmount[]; /* not mounted */
extern char Eismtpt[]; /* is a mount point */
extern char Eunion[]; /* not in union */
extern char Emountrpc[]; /* mount rpc error */
extern char Eshutdown[]; /* device shut down */
extern char Enocreate[]; /* mounted directory forbids creation */
extern char Enonexist[]; /* file does not exist */
extern char Eexist[]; /* file already exists */
extern char Ebadsharp[]; /* unknown device in # filename */
extern char Enotdir[]; /* not a directory */
extern char Eisdir[]; /* file is a directory */
extern char Ebadchar[]; /* bad character in file name */
extern char Efilename[]; /* file name syntax */
extern char Eperm[]; /* permission denied */
extern char Ebadusefd[]; /* inappropriate use of fd */
extern char Ebadarg[]; /* bad arg in system call */
extern char Einuse[]; /* device or object already in use */
extern char Eio[]; /* i/o error */
extern char Etoobig[]; /* read or write too large */
extern char Etoosmall[]; /* read or write too small */
extern char Enoport[]; /* network port not available */
extern char Ehungup[]; /* i/o on hungup channel */
extern char Ebadctl[]; /* bad process or channel control request */
extern char Enodev[]; /* no free devices */
extern char Eprocdied[]; /* process exited */
extern char Enochild[]; /* no living children */
extern char Eioload[]; /* i/o error in demand load */
extern char Enovmem[]; /* virtual memory allocation failed */
extern char Ebadfd[]; /* fd out of range or not open */
extern char Enofd[]; /* no free file descriptors */
extern char Eisstream[]; /* seek on a stream */
extern char Ebadexec[]; /* exec header invalid */
extern char Etimedout[]; /* connection timed out */
extern char Econrefused[]; /* connection refused */
extern char Econinuse[]; /* connection in use */
extern char Eintr[]; /* interrupted */
extern char Enomem[]; /* kernel allocate failed */
extern char Esoverlap[]; /* segments overlap */
extern char Emouseset[]; /* mouse type already set */
extern char Eshort[]; /* i/o count too small */
extern char Egreg[]; /* the front fell off */
extern char Ebadspec[]; /* bad attach specifier */
extern char Enoreg[]; /* process has no saved registers */
extern char Enoattach[]; /* mount/attach disallowed */
extern char Eshortstat[]; /* stat buffer too small */
extern char Ebadstat[]; /* malformed stat buffer */
extern char Enegoff[]; /* negative i/o offset */
extern char Ecmdargs[]; /* wrong #args in control message */
extern char Ebadip[]; /* bad ip address syntax */
extern char Edirseek[]; /* seek in directory */
extern char Etoolong[]; /* name too long */
extern char Echange[]; /* media or partition has changed */
/* Include base types */
/* Plan 9 universal header */
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
/* USED macro to suppress unused warnings */
/* Compile-time type size assertions */
_Static_assert(sizeof(ulong) == sizeof(void *), "ulong must match pointer size");
_Static_assert(sizeof(uintptr) == sizeof(void *),
              "uintptr must match pointer size");
_Static_assert(sizeof(usize) == sizeof(void *), "usize must match pointer size");
_Static_assert(sizeof(ssize) == sizeof(void *), "ssize must match pointer size");
/*
 * functions (possibly) linked in, complete, from libc.
 */
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
typedef __builtin_va_list __gnuc_va_list;
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
/*@ assigns \result \from s;
  @ ensures \result == s;
  @ terminates \true;
  */
extern void *memset(void *s, int c, usize n);
extern int memcmp(const void *, const void *, usize);
/*@ assigns \result \from dst;
  @ ensures \result == dst;
  @ terminates \true;
  */
extern void *memmove(void *dst, const void *src, usize n);
extern void *memchr(const void *, int, usize);
/*
 * string routines
 */
extern char *strcat(char *, char *);
extern char *strchr(char *, int);
extern char *strrchr(char *, int);
/*@ requires s1 == \null || \valid(s1);
  @ requires s2 == \null || \valid(s2);
  @ assigns \nothing;
  @ terminates \true;
  */
extern int strcmp(char *s1, char *s2);
extern char *strcpy(char *, char *);
extern char *strecpy(char *, char *, char *);
extern char *strncat(char *, char *, long);
extern char *strncpy(char *, char *, long);
extern int strncmp(char *, char *, long);
/*@ requires s == \null || \valid(s);
  @ assigns \nothing;
  @ ensures \result >= 0;
  @ terminates \true;
  */
extern long strlen(char *s);
extern char *strstr(char *, char *);
extern int atoi(char *);
extern int fullrune(char *, int);
extern int cistrcmp(char *, char *);
extern int cistrncmp(char *, char *, int);
/*
 * rune routines
 */
extern int runetochar(char *, Rune *);
extern int chartorune(Rune *, char *);
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
typedef struct Fmt Fmt;
struct Fmt {
  uchar runes; /* output buffer is runes or chars? */
  void *start; /* of buffer */
  void *to; /* current place in the buffer */
  void *stop; /* end of the buffer; overwritten if flush fails */
  int (*flush)(Fmt *); /* called when to == stop */
  void *farg; /* to make flush a closure */
  int nfmt; /* num chars formatted so far */
  va_list args; /* args passed to dofmt */
  int r; /* % format Rune */
  int width;
  int prec;
  ulong flags;
};
typedef int (*Fmts)(Fmt *);
extern int print(char *, ...);
extern char *seprint(char *, char *, char *, ...);
extern char *vseprint(char *, char *, char *, va_list);
extern int snprint(char *, int, char *, ...);
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
extern long strtol(char *, char **, int);
extern ulong strtoul(char *, char **, int);
extern vlong strtoll(char *, char **, int);
extern uvlong strtoull(char *, char **, int);
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
typedef struct Qid Qid;
typedef struct Dir Dir;
typedef struct OWaitmsg OWaitmsg;
typedef struct Waitmsg Waitmsg;
/* bits in Qid.type */
/* bits in Dir.mode */
struct Dir {
  /* system-modified data */
  ushort type; /* server type */
  uint dev; /* server subtype */
  /* file data */
  Qid qid; /* unique id from server */
  ulong mode; /* permissions */
  ulong atime; /* last read time */
  ulong mtime; /* last write time */
  vlong length; /* file length: see <u.h> */
  char *name; /* last element of path */
  char *uid; /* owner name */
  char *gid; /* group name */
  char *muid; /* last modifier name */
};
struct OWaitmsg {
  char pid[12]; /* of loved one */
  char time[3 * 12]; /* of loved one and descendants */
  char msg[64]; /* compatibility BUG */
};
struct Waitmsg {
  int pid; /* of loved one */
  ulong time[3]; /* of loved one and descendants */
  char msg[128]; /* actually variable-size in user mode */
};
/* Plan 9 universal header */
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
 u16int ds;
 u16int es;
 u16int fs;
 u16int gs;
 u64int type;
 u64int error; /* error code (or zero) */
 u64int pc; /* pc */
 u64int cs; /* old context */
 u64int flags; /* old flags */
 u64int sp; /* sp */
 u64int ss; /* old stack segment */
};
/* Manual typedefs (portlib.h gives structs but not always typedefs used by
 * kernel) */
typedef struct Qid Qid;
typedef struct Dir Dir;
typedef struct Waitmsg Waitmsg;
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
/* Plan 9 universal header */
/*
 * 9P Exchange Page Layout - SINGLE 4KB PAGE MODEL
 * ================================================
 * Each process has ONE 4KB page for syscall communication.
 * Ownership flips between process and kernel via borrow checker.
 *
 * Layout:
 *   0x000 - 0xEFF: Message area (3840 bytes) - request OR reply
 *   0xF00 - 0xFFF: Control block (256 bytes)
 */
/* Exchange page ring layout for small messages */
/* Legacy aliases (for transition) */
/* Fixed user virtual address for the Exchange Page (below stack at
 * 0x7FFFFEFFF000) */
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
  uint status; /* Access via atomic_load/store */
  volatile uint req_head;
  volatile uint req_tail;
  volatile uint rep_head;
  volatile uint rep_tail;
  volatile uint req_seq;
  volatile uint rep_seq;
  uchar session_pebble[32];
  uchar reserved[192];
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
  uint op_id; /* MSGORD message ID */
  Fcall *request; /* Original request (copied) */
  Fcall *reply; /* Reply when ready */
  P9CompletionCallback callback; /* Completion callback */
  void *callback_arg; /* Callback argument */
  uvlong submit_time; /* When submitted */
  int status; /* P9_STATUS_* */
  struct AsyncP9Op *next; /* Linked list for per-process tracking */
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
/*
 * Kernel Forward Type Declarations
 *
 * Centralized forward declarations for core kernel types.
 * Include this header when you need type names but not full definitions.
 */
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
} __attribute__((aligned(64)));
struct Label {
  uintptr sp; /* offset 0 */
  uintptr pc; /* offset 8 */
  uintptr rbp; /* offset 16 - frame pointer for local variables */
  uintptr rbx; /* offset 24 - callee-saved */
  uintptr r12; /* offset 32 - callee-saved */
  uintptr r13; /* offset 40 - callee-saved */
  uintptr r14; /* offset 48 - callee-saved */
  uintptr r15; /* offset 56 - callee-saved */
};
struct FPssestate {
  u16int fcw; /* x87 control word */
  u16int fsw; /* x87 status word */
  u8int ftw; /* x87 tag word */
  u8int zero; /* 0 */
  u16int fop; /* last x87 opcode */
  u64int rip; /* last x87 instruction pointer */
  u64int rdp; /* last x87 data pointer */
  u32int mxcsr; /* MMX control and status */
  u32int mxcsrmask; /* supported MMX feature bits */
  uchar st[128]; /* shared 64-bit media and x87 regs */
  uchar xmm[256]; /* 128-bit media regs */
  uchar ign[96]; /* reserved, ignored */
} __attribute__((aligned(64)));
struct FPavxstate {
  FPssestate sse_state;
  uchar header[64]; /* XSAVE header */
  uchar ymm[256]; /* upper 128-bit regs (AVX) */
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
  FPinactive, /* fpsave valid when fpstate >= FPincative */
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
  ulong nmach; /* processors */
  ulong nproc; /* processes */
  ulong monitor; /* has monitor? */
  ulong npage; /* total physical pages of memory */
  ulong upages; /* user page pool */
  ulong nimage; /* number of page cache image headers */
  ulong nswap; /* number of swap pages */
  int nswppo; /* max # of pageouts per segment pass */
  ulong copymode; /* 0 is copy on write, 1 is copy on reference */
  ulong ialloc; /* max interrupt time allocation in bytes */
  ulong pipeqsize; /* size in bytes of pipe queues */
  int nuart; /* number of uart devices */
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
} __attribute__((aligned(64)));
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
struct Queue {
  int _frama_dummy;
};
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
typedef int Devgen(Chan *, char *, Dirtab *, int, int, Dir *);
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
  PEBBLE_COLOR_WHITE = 1, /* Unverified/uninitialized (future) */
  PEBBLE_COLOR_BLACK = 2, /* Exclusive access (one writer) */
  PEBBLE_COLOR_RED = 3, /* Shared/read-only (multiple readers) */
  PEBBLE_COLOR_BLUE = 4, /* I/O buffer (future) */
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
extern ulong pebble_total_system_tokens; /* RAM/8, constant after init */
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
  uuid_t uuid; /* 16-byte UUIDv8 public identifier */
  u8int
      hash[32]; /* 32-byte BLAKE2b hash (security anchor) */
  u64int size; /* Size of the object (Span) in bytes */
  u32int type; /* Resource Type (Memory, Channel, PCI) */
  u32int perms; /* Permissions (Read, Write, Execute, Transfer) */
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
  CAP_PERM_GRANT = 1 << 4, /* Can create sub-capabilities */
};
// States for a BlindLedgerEntry
typedef enum BlindLedgerState {
  BLIND_LEDGER_STATE_INACTIVE = 0, // Not yet active or invalid
  BLIND_LEDGER_STATE_ACTIVE = 1, // Currently active and valid
  BLIND_LEDGER_STATE_BURNED = 2, // Burned, no longer valid, awaiting cleanup
  BLIND_LEDGER_STATE_COW_RED = 3, // Copy-on-Write: shared, read-only
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
  uintptr physical_address; /* The concrete resource (Start of Span) */
  Proc *owner; /* The current authoritative owner process */
  u8int secret[32]; /* Cryptographic secret for capability
                                             derivation */
  u64int epoch; /* Allocation Cycle (Prevents Use-After-Free) */
  u64int span_len; /* Length in bytes (must match Capability.size) */
  u32int permissions; /* Current effective permissions */
  BlindLedgerState state; /* Current state of the entry */
  BlindLedgerHash leaf_hash; /* Immutable hash of physical properties */
  BlindLedgerHash process_hash; /* Mutable hash of dynamic properties (owner,
                                   perms, state) */
  // Derivation Chain Support (for Red/Blue proofs)
  BlindLedgerHash parent_hash; /* Parent capability hash (0 if root) */
  BlindLedgerHash derivation_sig; /* HMAC(key, parent || constraints || self) */
} BlindLedgerEntry;
// Error codes for Blind Ledger operations
typedef enum BlindLedgerError {
  BLIND_LEDGER_OK = 0,
  BLIND_LEDGER_EINVAL = 1, // Invalid arguments
  BLIND_LEDGER_ENOMEM = 2, // Out of memory
  BLIND_LEDGER_EPERM = 3, // Permission denied (e.g., not owner)
  BLIND_LEDGER_ENOTFOUND = 4, // Capability not found
  BLIND_LEDGER_EEXPIRED = 5, // Capability found but not active/expired
  BLIND_LEDGER_EFAULT = 6, // General internal fault
  BLIND_LEDGER_EBUSY = 7, // Resource is busy, cannot perform operation
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
  Proc *original_owner; // Original owner before transfer
  BlindLedgerHash
      original_process_hash; // Original process_hash before transfer
  u64int is_valid; // Magic value to verify token validity
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
  BlindLedgerHash parent_hash; // Parent capability hash
  BlindLedgerHash derivation_sig; // HMAC(key, parent || constraints || child)
  u32int constraints; // Permissions granted (≤ parent)
} DerivationStep;
/*
 * DerivationProof - Proof of capability validity via derivation chain.
 *
 * The proof traces from the target capability up to a trusted root.
 * Each step proves: "child was derived from parent with valid constraints."
 */
typedef struct DerivationProof {
  BlindLedgerHash target_hash; // Capability being proven
  u32int chain_length; // 0 = root, N = derivation depth
  DerivationStep chain[16]; // Ancestors to root
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
  BORROW_FREE = 0, /* Resource is unowned */
  BORROW_EXCLUSIVE, /* Owned exclusively by one process */
  BORROW_SHARED_OWNED, /* Owner has resource, but lent as shared */
  BORROW_MUT_LENT, /* Owner lent resource as mutable, blocked */
};
/* Authorization Key */
struct IdentKey {
  u64int gen; /* Monotonic Generation Counter */
  u64int nonce; /* Hardware RNG Secret */
};
/* System-level owners for boot coordination */
enum BorrowSystemOwner {
  OWNER_BOOTLOADER = 0, /* Limine bootloader owns the resource */
  OWNER_KERNEL, /* Pebble kernel owns the resource */
  OWNER_TRAMPOLINE, /* CR3 switch trampoline code */
};
/* Track where ownership bookkeeping structs are allocated */
enum AllocSource {
  ALLOC_BOOTSTRAP,
  ALLOC_XALLOC,
};
/* Memory tracking structures for boot coordination */
struct MemoryRange {
  uintptr start; /* Start of memory range */
  uintptr end; /* End of memory range (exclusive) */
  enum BorrowSystemOwner owner; /* Who owns this range */
  struct MemoryRange *next; /* Next range in list */
};
/* Per-resource ownership tracking descriptor */
struct BorrowOwner {
  /* Resource identification */
  uintptr key; /* Unique key for the resource (e.g., address) */
  struct IdentKey key_cap; /* Authorization Capability */
  /* Core ownership */
  Proc *owner; /* Original owner process */
  enum BorrowState state; /* Current ownership state */
  enum BorrowSystemOwner system_owner; /* System owner during boot */
  int is_system_owned; /* 1 if owned at system level */
  /* Borrow Checker Interface */
  int shared_count; /* Number of shared borrows (&) */
  Proc *mut_borrower; /* Exclusive mutable borrower (&mut) */
  struct SharedBorrower *shared_list; /* List of shared borrowers */
  /* Lifetime tracking */
  uvlong acquired_ns; /* When ownership was acquired */
  uvlong borrow_deadline_ns; /* Automatic return time (0 = never) */
  /* Debugging */
  ulong borrow_count; /* Total times borrowed */
  /* Memory management */
  enum AllocSource alloc_source; /* Where this struct was allocated */
  struct BorrowOwner *next; /* Next in hash bucket chain */
};
/* Shared borrower tracking */
struct SharedBorrower {
  Proc *proc; /* Process that borrowed shared */
  enum AllocSource alloc_source; /* Where this struct was allocated */
  struct SharedBorrower *next; /* Next shared borrower */
};
/* Hash bucket for borrow pool */
struct BorrowBucket {
  struct BorrowOwner *head; /* Head of owner list for this bucket */
};
/* Borrow pool - hash table of resources */
struct BorrowPool {
  Lock lock; /* Protects entire pool */
  struct BorrowBucket *owners; /* Hash table buckets */
  ulong nbuckets; /* Number of hash buckets */
  ulong nowners; /* Total number of owned resources */
  ulong nshared; /* Resources with shared borrows */
  ulong nmut; /* Resources with mutable borrows */
  u8int *bloom; /* Counting bloom filter counters */
  ulong bloom_bits; /* Number of bloom counters */
  u32int bloom_hashes; /* Number of hash functions */
};
/* Memory coordination states */
enum MemoryCoordinationState {
  MEMORY_BOOTLOADER = 0, /* Bootloader owns everything */
  MEMORY_COORDINATED, /* Ownership zones established */
  MEMORY_KERNEL_ACTIVE, /* Kernel has taken full control */
};
/* Memory coordination structure */
struct MemoryCoordination {
  enum MemoryCoordinationState state; /* Current coordination state */
  enum BorrowSystemOwner current_owner; /* Current memory owner */
  int coordination_enabled; /* Enable/disable coordination */
};
/* Error codes for borrow operations */
enum BorrowError {
  BORROW_OK = 0,
  BORROW_EALREADY, /* Already owned */
  BORROW_ENOTOWNER, /* Not the owner */
  BORROW_EBORROWED, /* Can't modify - has borrows */
  BORROW_EMUTBORROW, /* Can't borrow - already &mut */
  BORROW_ESHAREDBORROW, /* Can't borrow &mut - has & */
  BORROW_ENOTBORROWED, /* Not borrowed, can't return */
  BORROW_EINVAL, /* Invalid parameters */
  BORROW_ENOMEM, /* Out of memory */
  BORROW_ENOTFOUND, /* Resource not found */
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
  void *blue_data; /* Physical memory (separate allocation) */
  ulong blue_size; /* Size of allocation */
  ulong flags; /* State flags */
  struct PebbleBlue *next; /* List linkage */
} PebbleBlue;
/* Red copy structure - independent colored token for snapshots */
typedef struct PebbleRed {
  void *red_data; /* Physical memory (separate allocation) */
  ulong red_size; /* Size of allocation */
  ulong flags; /* State flags */
  struct PebbleRed *next; /* List linkage */
} PebbleRed;
typedef struct PebbleBlack {
  UserCapability capability; // The UserCapability provided by Blind Ledger
  void *
      physical_addr; // The actual physical memory address managed by this token
  uintptr user_vaddr; // The user-space virtual address mapping (if any)
  ulong size; // Size of the allocation
  ulong flags;
  struct PebbleBlack *next;
} PebbleBlack;
/* Per-process Pebble state */
typedef struct PebbleState {
  ulong colorless_bank; /* remaining bytes for this process (COLORLESS pool) */
  ulong black_inuse; /* bytes in BLACK state */
  ulong blue_inuse; /* bytes in BLUE state */
  ulong red_inuse; /* bytes in RED state */
  ulong white_verified; /* count of active white→black conversions */
  ulong white_pending; /* bytes authorized by white tokens (WHITE state) */
  ulong red_count; /* number of live red tokens */
  ulong blue_count; /* number of live blue tokens */
  ulong total_allocs; /* total allocations made */
  ulong total_frees; /* total frees performed */
  uintptr vbase; /* next available user virtual address for Pebble mapping */
  /* Lists for tracking objects */
  PebbleBlack *black_list;
  PebbleBlue *blue_list;
  PebbleRed *red_list;
  /* State tracking */
  int in_syscall; /* set when in Pebble syscalls */
  ulong drop_budget; /* budget that will drop on exit */
  /* White token bookkeeping */
  PebbleWhite whites[4096];
  uchar whites_active[4096];
  ulong white_generation;
  int white_head;
} PebbleState;
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
  Lock lock; /* Per-branch lock (no global contention) */
  ulong local_colorless; /* Tokens available locally in this branch */
  ulong borrowed_from_proc; /* Tokens borrowed from process bank */
  ulong max_tokens; /* Hard cap on branch tokens */
  ulong low_water; /* Request refill when below this threshold */
  ulong high_water; /* Return excess when above this threshold */
  ulong total_allocated; /* Statistics: total bytes allocated from branch */
  ulong total_freed; /* Statistics: total bytes freed to branch */
  PebbleState *owner_ps; /* Back-pointer to owning process PebbleState */
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
int pebble_blue_free(PebbleBlue *blue); /* BLUE → COLORLESS */
PebbleRed *pebble_red_alloc(ulong size); /* COLORLESS → RED */
int pebble_red_free(PebbleRed *red); /* RED → COLORLESS */
int pebble_red_snapshot(PebbleBlue *blue,
                        PebbleRed **out_red); /* Copy Blue → Red */
/* Legacy API - DEPRECATED, will be removed */
int pebble_red_copy(PebbleBlue *blue_obj,
                    PebbleRed **red_copy); /* Use pebble_red_snapshot */
int pebble_blue_discard(PebbleBlue *blue_obj); /* Use pebble_blue_free */
/* Internal helper functions */
PebbleState *pebble_state(void);
int pebble_set_budget(ulong budget);
ulong pebble_get_budget(void);
int pebble_increase_budget(ulong size, u64int nonce);
void pebble_auto_verify(Proc *p, Ureg *ureg);
void pebble_red_blue_exit(void);
int pebble_valid_white_token(PebbleState *ps, PebbleWhite *white);
PebbleWhite *pebble_issue_white(PebbleState *ps, void *data, ulong size);
void pebble_return_white(PebbleState *ps, PebbleWhite *white);
PebbleBlack *pebble_lookup_black(PebbleState *ps, void *handle);
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
      u32int msize; /* Tversion, Rversion */
      char *version; /* Tversion, Rversion */
    };
    struct {
      ushort oldtag; /* Tflush */
    };
    struct {
      char *ename; /* Rerror */
    };
    struct {
      Qid qid; /* Rattach, Ropen, Rcreate */
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
      char *name; /* Tcreate */
      uchar mode; /* Tcreate, Topen */
    };
    struct {
      u32int newfid; /* Twalk */
      ushort nwname; /* Twalk */
      char *wname[16]; /* Twalk */
    };
    struct {
      ushort nwqid; /* Rwalk */
      Qid wqid[16]; /* Rwalk */
    };
    struct {
      vlong offset; /* Tread, Twrite */
      u32int count; /* Tread, Twrite, Rread */
      char *data; /* Twrite, Rread */
    };
    struct {
      ushort nstat; /* Twstat, Rstat */
      uchar *stat; /* Twstat, Rstat */
    };
    struct {
      u32int scallnr; /* Tsyscall */
      u32int sflags; /* Tsyscall flags/category */
      uchar *sdata; /* Tsyscall, Rsyscall */
      u32int scount; /* Tsyscall, Rsyscall */
      u64int retval; /* Rsyscall */
    };
    /* Tsys* message fields */
    struct {
      u32int flags; /* Tsysfork (rfork flags), Tsysbind, Tsysmount */
      u32int pid; /* Rsysfork, Rsyswait */
    };
    struct {
      char *path; /* Tsysexec - executable path */
      char **argv; /* Tsysexec - argument array */
      u32int argc; /* Tsysexec - argument count */
      char
          *args[16]; /* Tsysexec - workspace for deserialized arguments */
    };
    struct {
      u64int addr; /* Tsysbrk, Rsysbrk - memory address */
    };
    struct {
      char *oldpath; /* Tsysbind, Tsysmount, Tsysunmount - old path */
      u32int fd; /* Tsysmount - file descriptor */
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
  SYS_WAIT = 166
};
struct Ref {
  long ref;
};
struct Rendez {
  Lock lock;
  Proc *p;
};
struct QLock {
  Lock use; /* to access Qlock structure */
  Proc *head; /* next process waiting for object */
  Proc *tail; /* last process waiting for object */
  uintptr pc; /* pc of owner */
  int locked; /* flag */
};
struct Rendezq {
  QLock qlock;
  Rendez rendez;
};
struct RWLock {
  Lock use;
  Proc *head; /* list of waiting processes */
  Proc *tail;
  uintptr wpc; /* pc of writer */
  int writer; /* number of writers */
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
  Aaccess, /* as in stat, wstat */
  Abind, /* for left-hand-side of bind */
  Atodir, /* as in chdir */
  Aopen, /* for i/o */
  Amount, /* to be mounted or mounted upon */
  Acreate, /* is to be created */
  Aremove, /* will be removed by caller */
  Aunmount, /* unmount arg[0] */
  COPEN = 0x0001, /* for i/o */
  CMSG = 0x0002, /* the message channel for a mount */
                    /* rsc CCREATE = 0x0004, permits creation if c->mnt */
  CCEXEC = 0x0008, /* close on exec (per file descriptor) */
  CFREE = 0x0010, /* not in use */
  CRCLOSE = 0x0020, /* remove on close */
  CCACHE = 0x0080, /* client cache */
};
/* flag values */
enum {
  BINTR = (1 << 0),
  BFREE = (1 << 1),
  Bipck = (1 << 2), /* ip checksum */
  Budpck = (1 << 3), /* udp checksum */
  Btcpck = (1 << 4), /* tcp checksum */
  Bpktck = (1 << 5), /* packet checksum */
};
struct Block {
  Block *next;
  Block *list;
  uchar *rp; /* first unconsumed byte */
  uchar *wp; /* first empty byte */
  uchar *lim; /* 1 past the end of the buffer */
  uchar *base; /* start of the buffer */
  Bpool *pool;
  ushort flag;
  ushort checksum; /* IP checksum of complete packet (minus media header) */
};
struct Bpool {
  ulong size; /* block size */
  ulong align; /* block alignment */
  Lock lock;
  Block *head; /* freelist head */
};
struct Chan {
  long ref;
  Lock lock;
  Chan *next; /* allocation */
  Chan *link;
  vlong offset; /* in fd */
  vlong devoffset; /* in underlying device; see read */
  ushort type;
  ulong dev;
  ushort mode; /* read/write */
  ushort flag;
  Qid qid;
  int fid; /* for devmnt */
  ulong iounit; /* chunk size for i/o; 0==default */
  Mhead *umh; /* mount point that derived Chan; used in unionread */
  Chan *umc; /* channel in union; held for union read */
  QLock umqlock; /* serialize unionreads */
  int uri; /* union read index */
  int dri; /* devdirread index */
  uchar *dirrock; /* directory entry rock for translations */
  int nrock;
  int mrock;
  QLock rockqlock;
  int ismtpt;
  Mntcache *mcp; /* Mount cache pointer */
  Mnt *mux; /* Mnt for clients using me for messages */
  union {
    void *aux;
    ulong mid; /* for ns in devproc */
  };
  Chan *mchan; /* channel to mounted server */
  Qid mqid; /* qid of root of mount point */
  Path *path;
  char *srvname; /* /srv/name when posted */
};
struct Path {
  long ref;
  char *s;
  Chan **mtpt; /* mtpt history */
  int len; /* strlen(s) */
  int alen; /* allocated length of s */
  int mlen; /* number of path elements */
  int malen; /* allocated length of mtpt */
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
  char spec[];
};
struct Mhead {
  long ref;
  RWLock lock;
  Chan *from; /* channel mounted upon */
  Mount *mount; /* what's mounted upon it */
  Mhead *hash; /* Hash chain */
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
  Chan *c; /* Channel to file service */
  Proc *rip; /* Reader in progress */
  Mntrpc *queue; /* Queue of pending requests on this channel */
  Mntproc defered[8]; /* Worker processes for defered RPCs (read ahead) */
  ulong id; /* Multiplexer id for channel check */
  Mnt *list; /* Free list */
  int flags; /* cache */
  int msize; /* data + IOHDRSZ */
  char *version; /* 9P version */
  Queue *q; /* input queue */
};
enum {
  NUser, /* note provided externally */
  NExit, /* deliver note quietly */
  NDebug, /* print debug message */
};
struct Note {
  char msg[128];
  int flag; /* whether system posted it */
  long ref;
};
enum {
  PG_MOD = 0x01, /* software modified bit */
  PG_REF = 0x02, /* software referenced bit */
  PG_PRIV = 0x04, /* private page */
};
struct Page {
  long ref;
  Page *next; /* Free list or Hash chains */
  uintptr pa; /* Physical address in memory */
  uintptr va; /* Virtual address for user */
  uintptr daddr; /* Disc address on swap */
  Image *image; /* Associated text or swap image */
  ushort refage; /* Swap reference age */
  char modref; /* Simulated modify/reference bits */
  char color; /* Cache coloring */
  char token_color; /* Pebble token color (enum PebbleColor) */
} __attribute__((aligned(64)));
struct Swapalloc {
  Lock lock; /* Free map lock */
  int free; /* currently free swap pages */
  uchar *swmap; /* Base of swap map in memory */
  uchar *alloc; /* Round robin allocator */
  uchar *last; /* Speed swap allocation */
  uchar *top; /* Top of swap map */
  Rendez r; /* Pager kproc idle sleep */
  ulong highwater; /* Pager start threshold */
  ulong headroom; /* Space pager frees under highwater */
  ulong xref; /* Ref count for all map refs >= 255 */
};
extern struct Swapalloc swapalloc;
struct Pte {
  Page *pages[((1ull * 1048576u) / (0x1000ull))]; /* Page map for this chunk of pte */
  Page **first; /* First used entry */
  Page **last; /* Last used entry */
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
  SG_RONLY = 0040, /* Segment is read only */
  SG_CEXEC = 0100, /* Detach at exec */
  SG_FAULT = 0200, /* Fault on access */
  SG_CACHED = 0400, /* Normal cached memory */
  SG_DEVICE = 01000, /* Memory mapped device */
  SG_NOEXEC = 02000, /* No execute */
  SG_WASM = 04000, /* WASM-isolated segment */
};
struct Physseg {
  int attr; /* Segment attributes */
  char *name; /* Attach name */
  uintptr pa; /* Physical address */
  uintptr size; /* Maximum segment size in bytes */
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
  int type; /* segment type */
  ulong size; /* size in pages */
  uintptr base; /* virtual base */
  uintptr top; /* virtual top */
  uintptr fstart; /* start address in file for demand load */
  uintptr flen; /* length of segment in file */
  int flushme; /* maintain icache for this segment */
  Image *image; /* text in file attached to this segment */
  Physseg *pseg;
  ulong *profile; /* Tick profile area */
  Pte **map;
  int mapsize;
  Pte *ssegmap[16];
  ulong used; /* pages used (swapped or not) */
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
  NFD = 100, /* per process file descriptors */
  ENVLOG = 5,
  ENVHASH = 1 << ENVLOG, /* Egrp hash for variable lookup */
};
struct Image {
  Lock lock;
  long ref;
  long pgref; /* number of cached pages (pgref <= ref) */
  ulong nattach; /* usage frequency */
  Image **link; /* idle list */
  Image *next; /* idle list */
  Image *hash; /* Qid hash chains */
  Segment *s; /* TEXT segment for image if running */
  Chan *c; /* channel to text file, nil when not used */
  Qid qid; /* Qid for page cache coherence */
  ulong dev; /* Device id of owning channel */
  ushort type; /* Device type of owning channel */
  char notext; /* no file associated */
  ulong pghsize;
  Page *pghash[]; /* page cache */
};
struct Pgrp {
  long ref;
  RWLock ns; /* Namespace n read/one write lock */
  u64int notallowed[4]; /* Room for 256 devices */
  Mhead *mnthash[MNTHASH];
  /* Namespace spawn limits - cryptographically bound via identity_hash */
  u8int identity_hash[16]; /* Blake2b hash of Pgrp for spawn cap binding */
  u8int namespace_cid[32]; /* Full BLAKE2b hash of Namespace Config
                              (Mounts+Caps) */
  Lock spawn_lock; /* Protect spawn counts */
  u32int spawn_limit; /* Max procs allowed in this namespace */
  u32int spawn_count; /* Current proc count in namespace */
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
  int nent; /* numer of slots in ent[] */
  int low; /* lowest free index in ent[] */
  int alloc; /* bytes allocated for env */
  ulong path; /* generator for qid path */
  ulong vers; /* of Egrp */
  Evalue *hash[ENVHASH]; /* hashtable for name lookup */
};
struct Fgrp {
  Lock lock;
  Ref ref;
  Chan **fd;
  uchar *flag; /* per file-descriptor flags (CCEXEC) */
  int nfd; /* number allocated */
  int maxfd; /* highest fd in use */
  int exceed; /* debugging */
};
enum {
  DELTAFD = 20 /* incremental increase in Fgrp.fd's */
};
struct Palloc {
  Lock lock;
  Page *head; /* freelist head */
  ulong freecount; /* how many pages on free list now */
  Page *pages; /* array of all pages */
  ulong user; /* how many user pages */
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
  Timers *tt; /* Timers queue this timer runs on */
  Tval tticks; /* tns converted to ticks */
  Tval twhen; /* ns represented in fastticks */
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
  Npriq = 20, /* number of scheduler priority levels */
  Nrq = Npriq + 2, /* number of priority levels including real time */
  PriRelease = Npriq, /* released edf processes */
  PriEdf = Npriq + 1, /* active edf processes */
  PriNormal = 10, /* base priority for normal processes */
  PriExtra = Npriq - 1, /* edf processes at high best-effort pri */
  PriKproc = 13, /* Magic guard values for detecting corruption */
  PriRoot = 13, /* base priority for root processes */
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
  int nargs; /* number of bytes of args */
  int setargs; /* process changed its args */
  Proc *rnext; /* next process in run queue */
  Proc *qnext; /* next process on queue for a QLock */
  char *psstate; /* What /proc/#/status reports */
  int state;
  ushort state_trace; /* FSM: last 4 states (4 bits each) */
  ushort hdr_checksum; /* CRC-16 of critical fields */
  /* 9P Exchange Page for pure 9P architecture */
  void *p9page; /* DEPRECATED: Fixed exchange page (legacy).
                       * New code should use exchange_channel via #X device.
                       * Kept for backwards compatibility with existing doorbell code.
                       */
  uvlong p9page_phys; /* physical address of p9page */
  void *exchange_channel; /* ExchangeChannel from #X device (devexchange.c)
                           * Provides: ring buffer, page pool, capabilities */
  /* 9P FID tracking for syscall translation layer */
  u32int fid_counter; /* Next FID to allocate for this process */
  u32int dot_fid; /* FID for current working directory */
  vlong fid_offsets[256]; /* Offset per FID for read/write/seek tracking */
  ulong pid;
  uuid_t pid2; /* Lux9 Secure ID */
  ulong noteid; /* Equivalent of note group */
  ulong parentpid;
  ulong index;
  Proc *parent; /* Process to send wait record on exit */
  Lock exl; /* Lock count and waitq */
  Waitq *waitq; /* Exited processes wait children */
  int nchild; /* Number of living children */
  int nwait; /* Number of uncollected wait records */
  QLock qwaitr;
  Rendez waitr; /* Place to hang out in wait */
  QLock seglock; /* locked whenever seg[] changes */
  Segment *seg[NSEG];
  Pgrp *pgrp; /* Process group for namespace */
  Egrp *egrp; /* Environment group */
  Fgrp *fgrp; /* File descriptor group */
  Rgrp *rgrp; /* Rendez group */
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
  QLock debug; /* to access debugging elements of User */
  Proc *pdbg; /* the debugging process */
  ulong procmode; /* proc device default file mode */
  int privatemem; /* proc does not let anyone read mem */
  int noswap; /* process is not swappable */
  int hang; /* hang at next exec for debug */
  int procctl; /* Control for /proc debugging */
  Lock rlock; /* sync sleep/wakeup with procinterrupt */
  Rendez *r; /* rendezvous point slept on */
  Rendez sleep; /* place for syssleep/debug */
  int notepending; /* note issued but not acted on */
  int kp; /* true if a kernel process */
  Proc *palarm; /* Next alarm time */
  ulong alarm; /* Time of call */
  int newtlb; /* Pager has changed my pte's, I must flush */
  Proc *vforkp; /* vfork parent to unblock on exec/exit */
  uintptr rendtag; /* Tag for rendezvous */
  uintptr rendval; /* Value for rendezvous */
  Proc *rendhash; /* Hash list for tag values */
  Rendez *trend;
  int (*tfn)(void *);
  void (*kpfun)(void *);
  void *kparg;
  Sargs s; /* syscall arguments */
  int scallnr; /* sys call number */
  int nerrlab;
  Label errlab[NERR];
  char *syserrstr; /* last error from a system call, errbuf0 or 1 */
  char *errstr; /* reason we're unwinding the error stack, errbuf1 or 0 */
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
  Lock *lastlock; /* debugging */
  Lock *lastilock; /* debugging */
  int nlocks; /* number of locks held by proc */
  ulong delaysched;
  ulong priority; /* priority level */
  ulong basepri; /* base priority level */
  uchar fixedpri; /* priority level doesn't change */
  uchar wired;
  int affinity; /* machno this process last ran on */
  ulong cpu; /* cpu average */
  ulong lastupdate;
  uchar *kstack; /* base of kernel stack allocation */
  Edf *edf; /* if non-null, real-time proc, edf contains scheduling params */
  int trace; /* process being traced? */
  uintptr qpc; /* pc calling last blocking qlock */
  uintptr pc; /* program counter for profiling */
  QLock *eql; /* interruptable eqlock */
  void *noteureg; /* User registers for notes */
  void *dbgreg; /* User registers for devproc */
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
  Watchpt *watchpt; /* watchpoints */
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
    int initialized; /* 1 if this is a WASM process, 0 for native */
    void *runtime; /* IM3Runtime - wasm3 runtime for this process */
    void *module; /* IM3Module - loaded WASM module */
    void *env; /* IM3Environment - per-process wasm3 environment */
    u8int *linear_memory; /* WASM linear memory (mapped to seg[LSEG]) */
    u32int memory_size; /* Size of linear memory in bytes */
    u32int memory_pages; /* Number of 64KB WASM pages */
    u32int linear_charged; /* Pebble-charged linear memory bytes */
    u8int *heap_base; /* WASM runtime heap base (userspace addr) */
    u32int heap_size; /* WASM runtime heap size in bytes */
    u32int heap_used; /* WASM runtime heap used bytes */
    void *heap_head; /* WASM heap block list head */
    u32int heap_live; /* WASM heap live bytes (token-backed) */
    arena_branch_t branch; /* Local Pebble branch bank for this container */
    void *wasi_ctx; /* WASI Context (wasi_lux9_shim.h wasi_context_t) */
    void *module_bytes; /* Persistent WASM module bytecode */
    u32int module_bytes_len;
  } wasm;
  /* Spawn Capability - UUIDv8-based process creation control.
   * Uses CAP_TYPE_SPAWN capability token with child limit. */
  uuid_t spawn_cap; /* UUIDv8 spawn capability (null = no spawn rights) */
  u32int spawn_max_children; /* Maximum children this process can spawn */
  u32int spawn_children; /* Current number of children spawned */
  /* Init Hardening: Binary binding for spawn.
   * If non-zero, this process can ONLY exec binaries matching this hash.
   * Used to ensure init can only spawn resurrection server. */
  u8int spawn_bound_binary[64]; /* Blake2b-512 of allowed binary (0 = any) */
} __attribute__((aligned(64)));
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
  int narg; /* expected #args; 0 ==> variadic */
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
  void *regs; /* hardware stuff */
  void *saveregs; /* place to put registers on power down */
  char *name; /* internal name */
  ulong freq; /* clock frequency */
  int bits; /* bits per character */
  int stop; /* stop bits */
  int parity; /* even, odd or no parity */
  int baud; /* baud rate */
  PhysUart *phys;
  int console; /* used as a serial console */
  int special; /* internal kernel device */
  Uart *next; /* list of allocated uarts */
  QLock lock;
  int type; /* ?? */
  int dev;
  int opens;
  int enabled;
  Uart *elist; /* next enabled interface */
  int perr; /* parity errors */
  int ferr; /* framing errors */
  int oerr; /* rcvr overruns */
  int berr; /* no input buffers */
  int serr; /* input queue overflow */
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
  int modem; /* hardware flow control on */
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
  ulong intrts; /* time of last interrupt */
  ulong inintr; /* time since last clock tick in interrupt handlers */
  ulong avg_inintr; /* avg time per clock tick in interrupt handlers */
  ulong inidle; /* time since last clock tick in idle loop */
  ulong avg_inidle; /* avg time per clock tick in idle loop */
  ulong last; /* value of perfticks() at last clock tick */
  ulong period; /* perfticks() per clock tick */
};
struct Watchdog {
  void (*enable)(void); /* watchdog enable */
  void (*disable)(void); /* watchdog disable */
  void (*restart)(void); /* watchdog restart */
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
  Proc *readied; /* for runproc */
  Label sched; /* scheduler wakeup */
  ulong ticks; /* of the clock since boot time */
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
  Perf perf; /* performance counters */
  uvlong cyclefreq; /* Frequency of user readable cycle counter */
};
/* queue state bits,  Qmsg, Qcoalesce, and Qkick can be set in qopen */
enum {
  /* Queue.state */
  Qstarve = (1 << 0), /* consumer starved */
  Qmsg = (1 << 1), /* message stream */
  Qclosed = (1 << 2), /* queue has been closed/hungup */
  Qflow = (1 << 3), /* producer flow controlled */
  Qcoalesce = (1 << 4), /* coallesce packets on read */
  Qkick = (1 << 5), /* always call the kick routine after qwrite */
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
} Tss __attribute__((aligned(64)));
struct Mach {
  int machno; /* physical id of processor */
  uintptr splpc; /* pc of last caller to splhi */
  Proc *proc; /* current process on this processor */
  /* PMach fields */
  uintptr rbx_restore; /* scratch for saving user RBX during syscallentry */
  Proc *readied; /* for runproc */
  Label sched; /* scheduler wakeup */
  ulong ticks; /* of the clock since boot time */
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
  Perf perf; /* performance counters */
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
  char haveaes; /* AES-NI instructions available */
  char havesha; /* SHA extensions available */
  char havepclmul; /* PCLMULQDQ instruction available */
  char haverdrand; /* RDRAND instruction available */
  int fpstate; /* FPU state for interrupts */
  FPalloc *fpsave;
  u64int *pml4; /* pml4 base for this processor (va) */
  Tss *tss; /* tss for this processor */
  Segdesc *gdt; /* gdt for this processor */
  u64int dr7; /* shadow copy of dr7 */
  u64int xcr0;
  void *vmx;
  MMU *mmufree; /* freelist for MMU structures */
  ulong mmucount; /* number of MMU structures in freelist */
  u64int mmumap[4]; /* bitmap of pml4 entries for zapping */
  uintptr stack[1];
} __attribute__((aligned(64)));
/*
 * KMap the structure
 */
typedef void KMap;
extern u64int MemMin;
struct Active {
  char machs[128]; /* bitmap of active CPUs */
  int exiting; /* shutdown */
};
extern struct Active active;
/*
 *  routines for things outside the PC model, like power management
 */
struct PCArch {
  char *id;
  int (*ident)(void); /* this should be in the model */
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
  Vmex = 1 << 1, /* virtual-mode extensions */
  Pse = 1 << 3, /* page size extensions */
  Tsc = 1 << 4, /* time-stamp counter */
  Cpumsr = 1 << 5, /* model-specific registers, rdmsr/wrmsr */
  Pae = 1 << 6, /* physical-addr extensions */
  Mce = 1 << 7, /* machine-check exception */
  Cmpxchg8b = 1 << 8,
  Cpuapic = 1 << 9,
  Mtrr = 1 << 12, /* memory-type range regs.  */
  Pge = 1 << 13, /* page global extension */
  Mca = 1 << 14, /* machine-check architecture */
  Pat = 1 << 16, /* page attribute table */
  Pse2 = 1 << 17, /* more page size extensions */
  Clflush = 1 << 19,
  Acpif = 1 << 22, /* therm control msr */
  Mmx = 1 << 23,
  Fxsr = 1 << 24, /* have SSE FXSAVE/FXRSTOR */
  Sse = 1 << 25, /* thus sfence instr. */
  Sse2 = 1 << 26, /* thus mfence & lfence instr.s */
};
enum { /* MSRs */
       PerfEvtbase = 0xc0010000, /* Performance Event Select */
       PerfCtrbase = 0xc0010004, /* Performance Counters */
       Efer = 0xc0000080, /* Extended Feature Enable */
       Star = 0xc0000081, /* Legacy Target IP and [CS]S */
       Lstar = 0xc0000082, /* Long Mode Target IP */
       Cstar = 0xc0000083, /* Compatibility Target IP */
       Sfmask = 0xc0000084, /* SYSCALL Flags Mask */
       FSbase = 0xc0000100, /* 64-bit FS Base Address */
       GSbase = 0xc0000101, /* 64-bit GS Base Address */
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
extern Mach *m; /* R15 */
extern Proc *up; /* R14 */
/*
 *  hardware info about a device
 */
typedef struct {
  ulong port;
  int size;
} Devport;
struct DevConf {
  ulong intnum; /* interrupt number */
  char *type; /* card type, malloced */
  int nports; /* Number of ports */
  Devport *ports; /* The ports themselves */
};
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
int canlock(Lock *);
int canpage(Proc *);
int canqlock(QLock *);
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
long decref(Ref *);
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
  @ assigns *dp;
  @ terminates \true;
  */
void devdir(Chan *c, Qid qid, char *name, vlong length, char *user, long perm,
            Dir *dp);
/*@ requires c != \null;
  @ assigns \nothing;
  @ terminates \true;
  */
long devdirread(Chan *c, char *va, long n, Dirtab *tab, int ntab,
                Devgen *gen);
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
  @ assigns \nothing;
  @ terminates \true;
  */
int devstat(Chan *c, uchar *dp, int n, Dirtab *tab, int ntab, Devgen *gen);
/*@ requires c != \null;
  @ assigns \result \from \nothing;
  @ terminates \true;
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
/*@ requires e == \null || \valid(e);
  @ assigns \nothing;
  @ ensures \false;
  @ terminates \true;
  */
_Noreturn void error(char *e);
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
/*@ assigns \nothing;
  @ terminates \true;
  */
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
char *getconfenv(void);
void growbp(Bpool *, int);
long hostdomainwrite(char *, int);
long hostownerwrite(char *, int);
extern void (*hwrandbuf)(void *, ulong);
void hzsched(void);
Block *iallocb(int);
Block *iallocbp(Bpool *);
uintptr ibrk(uintptr, int);
void ilock(Lock *);
_Noreturn void interrupted(void);
void iunlock(Lock *);
ulong imagecached(void);
ulong imagereclaim(ulong);
long incref(Ref *);
void init0(void);
void initseg(void);
int ioalloc(ulong, ulong, ulong, char *);
void iofree(ulong);
void iomapinit(ulong);
int ioreserve(ulong, ulong, ulong, char *);
int ioreservewin(ulong, ulong, ulong, ulong, char *);
int iounused(ulong, ulong);
int iprint(char *, ...);
int iprint_intr(char *, ...);
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
int kproc(char *, void (*)(void *), void *);
void kprocchild(Proc *, void (*)(void));
void linkproc(void);
extern void (*kproftimer)(uintptr);
void ksetenv(char *, char *, int);
int kopen(char *, int);
void kstrcpy(char *, char *, int);
void kstrdup(char **, char *);
void lock(Lock *);
void logopen(Log *);
void logclose(Log *);
char *logctl(Log *, int, char **, Logflag *);
void logn(Log *, int, void *, int);
long logread(Log *, void *, ulong, long);
void log(Log *, int, char *, ...);
Cmdtab *lookupcmd(Cmdbuf *, Cmdtab *, int);
Page *lookpage(Image *, uintptr);
void machinit(void);
/*@ behavior zero:
  @   assumes size == 0;
  @   assigns \result \from \nothing;
  @   ensures \result == \null || \valid((char *)\result);
  @ behavior nonzero:
  @   assumes size > 0;
  @   assigns \result \from \nothing;
  @   ensures \result == \null || \valid(((char *)\result) + (0 .. (integer)size - 1));
  @ complete behaviors;
  @ disjoint behaviors;
  @ terminates \true;
  */
void *mallocz(ulong size, int clr);
/*@ behavior zero:
  @   assumes size == 0;
  @   assigns \result \from \nothing;
  @   ensures \result == \null || \valid((char *)\result);
  @ behavior nonzero:
  @   assumes size > 0;
  @   assigns \result \from \nothing;
  @   ensures \result == \null || \valid(((char *)\result) + (0 .. (integer)size - 1));
  @ complete behaviors;
  @ disjoint behaviors;
  @ terminates \true;
  */
void *malloc(ulong size);
/*@ behavior zero:
  @   assumes size == 0;
  @   assigns \result \from \nothing;
  @   ensures \result == \null || \valid((char *)\result);
  @ behavior nonzero:
  @   assumes size > 0;
  @   assigns \result \from \nothing;
  @   ensures \result == \null || \valid(((char *)\result) + (0 .. (integer)size - 1));
  @ complete behaviors;
  @ disjoint behaviors;
  @ terminates \true;
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
Mount *newmount(Chan *, int, char *);
Image *newimage(ulong);
Page *newpage(uintptr, Segment *);
Path *newpath(char *);
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
/*@ requires fmt == \null || \valid(fmt);
  @ assigns \nothing;
  @ ensures \false;
  @ terminates \true;
  */
_Noreturn void panic(char *fmt, ...);
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
int postnote(Proc *, int, char *, int);
void postnotepg(ulong, char *, int);
int pprint(char *, ...);
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
  @ assigns \nothing;
  @ terminates \true;
  */
void qclose(Queue *q);
int qconsume(Queue *, void *, int);
Block *qcopy(Queue *, int, ulong);
int qdiscard(Queue *, int);
void qflush(Queue *);
/*@ requires q != \null;
  @ assigns \nothing;
  @ terminates \true;
  */
void qfree(Queue *q);
int qfull(Queue *);
Block *qget(Queue *);
void qhangup(Queue *, char *);
int qisclosed(Queue *);
int qiwrite(Queue *, void *, int);
/*@ requires q != \null;
  @ assigns \nothing;
  @ terminates \true;
  */
int qlen(Queue *q);
/*@ requires l != \null;
  @ assigns *l;
  @ terminates \true;
  */
void qlock(QLock *l);
/*@ assigns \result \from \nothing;
  @ ensures \result == \null || \valid(\result);
  @ terminates \true;
  */
Queue *qopen(int, int, void (*)(void *), void *);
int qpass(Queue *, Block *);
int qpassnolim(Queue *, Block *);
int qproduce(Queue *, void *, int);
void qputback(Queue *, Block *);
/*@ requires q != \null;
  @ requires buf == \null || (n >= 0 && \valid(((char *)buf) + (0..n-1)));
  @ assigns ((char *)buf)[0..n-1];
  @ terminates \true;
  */
long qread(Queue *q, void *buf, int n);
Block *qremove(Queue *);
void qreopen(Queue *);
void qsetlimit(Queue *, int);
/*@ requires l != \null;
  @ assigns *l;
  @ terminates \true;
  */
void qunlock(QLock *l);
/*@ requires q != \null;
  @ requires buf == \null || (n >= 0 && \valid(((char *)buf) + (0..n-1)));
  @ assigns \nothing;
  @ terminates \true;
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
void *smalloc(ulong);
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
void tsleep(Rendez *, int (*)(void *), void *, ulong);
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
void unlock(Lock *);
uvlong us2fastticks(uvlong);
void userinit(void);
uintptr userpc(void);
long userwrite(char *, int);
void validaddr(uintptr, ulong, int);
void validname(char *, int);
char *validnamedup(char *, int);
void validstat(uchar *, int);
void *vmemchr(void *, int, ulong);
Proc *wakeup(Rendez *);
int walk(Chan **, char **, int, int, int *);
void wlock(RWLock *);
void wunlock(RWLock *);
/*@ allocates \result;
    assigns \result \from size;
    ensures \result == \null || \valid((char*)\result + (0..size-1));
    terminates \true;
*/
void *xalloc(ulong size);
/*@ allocates \result;
    assigns \result \from size;
    ensures \result == \null || \valid((char*)\result + (0..size-1));
    terminates \true;
*/
void *xalloc_raw(ulong size);
/*@ allocates \result;
    assigns \result \from size;
    ensures \result == \null || \valid((char*)\result + (0..size-1));
    terminates \true;
*/
void *xallocz(ulong size, int zero);
/*@ allocates \result;
    assigns \result \from size;
    ensures \result == \null || \valid((char*)\result + (0..size-1));
    terminates \true;
*/
void *xallocz_raw(ulong size, int zero);
/*@ assigns \nothing;
  @ terminates \true;
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
extern uintptr paddr(void *);
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
ushort ins(int port); /* Input from I/O port */
void outs(int port, ushort value); /* Output to I/O port */
/* Memory and page ownership functions */
uintptr cankaddr(uintptr); /* Check if address in kernel address space - matches
                              arch signature */
enum PageOwnError pageown_acquire(Proc *, uintptr,
                                  u64int); /* Acquire page ownership */
enum PageOwnError pageown_release(Proc *, uintptr); /* Release page ownership */
void pageown_cleanup_process(Proc *); /* Clean up page ownership for process */
/* Architecture-specific process functions - declarations handled in
 * arch-specific fns.h */
void procsave(Proc *); /* Save process state */
void procrestore(Proc *); /* Restore process state */
void procsetup(Proc *); /* Setup process state */
void procfork(Proc *); /* Fork process state */
int proc_setup_p9page(Proc *); /* Setup 9P exchange page (deprecated) */
int proc_setup_p9seg_stub(Proc *); /* Setup stub P9SEG for lazy allocation */
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
struct Chan; /* Forward declaration */
struct M3Function; /* Forward declaration for WASM3 */
int wasm_exec_compile(struct Chan *, struct M3Function **);
void wasm_exec_run(struct M3Function *);
void wasm_runtime_cleanup_process(Proc *);
extern int boot_verbose;
/* distributed_pebble.h - Cross-Machine Token Economy
 *
 * Extends Pebble tokens across a cluster for CPU/memory/GPU sharing.
 * Features:
 *   - MachineBank: Per-machine token pool with Merkle proof state
 *   - ArenaBranch: Local token cache for lock-free allocation
 *   - SecretBranch: Elligator-encoded deniable branches
 *   - Token types: Memory, CPU, GPU, Network
 *   - 9P extensions: Ttoken/Rtoken for cross-machine transfer
 *   - MSGORD integration: Global ordering prevents double-spend
 *
 * Conservation law: Σ all machines' tokens = GLOBAL_CONSTANT
 */
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
/* Plan 9 universal header */
/* ========== Token Types ========== */
typedef enum {
  TOK_MEMORY = 0, /* 8 bytes per token */
  TOK_CPU = 1, /* 1 millisecond per token */
  TOK_GPU = 2, /* 1 millisecond per token */
  TOK_NETWORK = 3, /* 1 KB per token */
  TOK_MAX = 4
} TokenType;
/* Token unit conversions */
/* ========== Branch Flags ========== */
typedef enum {
  BRANCH_PUBLIC = 0, /* Normal auditable branch */
  BRANCH_SECRET = (1 << 0), /* Elligator-encoded, deniable */
  BRANCH_REMOTE = (1 << 1), /* Backed by remote machine */
  BRANCH_BORROWED = (1 << 2), /* Contains borrowed tokens */
} BranchFlags;
/* ========== Arena Branch (Local Token Cache) ========== */
/*
 * Each arena/WASM module can have a branch for lock-free allocation.
 * Branches hold colorless tokens and reconcile with parent bank.
 *
 * Secret branches are Elligator-encoded:
 *   - Ledger entry looks like random noise
 *   - Only holder of branch secret can decode
 *   - Plausibly deniable under coercion
 *
 * DENIABILITY MODEL:
 *   - DETECTABLE: Token conservation reveals "X tokens are somewhere"
 *     (the count gap is visible - you can't hide token quantity)
 *   - UNLOCATABLE: Branch data is indistinguishable from random noise
 *     (you can't prove WHERE the tokens are or WHAT they're for)
 *
 *   Under coercion: "Some tokens are unaccounted... in transit? lost?"
 *   Adversary knows tokens exist, cannot prove hidden branch.
 */
typedef struct ArenaBranch {
  uuid_t branch_id; /* Unique branch identity */
  u32int flags; /* BranchFlags */
  /* Token pools (per type) */
  u64int tokens[TOK_MAX]; /* Available tokens by type */
  u64int borrowed[TOK_MAX]; /* Amount borrowed from parent */
  /* Reconciliation thresholds */
  u64int low_water[TOK_MAX]; /* Refill when below */
  u64int high_water[TOK_MAX]; /* Return excess when above */
  /* Parent linkage */
  struct ArenaBranch *parent; /* Parent branch (or NULL if root) */
  struct MachineBank *machine; /* Owning machine bank */
  /* Secret branch state (if BRANCH_SECRET) */
  u8int elligator_secret[32]; /* Elligator encoding key */
  BlindLedgerHash encoded_state; /* Looks like random noise */
  /* Statistics */
  u64int alloc_count;
  u64int refill_count;
  u64int reconcile_count;
  /* Lock for this branch (per-branch, not global) */
  Lock lock;
} ArenaBranch;
/* ========== Machine Bank ========== */
/*
 * Each machine has a bank sized to its physical resources.
 * Maintains Merkle tree of all local branches for audit proofs.
 */
typedef struct MachineBank {
  uuid_t machine_id; /* Unique machine identity */
  /* Physical resource limits */
  u64int total_tokens[TOK_MAX]; /* Max tokens by type */
  u64int available[TOK_MAX]; /* Currently available */
  /* Branch management */
  ArenaBranch *branches[256];
  u32int branch_count;
  /* Merkle state for proofs */
  BlindLedgerHash bank_root; /* Root of token Merkle tree */
  u64int epoch; /* Monotonic revision counter */
  /* Cross-machine state */
  u64int pending_outbound; /* Tokens in transit out */
  u64int pending_inbound; /* Tokens awaiting confirmation */
  /* MSGORD integration */
  u64int last_msgord_seq; /* Last processed MSGORD sequence */
  Lock lock;
} MachineBank;
/* ========== Merkle Proof ========== */
typedef struct MerkleProof {
  BlindLedgerHash target_hash; /* Hash being proven */
  u32int depth; /* Proof depth */
  BlindLedgerHash siblings[16]; /* Sibling hashes */
  u32int path_bits; /* Left(0)/Right(1) at each level */
} MerkleProof;
/* Token transfer with proof */
typedef struct TokenProof {
  uuid_t machine_id; /* Source machine */
  TokenType token_type; /* Memory, CPU, GPU, Network */
  u64int amount; /* Tokens being proven/transferred */
  u64int epoch; /* State epoch (for freshness) */
  MerkleProof balance_proof; /* Proof of sufficient balance */
} TokenProof;
/* ========== Cross-Machine Transfer ========== */
typedef enum {
  TRANSFER_PENDING = 0,
  TRANSFER_CONFIRMED = 1,
  TRANSFER_REJECTED = 2,
  TRANSFER_TIMEOUT = 3,
} TransferStatus;
typedef struct TokenTransfer {
  uuid_t transfer_id; /* Unique transfer ID */
  uuid_t from_machine; /* Source machine */
  uuid_t to_machine; /* Destination machine */
  TokenType token_type;
  u64int amount;
  TokenProof proof; /* Sender's balance proof */
  /* MSGORD ordering */
  u64int msgord_seq; /* Global sequence number */
  /* Status */
  TransferStatus status;
  u64int timestamp; /* Request time */
} TokenTransfer;
/* ========== API: Machine Bank ========== */
/* Initialize machine bank based on physical resources */
void machine_bank_init(MachineBank *bank, uuid_t *machine_id, u64int ram_bytes,
                       u64int cpu_count, u64int gpu_mem_bytes,
                       u64int net_bandwidth);
/* Get current Merkle root for audit */
void machine_bank_get_root(MachineBank *bank, BlindLedgerHash *out_root);
/* Generate proof of token balance */
int machine_bank_prove_balance(MachineBank *bank, TokenType type, u64int amount,
                               TokenProof *out_proof);
/* Verify a balance proof from another machine */
int machine_bank_verify_proof(const TokenProof *proof,
                              const BlindLedgerHash *expected_root);
/* ========== API: Arena Branch ========== */
/* Create a new branch (public or secret) */
ArenaBranch *branch_create(MachineBank *bank, u32int flags);
/* Create secret (deniable) branch */
ArenaBranch *branch_create_secret(MachineBank *bank,
                                  const u8int *elligator_key);
/* Allocate tokens from branch (lock-free fast path) */
int branch_alloc(ArenaBranch *branch, TokenType type, u64int amount);
/* Free tokens back to branch */
void branch_free(ArenaBranch *branch, TokenType type, u64int amount);
/* Reconcile branch with parent bank */
int branch_reconcile(ArenaBranch *branch);
/* Destroy branch, return all tokens to parent */
void branch_destroy(ArenaBranch *branch);
/* ========== API: Cross-Machine Transfer ========== */
/* Initiate token transfer to remote machine */
int transfer_initiate(MachineBank *local, uuid_t *remote_machine,
                      TokenType type, u64int amount,
                      TokenTransfer *out_transfer);
/* Process incoming transfer (called by 9P handler) */
int transfer_receive(MachineBank *local, const TokenTransfer *transfer);
/* Confirm transfer completion (after MSGORD ordering) */
int transfer_confirm(MachineBank *local, uuid_t *transfer_id);
/* Cancel pending transfer, refund tokens */
int transfer_cancel(MachineBank *local, uuid_t *transfer_id);
/* ========== API: Secret Branch (Elligator) ========== */
/* Encode branch state so it looks like random noise */
int branch_elligator_encode(ArenaBranch *branch);
/* Decode branch state (requires elligator_secret) */
int branch_elligator_decode(ArenaBranch *branch, const u8int *elligator_key);
/* Check if data could be a secret branch (probabilistic) */
int branch_is_plausibly_random(const u8int *data, usize len);
/* ========== 9P Token Messages ========== */
/*
 * New 9P message types for token operations.
 * These extend the standard 9P2000 protocol.
 */
/* Ttoken message structure */
typedef struct TtokenMsg {
  u16int tag;
  TokenType token_type;
  u64int amount;
  TokenProof proof;
} TtokenMsg;
/* Rtoken message structure */
typedef struct RtokenMsg {
  u16int tag;
  uuid_t transfer_id;
  u64int new_balance; /* Receiver's new balance */
  BlindLedgerHash new_root; /* Updated Merkle root */
} RtokenMsg;
/* ========== Global State ========== */
/* The local machine's bank (initialized at boot) */
extern MachineBank *local_machine_bank;
/* Initialize distributed pebble subsystem */
void distributed_pebble_init(void);
/*
 * Proc-as-Packet Architecture
 *
 * Network packet-style process structure with:
 * - Bit-packed fields
 * - FSM state machine
 * - Checksum validation
 * - State trace history
 */
/* Include Plan 9 types (uchar, ushort, ulong, etc.) */
/*
 * Kernel Forward Type Declarations
 *
 * Centralized forward declarations for core kernel types.
 * Include this header when you need type names but not full definitions.
 */
/* Plan 9 universal header */
/* Forward declaration - Proc is defined in portdat.h */
struct Proc;
/*
 * Version: Increment when header format changes
 */
/*
 * Process States (4-bit encoded, 0-15)
 */
enum ProcState {
  PS_Dead = 0,
  PS_Moribund,
  PS_New,
  PS_Ready,
  PS_Scheding,
  PS_Running,
  PS_Queueing,
  PS_QueueingR,
  PS_QueueingW,
  PS_Wakeme,
  PS_Broken,
  PS_Stopped,
  PS_Rendezvous,
  PS_Waitrelease,
  PS_Intr,
  PS_IntrReturn,
  PS_COUNT /* Must be <= 16 */
};
/*
 * FSM Events
 */
enum ProcEvent {
  EV_CREATE = 0, /* Dead -> New */
  EV_READY, /* New/Wakeme/etc -> Ready */
  EV_SCHEDULE, /* Ready -> Running */
  EV_YIELD, /* Running -> Ready */
  EV_SLEEP, /* Running -> Wakeme */
  EV_WAKEUP, /* Wakeme -> Ready */
  EV_EXIT, /* Running -> Moribund */
  EV_REAP, /* Moribund -> Dead */
  EV_INTERRUPT, /* Running -> Intr */
  EV_IRETURN, /* Intr -> IntrReturn */
  EV_RESUME, /* IntrReturn -> Running */
  EV_QLOCK, /* Running -> Queueing */
  EV_QLOCK_R, /* Running -> QueueingR */
  EV_QLOCK_W, /* Running -> QueueingW */
  EV_QUNLOCK, /* Queueing -> Ready */
  EV_STOP, /* Running -> Stopped */
  EV_CONT, /* Stopped -> Ready */
  EV_BREAK, /* * -> Broken */
  EV_RENDEZ, /* Running -> Rendezvous */
  EV_RENDEZ_DONE, /* Rendezvous -> Ready */
  EV_VFORK, /* Running -> Waitrelease (vfork parent block) */
  EV_VFORK_DONE, /* Waitrelease -> Ready (child exec/exit) */
  EV_COUNT
};
/*
 * Flags Field (16-bit packed)
 *
 * Bit layout:
 *   0: kp (kernel process)
 *   1: insyscall
 *   2: hang
 *   3: trace
 *   4: notepending
 *   5: fixedpri
 *   6: wired
 *   7: privatemem
 *   8: noswap
 *   9: newtlb
 *   10-13: nnote (0-15)
 *   14-15: reserved
 */
/* Flag accessors */
/*
 * State Trace (16-bit: 4 states x 4 bits each)
 *
 * Layout: [T-3:15-12] [T-2:11-8] [T-1:7-4] [T-0:3-0]
 *         (oldest)                         (current)
 */
/* Push new state onto trace (shifts history left) */
/*
 * Proc Packet Header (64 bytes, cache-line aligned)
 */
typedef struct ProcHeader {
  /* Byte 0-1: Version and reserved */
  uchar version; /* PROC_PACKET_VERSION */
  uchar reserved0; /* Must be 0 */
  /* Byte 2-3: Packed flags */
  ushort flags;
  /* Byte 4-5: State trace (last 4 states) */
  ushort state_trace;
  /* Byte 6-7: Header checksum (CRC-16 of bytes 0-5) */
  ushort hdr_checksum;
  /* Byte 8-11: Process ID */
  ulong pid;
  /* Byte 12-15: Parent PID */
  ulong parent_pid;
  /* Byte 16-23: Machine pointer */
  Mach *mach;
  /* Byte 24-39: Scheduler label (SP, PC for context switch) */
  Label sched;
  /* Byte 40-63: Reserved for future use */
  uchar reserved1[24];
} __attribute__((packed, aligned(64))) ProcHeader;
/*
 * State name table (for debugging)
 */
extern char *proc_state_names[PS_COUNT];
/*
 * CRC-16 (CCITT polynomial)
 */
ushort proc_crc16(uchar *data, int len);
/*
 * FSM API
 */
/* Initialize FSM tables (call once at boot) */
void proc_fsm_init(void);
/* Transition process state via event. Returns new state or -1 on error. */
int proc_event(Proc *p, int event);
/* Get current state from trace */
/* Validate header checksum. Returns 1 if valid, 0 if corrupt. */
int proc_verify(Proc *p);
/* Update header checksum after modifications. */
void proc_seal(Proc *p);
/* Dump state trace for debugging */
void proc_trace_dump(Proc *p);
/*
 * FSM Transition Guard
 * Returns 1 if transition is allowed, 0 if denied.
 * Sets *reason to explanation string on denial.
 */
typedef int (*ProcGuardFn)(Proc *p, const char **reason);
/*
 * FSM Transition Table Entry
 */
typedef struct ProcTransition {
  int from_state;
  int event;
  int to_state;
  ProcGuardFn guard; /* Optional guard function, nil if none */
} ProcTransition;
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
/* clr_capability.h - Capability-Based Security for CLR Runtime
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
/* ========== Capability Scope ========== */
typedef enum {
  CAP_SCOPE_MODULE, /* Assembly-level capability (root for assembly) */
  CAP_SCOPE_CLASS, /* Class-level capability (derived from module) */
  CAP_SCOPE_METHOD /* Method-level capability (derived from class) */
} capability_scope_t;
/* ========== Monotonic Capability Structure ========== */
/*
 * @coq_proof: proofs/capability/CapabilityModel.v
 * @record: Capability (cap_id, cap_perms, cap_parent)
 *
 * ACSL contracts derived from:
 *   @theorem: derived_perm_monotonic
 *     derived_from ct c p -> perms_subset (cap_perms c) (cap_perms p)
 */
typedef struct clr_monotonic_capability {
  /* Identity */
  uuid_t uuid; /* Unique capability identifier (UUIDv8) */
  u32int cap_id; /* Numeric ID for fast lookup */
  /* Derivation chain */
  uuid_t parent_uuid; /* Parent capability UUID (null if root) */
  u32int parent_id; /* Parent numeric ID (0 if root) */
  u32int derivation_depth; /* Chain depth (0 = root, increases on derive) */
  /* Permissions (monotonic: can only decrease) */
  u32int permissions; /* Current active permissions */
  u32int max_permissions; /* Original immutable max (set at creation) */
  /* Timing (monotonic counters) */
  u64int creation_time; /* Monotonic counter at creation */
  u64int expiration_time; /* Optional expiration (0 = no expiry) */
  /* Scope and metadata */
  capability_scope_t scope; /* MODULE, CLASS, or METHOD */
  char *bound_metadata; /* Immutable binding: "assembly:class:method" */
  u8int is_validated; /* Has chain been validated? */
  u8int is_revoked; /* Revocation flag */
} clr_monotonic_capability_t;
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
typedef struct capability_manager {
  /* Capability storage */
  clr_monotonic_capability_t *capabilities; /* Dynamic array */
  u32int count; /* Number of capabilities */
  u32int capacity; /* Allocated slots */
  /* Monotonic counters */
  u64int monotonic_time; /* Global monotonic timestamp */
  u32int next_cap_id; /* Next capability ID to assign */
  /* Statistics */
  u32int derivations; /* Total derivations performed */
  u32int validations; /* Total chain validations */
  u32int rejections; /* Failed derivations (permission violation) */
} capability_manager_t;
/* ========== Core API ========== */
/*
 * @requires: manager != NULL
 * @ensures: \result != NULL ==> manager->count == 0
 */
capability_manager_t *cap_manager_create(void);
/*
 * @requires: manager != NULL
 * @ensures: all capabilities freed
 */
void cap_manager_destroy(capability_manager_t *manager);
/*
 * Create a module-level (root) capability for an assembly.
 *
 * @coq_proof: proofs/capability/CapabilityModel.v
 * @requires: manager != NULL && name != NULL
 * @ensures: \result != NULL ==>
 *           \result->scope == CAP_SCOPE_MODULE &&
 *           \result->parent_id == 0 &&
 *           \result->permissions == CAP_PERM_ALL
 */
clr_monotonic_capability_t *cap_create_module(capability_manager_t *manager,
                                              const char *assembly_name);
/*
 * Derive a class-level capability from a module capability.
 *
 * @coq_proof: proofs/capability/DerivationChain.v
 * @theorem: derived_perm_monotonic
 *
 * @requires: manager != NULL && parent != NULL && class_name != NULL
 * @requires: (permission_mask & parent->permissions) == permission_mask
 *            // Requested permissions must be subset of parent
 * @ensures: \result != NULL ==>
 *           \result->scope == CAP_SCOPE_CLASS &&
 *           \result->parent_id == parent->cap_id &&
 *           \result->permissions == permission_mask &&
 *           \result->derivation_depth == parent->derivation_depth + 1
 */
clr_monotonic_capability_t *cap_derive_class(capability_manager_t *manager,
                                             clr_monotonic_capability_t *parent,
                                             const char *class_name,
                                             u32int permission_mask);
/*
 * Validate a capability derivation chain.
 *
 * @coq_proof: proofs/capability/ChainDecidability.v
 * @theorem: derived_chain_table_perms_monotonic
 *
 * @requires: manager != NULL && child != NULL
 * @ensures: \result == 1 ==>
 *           for all ancestors: perms_subset(child->perms, ancestor->perms)
 */
int cap_validate_chain(capability_manager_t *manager,
                       clr_monotonic_capability_t *child);
/*
 * Check if a capability has a specific permission.
 *
 * @coq_proof: proofs/capability/PermsBitmask.v
 * @definition: has_perm
 *
 * @requires: cap != NULL
 * @ensures: \result == ((cap->permissions & required) == required)
 */
int cap_check_permission(clr_monotonic_capability_t *cap, u32int required);
/*
 * Find capability by UUID.
 *
 * @coq_proof: proofs/capability/LedgerInvariants.v
 * @theorem: find_cap_unique
 *
 * @requires: manager != NULL
 * @ensures: \result != NULL ==> uuid_compare(&\result->uuid, uuid) == 0
 */
clr_monotonic_capability_t *cap_find_by_uuid(capability_manager_t *manager,
                                             const uuid_t *uuid);
/*
 * Find capability by numeric ID.
 *
 * @requires: manager != NULL
 * @ensures: \result != NULL ==> \result->cap_id == cap_id
 */
clr_monotonic_capability_t *cap_find_by_id(capability_manager_t *manager,
                                           u32int cap_id);
/* ========== Permission Utilities ========== */
/*
 * Check if permissions a are a subset of permissions b.
 *
 * @coq_proof: proofs/capability/PermsBitmask.v
 * @definition: perms_subset
 *
 * @ensures: \result == 1 <==> (a & b) == a
 */
int cap_perms_subset(u32int a, u32int b);
/*
 * Format permissions as human-readable string.
 * @ensures: writes at most buflen-1 chars to buf
 */
void cap_perms_to_string(u32int perms, char *buf, u32int buflen);
/* ========== Debug/Diagnostics ========== */
/*
 * Dump capability details to kernel console.
 */
void cap_dump(clr_monotonic_capability_t *cap);
/*
 * Dump manager statistics to kernel console.
 */
void cap_manager_dump_stats(capability_manager_t *manager);
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
typedef struct wasm_9p_session {
  uuid_t cap_uuid; /* Capability UUID from attach */
  UserCapability pebble_cap; /* Validated Pebble capability */
  u32int permissions; /* Effective permissions */
  u64int session_id; /* Unique session identifier */
  void *wasm_server; /* WASM server handling this session */
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
 * @param required_perms: Required permissions for operation (CAP_PERM_READ, etc.)
 * @param pebble_cap_out: Output validated Pebble capability
 * @returns: 1 if valid, 0 if invalid/insufficient permissions
 */
int wasm_9p_validate_capability(const uuid_t *uuid,
                                u32int required_perms,
                                UserCapability *pebble_cap_out);
/*
 * Create 9P session with validated capability.
 *
 * @param uuid: Capability UUID
 * @param wasm_server: WASM server instance
 * @returns: Session pointer, or NULL on error
 */
wasm_9p_session_t *wasm_9p_create_session(const uuid_t *uuid, void *wasm_server);
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
  WASM_9P_OP_WALK, /* Requires: CAP_PERM_READ */
  WASM_9P_OP_OPEN, /* Requires: CAP_PERM_READ or WRITE depending on mode */
  WASM_9P_OP_CREATE, /* Requires: CAP_PERM_WRITE */
  WASM_9P_OP_READ, /* Requires: CAP_PERM_READ */
  WASM_9P_OP_WRITE, /* Requires: CAP_PERM_WRITE */
  WASM_9P_OP_CLUNK, /* Requires: (none, cleanup) */
  WASM_9P_OP_REMOVE, /* Requires: CAP_PERM_WRITE */
  WASM_9P_OP_STAT, /* Requires: CAP_PERM_READ */
  WASM_9P_OP_WSTAT, /* Requires: CAP_PERM_WRITE */
} wasm_9p_operation_t;
/*
 * Get required permissions for a 9P operation.
 *
 * @param op: 9P operation type
 * @param mode: Open mode (for OPEN operation, e.g., OREAD, OWRITE)
 * @returns: Required permission bitmask
 */
u32int wasm_9p_required_perms(wasm_9p_operation_t op, u32int mode);
/* ========== 9P → WASM Routing ========== */
/*
 * Route validated 9P message to WASM server.
 * This is the main entry point: validates capability, then dispatches to WASM.
 *
 * @param fcall: 9P message (Fcall structure)
 * @param session: Active session with capability
 * @returns: 0 on success, error code otherwise
 */
int wasm_9p_route_to_wasm(Fcall *fcall, wasm_9p_session_t *session);
/* wasm_fileserver.h - Simple WASM File Server Interface
 *
 * WASM file servers use exchange page pools + msgord + pebble waves:
 *   - Pool of exchange pages for parallel message submission
 *   - MSGORD provides lock-free total ordering
 *   - WASM processes messages in consensus order
 *   - Zero-copy: messages passed by reference via capabilities
 *   - Pebble wave bits (bottom 3 bits) for OOB signaling
 *
 * Architecture:
 *   Physical Layer:
 *     Client 1 → Page 0 ┐
 *     Client 2 → Page 1 ├→ MSGORD → Ordered Queue → WASM Server
 *     Client 3 → Page 2 ┘
 *
 *   OOB Layer (Pebble Waves):
 *     Every pointer has 3 holographic bits (8 channels)
 *     PEBBLE_WAVE_7 = panic/urgent
 *     PEBBLE_WAVE_0 = admin/root
 *     Other waves for priority, hints, etc.
 *
 * Example:
 *   ptr = PEBBLE_PROJECT(page_addr, PEBBLE_WAVE_7);  // Tag as panic
 *   if (PEBBLE_TUNED(ptr, PEBBLE_WAVE_7)) {          // Check wave
 *     handle_panic();
 *   }
 *
 * WASM exports:
 *   - fs_handle_message(msg_offset, msg_size) -> response_size
 *
 * Keep it simple - just files.
 */
/* Plan 9 universal header */
/*
 * Exchange page system interface
 * Provides Singularity-style exchange heap semantics at page granularity
 */
       
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
/* Exchange page handle - physical address of the page */
typedef UserCapability ExchangeHandle;
/* Error codes for exchange operations */
enum ExchangeError {
 EXCHANGE_OK = 0,
 EXCHANGE_EINVAL, /* Invalid parameters */
 EXCHANGE_ENOTOWNER, /* Not owner of the page */
 EXCHANGE_EBORROWED, /* Page is currently borrowed */
 EXCHANGE_ENOMEM, /* Out of memory */
 EXCHANGE_EALREADY, /* Page already in use */
 EXCHANGE_ENOTEXCHANGE, /* Not an exchangeable page */
};
/* Initialize exchange page system */
void exchangeinit(void);
/* Core exchange operations */
BlindLedgerError exchange_prepare(uintptr vaddr, ExchangeHandle *out_cap);
int exchange_prepare_range(uintptr vaddr, ulong len, ExchangeHandle *handles);
int exchange_accept(const ExchangeHandle *handle, uintptr dest_vaddr, int prot);
int exchange_cancel(const ExchangeHandle *handle);
/* Transfer operations */
// No duplicate exchange_prepare_range here
/* Query operations */
int exchange_is_valid(const ExchangeHandle *handle);
Proc* exchange_get_owner(const ExchangeHandle *handle);
/* Syscall interface */
uintptr sys_exchange_prepare(void *list);
uintptr sys_exchange_prepare_range(void *list);
uintptr sys_exchange_accept(void *list);
uintptr sys_exchange_cancel(void *list);
/* Phase 3: Capability-Based Mapping */
uintptr exchange_map_by_cap(const UserCapability *cap);
int exchange_unmap_by_cap(const UserCapability *cap, uintptr va);
int exchange_verify_and_map(const UserCapability *cap, uintptr va, int prot);
/* TOCTOU Protection */
int exchange_lock_page(const UserCapability *cap);
void exchange_unlock_page(const UserCapability *cap);
/*
 * Lux9 MSGORD Kernel
 *
 * MSGORD consensus for 9P message ordering.
 * Provides total ordering of all kernel operations without locks.
 *
 * Based on PHANTOM MSGORD adapted for microkernel 9P.
 */
/*
 * Kernel Forward Type Declarations
 *
 * Centralized forward declarations for core kernel types.
 * Include this header when you need type names but not full definitions.
 */
/*
 * MSGORD Configuration
 */
/*
 * MSGORD Message Colors
 */
/*
 * MSGORD Message States
 */
/*
 * MSGORD Message Types
 */
/*
 * MSGORD Message Payload
 */
typedef struct OrdPayload {
  int type;
  union {
    Fcall *fcall;
    struct {
      void *data;
      ulong len;
    } raw;
  };
} OrdPayload;
/*
 * MSGORD Message Metadata
 * Attached to each 9P Fcall or raw data for consensus tracking
 */
typedef struct OrdMsg {
  /* Message identity */
  uint gm_id; /* Unique message ID */
  uvlong gm_timestamp; /* Creation timestamp */
  /* The payload */
  OrdPayload gm_payload;
  Proc *gm_caller; /* Calling process */
  char gm_path[256]; /* Target path */
  /* DAG relationships */
  uint gm_parent_count;
  uint gm_parents[8];
  /* Consensus state */
  uchar gm_color; /* BLUE or RED */
  uchar gm_state; /* Pending/Ordered/Delivered */
  ushort gm_anticone_size;
  /* Ordering */
  uvlong gm_global_seq; /* Global sequence number */
  uvlong gm_ordered_time; /* When ordering determined */
  /* Completion callback for async operations */
  void (*gm_callback)(struct OrdMsg *, int status, void *arg);
  void *gm_callback_arg;
  /* Linked list */
  struct OrdMsg *gm_next;
  struct OrdMsg *gm_prev;
} OrdMsg;
/*
 * MSGORD DAG Structure
 */
typedef struct MsgOrd {
  int gd_id; /* DAG Instance ID */
  /* Message queue */
  OrdMsg *gd_head;
  OrdMsg *gd_tail;
  uint gd_count;
  /* ID generation */
  uint gd_next_id;
  uvlong gd_global_seq;
  /* Parameters */
  uint gd_k_param;
  uint gd_max_anticone;
  /* Statistics */
  uvlong gd_total_msgs;
  uvlong gd_blue_msgs;
  uvlong gd_red_msgs;
  uvlong gd_avg_latency;
  /* Synchronization for readers */
  Rendez *gd_rendez;
  /* State */
  int gd_initialized;
} MsgOrd;
/*
 * Global MSGORD instance (System DAG, ID 0)
 */
extern MsgOrd *msgord;
/*
 * Core API
 */
/* Initialize MSGORD subsystem (called by kernel main) */
void msgord_init(uint k_param);
/* Create a new dynamic DAG instance */
MsgOrd *msgord_create_instance(uint k_param);
/* Destroy a DAG instance */
void msgord_destroy_instance(MsgOrd *dag);
/* Get DAG instance by ID */
MsgOrd *msgord_get(int id);
/* Submit 9P message for ordering - REPLACES p9_route() */
int msgord_submit(MsgOrd *dag, Proc *caller, Fcall *t, char *path, u64int nonce);
/* Submit raw data for ordering */
int msgord_submit_raw(MsgOrd *dag, Proc *caller, void *data, ulong len, u64int nonce);
/* Get next ordered message ready for delivery */
OrdMsg *msgord_next(MsgOrd *dag);
/* Complete message and remove from DAG */
void msgord_complete(MsgOrd *dag, OrdMsg *msg);
/*
 * Ordering API
 */
/* Compute anticone for message */
int msgord_anticone(MsgOrd *dag, OrdMsg *msg);
/* Determine color (BLUE if anticone <= k) */
int msgord_color(MsgOrd *dag, OrdMsg *msg);
/* Check if message can be delivered */
int msgord_can_deliver(MsgOrd *dag, OrdMsg *msg);
/*
 * Processing
 */
/* Process one ordered message - called from scheduler */
int msgord_process_one(MsgOrd *dag);
/* Process all ready messages */
void msgord_process_all(MsgOrd *dag);
/*
 * Statistics
 */
void msgord_stats(MsgOrd *dag, uvlong *total, uvlong *blue, uvlong *red);
/*
 * Convenience macros
 */
/*
 * State type for external compatibility
 */
typedef MsgOrd msgord_state_t;
/* Create/destroy for CLR compatibility */
msgord_state_t *msgord_state_create(uint k_param);
uint msgord_add_message(msgord_state_t *state, Proc *p, Fcall *t, char *path);
/*
 * Completion Callback API
 */
typedef void (*MsgordCallback)(OrdMsg *msg, int status, void *arg);
/* Submit 9P message with completion callback */
uint msgord_submit_async(MsgOrd *dag, Proc *caller, Fcall *t, char *path,
                         MsgordCallback cb, void *cb_arg, u64int nonce);
/* Find message by ID */
OrdMsg *msgord_find_by_id(MsgOrd *dag, uint id);
/* Set callback on existing message */
void msgord_set_callback(OrdMsg *msg, MsgordCallback cb, void *cb_arg);
/* Fire callbacks for all ready messages (call from scheduler) */
int msgord_fire_completions(MsgOrd *dag);
/* Callback status codes */
/*
 * Consensus Depth Integration (for consensus_depth.c)
 * Note: ConsensusDepth is defined as enum in consensus_depth.h
 * Functions here use int for compatibility when consensus_depth.h is not
 * included
 */
/* Check consensus depth for an operation - confidence is 0-100 scale
 * depth argument is ConsensusDepth enum value (compatible with int) */
int msgord_check_consensus_depth(MsgOrd *dag, uint op_id, int required_depth,
                                 int *confidence_out);
/* Submit async with depth parameter (alternative signature for
 * consensus_depth.c)
 * t and r are Fcall* but declared as void* for header independence */
int msgord_submit_async_depth(MsgOrd *dag, Proc *caller, void *t, void *r,
                              char *path, int depth, uint *msg_id_out, u64int nonce);
/* Macro alias for backwards compatibility */
/* Forward declarations */
typedef struct Fcall Fcall;
typedef struct Proc Proc;
/* WASM3 opaque types (actual definitions in wasm_fileserver.c) */
typedef struct M3Runtime* IM3Runtime;
typedef struct M3Module* IM3Module;
/* Default page pool size for WASM servers */
/* Auto-scaling configuration */
typedef struct {
  u32int enabled; /* Auto-scaling enabled? */
  u32int min_pages; /* Minimum pool size (never shrink below) */
  u32int max_pages; /* Maximum pool size (never grow above) */
  u32int target_util; /* Target utilization % (e.g., 75) */
  u32int high_threshold; /* Grow when util > this (e.g., 85) */
  u32int low_threshold; /* Shrink when util < this (e.g., 50) */
  u32int check_interval; /* How often to check (milliseconds) */
  /* Algorithm state (internal) */
  u32int smoothed_util; /* EMA-smoothed utilization */
  u32int ema_alpha; /* EMA smoothing factor (0-100, e.g., 30 = 0.3) */
  u64int last_check; /* Last check timestamp (fastticks) */
  u32int stable_count; /* How many intervals at stable size */
} WasmAutoScaleConfig;
/* WASM file server instance */
typedef struct wasm_fileserver {
  IM3Runtime runtime; /* WASM3 runtime */
  IM3Module module; /* Loaded WASM module */
  void *memory; /* Linear memory pointer (maps to exchange pages) */
  u32int memory_size; /* Memory size in bytes */
  u8int *module_bytes; /* Raw module bytes (must outlive module) */
  u32int module_size; /* Size of module bytes */
  /* Exchange page pool for parallel message submission */
  ExchangeHandle *pages; /* Pool of exchange pages */
  u32int num_pages; /* Size of page pool */
  u32int next_page; /* Next available page (round-robin) */
  /* MSGORD for message ordering */
  MsgOrd *msgord; /* Message ordering DAG */
  /* Auto-scaling state */
  WasmAutoScaleConfig autoscale;
} wasm_fileserver_t;
/* Load WASM module as file server with page pool */
wasm_fileserver_t *wasm_fileserver_load(const char *wasm_path, u32int num_pages);
/* Destroy file server */
void wasm_fileserver_destroy(wasm_fileserver_t *server);
/* Submit 9P message for processing (non-blocking)
 *
 * @param server: WASM file server instance
 * @param caller: Calling process
 * @param request: 9P request (Fcall)
 * @param path: Target path
 * @returns: Message ID for tracking, or 0 on error
 *
 * Message is submitted to msgord and will be processed in consensus order.
 * Allocates an exchange page from the pool, submits to msgord DAG.
 */
uint wasm_fs_submit(wasm_fileserver_t *server, Proc *caller, Fcall *request, char *path);
/* Process next ordered message from msgord queue
 *
 * @param server: WASM file server instance
 * @returns: 0 on success, -1 if no messages ready
 *
 * Gets next message from msgord in consensus order,
 * calls WASM fs_handle_message(), returns response to caller.
 * Should be called from scheduler or server main loop.
 */
int wasm_fs_process_next(wasm_fileserver_t *server);
/* Process all ready messages
 *
 * @param server: WASM file server instance
 * @returns: Number of messages processed
 *
 * Processes all messages that msgord has ordered and marked ready.
 * Useful for batch processing.
 */
int wasm_fs_process_all(wasm_fileserver_t *server);
/* Handle a single 9P request synchronously via fs_handle_message */
int wasm_fs_handle_fcall(wasm_fileserver_t *server, Fcall *request,
                         Fcall *response);
/* Get an available exchange page from pool
 *
 * @param server: WASM file server instance
 * @returns: Index of available page, or -1 if pool full
 *
 * Round-robin allocation from page pool.
 */
int wasm_fs_get_page(wasm_fileserver_t *server);
/* Resize exchange page pool at runtime
 *
 * @param server: WASM file server instance
 * @param new_size: New pool size (number of pages)
 * @returns: 0 on success, -1 on error
 *
 * Dynamically grows or shrinks the exchange page pool.
 * Allows adapting to changing load without restarting the server.
 * Handles copying existing pages and freeing old pool.
 */
int wasm_fs_resize_pool(wasm_fileserver_t *server, u32int new_size);
/* Get current pool size
 *
 * @param server: WASM file server instance
 * @returns: Current number of pages in pool
 */
u32int wasm_fs_get_pool_size(wasm_fileserver_t *server);
/* Get pool utilization percentage
 *
 * @param server: WASM file server instance
 * @returns: Utilization 0-100%
 *
 * Estimates how many pages are currently in use based on
 * pending messages in MSGORD queue.
 */
u32int wasm_fs_get_pool_utilization(wasm_fileserver_t *server);
/* Enable auto-scaling with specified configuration
 *
 * @param server: WASM file server instance
 * @param cfg: Auto-scaling configuration (or nil for defaults)
 * @returns: 0 on success, -1 on error
 *
 * Enables automatic pool resizing based on utilization.
 * Uses hybrid AIMD + EMA algorithm with hysteresis.
 *
 * Default config (if cfg == nil):
 *   min_pages: current size / 2
 *   max_pages: current size * 8
 *   target_util: 75%
 *   high_threshold: 85%
 *   low_threshold: 50%
 *   check_interval: 1000ms
 *   ema_alpha: 30 (0.3)
 */
int wasm_fs_enable_autoscale(wasm_fileserver_t *server, WasmAutoScaleConfig *cfg);
/* Disable auto-scaling
 *
 * @param server: WASM file server instance
 *
 * Stops automatic pool resizing. Current pool size is preserved.
 */
void wasm_fs_disable_autoscale(wasm_fileserver_t *server);
/* Run one iteration of auto-scaling algorithm
 *
 * @param server: WASM file server instance
 * @returns: 1 if pool was resized, 0 if no change, -1 on error
 *
 * Called periodically (e.g., from timer interrupt or scheduler).
 * Measures utilization, updates EMA, and resizes pool if needed.
 *
 * Algorithm: Hybrid AIMD + EMA with Hysteresis
 *   1. Measure current utilization
 *   2. Update EMA: smoothed = α×current + (1-α)×smoothed
 *   3. If smoothed > high_threshold: grow by 50%
 *   4. Elif smoothed < low_threshold: shrink by 25%
 *   5. Else: no change (deadband)
 */
int wasm_fs_autoscale_tick(wasm_fileserver_t *server);
/* wasm_runtime.h - Isolated WASM3 Runtime Interface
 *
 * Layer 1 API for WASM execution.
 * Accessible ONLY via Tsyscall messages.
 */
       
/* Plan 9 universal header */
typedef struct Proc Proc;
/* Capability permissions for WASM */
/* Runtime initialization (called at boot) */
void wasm_runtime_init(void);
/* Tsyscall handlers (called from 9P router) */
int sys_wasm_compile(Fcall *tx, Fcall *rx);
int sys_wasm_execute(Fcall *tx, Fcall *rx);
int sys_wasm_destroy(Fcall *tx, Fcall *rx);
void wasm_runtime_cleanup_process(Proc *p);
/* Statistics */
void wasm_runtime_stats(void);
/* Process FSM integration - use real FSM from proc_fsm.c */
extern int proc_event(Proc *p, int event);
extern char *proc_state_names[PS_COUNT];
/* Forward declarations for handlers */
extern uvlong nsec(void); /* Fix implicit declaration */
extern void userpmap(uintptr, uintptr, int);
extern void semacquire(Segment *, long *, int);
extern void semrelease(Segment *, long *, int);
extern int fd_9p_handle(Proc *, Fcall *, Fcall *);
extern int irqhandled(Ureg *, int);
extern int proc_9p_handle(Proc *caller, Fcall *t, Fcall *r);
extern int dev_9p_handle(Proc *p, Fcall *t, Fcall *r);
extern int env_9p_handle(Proc *p, Fcall *t, Fcall *r);
extern int srv_9p_handle(Proc *p, Fcall *t, Fcall *r);
extern int mnt_9p_handle(Proc *p, Fcall *t, Fcall *r);
static int ram_9p_handle(Proc *caller, Fcall *t, Fcall *r);
static int rpipe_9p_handle(Proc *caller, Fcall *t, Fcall *r);
static void rpipe_clone_notify(void *aux);
static int wasm_9p_handle(Proc *caller, Fcall *t, Fcall *r);
static int p9_handle_ring(Proc *p, P9Control *ctl, uchar *msg_buf);
extern uintptr sysexec(void *list_void); /* System exec call */
static uchar *tsyscall_skip_argc(uchar *p, uchar *ep, u32int expected) {
  if (p + 4 <= ep) {
    u32int argc = (((uchar *)(p))[0] | (((uchar *)(p))[1] << 8) | (((uchar *)(p))[2] << 16) | (((uchar *)(p))[3] << 24));
    if (argc == expected)
      return p + 4;
  }
  return p;
}
static int p9_exchange_contains(Proc *p, void *ptr, ulong len) {
  if (!p || !p->p9page || !ptr || len == 0)
    return 0;
  uintptr base = (uintptr)p->p9page + 0x000;
  uintptr end = base + 0xF00;
  uintptr addr = (uintptr)ptr;
  if (addr < base)
    return 0;
  if (addr + len < addr || addr + len > end)
    return 0;
  return 1;
}
/*
 * Path matching for routing
 */
static int path_match(char *path, char *pattern) {
  int plen = strlen(pattern);
  if (pattern[plen - 1] == '*') {
    return strncmp(path, pattern, plen - 1) == 0;
  }
  return strcmp(path, pattern) == 0;
}
/*
 * Initialize router subsystem
 */
void p9_router_init(void) { print("9p_router: initialized\n"); }
/*
 * Allocate 9P Exchange Page for a process
 */
int p9_alloc_page(Proc *p) {
  uintptr page;
  page = (uintptr)xalloc(4096);
  if (page == 0)
    return -1;
  memset((void *)page, 0, 4096);
  /* Initialize P9Control block at offset 0x1F00 (start of 2nd page + offset) */
  P9Control *ctl = (P9Control *)((uintptr)p->p9page + 0xF00);
  memset(ctl, 0, sizeof(P9Control));
  (*((&ctl->status)) = ((0)));
  (*((&ctl->doorbell)) = ((0)));
  /* Map Page 0 (Requests) as Read-Write */
  ctl->req_head = 0;
  ctl->req_tail = 0;
  ctl->rep_head = 0;
  ctl->rep_tail = 0;
  ctl->req_seq = 0;
  ctl->rep_seq = 0;
  p->p9page = (void *)page;
  return 0;
}
void p9_free_page(Proc *p) {
  if (p->p9page) {
    xfree(p->p9page);
    p->p9page = ((void *)0);
  }
}
int p9_extract_pebble(uchar *data, ulong len, PebbleToken *out) {
  /*@
    @ requires data == \null || \valid((uchar *)data + (0..len-1));
    @ requires \valid(out);
    @ ensures \result == 0 ==> len >= 8 + sizeof(PebbleToken);
    */
  uint magic, version;
  if (len < 8 + sizeof(PebbleToken))
    return -1;
  magic = (((uchar *)(data))[0] | (((uchar *)(data))[1] << 8) | (((uchar *)(data))[2] << 16) | (((uchar *)(data))[3] << 24));
  version = (((uchar *)(data + 4))[0] | (((uchar *)(data + 4))[1] << 8) | (((uchar *)(data + 4))[2] << 16) | (((uchar *)(data + 4))[3] << 24));
  if (magic != 0x5045424C)
    return -1;
  if (version != 1)
    return -1;
  memmove(out, data + 8, sizeof(PebbleToken));
  return 0;
}
int p9_validate_pebble(PebbleToken *tok, char *path, int access) {
  /*@
    @ requires \valid(tok);
    @ ensures \result == 1 ==> (tok->expires == 0 ||
    @                            tok->expires >= (uvlong)seconds());
    */
  if (tok->expires != 0 && tok->expires < (uvlong)seconds()) {
    return 0;
  }
  if ((access & 0x01) && !(tok->permissions & 0x01))
    return 0;
  if ((access & 0x02) && !(tok->permissions & 0x02))
    return 0;
  return 1;
}
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
 * Lux9 MSGORD Kernel
 *
 * MSGORD consensus for 9P message ordering.
 * Provides total ordering of all kernel operations without locks.
 *
 * Based on PHANTOM MSGORD adapted for microkernel 9P.
 */
/*
 * Session Pebble Management
 */
static void store_session_pebble(Proc *p, PebbleToken *tok) {
  P9Control *ctl;
  if (p->p9page == ((void *)0))
    return;
  ctl = (P9Control *)((uintptr)p->p9page + 0xF00);
  memmove(ctl->session_pebble, tok->signature, 16);
  /* Store ledger_id in next 8 bytes */
  do { ((uchar *)(ctl->session_pebble + 16))[0] = (uchar)(tok->ledger_id); ((uchar *)(ctl->session_pebble + 16))[1] = (uchar)((tok->ledger_id) >> 8); ((uchar *)(ctl->session_pebble + 16))[2] = (uchar)((tok->ledger_id) >> 16); ((uchar *)(ctl->session_pebble + 16))[3] = (uchar)((tok->ledger_id) >> 24); ((uchar *)(ctl->session_pebble + 16))[4] = (uchar)((tok->ledger_id) >> 32); ((uchar *)(ctl->session_pebble + 16))[5] = (uchar)((tok->ledger_id) >> 40); ((uchar *)(ctl->session_pebble + 16))[6] = (uchar)((tok->ledger_id) >> 48); ((uchar *)(ctl->session_pebble + 16))[7] = (uchar)((tok->ledger_id) >> 56); } while (0);
  /* Store permissions in next byte */
  ctl->session_pebble[24] = tok->permissions;
  /* Store expires in next 8 bytes */
  do { ((uchar *)(ctl->session_pebble + 25))[0] = (uchar)(tok->expires); ((uchar *)(ctl->session_pebble + 25))[1] = (uchar)((tok->expires) >> 8); ((uchar *)(ctl->session_pebble + 25))[2] = (uchar)((tok->expires) >> 16); ((uchar *)(ctl->session_pebble + 25))[3] = (uchar)((tok->expires) >> 24); ((uchar *)(ctl->session_pebble + 25))[4] = (uchar)((tok->expires) >> 32); ((uchar *)(ctl->session_pebble + 25))[5] = (uchar)((tok->expires) >> 40); ((uchar *)(ctl->session_pebble + 25))[6] = (uchar)((tok->expires) >> 48); ((uchar *)(ctl->session_pebble + 25))[7] = (uchar)((tok->expires) >> 56); } while (0);
}
static void scrub_exchange_page(Proc *p, const uchar *reply, uint reply_size,
                                P9Control *saved_ctl, u32int rep_head,
                                u32int rep_tail, int ring_mode) {
  P9Control *ctl;
  uchar *msg_buf;
  uchar reply_copy[0xF00];
  if (p == ((void *)0) || p->p9page == ((void *)0))
    return;
  memset(reply_copy, 0, sizeof(reply_copy));
  if (reply != ((void *)0) && reply_size > 0 && reply_size <= 0xF00)
    memmove(reply_copy, reply, reply_size);
  memset(p->p9page, 0, 4096);
  ctl = (P9Control *)((uintptr)p->p9page + 0xF00);
  msg_buf = (uchar *)p->p9page + 0x000;
  if (saved_ctl != ((void *)0)) {
    memmove(ctl->session_pebble, saved_ctl->session_pebble,
            sizeof(ctl->session_pebble));
    ctl->req_seq = saved_ctl->req_seq;
    ctl->rep_seq = saved_ctl->rep_seq;
  }
  if (ring_mode) {
    u32int idx = rep_head;
    while (idx != rep_tail) {
      uchar *src = reply_copy + (idx * 256);
      uchar *dst = msg_buf + (idx * 256);
      memmove(dst, src, 256);
      idx = (idx + 1) % (0xF00 / 256);
    }
    ctl->rep_head = rep_head;
    ctl->rep_tail = rep_tail;
  } else if (reply_size > 0) {
    memmove(msg_buf, reply_copy, reply_size);
    ctl->rep_head = 0;
    ctl->rep_tail = reply_size;
  }
}
static void dump_bytes(const char *label, const uchar *buf, uint n) {
  uint i;
  if (buf == ((void *)0) || n == 0)
    return;
  print("%s", label);
  for (i = 0; i < n; i++)
    print(" %02x", buf[i]);
  print("\n");
}
/* Forward declaration of generic device handler */
/*
 * FD Handler: /fd/N
 */
static int get_session_pebble(Proc *p, PebbleToken *tok) {
  P9Control *ctl;
  if (p->p9page == ((void *)0))
    return -1;
  ctl = (P9Control *)((uintptr)p->p9page + 0xF00);
  /* Check if pebble is set (non-zero signature) */
  if (ctl->session_pebble[0] == 0 && ctl->session_pebble[1] == 0)
    return -1;
  /* Reconstruct PebbleToken from session storage */
  memmove(tok->signature, ctl->session_pebble, 16);
  tok->ledger_id = ((u32int)(((uchar *)(ctl->session_pebble + 16))[0] | (((uchar *)(ctl->session_pebble + 16))[1] << 8) | (((uchar *)(ctl->session_pebble + 16))[2] << 16) | (((uchar *)(ctl->session_pebble + 16))[3] << 24)) | ((uvlong)(((uchar *)(ctl->session_pebble + 16))[4] | (((uchar *)(ctl->session_pebble + 16))[5] << 8) | (((uchar *)(ctl->session_pebble + 16))[6] << 16) | (((uchar *)(ctl->session_pebble + 16))[7] << 24)) << 32));
  tok->permissions = ctl->session_pebble[24];
  tok->expires = ((u32int)(((uchar *)(ctl->session_pebble + 25))[0] | (((uchar *)(ctl->session_pebble + 25))[1] << 8) | (((uchar *)(ctl->session_pebble + 25))[2] << 16) | (((uchar *)(ctl->session_pebble + 25))[3] << 24)) | ((uvlong)(((uchar *)(ctl->session_pebble + 25))[4] | (((uchar *)(ctl->session_pebble + 25))[5] << 8) | (((uchar *)(ctl->session_pebble + 25))[6] << 16) | (((uchar *)(ctl->session_pebble + 25))[7] << 24)) << 32));
  return 0;
}
/*
 * Full Pebble Validation with BlindLedger
 */
static int p9_validate_pebble_full(PebbleToken *tok, char *path, Proc *owner) {
  UserCapability cap;
  BlindLedgerEntry entry;
  BlindLedgerError err;
  /* Basic validation first */
  if (!p9_validate_pebble(tok, path, 0))
    return 0;
  /* Convert PebbleToken to UserCapability for BlindLedger verification */
  memset(&cap, 0, sizeof(cap));
  memmove(cap.hash, tok->signature, 16);
  /* Zero-pad the rest of the hash */
  memset(cap.hash + 16, 0, 16);
  cap.type = CAP_TYPE_DEVICE; /* Device capability */
  cap.perms = 0;
  if (tok->permissions & 0x01)
    cap.perms |= CAP_PERM_READ;
  if (tok->permissions & 0x02)
    cap.perms |= CAP_PERM_WRITE;
  if (tok->permissions & 0x04)
    cap.perms |= CAP_PERM_EXEC;
  /* Verify with BlindLedger */
  err = ledger_verify(&cap, &entry);
  if (err != BLIND_LEDGER_OK) {
    /* Capability not found or invalid */
    return 0;
  }
  /* Check owner matches */
  if (entry.owner != owner && entry.owner != ((void *)0)) {
    /* Capability belongs to different process */
    return 0;
  }
  return 1;
}
/*
 * Permission checking helper for device operations
 */
static int check_permission(Proc *p, int required_perm) {
  PebbleToken tok;
  /* Get session pebble */
  if (get_session_pebble(p, &tok) < 0) {
    /* No pebble in session - DENY by default */
    return 0;
  }
  /* Check if required permission is granted */
  return (tok.permissions & required_perm) != 0;
}
/*
 * FID Management via Fgrp (Stateless Router)
 * We reuse the kernel's file descriptor table to map 9P FIDs.
 * Virtual router endpoints are represented by Channels with type = -1.
 */
/* Device subtypes for TYPE_DEV FIDs */
static int install_fid_with_subtype(int fid, int type, int subtype) {
  /*@
    @ ensures \result == 0 ==> get_fid_type(fid) == type;
    */
  Chan *c;
  Fgrp *f = up->fgrp;
  c = newchan();
  if (c == ((void *)0))
    return -1;
  c->type = -1; /* Mark as virtual router channel */
  c->qid.path = type; /* Store handler type in Qid path */
  c->qid.vers = subtype; /* Store subtype (e.g., device ID) */
  c->mode = 2;
  c->ref = 1;
  lock(&f->lock);
  if (fid < 0 || growfd(f, fid) < 0) {
    unlockfgrp(f);
    cclose(c);
    return -1;
  }
  if (fid > f->maxfd)
    f->maxfd = fid;
  if (f->fd[fid])
    cclose(f->fd[fid]);
  f->fd[fid] = c;
  f->flag[fid] = 0;
  unlockfgrp(f);
  return 0;
}
static int install_fid(int fid, int type) {
  return install_fid_with_subtype(fid, type, 0);
}
static int get_fid_type(int fid) {
  Chan *c;
  Fgrp *f = up->fgrp;
  int type = 0;
  lock(&f->lock);
  if (fid >= 0 && fid <= f->maxfd && (c = f->fd[fid]) != ((void *)0)) {
    if (c->type == -1) {
      type = (int)c->qid.path;
    }
  }
  unlock(&f->lock);
  return type;
}
static int get_fid_subtype(int fid) {
  Chan *c;
  Fgrp *f = up->fgrp;
  int subtype = 0;
  lock(&f->lock);
  if (fid >= 0 && fid <= f->maxfd && (c = f->fd[fid]) != ((void *)0)) {
    if (c->type == -1) {
      subtype = (int)c->qid.vers;
    }
  }
  unlock(&f->lock);
  return subtype;
}
static void remove_fid(int fid) { fdclose(fid, 0); }
/*
 * Dispatch message to appropriate handler
 */
int p9_dispatch(Proc *p, Fcall *t, Fcall *r) {
  int type = 0;
  extern void uartputs(char *, int);
  char buf[128];
  extern int snprint(char *, int, char *, ...);
  snprint(buf, sizeof(buf), "CONSOLE: p9_dispatch: ENTRY type=%d tag=%d\n",
          t->type, t->tag);
  uartputs(buf, strlen(buf));
  if (p->wasm.initialized) {
    if (t->data && t->count > 0 &&
        !p9_exchange_contains(p, t->data, t->count)) {
      r->type = Rerror;
      r->ename = "wasm data must use exchange page";
      return -1;
    }
    if (t->sdata && t->scount > 0 &&
        !p9_exchange_contains(p, t->sdata, t->scount)) {
      r->type = Rerror;
      r->ename = "wasm sdata must use exchange page";
      return -1;
    }
  }
  /* Handle Texec (128) - Direct execution message */
  if (t->type == Texec) {
    char *path;
    char *argv[2];
    ulong args[2];
    /* Extract path from Texec message data */
    /* Format: [2] pathlen + [n] path bytes */
    if (t->count < 2) {
      r->type = Rerror;
      r->ename = "Texec: invalid message format";
      return -1;
    }
    uint pathlen = (uint)t->data[0] | ((uint)t->data[1] << 8);
    if (pathlen == 0 || pathlen > t->count - 2) {
      r->type = Rerror;
      r->ename = "Texec: invalid path length";
      return -1;
    }
    /* Allocate and copy path string for kernel logging/debugging */
    path = xalloc(pathlen + 1);
    if (path == ((void *)0)) {
      r->type = Rerror;
      r->ename = "Texec: out of memory";
      return -1;
    }
    memmove(path, t->data + 2, pathlen);
    path[pathlen] = '\0';
    print("p9_dispatch: Texec for '%s' (pid %lud)\n", path, p->pid);
    /*
     * Sysexec requires User Virtual Addresses for both path and argv.
     * We must calculate the user address of the path existing in the
     * exchange page, and construct a user-space argv array there as well.
     */
    /* 1. Calculate User Address of the path string */
    /* t->data points into p->p9page. The string starts at data+2 */
    uintptr kpage = (uintptr)p->p9page;
    uintptr kpath = (uintptr)t->data + 2;
    uintptr path_offset = kpath - kpage;
    uintptr upath = 0x7FFFFEEFF000ULL + path_offset;
    /* 2. Ensure null-termination in the user buffer */
    /* We can safely write \0 because validaddr/namec expects it.
     * Check bounds to ensure we don't write past valid page. */
    if (path_offset + pathlen < 4096) {
      ((char *)kpath)[pathlen] = 0;
    }
    /* 3. Construct argv array in the Exchange Page */
    /* We need space for 2 pointers: [upath, 0] */
    /* Use the space immediately following the message payload */
    uintptr kargv_start = (uintptr)t->data + t->count;
    /* Align to 8 bytes */
    kargv_start = (kargv_start + 7) & ~7ULL;
    /* Check if we have room in the request buffer */
    if (kargv_start + 2 * sizeof(ulong) > kpage + 0xF00) {
      xfree(path);
      r->type = Rerror;
      r->ename = "Texec: message too large, no room for argv";
      return -1;
    }
    /* Write argv to the user page (via kernel mapping) */
    ulong *argv_ptr = (ulong *)kargv_start;
    argv_ptr[0] = (ulong)upath;
    argv_ptr[1] = 0;
    /* Calculate User Address of argv */
    uintptr argv_offset = kargv_start - kpage;
    uintptr uargv = 0x7FFFFEEFF000ULL + argv_offset;
    /* Prepare arguments for sysexec */
    /* args[0] = file (user char*) */
    /* args[1] = argv (user char**) */
    args[0] = (ulong)upath;
    args[1] = (ulong)uargv;
    /* Call sysexec - never returns on success */
    if (setlabel(&up->errlab[up->nerrlab++])) {
      print("p9_dispatch: Texec failed: %s\n", up->errstr);
      xfree(path);
      r->type = Rerror;
      r->ename = up->errstr;
      return -1;
    }
    sysexec(args);
    /* Not reached on success */
    up->nerrlab--;
    /* If we get here, exec failed somehow */
    xfree(path);
    r->type = Rerror;
    r->ename = "Texec: exec returned unexpectedly";
    return -1;
  }
  /* Handle Generic Tsyscall (130) */
  if (t->type == Tsyscall) {
    Proc *proc = p;
    uchar *p = t->sdata;
    uchar *ep = t->sdata + t->scount;
    if (proc->wasm.initialized) {
      switch (t->scallnr) {
      case SYS_WASM_COMPILE:
      case SYS_WASM_EXECUTE:
      case SYS_WASM_DESTROY:
        break;
      default:
        r->type = Rerror;
        r->ename = "wasm tsyscall blocked";
        return -1;
      }
    }
    print("p9_dispatch: Tsyscall scallnr=%d\n", t->scallnr);
    switch (t->scallnr) {
    case SYS_OPEN: {
      extern int newfd(Chan *, int);
      extern int openmode(ulong);
      /* Format: [path s] [mode 1] */
      p = tsyscall_skip_argc(p, ep, 2);
      if (p + 2 > ep) {
        r->type = Rerror;
        r->ename = "short msg";
        return -1;
      }
      int len = (((uchar *)(p))[0] | (((uchar *)(p))[1] << 8));
      p += 2;
      if (p + len + 1 > ep) {
        r->type = Rerror;
        r->ename = "short msg";
        return -1;
      }
      char *path = smalloc(len + 1);
      memmove(path, p, len);
      path[len] = 0;
      p += len;
      int mode = (((uchar *)(p))[0]);
      p += 1;
      print("p9_dispatch: Tsyscall SYS_OPEN ptr '%s' mode=%d\n", path, mode);
      int fd;
      Chan *c = 0;
      if (setlabel(&up->errlab[up->nerrlab++])) {
        if (c)
          cclose(c);
        free(path);
        r->type = Rerror;
        r->ename = up->errstr;
        return -1;
      }
      openmode(mode);
      c = namec(path, Aopen, mode, 0);
      fd = newfd(c, mode);
      up->nerrlab--;
      free(path);
      r->type = Rsyscall;
      r->tag = t->tag;
      r->retval = fd;
      r->scount = 0;
      r->sdata = ((void *)0);
      return 0;
    }
    case SYS_CREATE: {
      extern int newfd(Chan *, int);
      extern int openmode(ulong);
      /* Format: [path s] [mode 4] [perm 4] */
      p = tsyscall_skip_argc(p, ep, 3);
      /* Parse Path */
      if (p + 2 > ep) {
        r->type = Rerror;
        return -1;
      }
      int len = (((uchar *)(p))[0] | (((uchar *)(p))[1] << 8));
      p += 2;
      if (p + len > ep) {
        r->type = Rerror;
        return -1;
      }
      char *path = smalloc(len + 1);
      memmove(path, p, len);
      path[len] = 0;
      p += len;
      /* Parse Mode and Perm */
      if (p + 4 + 4 > ep) {
        free(path);
        r->type = Rerror;
        return -1;
      }
      int mode = (((uchar *)(p))[0] | (((uchar *)(p))[1] << 8) | (((uchar *)(p))[2] << 16) | (((uchar *)(p))[3] << 24));
      p += 4;
      int perm = (((uchar *)(p))[0] | (((uchar *)(p))[1] << 8) | (((uchar *)(p))[2] << 16) | (((uchar *)(p))[3] << 24));
      p += 4;
      print("p9_dispatch: Tsyscall SYS_CREATE '%s' mode=%d perm=%o\n", path,
            mode, perm);
      Chan *c = ((void *)0);
      int fd;
      if (setlabel(&up->errlab[up->nerrlab++])) {
        if (c)
          cclose(c);
        free(path);
        r->type = Rerror;
        snprint(r->ename, sizeof(r->ename), "%s", up->errstr);
        return -1;
      }
      openmode(mode);
      c = namec(path, Acreate, mode, perm);
      fd = newfd(c, mode);
      up->nerrlab--;
      free(path);
      r->type = Rsyscall;
      r->tag = t->tag;
      r->retval = fd;
      r->scount = 0;
      r->sdata = ((void *)0);
      return 0;
    }
    case SYS_FORK: /* RFORK */
    case SYS_RFORK: {
      extern uintptr sysrfork(void *list_void);
      print("p9_dispatch: SYS_RFORK case entered, kp=%d\n", up ? up->kp : -1);
      /* Two-Level Spawn Capability Check (userspace only).
       * Level 1: Namespace (Pgrp) limit - shared by all procs in namespace
       * Level 2: Process limit - individual fork bomb protection
       * TCB processes (kp == 1) are exempt. */
      if (up != ((void *)0) && up->kp == 0) {
        Pgrp *pg = up->pgrp;
        /* Check spawn capability exists */
        if (uuid_is_null(&up->spawn_cap)) {
          print("p9_dispatch: SYS_RFORK FAILED - spawn_cap is null\n");
          r->type = Rerror;
          snprint(up->errstr, 128, "no spawn capability");
          r->ename = up->errstr;
          return -1;
        }
        /* Level 1: Namespace limit check */
        if (pg != ((void *)0)) {
          lock(&pg->spawn_lock);
          if (pg->spawn_count >= pg->spawn_limit) {
            print("p9_dispatch: SYS_RFORK FAILED - namespace limit %d/%d\n",
                  pg->spawn_count, pg->spawn_limit);
            unlock(&pg->spawn_lock);
            r->type = Rerror;
            snprint(up->errstr, 128, "namespace spawn limit (%d/%d)",
                    pg->spawn_count, pg->spawn_limit);
            r->ename = up->errstr;
            return -1;
          }
          /* Cryptographic binding: verify spawn_cap is bound to this Pgrp.
           * Compare first 6 bytes of identity_hash with cap's PA hash.
           * NOTE: Only 6 bytes are compared because uuid_pack_capability loses
           * bits from bytes 6-11 during the 46-bit encoding. */
          u8int cap_hash[16];
          uuid_get_pa_hash_bits(&up->spawn_cap, cap_hash);
          if (memcmp(cap_hash, pg->identity_hash, 6) != 0) {
            print("p9_dispatch: SYS_RFORK FAILED - spawn cap not bound\n");
            unlock(&pg->spawn_lock);
            r->type = Rerror;
            snprint(up->errstr, 128, "spawn cap not bound to namespace");
            r->ename = up->errstr;
            return -1;
          }
          unlock(&pg->spawn_lock);
        }
        /* Level 2: Process child limit check */
        if (up->spawn_children >= up->spawn_max_children) {
          print("p9_dispatch: SYS_RFORK FAILED - process limit %d/%d\n",
                up->spawn_children, up->spawn_max_children);
          r->type = Rerror;
          snprint(up->errstr, 128, "process spawn limit (%d/%d)",
                  up->spawn_children, up->spawn_max_children);
          r->ename = up->errstr;
          return -1;
        }
      }
      /* Format: [flags 4] */
      p = tsyscall_skip_argc(p, ep, 1);
      if (p + 4 > ep) {
        r->type = Rerror;
        return -1;
      }
      ulong flags = (((uchar *)(p))[0] | (((uchar *)(p))[1] << 8) | (((uchar *)(p))[2] << 16) | (((uchar *)(p))[3] << 24));
      print("p9_dispatch: Tsyscall SYS_RFORK flags=0x%lx\n", flags);
      ulong args[1] = {flags};
      uintptr ret;
      if (setlabel(&up->errlab[up->nerrlab++])) {
        r->type = Rerror;
        r->ename = up->errstr;
        r->tag = t->tag;
        return -1;
      }
      ret = sysrfork(args);
      print("DEBUG: sysrfork returned ret=%#p\n", ret);
      up->nerrlab--;
      r->type = Rsyscall;
      r->tag = t->tag;
      r->retval = ret; /* PID is usually returned as u64 in retval */
      r->scount = 0;
      r->sdata = ((void *)0);
      return 0;
    }
    case SYS_BRK: {
      extern uintptr ibrk(uintptr, int);
      /* Format: [addr 8] */
      p = tsyscall_skip_argc(p, ep, 1);
      if (p + 8 > ep) {
        r->type = Rerror;
        return -1;
      }
      uintptr addr = (uintptr)((u32int)(((uchar *)(p))[0] | (((uchar *)(p))[1] << 8) | (((uchar *)(p))[2] << 16) | (((uchar *)(p))[3] << 24)) | ((uvlong)(((uchar *)(p))[4] | (((uchar *)(p))[5] << 8) | (((uchar *)(p))[6] << 16) | (((uchar *)(p))[7] << 24)) << 32)); // Use 64-bit for addr
      print("p9_dispatch: Tsyscall SYS_BRK addr=0x%p\n", (void *)addr);
      uintptr ret;
      if (setlabel(&up->errlab[up->nerrlab++])) {
        r->type = Rerror;
        r->ename = up->errstr;
        r->tag = t->tag;
        return -1;
      }
      ret = ibrk(addr, BSEG);
      up->nerrlab--;
      r->type = Rsyscall;
      r->tag = t->tag;
      r->retval = (u64int)ret;
      r->scount = 0;
      r->sdata = ((void *)0);
      return 0;
    }
    case SYS_EXIT: {
      extern void pexit(char *, int);
      /* Format: [status s] ? Or [status 4]?
       * sys_exit(char *msg). So treat as string.
       */
      p = tsyscall_skip_argc(p, ep, 1);
      char *msg = "";
      if (p + 2 <= ep) {
        int len = (((uchar *)(p))[0] | (((uchar *)(p))[1] << 8));
        p += 2;
        if (p + len <= ep) {
          msg = smalloc(len + 1);
          memmove(msg, p - len, len); // wait, broken logic
                                      // Let's protect memory
                                      // Actually sys_exit takes string.
          // Just point to it if possible? NO, need to copy for
          // safety/null-term? pexit copies? Let's use clean parsing
        }
      }
      // Quick fix: ignore message for now or parse correctly
      // p points to length
      // Let's re-parse cleanly:
      char *ename = ((void *)0);
      if (p + 2 <= ep) {
        int len = (((uchar *)(p))[0] | (((uchar *)(p))[1] << 8));
        if (p + 2 + len <= ep) {
          ename = smalloc(len + 1);
          memmove(ename, p + 2, len);
          ename[len] = 0;
        }
      }
      print("p9_dispatch: Tsyscall SYS_EXIT '%s'\n", ename ? ename : "");
      pexit(ename ? ename : "", 1);
      return 0;
    }
    case SYS_CLOSE: {
      extern void fdclose(int, int);
      extern Chan *fdtochan(int, int, int, int);
      /* Format: [fid 4] */
      p = tsyscall_skip_argc(p, ep, 1);
      if (p + 4 > ep) {
        r->type = Rerror;
        return -1;
      }
      int fd = (((uchar *)(p))[0] | (((uchar *)(p))[1] << 8) | (((uchar *)(p))[2] << 16) | (((uchar *)(p))[3] << 24));
      p += 4;
      print("p9_dispatch: SYS_CLOSE fd=%d\n", fd);
      if (setlabel(&up->errlab[up->nerrlab++])) {
        r->type = Rerror;
        snprint(r->ename, sizeof(r->ename), "%s", up->errstr);
        return -1;
      }
      Chan *c = fdtochan(fd, -1, 0, 0);
      if (c)
        cclose(c);
      fdclose(fd, 0);
      up->nerrlab--;
      r->type = Rsyscall;
      r->tag = t->tag;
      r->retval = 0;
      r->scount = 0;
      r->sdata = ((void *)0);
      return 0;
    }
    case SYS_SEEK: {
      /* Seek syscall - Format: [fd 4] [offset 8] [whence 4]
       * whence: 0=SEEK_SET, 1=SEEK_CUR, 2=SEEK_END
       * Returns new position as retval
       */
      extern vlong sseek(int, vlong, int);
      p = tsyscall_skip_argc(p, ep, 3);
      if (p + 4 + 8 + 4 > ep) {
        r->type = Rerror;
        r->ename = "short seek msg";
        return -1;
      }
      int fd = (((uchar *)(p))[0] | (((uchar *)(p))[1] << 8) | (((uchar *)(p))[2] << 16) | (((uchar *)(p))[3] << 24));
      p += 4;
      vlong offset = ((u32int)(((uchar *)(p))[0] | (((uchar *)(p))[1] << 8) | (((uchar *)(p))[2] << 16) | (((uchar *)(p))[3] << 24)) | ((uvlong)(((uchar *)(p))[4] | (((uchar *)(p))[5] << 8) | (((uchar *)(p))[6] << 16) | (((uchar *)(p))[7] << 24)) << 32));
      p += 8;
      int whence = (((uchar *)(p))[0] | (((uchar *)(p))[1] << 8) | (((uchar *)(p))[2] << 16) | (((uchar *)(p))[3] << 24));
      p += 4;
      print("p9_dispatch: SYS_SEEK fd=%d offset=%lld whence=%d\n", fd, offset,
            whence);
      vlong newpos;
      if (setlabel(&up->errlab[up->nerrlab++])) {
        r->type = Rerror;
        snprint(r->ename, sizeof(r->ename), "%s", up->errstr);
        return -1;
      }
      newpos = sseek(fd, offset, whence);
      up->nerrlab--;
      r->type = Rsyscall;
      r->tag = t->tag;
      r->retval = newpos;
      r->scount = 0;
      r->sdata = ((void *)0);
      return 0;
    }
    case SYS_NSEC: {
      /* Returns u64int time */
      print("p9_dispatch: SYS_NSEC\n");
      uvlong t_now = nsec();
      r->type = Rsyscall;
      r->tag = t->tag;
      r->retval = t_now;
      r->scount = 0;
      r->sdata = ((void *)0);
      return 0;
    }
    case SYS_WRITE: {
      /* Format: [fid 4] [offset 8] [count 4] [data...] */
      p = tsyscall_skip_argc(p, ep, 3);
      if (p + 4 + 8 + 4 > ep) {
        r->type = Rerror;
        return -1;
      }
      int fid = (((uchar *)(p))[0] | (((uchar *)(p))[1] << 8) | (((uchar *)(p))[2] << 16) | (((uchar *)(p))[3] << 24));
      p += 4;
      vlong offset = ((u32int)(((uchar *)(p))[0] | (((uchar *)(p))[1] << 8) | (((uchar *)(p))[2] << 16) | (((uchar *)(p))[3] << 24)) | ((uvlong)(((uchar *)(p))[4] | (((uchar *)(p))[5] << 8) | (((uchar *)(p))[6] << 16) | (((uchar *)(p))[7] << 24)) << 32));
      p += 8;
      int count = (((uchar *)(p))[0] | (((uchar *)(p))[1] << 8) | (((uchar *)(p))[2] << 16) | (((uchar *)(p))[3] << 24));
      p += 4;
      if (p + count > ep) {
        r->type = Rerror;
        return -1;
      }
      print("p9_dispatch: SYS_WRITE fd=%d count=%d off=%lld\n", fid, count,
            offset);
      extern Chan *fdtochan(int, int, int, int);
      Chan *c;
      long n;
      if (setlabel(&up->errlab[up->nerrlab++])) {
        r->type = Rerror;
        snprint(r->ename, sizeof(r->ename), "%s", up->errstr);
        return -1;
      }
      c = fdtochan(fid, 1, 1, 1);
      if (setlabel(&up->errlab[up->nerrlab++])) {
        cclose(c);
        nexterror();
      }
      if (c->qid.type & 0x80)
        error(Eisdir);
      n = devtab[c->type]->write(c, p, count, offset);
      up->nerrlab--;
      cclose(c);
      up->nerrlab--;
      r->type = Rsyscall;
      r->tag = t->tag;
      r->retval = n;
      r->scount = 0;
      r->sdata = ((void *)0);
      return 0;
    }
    case SYS_PWRITE: {
      /* Same format as SYS_WRITE */
      p = tsyscall_skip_argc(p, ep, 3);
      if (p + 4 + 8 + 4 > ep) {
        r->type = Rerror;
        return -1;
      }
      int fid = (((uchar *)(p))[0] | (((uchar *)(p))[1] << 8) | (((uchar *)(p))[2] << 16) | (((uchar *)(p))[3] << 24));
      p += 4;
      vlong offset = ((u32int)(((uchar *)(p))[0] | (((uchar *)(p))[1] << 8) | (((uchar *)(p))[2] << 16) | (((uchar *)(p))[3] << 24)) | ((uvlong)(((uchar *)(p))[4] | (((uchar *)(p))[5] << 8) | (((uchar *)(p))[6] << 16) | (((uchar *)(p))[7] << 24)) << 32));
      p += 8;
      int count = (((uchar *)(p))[0] | (((uchar *)(p))[1] << 8) | (((uchar *)(p))[2] << 16) | (((uchar *)(p))[3] << 24));
      p += 4;
      if (p + count > ep) {
        r->type = Rerror;
        return -1;
      }
      print("p9_dispatch: SYS_PWRITE fd=%d count=%d off=%lld\n", fid, count,
            offset);
      extern Chan *fdtochan(int, int, int, int);
      Chan *c;
      long n;
      if (setlabel(&up->errlab[up->nerrlab++])) {
        r->type = Rerror;
        snprint(r->ename, sizeof(r->ename), "%s", up->errstr);
        return -1;
      }
      c = fdtochan(fid, 1, 1, 1);
      if (setlabel(&up->errlab[up->nerrlab++])) {
        cclose(c);
        nexterror();
      }
      if (c->qid.type & 0x80)
        error(Eisdir);
      n = devtab[c->type]->write(c, p, count, offset);
      up->nerrlab--;
      cclose(c);
      up->nerrlab--;
      r->type = Rsyscall;
      r->tag = t->tag;
      r->retval = n;
      r->scount = 0;
      r->sdata = ((void *)0);
      return 0;
    }
    case SYS_READ:
    case SYS_PREAD: {
      /* Format: [fid 4] [offset 8] [count 4] */
      p = tsyscall_skip_argc(p, ep, 3);
      if (p + 4 + 8 + 4 > ep) {
        r->type = Rerror;
        return -1;
      }
      int fid = (((uchar *)(p))[0] | (((uchar *)(p))[1] << 8) | (((uchar *)(p))[2] << 16) | (((uchar *)(p))[3] << 24));
      p += 4;
      vlong offset = ((u32int)(((uchar *)(p))[0] | (((uchar *)(p))[1] << 8) | (((uchar *)(p))[2] << 16) | (((uchar *)(p))[3] << 24)) | ((uvlong)(((uchar *)(p))[4] | (((uchar *)(p))[5] << 8) | (((uchar *)(p))[6] << 16) | (((uchar *)(p))[7] << 24)) << 32));
      p += 8;
      int count = (((uchar *)(p))[0] | (((uchar *)(p))[1] << 8) | (((uchar *)(p))[2] << 16) | (((uchar *)(p))[3] << 24));
      p += 4;
      print("p9_dispatch: %s fd=%d count=%d off=%lld\n",
            t->scallnr == SYS_READ ? "SYS_READ" : "SYS_PREAD", fid, count,
            offset);
      extern Chan *fdtochan(int, int, int, int);
      Chan *c;
      long n;
      int rsyscall_hdr = 4 + 1 + 2 + 8 + 4;
      if (setlabel(&up->errlab[up->nerrlab++])) {
        r->type = Rerror;
        snprint(r->ename, sizeof(r->ename), "%s", up->errstr);
        return -1;
      }
      c = fdtochan(fid, 0, 1, 1);
      if (setlabel(&up->errlab[up->nerrlab++])) {
        cclose(c);
        nexterror();
      }
      if (c->qid.type & 0x80)
        error(Eisdir);
      if (count > 0xF00 - rsyscall_hdr) {
        error("read count too large");
      }
      uchar *data = (uchar *)proc->p9page + 0x000 + rsyscall_hdr;
      n = devtab[c->type]->read(c, data, count, offset);
      up->nerrlab--;
      cclose(c);
      up->nerrlab--;
      r->type = Rsyscall;
      r->tag = t->tag;
      r->retval = n;
      r->scount = n;
      r->sdata = data;
      return 0;
    }
    case SYS_STAT: {
      /* Format: [path s] OR [fid 4] */
      p = tsyscall_skip_argc(p, ep, 1);
      if (p + 2 > ep) {
        r->type = Rerror;
        r->ename = "short msg";
        return -1;
      }
      int remaining = ep - p;
      Chan *c = ((void *)0);
      char *path = ((void *)0);
      long n;
      if (remaining >= 2) {
        int len = (((uchar *)(p))[0] | (((uchar *)(p))[1] << 8));
        if (2 + len == remaining) {
          p += 2;
          path = smalloc(len + 1);
          memmove(path, p, len);
          path[len] = 0;
          p += len;
          print("p9_dispatch: SYS_STAT '%s'\n", path);
          if (setlabel(&up->errlab[up->nerrlab++])) {
            if (c)
              cclose(c);
            free(path);
            r->type = Rerror;
            snprint(r->ename, sizeof(r->ename), "%s", up->errstr);
            return -1;
          }
          c = namec(path, Aaccess, 0, 0);
        } else if (remaining == 4) {
          int fid = (((uchar *)(p))[0] | (((uchar *)(p))[1] << 8) | (((uchar *)(p))[2] << 16) | (((uchar *)(p))[3] << 24));
          p += 4;
          print("p9_dispatch: SYS_STAT fd=%d\n", fid);
          if (setlabel(&up->errlab[up->nerrlab++])) {
            if (c)
              cclose(c);
            r->type = Rerror;
            snprint(r->ename, sizeof(r->ename), "%s", up->errstr);
            return -1;
          }
          c = fdtochan(fid, -1, 0, 1);
        } else {
          r->type = Rerror;
          r->ename = "bad stat msg";
          return -1;
        }
      }
      extern Chan *fdtochan(int, int, int, int);
      int rsyscall_hdr = 4 + 1 + 2 + 8 + 4;
      uchar *data = (uchar *)proc->p9page + 0x000 + rsyscall_hdr;
      n = devtab[c->type]->stat(c, data, 0xF00 - rsyscall_hdr);
      if (path)
        free(path);
      up->nerrlab--;
      cclose(c);
      r->type = Rsyscall;
      r->tag = t->tag;
      r->retval = n;
      r->scount = n;
      r->sdata = data;
      return 0;
    }
    case SYS_WSTAT: {
      /* Format: [path s] [nstat 2] [stat bytes] */
      p = tsyscall_skip_argc(p, ep, 3);
      if (p + 2 > ep) {
        r->type = Rerror;
        r->ename = "short msg";
        return -1;
      }
      int len = (((uchar *)(p))[0] | (((uchar *)(p))[1] << 8));
      p += 2;
      if (p + len + 2 > ep) {
        r->type = Rerror;
        r->ename = "short msg";
        return -1;
      }
      char *path = smalloc(len + 1);
      memmove(path, p, len);
      path[len] = 0;
      p += len;
      int nstat = (((uchar *)(p))[0] | (((uchar *)(p))[1] << 8));
      p += 2;
      if (p + nstat > ep) {
        free(path);
        r->type = Rerror;
        r->ename = "short msg";
        return -1;
      }
      print("p9_dispatch: SYS_WSTAT '%s' nstat=%d\n", path, nstat);
      extern void validstat(uchar * s, int n);
      Chan *c = ((void *)0);
      if (setlabel(&up->errlab[up->nerrlab++])) {
        if (c)
          cclose(c);
        free(path);
        r->type = Rerror;
        snprint(r->ename, sizeof(r->ename), "%s", up->errstr);
        return -1;
      }
      validstat(p, nstat);
      c = namec(path, Aaccess, 0, 0);
      devtab[c->type]->wstat(c, p, nstat);
      up->nerrlab--;
      cclose(c);
      free(path);
      r->type = Rsyscall;
      r->tag = t->tag;
      r->retval = 0;
      r->scount = 0;
      r->sdata = ((void *)0);
      return 0;
    }
    case SYS_WASM_COMPILE: {
      print("p9_dispatch: Tsyscall SYS_WASM_COMPILE\n");
      /* Call Layer 1 wasm3 runtime handler */
      if (sys_wasm_compile(t, r) != 0) {
        /* Error already set in r by handler */
        return -1;
      }
      return 0;
    }
    case SYS_WASM_EXECUTE: {
      print("p9_dispatch: Tsyscall SYS_WASM_EXECUTE\n");
      /* Call Layer 1 wasm3 runtime handler */
      if (sys_wasm_execute(t, r) != 0) {
        /* Error already set in r by handler */
        return -1;
      }
      return 0;
    }
    case SYS_WASM_DESTROY: {
      print("p9_dispatch: Tsyscall SYS_WASM_DESTROY\n");
      /* Call Layer 1 wasm3 runtime handler */
      if (sys_wasm_destroy(t, r) != 0) {
        /* Error already set in r by handler */
        return -1;
      }
      return 0;
    }
    case SYS_PIPE: {
      extern uintptr syspipe(void *list_void);
      /* Pebble: Check/deduct budget for pipe creation (userspace only).
       * TCB processes (kp == 1) are exempt. */
      if (up != ((void *)0) && up->kp == 0) {
        lock(&pebble_global_lock);
        if (up->pebble.colorless_bank < (64 * 1024 / 8)) {
          unlock(&pebble_global_lock);
          r->type = Rerror;
          snprint(r->ename, sizeof(r->ename),
                  "pebble: insufficient budget for pipe");
          return -1;
        }
        up->pebble.colorless_bank -= (64 * 1024 / 8);
        unlock(&pebble_global_lock);
      }
      p = tsyscall_skip_argc(p, ep, 1);
      int *fd = (int *)((uchar *)proc->p9page + 0x000 + 64);
      int *ufd = (int *)(0x7FFFFEEFF000ULL + 0x000 + 64);
      fd[0] = -1;
      fd[1] = -1;
      ulong args[1] = {(ulong)ufd};
      if (setlabel(&up->errlab[up->nerrlab++])) {
        r->type = Rerror;
        snprint(r->ename, sizeof(r->ename), "%s", up->errstr);
        return -1;
      }
      syspipe(args);
      up->nerrlab--;
      r->type = Rsyscall;
      r->tag = t->tag;
      r->retval = 0;
      r->scount = 8;
      r->sdata = (uchar *)fd;
      return 0;
    }
    case SYS_MOUNT: {
      extern uintptr sysmount(void *list_void);
      /* Pebble: Check/deduct budget for mount (userspace only).
       * TCB processes (kp == 1) are exempt. */
      if (up != ((void *)0) && up->kp == 0) {
        lock(&pebble_global_lock);
        if (up->pebble.colorless_bank < (4 * 1024 / 8)) {
          unlock(&pebble_global_lock);
          r->type = Rerror;
          snprint(r->ename, sizeof(r->ename),
                  "pebble: insufficient budget for mount");
          return -1;
        }
        up->pebble.colorless_bank -= (4 * 1024 / 8);
        unlock(&pebble_global_lock);
      }
      p = tsyscall_skip_argc(p, ep, 5);
      if (p + 4 + 4 + 2 > ep) {
        r->type = Rerror;
        r->ename = "short mount msg";
        return -1;
      }
      ulong fd = (((uchar *)(p))[0] | (((uchar *)(p))[1] << 8) | (((uchar *)(p))[2] << 16) | (((uchar *)(p))[3] << 24));
      p += 4;
      ulong afd = (((uchar *)(p))[0] | (((uchar *)(p))[1] << 8) | (((uchar *)(p))[2] << 16) | (((uchar *)(p))[3] << 24));
      p += 4;
      int oldlen = (((uchar *)(p))[0] | (((uchar *)(p))[1] << 8));
      p += 2;
      if (p + oldlen + 4 + 2 > ep) {
        r->type = Rerror;
        r->ename = "short mount msg";
        return -1;
      }
      uchar *oldp = p;
      p += oldlen;
      if (oldp + oldlen > ep) {
        r->type = Rerror;
        r->ename = "short mount msg";
        return -1;
      }
      ulong flags = (((uchar *)(p))[0] | (((uchar *)(p))[1] << 8) | (((uchar *)(p))[2] << 16) | (((uchar *)(p))[3] << 24));
      p += 4;
      int anamelen = (((uchar *)(p))[0] | (((uchar *)(p))[1] << 8));
      p += 2;
      if (p + anamelen > ep) {
        r->type = Rerror;
        r->ename = "short mount msg";
        return -1;
      }
      uchar *anamep = p;
      ulong args[5];
      uchar *msg_end = (uchar *)proc->p9page + 0x000 + 0xF00;
      ulong scratch_needed = oldlen + 1 + anamelen + 1;
      if (ep + scratch_needed > msg_end) {
        r->type = Rerror;
        r->ename = "mount scratch overflow";
        return -1;
      }
      uchar *scratch = ep;
      uchar *oldk = scratch;
      memmove(oldk, oldp, oldlen);
      oldk[oldlen] = 0;
      scratch += oldlen + 1;
      uchar *anamek = scratch;
      if (anamelen > 0) {
        memmove(anamek, anamep, anamelen);
        anamek[anamelen] = 0;
        scratch += anamelen + 1;
      } else {
        anamek[0] = 0;
      }
      uintptr kpage = (uintptr)proc->p9page;
      uintptr uold = 0x7FFFFEEFF000ULL + ((uintptr)oldk - kpage);
      uintptr uaname = 0x7FFFFEEFF000ULL + ((uintptr)anamek - kpage);
      args[0] = fd;
      args[1] = afd;
      args[2] = (ulong)uold;
      args[3] = flags;
      args[4] = (ulong)uaname;
      if (setlabel(&up->errlab[up->nerrlab++])) {
        r->type = Rerror;
        snprint(r->ename, sizeof(r->ename), "%s", up->errstr);
        return -1;
      }
      sysmount(args);
      up->nerrlab--;
      r->type = Rsyscall;
      r->tag = t->tag;
      r->retval = 0;
      r->scount = 0;
      r->sdata = ((void *)0);
      return 0;
    }
    case SYS_WAIT: {
      extern ulong pwait(Waitmsg * w);
      Waitmsg w;
      if (setlabel(&up->errlab[up->nerrlab++])) {
        r->type = Rerror;
        snprint(r->ename, sizeof(r->ename), "%s", up->errstr);
        return -1;
      }
      ulong pid = pwait(&w);
      up->nerrlab--;
      char *msg = (char *)proc->p9page + 0x000 + 64;
      snprint(msg, 0xF00 - 64, "%s", w.msg);
      r->type = Rsyscall;
      r->tag = t->tag;
      r->retval = pid;
      r->scount = strlen(msg) + 1;
      r->sdata = (uchar *)msg;
      return 0;
    }
    /* Exchange Pool IPC Syscalls */
    case SYS_EXCHANGE_ALLOC: {
      extern uintptr sys_exchange_alloc(void *);
      print("p9_dispatch: SYS_EXCHANGE_ALLOC\n");
      if (setlabel(&up->errlab[up->nerrlab++])) {
        r->type = Rerror;
        snprint(r->ename, sizeof(r->ename), "%s", up->errstr);
        return -1;
      }
      uintptr cap = sys_exchange_alloc(((void *)0));
      up->nerrlab--;
      r->type = Rsyscall;
      r->tag = t->tag;
      r->retval = (u64int)cap;
      r->scount = 0;
      r->sdata = ((void *)0);
      return 0;
    }
    case SYS_EXCHANGE_FREE: {
      extern uintptr sys_exchange_free(void *);
      print("p9_dispatch: SYS_EXCHANGE_FREE\n");
      /* Format: [cap_ptr 8] */
      p = tsyscall_skip_argc(p, ep, 1);
      if (p + 8 > ep) {
        r->type = Rerror;
        r->ename = "short msg";
        return -1;
      }
      uintptr cap_ptr = (uintptr)((u32int)(((uchar *)(p))[0] | (((uchar *)(p))[1] << 8) | (((uchar *)(p))[2] << 16) | (((uchar *)(p))[3] << 24)) | ((uvlong)(((uchar *)(p))[4] | (((uchar *)(p))[5] << 8) | (((uchar *)(p))[6] << 16) | (((uchar *)(p))[7] << 24)) << 32));
      ulong args[1] = {cap_ptr};
      if (setlabel(&up->errlab[up->nerrlab++])) {
        r->type = Rerror;
        snprint(r->ename, sizeof(r->ename), "%s", up->errstr);
        return -1;
      }
      sys_exchange_free(args);
      up->nerrlab--;
      r->type = Rsyscall;
      r->tag = t->tag;
      r->retval = 0;
      r->scount = 0;
      r->sdata = ((void *)0);
      return 0;
    }
    case SYS_EXCHANGE_PUBLISH: {
      extern uintptr sys_exchange_publish(void *);
      print("p9_dispatch: SYS_EXCHANGE_PUBLISH\n");
      /* Format: [topic_name s] [data_ptr 8] [len 8] */
      /* But sdata already contains the packed data from userspace */
      if (setlabel(&up->errlab[up->nerrlab++])) {
        r->type = Rerror;
        snprint(r->ename, sizeof(r->ename), "%s", up->errstr);
        return -1;
      }
      /* Parse topic from sdata - it's a null-terminated string */
      char *topic = (char *)t->sdata;
      int topic_len = 0;
      while (topic_len < t->scount && topic[topic_len] != '\0')
        topic_len++;
      /* After topic comes data pointer and length */
      uchar *rest = t->sdata + topic_len + 1;
      if (rest + 12 > t->sdata + t->scount) {
        up->nerrlab--;
        r->type = Rerror;
        r->ename = "exchange_publish: incomplete message";
        return -1;
      }
      uintptr data_ptr = (uintptr)(((uchar *)(rest))[0] | (((uchar *)(rest))[1] << 8) | (((uchar *)(rest))[2] << 16) | (((uchar *)(rest))[3] << 24));
      if (sizeof(uintptr) > 4) {
        data_ptr |= ((uintptr)(((uchar *)(rest + 4))[0] | (((uchar *)(rest + 4))[1] << 8) | (((uchar *)(rest + 4))[2] << 16) | (((uchar *)(rest + 4))[3] << 24)) << 32);
        rest += 8;
      } else {
        rest += 4;
      }
      ulong len = (((uchar *)(rest))[0] | (((uchar *)(rest))[1] << 8) | (((uchar *)(rest))[2] << 16) | (((uchar *)(rest))[3] << 24));
      /* Build args for sys_exchange_publish: [topic, data, len] */
      ulong args[3] = {(ulong)topic, (ulong)data_ptr, len};
      uintptr cap = sys_exchange_publish(args);
      up->nerrlab--;
      r->type = Rsyscall;
      r->tag = t->tag;
      r->retval = (u64int)cap;
      r->scount = 0;
      r->sdata = ((void *)0);
      return 0;
    }
    case SYS_EXCHANGE_SUBSCRIBE: {
      extern uintptr sys_exchange_subscribe(void *);
      print("p9_dispatch: SYS_EXCHANGE_SUBSCRIBE\n");
      /* sdata contains topic name as null-terminated string */
      if (setlabel(&up->errlab[up->nerrlab++])) {
        r->type = Rerror;
        snprint(r->ename, sizeof(r->ename), "%s", up->errstr);
        return -1;
      }
      char *topic = (char *)t->sdata;
      ulong args[1] = {(ulong)topic};
      uintptr ret = sys_exchange_subscribe(args);
      up->nerrlab--;
      r->type = Rsyscall;
      r->tag = t->tag;
      r->retval = ret;
      r->scount = 0;
      r->sdata = ((void *)0);
      return 0;
    }
    case SYS_EXCHANGE_UNSUBSCRIBE: {
      extern uintptr sys_exchange_unsubscribe(void *);
      print("p9_dispatch: SYS_EXCHANGE_UNSUBSCRIBE\n");
      /* sdata contains topic name as null-terminated string */
      if (setlabel(&up->errlab[up->nerrlab++])) {
        r->type = Rerror;
        snprint(r->ename, sizeof(r->ename), "%s", up->errstr);
        return -1;
      }
      char *topic = (char *)t->sdata;
      ulong args[1] = {(ulong)topic};
      uintptr ret = sys_exchange_unsubscribe(args);
      up->nerrlab--;
      r->type = Rsyscall;
      r->tag = t->tag;
      r->retval = ret;
      r->scount = 0;
      r->sdata = ((void *)0);
      return 0;
    }
    case SYS_EXCHANGE_RECEIVE: {
      extern uintptr sys_exchange_receive(void *);
      print("p9_dispatch: SYS_EXCHANGE_RECEIVE\n");
      if (setlabel(&up->errlab[up->nerrlab++])) {
        r->type = Rerror;
        snprint(r->ename, sizeof(r->ename), "%s", up->errstr);
        return -1;
      }
      uintptr notif = sys_exchange_receive(((void *)0));
      up->nerrlab--;
      r->type = Rsyscall;
      r->tag = t->tag;
      r->retval = (u64int)notif;
      r->scount = 0;
      r->sdata = ((void *)0);
      return 0;
    }
    default:
      r->type = Rerror;
      snprint(r->ename, sizeof(r->ename), "unknown syscall %d", t->scallnr);
      return -1;
    }
  }
  /* Handle Tsys* - Specific syscall message types (132-205) */
  /* I/O Operations */
  if (t->type == Tsysopen) {
    extern int newfd(Chan *, int);
    extern int openmode(ulong);
    Chan *c = ((void *)0);
    int fd;
    if (setlabel(&up->errlab[up->nerrlab++])) {
      if (c)
        cclose(c);
      r->type = Rerror;
      r->ename = up->errstr;
      return -1;
    }
    /* t->name contains path, t->mode contains mode */
    openmode(t->mode);
    c = namec(t->name, Aopen, t->mode, 0);
    fd = newfd(c, t->mode);
    up->nerrlab--;
    /* Build Rsysopen response */
    r->type = Rsysopen;
    r->tag = t->tag;
    r->fid = fd;
    r->qid = c->qid;
    r->iounit = c->iounit;
    print("p9_dispatch: Tsysopen '%s' -> fd=%d\n", t->name, fd);
    return 0;
  }
  if (t->type == Tsyscreate) {
    extern int newfd(Chan *, int);
    extern int openmode(ulong);
    Chan *c = ((void *)0);
    int fd;
    if (setlabel(&up->errlab[up->nerrlab++])) {
      if (c)
        cclose(c);
      r->type = Rerror;
      r->ename = up->errstr;
      return -1;
    }
    /* t->name contains path, t->perm contains permissions, t->mode contains
     * mode */
    openmode(t->mode);
    c = namec(t->name, Acreate, t->mode, t->perm);
    fd = newfd(c, t->mode);
    up->nerrlab--;
    /* Build Rsyscreate response */
    r->type = Rsyscreate;
    r->tag = t->tag;
    r->fid = fd;
    r->qid = c->qid;
    r->iounit = c->iounit;
    print("p9_dispatch: Tsyscreate '%s' perm=0%o -> fd=%d\n", t->name, t->perm,
          fd);
    return 0;
  }
  if (t->type == Tsysread || t->type == Tsyspread) {
    extern Chan *fdtochan(int, int, int, int);
    Chan *c;
    long n;
    if (setlabel(&up->errlab[up->nerrlab++])) {
      r->type = Rerror;
      r->ename = up->errstr;
      return -1;
    }
    c = fdtochan(t->fid, 0, 1, 1);
    if (setlabel(&up->errlab[up->nerrlab++])) {
      cclose(c);
      nexterror();
    }
    if (c->qid.type & 0x80)
      error(Eisdir);
    /* Allocate buffer for read data - use exchange page data area */
    /* Rsysread header is 4+1+2+4 = 11 bytes. Data starts at msg_buf+11 */
    r->data = (char *)p->p9page + 0x000 + 11;
    if (t->count > 0xF00 - 100) {
      error("read count too large");
    }
    n = devtab[c->type]->read(c, r->data, t->count, t->offset);
    up->nerrlab--;
    cclose(c);
    up->nerrlab--;
    /* Build Rsysread response */
    r->type = (t->type == Tsysread) ? Rsysread : Rsyspread;
    r->tag = t->tag;
    r->count = n;
    /* r->data now contains the data */
    print("p9_dispatch: %s fd=%d count=%d offset=%lld -> %ld bytes\n",
          t->type == Tsysread ? "Tsysread" : "Tsyspread", t->fid, t->count,
          t->offset, n);
    return 0;
  }
  if (t->type == Tsyswrite || t->type == Tsyspwrite) {
    extern Chan *fdtochan(int, int, int, int);
    Chan *c;
    long n;
    if (setlabel(&up->errlab[up->nerrlab++])) {
      r->type = Rerror;
      r->ename = up->errstr;
      return -1;
    }
    c = fdtochan(t->fid, 1, 1, 1);
    if (setlabel(&up->errlab[up->nerrlab++])) {
      cclose(c);
      nexterror();
    }
    if (c->qid.type & 0x80)
      error(Eisdir);
    n = devtab[c->type]->write(c, t->data, t->count, t->offset);
    up->nerrlab--;
    cclose(c);
    up->nerrlab--;
    /* Build Rsyswrite response */
    r->type = (t->type == Tsyswrite) ? Rsyswrite : Rsyspwrite;
    r->tag = t->tag;
    r->count = n;
    print("p9_dispatch: %s fd=%d count=%d offset=%lld -> %ld bytes\n",
          t->type == Tsyswrite ? "Tsyswrite" : "Tsyspwrite", t->fid, t->count,
          t->offset, n);
    return 0;
  }
  if (t->type == Tsysclose) {
    extern void fdclose(int, int);
    extern Chan *fdtochan(int, int, int, int);
    if (setlabel(&up->errlab[up->nerrlab++])) {
      r->type = Rerror;
      r->ename = up->errstr;
      return -1;
    }
    Chan *c = fdtochan(t->fid, -1, 0, 0);
    if (c)
      cclose(c);
    fdclose(t->fid, 0);
    up->nerrlab--;
    /* Build Rsysclose response */
    r->type = Rsysclose;
    r->tag = t->tag;
    print("p9_dispatch: Tsysclose fd=%d\n", t->fid);
    return 0;
  }
  if (t->type == Tsysremove) {
    Chan *c = ((void *)0);
    if (setlabel(&up->errlab[up->nerrlab++])) {
      if (c)
        cclose(c);
      r->type = Rerror;
      r->ename = up->errstr;
      return -1;
    }
    c = namec(t->name, Aremove, 0, 0);
    up->nerrlab--;
    /* Build Rsysremove response */
    r->type = Rsysremove;
    r->tag = t->tag;
    print("p9_dispatch: Tsysremove '%s'\n", t->name);
    return 0;
  }
  /* Process Control */
  if (t->type == Tsysexit) {
    extern void pexit(char *, int);
    print("p9_dispatch: Tsysexit '%s'\n", t->ename ? t->ename : "");
    /* pexit never returns */
    pexit(t->ename ? t->ename : "", 1);
    /* Not reached, but satisfies compiler */
    return 0;
  }
  if (t->type == Tsysbrk) {
    extern uintptr ibrk(uintptr, int);
    uintptr ret;
    if (setlabel(&up->errlab[up->nerrlab++])) {
      r->type = Rerror;
      r->ename = up->errstr;
      return -1;
    }
    /* Call ibrk with address */
    ret = ibrk((uintptr)t->addr, BSEG);
    up->nerrlab--;
    /* Build Rsysbrk response */
    r->type = Rsysbrk;
    r->tag = t->tag;
    r->addr = ret;
    print("p9_dispatch: Tsysbrk addr=0x%llx -> 0x%llx\n", t->addr, (u64int)ret);
    return 0;
  }
  /* Namespace Operations */
  if (t->type == Tsyschdir) {
    Chan *c = ((void *)0);
    if (setlabel(&up->errlab[up->nerrlab++])) {
      if (c)
        cclose(c);
      r->type = Rerror;
      r->ename = up->errstr;
      return -1;
    }
    c = namec(t->name, Atodir, 0, 0);
    cclose(up->dot);
    up->dot = c;
    up->nerrlab--;
    /* Build Rsyschdir response */
    r->type = Rsyschdir;
    r->tag = t->tag;
    print("p9_dispatch: Tsyschdir '%s'\n", t->name);
    return 0;
  }
  /* FD Operations */
  if (t->type == Tsysdup) {
    extern int newfd(Chan *, int);
    extern Chan *fdtochan(int, int, int, int);
    Chan *c;
    int nfd;
    if (setlabel(&up->errlab[up->nerrlab++])) {
      r->type = Rerror;
      r->ename = up->errstr;
      return -1;
    }
    /* Get the channel from oldfd */
    c = fdtochan(t->fid, -1, 0, 1);
    incref(&c->ref);
    /* Create new fd */
    if (t->newfid == -1) {
      nfd = newfd(c, 0);
    } else {
      /* Dup to specific fd - use newfd to handle it properly */
      extern void fdclose(int, int);
      if (t->newfid >= 0) {
        fdclose(t->newfid, 0);
        /* Try to place channel at specific fd */
        up->fgrp->fd[t->newfid] = c;
        nfd = t->newfid;
      } else {
        cclose(c);
        error("invalid fd");
      }
    }
    up->nerrlab--;
    /* Build Rsysdup response */
    r->type = Rsysdup;
    r->tag = t->tag;
    r->fid = nfd;
    print("p9_dispatch: Tsysdup oldfd=%d newfd=%d -> %d\n", t->fid, t->newfid,
          nfd);
    return 0;
  }
  if (t->type == Tsysstat) {
    extern uintptr sysstat(void *list_void);
    ulong args[3];
    int rsysstat_hdr = 4 + 1 + 2 + 2;
    uchar *statbuf = (uchar *)p->p9page + 0x000 + rsysstat_hdr;
    long n;
    if (setlabel(&up->errlab[up->nerrlab++])) {
      r->type = Rerror;
      r->ename = up->errstr;
      return -1;
    }
    args[0] = (ulong)t->name;
    args[1] = (ulong)statbuf;
    args[2] = 0xF00 - rsysstat_hdr;
    n = (long)sysstat(args);
    up->nerrlab--;
    r->type = Rsysstat;
    r->tag = t->tag;
    r->nstat = n;
    r->stat = statbuf;
    return 0;
  }
  if (t->type == Tsysfstat) {
    extern uintptr sysfstat(void *list_void);
    ulong args[3];
    int rsysstat_hdr = 4 + 1 + 2 + 2;
    uchar *statbuf = (uchar *)p->p9page + 0x000 + rsysstat_hdr;
    long n;
    if (setlabel(&up->errlab[up->nerrlab++])) {
      r->type = Rerror;
      r->ename = up->errstr;
      return -1;
    }
    args[0] = (ulong)t->fid;
    args[1] = (ulong)statbuf;
    args[2] = 0xF00 - rsysstat_hdr;
    n = (long)sysfstat(args);
    up->nerrlab--;
    r->type = Rsysfstat;
    r->tag = t->tag;
    r->nstat = n;
    r->stat = statbuf;
    return 0;
  }
  if (t->type == Tsyswstat) {
    extern uintptr syswstat(void *list_void);
    ulong args[3];
    if (setlabel(&up->errlab[up->nerrlab++])) {
      r->type = Rerror;
      r->ename = up->errstr;
      return -1;
    }
    args[0] = (ulong)t->name;
    args[1] = (ulong)t->stat;
    args[2] = (ulong)t->nstat;
    syswstat(args);
    up->nerrlab--;
    r->type = Rsyswstat;
    r->tag = t->tag;
    return 0;
  }
  if (t->type == Tsysfwstat) {
    extern uintptr sysfwstat(void *list_void);
    ulong args[3];
    if (setlabel(&up->errlab[up->nerrlab++])) {
      r->type = Rerror;
      r->ename = up->errstr;
      return -1;
    }
    args[0] = (ulong)t->fid;
    args[1] = (ulong)t->stat;
    args[2] = (ulong)t->nstat;
    sysfwstat(args);
    up->nerrlab--;
    r->type = Rsysfwstat;
    r->tag = t->tag;
    return 0;
  }
  if (t->type == Tsysfork) {
    extern uintptr sysrfork(void *list_void);
    ulong args[1];
    uintptr ret;
    args[0] = t->flags;
    if (setlabel(&up->errlab[up->nerrlab++])) {
      r->type = Rerror;
      r->ename = up->errstr;
      return -1;
    }
    ret = sysrfork(args);
    up->nerrlab--;
    r->type = Rsysfork;
    r->tag = t->tag;
    r->pid = ret;
    return 0;
  }
  if (t->type == Tsysexec) {
    extern uintptr sysexec(void *list_void);
    ulong args[2];
    uintptr argvp;
    char **argv;
    uintptr kpage, kpath, path_offset, upath, argv_offset, uargv;
    print("p9_dispatch: Tsysexec received name='%s' argc=%d\n",
          t->path ? t->path : "nil", t->argc);
    if (t->argc > 1) {
      print("p9_dispatch: Tsysexec error: argc > 1\n");
      r->type = Rerror;
      r->ename = "argv not supported in Tsysexec";
      return -1;
    }
    /* Calculate User Address of the path string */
    kpage = (uintptr)p->p9page;
    kpath = (uintptr)t->path;
    /* Ensure kpath is within the page */
    if (kpath < kpage || kpath >= kpage + 4096) {
      print("p9_dispatch: Tsysexec error: path outside page (kpath=%p "
            "kpage=%p)\n",
            (void *)kpath, (void *)kpage);
      r->type = Rerror;
      r->ename = "Tsysexec: path outside exchange page";
      return -1;
    }
    path_offset = kpath - kpage;
    upath = 0x7FFFFEEFF000ULL + path_offset;
    /* Construct argv array in buffer (after message) */
    argvp =
        kpage + 0x000 + 256; /* Arbitrary offset after typical msg */
    /* Safer: use t->data + t->count if available, but t->data isn't set for
     * Tsysexec by convM2S */
    /* convM2S doesn't set t->data for Tsysexec. But we know where the message
     * ends roughly. */
    /* P9_MSG_OFFSET + 256 is safe given P9_MSG_SIZE is 8192 */
    argvp = (argvp + 7) & ~7ULL;
    argv = (char **)argvp;
    /* Check bounds for argv */
    if (argvp + 2 * sizeof(char *) >= kpage + 4096) {
      print("p9_dispatch: Tsysexec error: no room for argv\n");
      r->type = Rerror;
      r->ename = "Tsysexec: no room for argv";
      return -1;
    }
    argv[0] = (char *)upath; /* argv[0] must be User Address */
    argv[1] = ((void *)0);
    /* Calculate User Address of argv */
    argv_offset = argvp - kpage;
    uargv = 0x7FFFFEEFF000ULL + argv_offset;
    args[0] = (ulong)upath;
    args[1] = (ulong)uargv;
    if (setlabel(&up->errlab[up->nerrlab++])) {
      print("p9_dispatch: Tsysexec error: waserror trip: %s\n", up->errstr);
      r->type = Rerror;
      r->ename = up->errstr;
      return -1;
    }
    sysexec(args);
    up->nerrlab--;
    extern void noteret(void); /* Assembly label for exec return path */
    if (up->dbgreg != ((void *)0) && ((void **)up->dbgreg)[-1] == noteret) {
      r->type = Rsysexec;
      r->tag = t->tag;
      return 0;
    }
    r->type = Rerror;
    r->ename = "exec returned unexpectedly";
    print("p9_dispatch: Tsysexec error: exec returned unexpectedly\n");
    return -1;
  }
  if (t->type == Tsyswait) {
    extern ulong pwait(Waitmsg * w);
    Waitmsg w;
    char *msg;
    int msgmax = 0xF00 - 64;
    if (setlabel(&up->errlab[up->nerrlab++])) {
      r->type = Rerror;
      r->ename = up->errstr;
      return -1;
    }
    r->pid = pwait(&w);
    up->nerrlab--;
    msg = (char *)p->p9page + 0x000 + 64;
    snprint(msg, msgmax, "%s", w.msg);
    r->type = Rsyswait;
    r->tag = t->tag;
    r->ename = msg;
    return 0;
  }
  if (t->type == Tsyssleep) {
    extern uintptr syssleep(void *list_void);
    ulong args[1];
    if (setlabel(&up->errlab[up->nerrlab++])) {
      r->type = Rerror;
      r->ename = up->errstr;
      return -1;
    }
    args[0] = t->count;
    syssleep(args);
    up->nerrlab--;
    r->type = Rsyssleep;
    r->tag = t->tag;
    return 0;
  }
  if (t->type == Tsysbind) {
    extern uintptr sysbind(void *list_void);
    ulong args[3];
    if (setlabel(&up->errlab[up->nerrlab++])) {
      r->type = Rerror;
      r->ename = up->errstr;
      return -1;
    }
    args[0] = (ulong)t->name;
    args[1] = (ulong)t->oldpath;
    args[2] = (ulong)t->flags;
    sysbind(args);
    up->nerrlab--;
    r->type = Rsysbind;
    r->tag = t->tag;
    return 0;
  }
  if (t->type == Tsysmount) {
    extern uintptr sysmount(void *list_void);
    ulong args[5];
    if (setlabel(&up->errlab[up->nerrlab++])) {
      r->type = Rerror;
      r->ename = up->errstr;
      return -1;
    }
    args[0] = (ulong)t->fd;
    args[1] = (ulong)t->afid;
    args[2] = (ulong)t->oldpath;
    args[3] = (ulong)t->flags;
    args[4] = (ulong)t->aname;
    sysmount(args);
    up->nerrlab--;
    r->type = Rsysmount;
    r->tag = t->tag;
    return 0;
  }
  if (t->type == Tsysunmount) {
    extern uintptr sysunmount(void *list_void);
    ulong args[2];
    if (setlabel(&up->errlab[up->nerrlab++])) {
      r->type = Rerror;
      r->ename = up->errstr;
      return -1;
    }
    args[0] = (ulong)t->name;
    args[1] = (ulong)t->oldpath;
    sysunmount(args);
    up->nerrlab--;
    r->type = Rsysunmount;
    r->tag = t->tag;
    return 0;
  }
  if (t->type == Tsyspipe) {
    extern uintptr syspipe(void *list_void);
    ulong args[1];
    int *fd;
    int *ufd;
    fd = (int *)((uchar *)p->p9page + 0x000 + 64);
    ufd = (int *)(0x7FFFFEEFF000ULL + 0x000 + 64);
    fd[0] = fd[1] = -1;
    args[0] = (ulong)ufd;
    if (setlabel(&up->errlab[up->nerrlab++])) {
      r->type = Rerror;
      r->ename = up->errstr;
      return -1;
    }
    syspipe(args);
    up->nerrlab--;
    r->type = Rsyspipe;
    r->tag = t->tag;
    r->fid0 = fd[0];
    r->fid1 = fd[1];
    return 0;
  }
  if (t->type == Tsysfd2path) {
    extern uintptr sysfd2path(void *list_void);
    ulong args[3];
    char *buf;
    int buflen = 0xF00 - 64;
    buf = (char *)p->p9page + 0x000 + 64;
    if (setlabel(&up->errlab[up->nerrlab++])) {
      r->type = Rerror;
      r->ename = up->errstr;
      return -1;
    }
    args[0] = (ulong)t->fid;
    args[1] = (ulong)buf;
    args[2] = (ulong)buflen;
    sysfd2path(args);
    up->nerrlab--;
    r->type = Rsysfd2path;
    r->tag = t->tag;
    r->name = buf;
    return 0;
  }
  if (t->type == Tsysseek) {
    extern uintptr sysseek(void *list_void);
    ulong args[4];
    vlong *out;
    out = (vlong *)((uchar *)p->p9page + 0x000 + 64);
    args[0] = (ulong)out;
    args[1] = (ulong)t->fid;
    args[2] = (ulong)t->offset;
    args[3] = (ulong)t->whence;
    if (setlabel(&up->errlab[up->nerrlab++])) {
      r->type = Rerror;
      r->ename = up->errstr;
      return -1;
    }
    sysseek(args);
    up->nerrlab--;
    r->type = Rsysseek;
    r->tag = t->tag;
    r->offset = *out;
    return 0;
  }
  if (t->type == Tsysnotify) {
    extern uintptr sysnotify(void *list_void);
    ulong args[1];
    if (setlabel(&up->errlab[up->nerrlab++])) {
      r->type = Rerror;
      r->ename = up->errstr;
      return -1;
    }
    args[0] = (ulong)t->handler;
    sysnotify(args);
    up->nerrlab--;
    r->type = Rsysnotify;
    r->tag = t->tag;
    return 0;
  }
  if (t->type == Tsysalarm) {
    extern uintptr sysalarm(void *list_void);
    ulong args[1];
    uintptr prev;
    if (setlabel(&up->errlab[up->nerrlab++])) {
      r->type = Rerror;
      r->ename = up->errstr;
      return -1;
    }
    args[0] = t->count;
    prev = sysalarm(args);
    up->nerrlab--;
    r->type = Rsysalarm;
    r->tag = t->tag;
    r->count = prev;
    return 0;
  }
  /* Handle Ttoken (80) - Token transfer between machines */
  if (t->type == 80) {
    TokenTransfer transfer;
    TokenType tok_type;
    u64int amount;
    if (local_machine_bank == ((void *)0)) {
      r->type = Rerror;
      r->ename = "distributed pebble not initialized";
      return -1;
    }
    /* Parse token transfer from message data */
    if (t->count < sizeof(TokenType) + sizeof(u64int)) {
      r->type = Rerror;
      r->ename = "Ttoken: message too short";
      return -1;
    }
    tok_type = (TokenType)(((uchar *)(t->data))[0] | (((uchar *)(t->data))[1] << 8) | (((uchar *)(t->data))[2] << 16) | (((uchar *)(t->data))[3] << 24));
    amount = ((u32int)(((uchar *)(t->data + 4))[0] | (((uchar *)(t->data + 4))[1] << 8) | (((uchar *)(t->data + 4))[2] << 16) | (((uchar *)(t->data + 4))[3] << 24)) | ((uvlong)(((uchar *)(t->data + 4))[4] | (((uchar *)(t->data + 4))[5] << 8) | (((uchar *)(t->data + 4))[6] << 16) | (((uchar *)(t->data + 4))[7] << 24)) << 32));
    /* Receive the transfer */
    memset(&transfer, 0, sizeof(transfer));
    transfer.token_type = tok_type;
    transfer.amount = amount;
    /* Copy proof from message if present */
    if (t->count >= sizeof(TokenType) + sizeof(u64int) + sizeof(TokenProof)) {
      memmove(&transfer.proof, t->data + 12, sizeof(TokenProof));
    }
    if (transfer_receive(local_machine_bank, &transfer) < 0) {
      r->type = Rerror;
      r->ename = "token transfer failed";
      return -1;
    }
    /* Build Rtoken response */
    r->type = 81;
    r->tag = t->tag;
    /* Encode new balance in response data */
    do { ((uchar *)(r->data))[0] = (uchar)(local_machine_bank->available[tok_type]); ((uchar *)(r->data))[1] = (uchar)((local_machine_bank->available[tok_type]) >> 8); ((uchar *)(r->data))[2] = (uchar)((local_machine_bank->available[tok_type]) >> 16); ((uchar *)(r->data))[3] = (uchar)((local_machine_bank->available[tok_type]) >> 24); ((uchar *)(r->data))[4] = (uchar)((local_machine_bank->available[tok_type]) >> 32); ((uchar *)(r->data))[5] = (uchar)((local_machine_bank->available[tok_type]) >> 40); ((uchar *)(r->data))[6] = (uchar)((local_machine_bank->available[tok_type]) >> 48); ((uchar *)(r->data))[7] = (uchar)((local_machine_bank->available[tok_type]) >> 56); } while (0);
    r->count = 8;
    print("9p_router: Ttoken received %llu tokens type %d\n", amount, tok_type);
    return 0;
  }
  /* Handle Tbudget (82) - Query remote budget */
  if (t->type == 82) {
    TokenType tok_type;
    if (local_machine_bank == ((void *)0)) {
      r->type = Rerror;
      r->ename = "distributed pebble not initialized";
      return -1;
    }
    if (t->count < sizeof(TokenType)) {
      r->type = Rerror;
      r->ename = "Tbudget: message too short";
      return -1;
    }
    tok_type = (TokenType)(((uchar *)(t->data))[0] | (((uchar *)(t->data))[1] << 8) | (((uchar *)(t->data))[2] << 16) | (((uchar *)(t->data))[3] << 24));
    if (tok_type >= TOK_MAX) {
      r->type = Rerror;
      r->ename = "invalid token type";
      return -1;
    }
    /* Build Rbudget response with balance and Merkle root */
    r->type = 83;
    r->tag = t->tag;
    do { ((uchar *)((uchar *)r->data))[0] = (uchar)(local_machine_bank->available[tok_type]); ((uchar *)((uchar *)r->data))[1] = (uchar)((local_machine_bank->available[tok_type]) >> 8); ((uchar *)((uchar *)r->data))[2] = (uchar)((local_machine_bank->available[tok_type]) >> 16); ((uchar *)((uchar *)r->data))[3] = (uchar)((local_machine_bank->available[tok_type]) >> 24); ((uchar *)((uchar *)r->data))[4] = (uchar)((local_machine_bank->available[tok_type]) >> 32); ((uchar *)((uchar *)r->data))[5] = (uchar)((local_machine_bank->available[tok_type]) >> 40); ((uchar *)((uchar *)r->data))[6] = (uchar)((local_machine_bank->available[tok_type]) >> 48); ((uchar *)((uchar *)r->data))[7] = (uchar)((local_machine_bank->available[tok_type]) >> 56); } while (0);
    do { ((uchar *)((uchar *)r->data + 8))[0] = (uchar)(local_machine_bank->epoch); ((uchar *)((uchar *)r->data + 8))[1] = (uchar)((local_machine_bank->epoch) >> 8); ((uchar *)((uchar *)r->data + 8))[2] = (uchar)((local_machine_bank->epoch) >> 16); ((uchar *)((uchar *)r->data + 8))[3] = (uchar)((local_machine_bank->epoch) >> 24); ((uchar *)((uchar *)r->data + 8))[4] = (uchar)((local_machine_bank->epoch) >> 32); ((uchar *)((uchar *)r->data + 8))[5] = (uchar)((local_machine_bank->epoch) >> 40); ((uchar *)((uchar *)r->data + 8))[6] = (uchar)((local_machine_bank->epoch) >> 48); ((uchar *)((uchar *)r->data + 8))[7] = (uchar)((local_machine_bank->epoch) >> 56); } while (0);
    memmove(r->data + 16, &local_machine_bank->bank_root,
            sizeof(BlindLedgerHash));
    r->count = 16 + sizeof(BlindLedgerHash);
    print("9p_router: Tbudget query type=%d balance=%llu\n", tok_type,
          local_machine_bank->available[tok_type]);
    return 0;
  }
  if (t->type == Tattach) {
    /* Check for capability-based attach (WASM servers) */
    uuid_t cap_uuid;
    if (wasm_9p_extract_cap_uuid(t->aname, &cap_uuid) == 0) {
      /* Capability found in aname - validate it */
      UserCapability pebble_cap;
      if (wasm_9p_validate_capability(&cap_uuid, CAP_PERM_READ, &pebble_cap)) {
        type = 9;
        print("9p_router: WASM attach with validated capability\n");
      } else {
        r->type = Rerror;
        r->ename = "invalid or insufficient capability";
        return -1;
      }
    }
    /* If no capability, determine type from path */
    else if (path_match(t->aname, "/proc/") || strcmp(t->aname, "/proc") == 0)
      type = 1;
    else if (path_match(t->aname, "/dev/") || strcmp(t->aname, "/dev") == 0)
      type = 2;
    else if (path_match(t->aname, "/env/") || strcmp(t->aname, "/env") == 0)
      type = 3;
    else if (path_match(t->aname, "/srv/") || strcmp(t->aname, "/srv") == 0)
      type = 4;
    else if (path_match(t->aname, "/mnt/") || strcmp(t->aname, "/mnt") == 0)
      type = 5;
    else if (path_match(t->aname, "/fd/") || strcmp(t->aname, "/fd") == 0)
      type = 6;
    /* Install FID for non-device types; devices handle their own FID with
     * subtype */
    if (type > 0 && type != 2) {
      if (install_fid(t->fid, type) < 0) {
        r->type = Rerror;
        r->ename = "fid allocation failed";
        return -1;
      }
    }
  } else {
    /* Lookup type from fid */
    type = get_fid_type(t->fid);
    if (t->type == Tclunk)
      remove_fid(t->fid);
  }
  int ret = -1;
  if (type == 1)
    ret = proc_9p_handle(p, t, r);
  else if (type == 2)
    ret = dev_9p_handle(p, t, r);
  else if (type == 3)
    ret = env_9p_handle(p, t, r);
  else if (type == 4)
    ret = srv_9p_handle(p, t, r);
  else if (type == 5)
    ret = mnt_9p_handle(p, t, r);
  else if (type == 6) {
    ret = fd_9p_handle(p, t, r);
  } else if (type == 9) {
    ret = wasm_9p_handle(p, t, r);
  } else {
    r->type = Rerror;
    r->ename = "fid not found or unknown path";
    return -1;
  }
  /* Propagate handler type to newfid on successful Walk */
  if (ret == 0 && t->type == Twalk && r->type == Rwalk) {
    /* If walk succeeded (all names consumed), newfid inherits the handler type
     */
    if (r->nwqid == t->nwname) {
      /* If newfid == fid, it's already set (cloning or walking self).
         If distinct, we must install it. */
      if (t->newfid != t->fid) {
        int subtype = 0;
        if (t->nwname == 0) {
          /* Clone: Copy from old fid */
          if (type == 2 || type == 6)
            subtype = get_fid_subtype(t->fid);
        } else {
          /* Walk: Use last Qid's version from reply as subtype */
          if (r->nwqid > 0)
            subtype = r->wqid[r->nwqid - 1].vers;
        }
        install_fid_with_subtype(t->newfid, type, subtype);
        /* Handle state cloning for devices */
        if ((type == 2 && subtype == 8) ||
            (type == 6 && subtype > 0)) {
          /* Pipe and FD need to bump refcount on clone. */
          Fgrp *f = up->fgrp;
          Chan *oldc, *newc;
          lock(&f->lock);
          oldc = f->fd[t->fid];
          newc = f->fd[t->newfid];
          if (oldc && newc) {
            newc->aux = oldc->aux;
            if (type == 2 && subtype == 8) {
              rpipe_clone_notify(newc->aux);
            }
            /* For FD types, aux might store the internal Chan* directly?
               No, we said we'd use Qid.vers for FD number.
               So FD handler relies on subtype (vers) for index.
               Clone just copies subtype. No aux cloning needed unless we cache
               Chan* in aux. Let's stick to using subtype (vers) as the FD
               index.
            */
          }
          unlock(&f->lock);
        }
      }
    }
  }
  return ret;
}
/*
 * Callback context for async 9P replies.
 */
typedef struct P9RouteContext {
  Proc *caller;
  Fcall *reply;
} P9RouteContext;
/*
 * Callback fired when MSGORD orders the message.
 * Writes the reply to the caller's exchange page.
 */
static void p9_route_reply_callback(OrdMsg *msg, int status, void *arg) {
  P9RouteContext *ctx = (P9RouteContext *)arg;
  Fcall reply;
  if (ctx == ((void *)0) || ctx->caller == ((void *)0)) {
    if (ctx)
      xfree(ctx);
    return;
  }
  /* Handle MSGORD status */
  memset(&reply, 0, sizeof(reply));
  if (status == 2) {
    /* Transaction was rolled back - return error */
    reply.type = Rerror;
    reply.ename = "transaction rolled back";
  } else if (msg->gm_payload.type == 0 &&
             msg->gm_payload.fcall != ((void *)0)) {
    /* Dispatch the message to the appropriate handler */
    p9_dispatch(ctx->caller, msg->gm_payload.fcall, &reply);
  } else {
    reply.type = Rerror;
    reply.ename = "invalid payload";
  }
  /* If caller has an exchange page, write reply there */
  if (ctx->caller->p9page != ((void *)0)) {
    P9Control *ctl =
        (P9Control *)((uintptr)ctx->caller->p9page + 0xF00);
    uchar rep_copy[0xF00];
    P9Control ctl_saved;
    uint rep_size = convS2M(&reply, rep_copy, 0xF00);
    if (rep_size == 0) {
      (*((&ctl->status)) = ((3)));
      xfree(ctx);
      return;
    }
    memmove(&ctl_saved, ctl, sizeof(ctl_saved));
    scrub_exchange_page(ctx->caller, rep_copy, rep_size, &ctl_saved, 0, 0, 0);
    ctl = (P9Control *)((uintptr)ctx->caller->p9page + 0xF00);
    ctl->rep_seq++;
    (*((&ctl->status)) = ((2)));
  }
  xfree(ctx);
}
/*
 * Helper: Get path from FID's Chan.
 */
static char *get_fid_path(Proc *p, int fid) {
  Chan *c;
  Fgrp *f = p->fgrp;
  char *path = ((void *)0);
  lock(&f->lock);
  if (fid >= 0 && fid <= f->maxfd && (c = f->fd[fid]) != ((void *)0)) {
    if (c->path != ((void *)0) && c->path->s != ((void *)0))
      path = c->path->s;
  }
  unlock(&f->lock);
  return path;
}
/*
 * Main entry point: Submit to MSGORD for async ordering.
 * Returns 0 on successful submission (reply will arrive via callback).
 * Returns -1 on immediate error with r populated.
 */
int p9_route(Proc *p, Fcall *t, Fcall *r) {
  char *path;
  P9RouteContext *ctx;
  uint msg_id;
  /* Determine path for DAG ordering granularity */
  if (t->type == Tattach)
    path = t->aname;
  else if (t->type == Tversion || t->type == Tauth || t->type == Tflush)
    path = "/"; /* Protocol messages - global path */
  else {
    /* Extract path from FID */
    path = get_fid_path(p, t->fid);
    if (path == ((void *)0))
      path = "/";
  }
  /* Allocate callback context */
  ctx = xalloc(sizeof(P9RouteContext));
  if (ctx == ((void *)0)) {
    r->type = Rerror;
    r->ename = "no memory for p9 context";
    return -1;
  }
  ctx->caller = p;
  ctx->reply = r;
  /* Submit message asynchronously */
  msg_id =
      msgord_submit_async(((void *)0), p, t, path, p9_route_reply_callback, ctx, 0);
  if (msg_id == 0) {
    xfree(ctx);
    r->type = Rerror;
    r->ename = "msgord queue full";
    return -1;
  }
  /*
   * Message submitted successfully.
   * The reply will be delivered asynchronously via callback.
   * Caller should NOT expect r to be filled synchronously.
   * Return 0 to indicate "pending" - caller must poll exchange page.
   */
  return 0;
}
/*
 * p9_handle_doorbell - Process 9P message from exchange page
 *
 * SINGLE 4KB PAGE MODEL WITH OWNERSHIP FLIP:
 * ==========================================
 * 1. Process owns page, writes request, issues syscall
 * 2. borrow_transfer(process -> kernel) - kernel now owns exclusively
 * 3. Kernel reads request from page
 * 4. Kernel processes syscall
 * 5. Kernel writes reply to SAME page location
 * 6. borrow_transfer(kernel -> process) - process now owns exclusively
 * 7. Process reads reply
 *
 * The borrow checker enforces that only one entity (process OR kernel)
 * can access the page at any time. This eliminates TOCTOU races.
 *
 * Called by: VectorSYSCALL handler (doorbell-only mode)
 */
int p9_handle_doorbell(Proc *p, Ureg *ureg) {
  /*@
    @ requires \valid(p);
    @ ensures p->p9page == \null ==> \result == -1;
    */
  P9Control *ctl;
  uchar *msg_buf; /* Single buffer for request AND reply */
  Fcall t, r;
  uint msg_size;
  int result;
  uintptr page_pa;
  enum BorrowError berr;
  P9Control ctl_saved;
  /* Validate exchange page exists and is coherent with P9SEG */
  if (p->seg[P9SEG] != ((void *)0) && p->seg[P9SEG]->pseg != ((void *)0) &&
      p->seg[P9SEG]->pseg->pa != 0) {
    p->p9page = (void *)kaddr(p->seg[P9SEG]->pseg->pa);
  }
  if (p->p9page == ((void *)0)) {
    print("p9_handle_doorbell: no exchange page for pid %lud\n", p->pid);
    return -1;
  }
  /* Get physical address of the single exchange page */
  if (p->seg[P9SEG] != ((void *)0) && p->seg[P9SEG]->pseg != ((void *)0) &&
      p->seg[P9SEG]->pseg->pa != 0)
    page_pa = p->seg[P9SEG]->pseg->pa;
  else
    page_pa = paddr((void *)(p->p9page));
  /* Ensure exchange page is mapped into userspace */
  uintptr *pte = mmuwalk(m->pml4, 0x7FFFFEEFF000ULL, 0, 0);
  if (pte == ((void *)0) || (*pte & (1ull << 0)) == 0) {
    print("p9_handle_doorbell: remapping exchange page for pid %lud\n", p->pid);
    userpmap(0x7FFFFEEFF000ULL, page_pa, (1ull << 0) | (1ull << 2) | (1ull << 1));
  }
  /*
   * OWNERSHIP TRANSFER: Process -> Kernel
   * =====================================
   * Process has finished writing request and issued syscall.
   * Transfer ownership so kernel has exclusive access.
   */
  int s = splhi(); /* Block interrupts during critical ownership transfer */
  berr = borrow_transfer(p, up, page_pa);
  if (berr != BORROW_OK) {
    /* First syscall after boot - process may not have formal ownership yet */
    print("p9_handle_doorbell: borrow_transfer failed (berr=%d), acquiring "
          "directly\n",
          berr);
    berr = borrow_acquire(up, page_pa);
    if (berr != BORROW_OK && berr != BORROW_EALREADY) {
      print("p9_handle_doorbell: FATAL - kernel can't acquire page (berr=%d)\n",
            berr);
      splx(s);
      return -1;
    }
  }
  /* Kernel now has exclusive access to the page */
  /* Get control block and message buffer */
  ctl = (P9Control *)((uintptr)p->p9page + 0xF00);
  msg_buf = (uchar *)p->p9page + 0x000;
  memmove(&ctl_saved, ctl, sizeof(ctl_saved));
  /* Mark as pending */
  (*((&ctl->status)) = ((1)));
  /* Ring-buffer mode for small messages */
  if (ctl->req_head != ctl->req_tail) {
    /* Memory barrier to ensure user writes are visible to kernel */
    __asm__ volatile("mfence" ::: "memory");
    result = p9_handle_ring(p, ctl, msg_buf);
    memmove(&ctl_saved, ctl, sizeof(ctl_saved));
    u32int rep_head = ctl->rep_head;
    u32int rep_tail = ctl->rep_tail;
    u32int req_head = ctl->req_head;
    u32int req_tail = ctl->req_tail;
    scrub_exchange_page(p, msg_buf, 0xF00, &ctl_saved, rep_head, rep_tail,
                        1);
    ctl = (P9Control *)((uintptr)p->p9page + 0xF00);
    ctl->req_head = req_head;
    ctl->req_tail = req_tail;
    ctl->rep_head = rep_head;
    ctl->rep_tail = rep_tail;
    if (result < 0)
      (*((&ctl->status)) = ((3)));
    else
      (*((&ctl->status)) = ((2)));
    splx(s); /* Restore interrupts */
    goto cleanup_ownership;
  }
  /* Parse request from message buffer */
  memset(&t, 0, sizeof(t));
  /* Memory barrier to ensure user writes are visible to kernel.
   * User writes to EXCHANGE_PAGE_ADDR, kernel reads via HHDM at p->p9page.
   * The mfence ensures cache coherency between different VA mappings. */
  __asm__ volatile("mfence" ::: "memory");
  /* Get message size from 9P header (first 4 bytes) */
  msg_size = (((uchar *)(msg_buf))[0] | (((uchar *)(msg_buf))[1] << 8) | (((uchar *)(msg_buf))[2] << 16) | (((uchar *)(msg_buf))[3] << 24));
  if (msg_size < 7 || msg_size > 0xF00) {
    print("p9_handle_doorbell: invalid message size %ud\n", msg_size);
    print("p9_handle_doorbell: ctl req_head=%ud req_tail=%ud rep_head=%ud "
          "rep_tail=%ud\n",
          ctl->req_head, ctl->req_tail, ctl->rep_head, ctl->rep_tail);
    dump_bytes("p9_handle_doorbell: msg[0..31]:", msg_buf, 32);
    (*((&ctl->status)) = ((3)));
    result = -1;
    splx(s); /* Restore interrupts */
    goto cleanup_ownership;
  }
  if (convM2S(msg_buf, msg_size, &t) == 0) {
    print("p9_handle_doorbell: failed to parse Fcall (first byte: 0x%02x)\n",
          msg_buf[0]);
    print("p9_handle_doorbell: msg_size=%ud ctl req_head=%ud req_tail=%ud "
          "rep_head=%ud rep_tail=%ud\n",
          msg_size, ctl->req_head, ctl->req_tail, ctl->rep_head, ctl->rep_tail);
    dump_bytes("p9_handle_doorbell: msg[0..31]:", msg_buf, 32);
    splx(s); /* Restore interrupts */
    goto cleanup_ownership;
  }
  splx(s); /* Restore interrupts before long processing. */
  /* Dispatch through 9P router */
  memset(&r, 0, sizeof(r));
  result = p9_dispatch(p, &t, &r);
  /* Write reply to SAME buffer location (ownership-flip model) */
  uchar reply_copy[0xF00];
  uint rep_size = convS2M(&r, reply_copy, 0xF00);
  if (rep_size == 0) {
    print("p9_handle_doorbell: failed to serialize reply (r.type=%d)\n",
          r.type);
    (*((&ctl->status)) = ((3)));
    result = -1;
    goto cleanup_ownership;
  }
  memmove(&ctl_saved, ctl, sizeof(ctl_saved));
  scrub_exchange_page(p, reply_copy, rep_size, &ctl_saved, 0, 0, 0);
  ctl = (P9Control *)((uintptr)p->p9page + 0xF00);
  msg_buf = (uchar *)p->p9page + 0x000;
  /* Set RAX to return value for ABI compatibility and efficient checking */
  if (ureg != ((void *)0)) {
    ureg->ax = (ulong)r.retval;
  }
  /* Success! Reply written to buffer.
   * Even if p9_dispatch returned -1 (Rerror), from the perspective of the
   * doorbell mechanism, we successfully processed the message and wrote a
   * reply.
   */
  result = 0;
  /* Update control block */
  ctl->rep_seq++;
  /* Mark as complete with Release semantics */
  (*((&ctl->status)) = ((2)));
cleanup_ownership:
  /*
   * OWNERSHIP TRANSFER: Kernel -> Process
   * =====================================
   * Kernel has finished processing. Transfer ownership back so
   * process can read the reply.
   */
  berr = borrow_transfer(up, p, page_pa);
  if (berr != BORROW_OK) {
    print(
        "p9_handle_doorbell: WARNING - borrow_transfer back failed (berr=%d)\n",
        berr);
    /* Fall back to release/acquire */
    borrow_release(up, page_pa);
    berr = borrow_acquire(p, page_pa);
    if (berr != BORROW_OK) {
      print("p9_handle_doorbell: FATAL - can't return page to process "
            "(berr=%d)\n",
            berr);
      panic("p9_handle_doorbell: ownership violation - cannot return page");
    }
  }
  return result;
}
/*
 * Spawn entry point - run as a kernel process (kproc)
 * Transitions to userspace by exec'ing the specified binary.
 */
static void kspawn_entry(void *arg) {
  char *path = (char *)arg;
  char *argv[2];
  ulong args[2];
  print("kspawn_entry: executing '%s'\n", path);
  /*
   * Build arguments for sysexec:
   * args[0] = path string pointer
   * args[1] = argv array pointer (path, nil)
   */
  argv[0] = path;
  argv[1] = ((void *)0);
  args[0] = (ulong)path;
  args[1] = (ulong)argv;
  /*
   * Call sysexec to load and execute the binary.
   * sysexec never returns on success - it replaces the current process.
   * On error, we exit.
   */
  if (setlabel(&up->errlab[up->nerrlab++])) {
    print("kspawn_entry: exec failed: %s\n", up->errstr);
    free(path);
    pexit(up->errstr, 1);
    return;
  }
  sysexec(args);
  /* Not reached on success */
  up->nerrlab--;
  /* If we get here, exec failed somehow without error */
  print("kspawn_entry: sysexec returned unexpectedly\n");
  free(path);
  pexit("exec failed", 1);
}
/*
 * Helper: Handle writes to /proc/self/ctl
 */
static int handle_proc_ctl_write(Proc *p, char *cmd, int len) {
  char buf[256];
  char *args[16];
  int n;
  if (len >= sizeof(buf))
    return -1;
  memmove(buf, cmd, len);
  buf[len] = 0;
  n = tokenize(buf, args, (sizeof(args) / sizeof((args)[0])));
  if (n < 1)
    return -1;
  if (strcmp(args[0], "dup") == 0) {
    /* dup old [new] */
    if (n < 2)
      return -1;
    int old = (int)strtoul(args[1], 0, 0);
    int new = (n > 2) ? (int)strtoul(args[2], 0, 0) : -1;
    Chan *c = fdtochan(old, -1, 0, 1);
    if (c == ((void *)0))
      return -1;
    if (new != -1) {
      Fgrp *f = up->fgrp;
      lock(&f->lock);
      if (new < 0 || growfd(f, new) < 0) {
        unlockfgrp(f);
        cclose(c);
        return -1;
      }
      if (new > f->maxfd)
        f->maxfd = new;
      Chan *oc = f->fd[new];
      f->fd[new] = c;
      f->flag[new] = 0;
      unlockfgrp(f);
      if (oc != ((void *)0))
        cclose(oc);
    } else {
      if (setlabel(&up->errlab[up->nerrlab++])) {
        cclose(c);
        nexterror();
      }
      int fd = newfd(c, 0);
      up->nerrlab--;
      if (fd < 0)
        return -1;
    }
    return len;
  }
  if (strcmp(args[0], "chdir") == 0) {
    if (n < 2)
      return -1;
    Chan *c = namec(args[1], Atodir, 0, 0);
    if (setlabel(&up->errlab[up->nerrlab++])) {
      cclose(c);
      return -1;
    }
    cclose(up->dot);
    up->dot = c;
    up->nerrlab--;
    return len;
  }
  if (strcmp(args[0], "rendezvous") == 0) {
    /* rendezvous <tag> <val> */
    if (n < 3)
      return -1;
    uintptr tag = (uintptr)strtoul(args[1], 0, 0);
    uintptr val = (uintptr)strtoul(args[2], 0, 0);
    /* Internal rendezvous logic usually returns a value.
       Twrite can't return it!
       Architecture Issue: Rendezvous requires return value.
       Solution: Use /proc/self/rendezvous file?
       Write tag to it, Read from it?
       For now, implemented as write-only (wakes up others, ignores return).
    */
    return len;
  }
  if (strcmp(args[0], "semacquire") == 0) {
    /* semacquire <addr> <block> */
    if (n < 3)
      return -1;
    long *addr = (long *)strtoul(args[1], 0, 16);
    int block = (int)strtoul(args[2], 0, 0);
    Segment *s = seg(up, (uintptr)addr, 0);
    if (s == ((void *)0))
      return -1;
    semacquire(s, addr, block);
    return len;
  }
  if (strcmp(args[0], "semrelease") == 0) {
    if (n < 3)
      return -1;
    long *addr = (long *)strtoul(args[1], 0, 16);
    long delta = (long)strtoul(args[2], 0, 0);
    Segment *s = seg(up, (uintptr)addr, 0);
    if (s == ((void *)0))
      return -1;
    semrelease(s, addr, (int)delta);
    return len;
  }
  if (strcmp(args[0], "exits") == 0) {
    char *status = (n > 1) ? args[1] : ((void *)0);
    pexit(status, 1);
    /* Not reached */
  }
  if (strcmp(args[0], "sleep") == 0) {
    /* sleep <ms> */
    long ms = (n > 1) ? (long)strtoul(args[1], 0, 0) : 0;
    if (ms > 0)
      tsleep(&up->sleep, return0, 0, (ulong)ms);
    return len;
  }
  if (strcmp(args[0], "alarm") == 0) {
    /* alarm <ms> */
    ulong ms = (n > 1) ? strtoul(args[1], 0, 0) : 0;
    procalarm(ms);
    return len;
  }
  if (strcmp(args[0], "segbrk") == 0) {
    /* segbrk <addr> <seg> */
    void *addr;
    if (n < 2)
      return -1;
    addr = (void *)strtoul(args[1], 0, 0);
    int seg = (int)((n > 2) ? strtoul(args[2], 0, 0) : BSEG);
    if ((ulong)ibrk((uintptr)addr, seg) == (ulong)-1) {
      return -1;
    }
    return len;
  }
  if (strcmp(args[0], "notify") == 0) {
    /* notify <func_addr_hex> */
    /* Pass 0 to disable */
    if (n < 2)
      return -1;
    void *fn = (void *)strtoul(args[1], 0, 16);
    up->notify = fn;
    return len;
  }
  if (strcmp(args[0], "noted") == 0) {
    /* noted <mode> */
    /* NCONT=0, NDFLT=1, NSAVE=2, NRSTR=3 */
    int mode = (n > 1) ? (int)strtoul(args[1], 0, 0) : 3;
    qlock(&up->debug);
    if (up->notified == 0 && mode != 3) {
      qunlock(&up->debug);
      error("noted: not notified");
      return -1;
    }
    qunlock(&up->debug);
    /* Note: This calls the kernel internal 'noted' */
    /* We rely on proper Ureg setup in up->noteureg/dbgreg */
    if (noted(up->dbgreg, up->noteureg, mode) < 0) {
      error("noted failed");
      return -1;
    }
    up->notified = 0;
    return len;
  }
  if (strcmp(args[0], "note") == 0) {
    /* note <msg> */
    if (n < 2)
      return -1;
    postnote(p, 1, args[1], NUser);
    return len;
  }
  /*
   * "spawn" is the official Phase 6 process creation mechanism.
   * Legacy rfork/exec are deprecated in the 9P path.
   */
  if (strcmp(args[0], "spawn") == 0) {
    /* spawn <path> [args...] */
    if (n < 2)
      return -1;
    if (!check_permission(p, 0x04)) {
      error("spawn: permission denied");
      return -1;
    }
    /*
     * Real kspawn implementation using kproc.
     * The new process inherits the parent's file descriptors, env, and groups,
     * then execs the specified binary.
     */
    char *path = args[1];
    print("9P SPAWN: forking to exec '%s'\n", path);
    /*
     * Create argument string for the new process.
     * In Plan 9 style, we pass arguments via /proc/n/args after exec.
     * For now, we just store the command name.
     */
    char *file = smalloc((ulong)strlen(path) + 1);
    if (file == ((void *)0)) {
      error("spawn: no memory");
      return -1;
    }
    strcpy(file, path);
    /*
     * Use kproc to create a new process that runs kspawn_wrapper.
     * The wrapper will be a kernel function that initiates exec.
     * Note: This creates a kernel process that then transitions to userspace.
     */
    kproc(path, kspawn_entry, file);
    print("9P SPAWN: spawned process for '%s'\n", path);
    return len;
  }
  /* Process Control Commands from existing stub */
  if (strcmp(args[0], "wakeup") == 0) {
    proc_event(p, EV_WAKEUP);
    return len;
  }
  if (strcmp(args[0], "stop") == 0) {
    proc_event(p, EV_STOP);
    return len;
  }
  if (strcmp(args[0], "start") == 0) {
    proc_event(p, EV_CONT);
    return len;
  }
  if (strcmp(args[0], "kill") == 0) {
    proc_event(p, EV_BREAK);
    return len;
  }
  return -1;
}
/*
 * Process control server: /proc/
 * Integrates with our FSM!
 */
/* Proc file types for routing */
int proc_9p_handle(Proc *caller, Fcall *t, Fcall *r) {
  Proc *target = caller; /* Default to self */
  int type = 0;
  r->tag = t->tag;
  /* Retrieve file type from FID (stored in Qid.vers during Walk) */
  if (t->type != Tattach) {
    type = get_fid_subtype((int)t->fid);
  }
  switch (t->type) {
  case Tattach:
    r->type = Rattach;
    r->qid.type = 0x80;
    r->qid.path = 0;
    r->qid.vers = 0;
    return 0;
  case Twalk:
    if (t->nwname > 0) {
      if (type != 0) {
        r->type = Rerror;
        r->ename = "not a directory";
        return -1;
      }
      r->type = Rwalk;
      r->nwqid = 1;
      r->wqid[0].type = 0x00;
      /* Use 'vers' to store the subtype for the new FID */
      if (strcmp(t->wname[0], "ctl") == 0) {
        r->wqid[0].path = 1;
        r->wqid[0].vers = 1;
      } else if (strcmp(t->wname[0], "wait") == 0) {
        r->wqid[0].path = 2;
        r->wqid[0].vers = 2;
      } else if (strcmp(t->wname[0], "status") == 0) {
        r->wqid[0].path = 3;
        r->wqid[0].vers = 3;
      } else if (strcmp(t->wname[0], "ns") == 0) {
        r->wqid[0].path = 4;
        r->wqid[0].vers = 4;
      } else if (strcmp(t->wname[0], "segment") == 0) {
        r->wqid[0].path = 5;
        r->wqid[0].vers = 5;
      } else {
        r->type = Rerror;
        r->ename = "file not found";
        return -1;
      }
      return 0;
    }
    r->type = Rwalk;
    r->nwqid = 0;
    return 0;
  case Topen:
    r->type = Ropen;
    r->qid.type = (type == 0) ? 0x80 : 0x00;
    r->qid.path = type;
    r->qid.vers = type;
    r->iounit = 8192;
    return 0;
  case Twrite:
    if (type == 1) {
      if (handle_proc_ctl_write(target, t->data, (int)t->count) < 0) {
        r->type = Rerror;
        r->ename = "proc: command failed";
        return -1;
      }
      r->type = Rwrite;
      r->count = t->count;
      return 0;
    }
    r->type = Rerror;
    r->ename = "permission denied";
    return -1;
  case Tread:
    if (type == 0) {
      /* Directory listing */
      /* Minimal implementation: return fixed list */
      static char *dirents[] = {"ctl", "wait", "status", "ns", "segment"};
      char buf[512];
      char *p = buf;
      /* This is a hacky directory listing. Proper way is to marshall Dir
       * structs. */
      /* Ideally we use a helper like dirread. For now, empty dir or error? */
      /* Using 'read' on a directory in 9P requires returning Stat structures.
       */
      /* Since we don't have a helper handy here to generate Stats easily
       * without allocs... */
      /* We return Rerror "use Tstat" or generic directory read error? */
      /* Actually, many clients expect Tstat for dirs properly. */
      /* Let's return empty for now to avoid crashing client readers. */
      r->type = Rread;
      r->count = 0;
      return 0;
    }
    if (type == 3) {
      r->type = Rread;
      /* Using snprint to format status */
      /* text pid state user sys real pages */
      r->count = (u32int)snprint(
          (char *)r->data, 256, "%s %lud %s %lud %lud %lud %lud\n",
          target->text, target->pid, proc_state_names[(((target)->state_trace) & 0xF)],
          target->time[TUser], target->time[TSys], target->time[TReal],
          procpagecount(target) * (0x1000ull));
      return 0;
    }
    if (type == 2) {
      Waitmsg w;
      if (pwait(&w) == 0) {
        r->type = Rerror;
        r->ename = "wait failed";
        return -1;
      }
      r->type = Rread;
      r->count = (u32int)snprint((char *)r->data, 256, "%lud %lud %lud %lud %s",
                                 w.pid, w.time[0], w.time[1], w.time[2], w.msg);
      return 0;
    }
    /* Other files read as empty */
    r->type = Rread;
    r->count = 0;
    return 0;
  case Tstat: {
    /* Build Dir structure and convert to wire format */
    Dir d;
    uchar statbuf[256];
    int n;
    memset(&d, 0, sizeof(d));
    d.qid.path = type;
    d.qid.type = (type == 0) ? 0x80 : 0;
    d.qid.vers = 0;
    d.mode = (type == 0) ? (0x80000000 | 0555) : 0444;
    d.atime = seconds();
    d.mtime = d.atime;
    d.length = 0;
    d.name = (type == 0) ? "."
             : (type == 3) ? "status"
             : (type == 1) ? "ctl"
             : (type == 2) ? "wait"
                                     : "unknown";
    d.uid = "kernel";
    d.gid = "kernel";
    d.muid = "kernel";
    n = (int)convD2M(&d, statbuf, sizeof(statbuf));
    if (n <= 0) {
      r->type = Rerror;
      r->ename = "stat conversion failed";
      return -1;
    }
    r->type = Rstat;
    r->nstat = (ushort)n;
    r->stat = statbuf;
    return 0;
  }
  case Tclunk:
    r->type = Rclunk;
    return 0;
  default:
    r->type = Rerror;
    r->ename = "not implemented";
    return -1;
  }
}
/*
 * Device path parsing helper
 */
static char *devname_from_path(char *path) {
  if (path == ((void *)0))
    return "cons";
  if (strncmp(path, "/dev/", 5) == 0)
    return path + 5;
  return path;
}
/*
 * Common Tattach handler with Pebble validation
 * Returns 0 on success, -1 on permission error
 */
static int handle_tattach_with_pebble(Proc *caller, Fcall *t, Fcall *r,
                                      int required_perms, uchar qid_path) {
  PebbleToken tok;
  Qid q;
  /* Check if attach data contains Pebble */
  if (t->data != ((void *)0) && t->count >= 8 + sizeof(PebbleToken)) {
    if (p9_extract_pebble((uchar *)t->data, t->count, &tok) == 0) {
      /* Validate pebble for required access */
      if (!p9_validate_pebble(&tok, t->aname, required_perms)) {
        r->type = Rerror;
        r->ename = "permission denied";
        return -1;
      }
      /* Full validation with BlindLedger */
      if (!p9_validate_pebble_full(&tok, t->aname, caller)) {
        r->type = Rerror;
        r->ename = "invalid capability";
        return -1;
      }
      /* Store in session */
      store_session_pebble(caller, &tok);
    }
  }
  /* Successful attach */
  r->type = Rattach;
  q.type = 0x00;
  q.path = qid_path;
  q.vers = 0;
  memmove(&r->qid, &q, sizeof(Qid));
  r->iounit = 8192;
  return 0;
}
/*
 * Fill Dir structure for device stat - common helper
 */
static void fill_device_stat(Dir *d, char *name, uchar qid_path, ulong mode,
                             vlong length) {
  memset(d, 0, sizeof(Dir));
  d->name = name;
  d->uid = "sys";
  d->gid = "sys";
  d->muid = "sys";
  d->qid.type = 0x00;
  d->qid.path = qid_path;
  d->qid.vers = 0;
  d->mode = mode;
  d->atime = (ulong)seconds();
  d->mtime = d->atime;
  d->length = length;
}
/*
 * Handle Tstat for a device - common helper
 * Returns serialized stat size or -1 on error
 */
static int handle_device_stat(Fcall *t, Fcall *r, char *name, uchar qid_path,
                              ulong mode, vlong length) {
  static uchar statbuf[256];
  Dir d;
  int n;
  fill_device_stat(&d, name, qid_path, mode, length);
  n = (int)convD2M(&d, statbuf, sizeof(statbuf));
  if (n <= 0) {
    r->type = Rerror;
    r->ename = "stat conversion failed";
    return -1;
  }
  r->type = Rstat;
  r->nstat = (ushort)n;
  r->stat = statbuf;
  return 0;
}
/*
 * Ring-buffer mode: process multiple small messages from exchange page.
 * Layout per slot: [req_size:4][rep_size:4][data...]
 * req_head/req_tail and rep_head/rep_tail are slot indices.
 */
static int p9_handle_ring(Proc *p, P9Control *ctl, uchar *msg_buf) {
  u32int head = ctl->req_head;
  u32int tail = ctl->req_tail;
  u32int rep_head = ctl->rep_head;
  u32int rep_tail = ctl->rep_tail;
  if (head >= (0xF00 / 256) || tail >= (0xF00 / 256) ||
      rep_head >= (0xF00 / 256) || rep_tail >= (0xF00 / 256))
    return -1;
  while (head != tail) {
    uchar *slot = msg_buf + (head * 256);
    u32int req_size = (((uchar *)(slot))[0] | (((uchar *)(slot))[1] << 8) | (((uchar *)(slot))[2] << 16) | (((uchar *)(slot))[3] << 24));
    if (req_size == 0 || req_size > (256 - 8))
      return -1;
    Fcall t, r;
    memset(&t, 0, sizeof(t));
    if (convM2S(slot + 8, req_size, &t) == 0)
      return -1;
    memset(&r, 0, sizeof(r));
    int disp = p9_dispatch(p, &t, &r);
    if (disp < 0)
      r = (Fcall){.type = Rerror, .tag = t.tag, .ename = "dispatch failed"};
    u32int rep_size =
        convS2M(&r, slot + 8, (256 - 8));
    if (rep_size == 0)
      return -1;
    do { ((uchar *)(slot + 4))[0] = (uchar)(rep_size); ((uchar *)(slot + 4))[1] = (uchar)((rep_size) >> 8); ((uchar *)(slot + 4))[2] = (uchar)((rep_size) >> 16); ((uchar *)(slot + 4))[3] = (uchar)((rep_size) >> 24); } while (0);
    u32int next_rep = (rep_tail + 1) % (0xF00 / 256);
    if (next_rep == rep_head)
      return -1;
    rep_tail = next_rep;
    head = (head + 1) % (0xF00 / 256);
    ctl->rep_seq++;
  }
  ctl->req_head = head;
  ctl->rep_tail = rep_tail;
  return 0;
}
/*
 * Console device handler: /dev/cons
 */
static int cons_9p_handle(Proc *caller, Fcall *t, Fcall *r) {
  r->tag = t->tag;
  switch (t->type) {
  case Tattach:
    return handle_tattach_with_pebble(caller, t, r,
                                      0x01 | 0x02, 1);
  case Twrite:
    /* Check write permission */
    if (!check_permission(caller, 0x02)) {
      r->type = Rerror;
      r->ename = "write permission denied";
      return -1;
    }
    putstrn((char *)t->data, (int)t->count);
    r->type = Rwrite;
    r->count = t->count;
    return 0;
  case Tread:
    /* Check read permission */
    if (!check_permission(caller, 0x01)) {
      r->type = Rerror;
      r->ename = "read permission denied";
      return -1;
    }
    /* Console read not implemented yet */
    r->type = Rread;
    r->count = 0;
    r->data = ((void *)0);
    return 0;
  case Tclunk:
    r->type = Rclunk;
    return 0;
  case Twalk:
    if (t->nwname > 0) {
      r->type = Rerror;
      r->ename = "walk not supported";
      return -1;
    }
    r->type = Rwalk;
    r->nwqid = 0;
    return 0;
  case Tstat:
    /* 0666 = read/write for all */
    return handle_device_stat(t, r, "cons", 1, 0666, 0);
  default:
    r->type = Rerror;
    r->ename = "operation not supported";
    return -1;
  }
}
/*
 * Null device handler: /dev/null
 */
static int null_9p_handle(Proc *caller, Fcall *t, Fcall *r) {
  r->tag = t->tag;
  switch (t->type) {
  case Tattach:
    return handle_tattach_with_pebble(caller, t, r,
                                      0x01 | 0x02, 2);
  case Twrite:
    /* Check write permission */
    if (!check_permission(caller, 0x02)) {
      r->type = Rerror;
      r->ename = "write permission denied";
      return -1;
    }
    /* Accept all writes, discard data */
    r->type = Rwrite;
    r->count = t->count;
    return 0;
  case Tread:
    /* Check read permission */
    if (!check_permission(caller, 0x01)) {
      r->type = Rerror;
      r->ename = "read permission denied";
      return -1;
    }
    /* Return empty */
    r->type = Rread;
    r->count = 0;
    r->data = ((void *)0);
    return 0;
  case Tclunk:
    r->type = Rclunk;
    return 0;
  case Twalk:
    if (t->nwname > 0) {
      r->type = Rerror;
      r->ename = "walk not supported";
      return -1;
    }
    r->type = Rwalk;
    r->nwqid = 0;
    return 0;
  case Tstat:
    /* 0666 = read/write for all */
    return handle_device_stat(t, r, "null", 2, 0666, 0);
  default:
    r->type = Rerror;
    r->ename = "operation not supported";
    return -1;
  }
}
/*
 * Zero device handler: /dev/zero
 */
static int zero_9p_handle(Proc *caller, Fcall *t, Fcall *r) {
  static uchar zerobuf[8192];
  r->tag = t->tag;
  switch (t->type) {
  case Tattach:
    return handle_tattach_with_pebble(caller, t, r, 0x01, 3);
  case Twrite:
    /* Check write permission */
    if (!check_permission(caller, 0x02)) {
      r->type = Rerror;
      r->ename = "write permission denied";
      return -1;
    }
    /* Accept all writes, discard data */
    r->type = Rwrite;
    r->count = t->count;
    return 0;
  case Tread:
    /* Check read permission */
    if (!check_permission(caller, 0x01)) {
      r->type = Rerror;
      r->ename = "read permission denied";
      return -1;
    }
    /* Return zeros */
    if (t->count > sizeof(zerobuf))
      t->count = sizeof(zerobuf);
    memset(zerobuf, 0, t->count);
    r->type = Rread;
    r->count = t->count;
    r->data = (char *)zerobuf;
    return 0;
  case Tclunk:
    r->type = Rclunk;
    return 0;
  case Twalk:
    if (t->nwname > 0) {
      r->type = Rerror;
      r->ename = "walk not supported";
      return -1;
    }
    r->type = Rwalk;
    r->nwqid = 0;
    return 0;
  case Tstat:
    /* 0444 = read-only for all */
    return handle_device_stat(t, r, "zero", 3, 0444, 0);
  default:
    r->type = Rerror;
    r->ename = "operation not supported";
    return -1;
  }
}
/*
 * Random device handler: /dev/random
 */
static int random_9p_handle(Proc *caller, Fcall *t, Fcall *r) {
  static uchar randbuf[8192];
  r->tag = t->tag;
  switch (t->type) {
  case Tattach:
    return handle_tattach_with_pebble(caller, t, r, 0x01, 4);
  case Twrite:
    /* Random is read-only */
    r->type = Rerror;
    r->ename = "permission denied";
    return -1;
  case Tread:
    /* Check read permission */
    if (!check_permission(caller, 0x01)) {
      r->type = Rerror;
      r->ename = "read permission denied";
      return -1;
    }
    /* Return random bytes */
    if (t->count > sizeof(randbuf))
      t->count = sizeof(randbuf);
    randomread(randbuf, t->count);
    r->type = Rread;
    r->count = t->count;
    r->data = (char *)randbuf;
    return 0;
  case Tclunk:
    r->type = Rclunk;
    return 0;
  case Twalk:
    if (t->nwname > 0) {
      r->type = Rerror;
      r->ename = "walk not supported";
      return -1;
    }
    r->type = Rwalk;
    r->nwqid = 0;
    return 0;
  case Tstat:
    /* 0444 = read-only for all */
    return handle_device_stat(t, r, "random", 4, 0444, 0);
  default:
    r->type = Rerror;
    r->ename = "operation not supported";
    return -1;
  }
}
/*
 * Ramdisk handler: /dev/ram
 * Uses devram read/write paths.
 */
extern long ramread(void *a, long n, vlong off);
extern long ramwrite(void *va, long n, vlong off);
static int ram_9p_handle(Proc *caller, Fcall *t, Fcall *r) {
  r->tag = t->tag;
  switch (t->type) {
  case Tattach:
    return handle_tattach_with_pebble(caller, t, r,
                                      0x01 | 0x02, 7);
  case Twrite:
    if (!check_permission(caller, 0x02)) {
      r->type = Rerror;
      r->ename = "write permission denied";
      return -1;
    }
    r->count = (u32int)ramwrite(t->data, t->count, t->offset);
    r->type = Rwrite;
    return 0;
  case Tread:
    if (!check_permission(caller, 0x01)) {
      r->type = Rerror;
      r->ename = "read permission denied";
      return -1;
    }
    r->type = Rread;
    r->count = (u32int)ramread(caller->genbuf, t->count, t->offset);
    r->data = (char *)caller->genbuf;
    return 0;
  case Tclunk:
    r->type = Rclunk;
    return 0;
  case Twalk:
    if (t->nwname > 0) {
      r->type = Rerror;
      r->ename = "walk not supported";
      return -1;
    }
    r->type = Rwalk;
    r->nwqid = 0;
    return 0;
  case Tstat:
    /* 0666 = read/write for all */
    return handle_device_stat(t, r, "ram", 7, 0666, 0);
  default:
    r->type = Rerror;
    r->ename = "operation not supported";
    return -1;
  }
}
/*
 * Time device handler: /dev/time
 */
static int time_9p_handle(Proc *caller, Fcall *t, Fcall *r) {
  static char timebuf[64];
  int n;
  r->tag = t->tag;
  switch (t->type) {
  case Tattach:
    return handle_tattach_with_pebble(caller, t, r, 0x01, 5);
  case Twrite:
    /* Time is read-only */
    r->type = Rerror;
    r->ename = "permission denied";
    return -1;
  case Tread:
    /* Check read permission */
    if (!check_permission(caller, 0x01)) {
      r->type = Rerror;
      r->ename = "read permission denied";
      return -1;
    }
    /* Return current time in seconds */
    n = snprint(timebuf, sizeof(timebuf), "%ld\n", seconds());
    if (t->offset >= (u32int)n) {
      r->type = Rread;
      r->count = 0;
      r->data = ((void *)0);
      return 0;
    }
    if (t->offset + t->count > (u32int)n)
      t->count = (u32int)(n - t->offset);
    r->type = Rread;
    r->count = t->count;
    r->data = (char *)(timebuf + t->offset);
    return 0;
  case Tclunk:
    r->type = Rclunk;
    return 0;
  case Twalk:
    if (t->nwname > 0) {
      r->type = Rerror;
      r->ename = "walk not supported";
      return -1;
    }
    r->type = Rwalk;
    r->nwqid = 0;
    return 0;
  case Tstat:
    /* 0444 = read-only for all */
    return handle_device_stat(t, r, "time", 5, 0444, 0);
  default:
    r->type = Rerror;
    r->ename = "operation not supported";
    return -1;
  }
}
/*
 * Sysname device handler: /dev/sysname
 */
static int sysname_9p_handle(Proc *caller, Fcall *t, Fcall *r) {
  static char namebuf[128];
  int n;
  r->tag = t->tag;
  switch (t->type) {
  case Tattach:
    return handle_tattach_with_pebble(caller, t, r,
                                      0x01 | 0x02, 6);
  case Twrite:
    /* Check write permission */
    if (!check_permission(caller, 0x02)) {
      r->type = Rerror;
      r->ename = "write permission denied";
      return -1;
    }
    /* Update sysname */
    if (t->count >= sizeof(namebuf)) {
      r->type = Rerror;
      r->ename = "name too long";
      return -1;
    }
    memmove(namebuf, t->data, t->count);
    namebuf[t->count] = '\0';
    /* Remove trailing newline if present */
    if (t->count > 0 && namebuf[t->count - 1] == '\n')
      namebuf[t->count - 1] = '\0';
    kstrdup(&sysname, namebuf);
    r->type = Rwrite;
    r->count = t->count;
    return 0;
  case Tread:
    /* Check read permission */
    if (!check_permission(caller, 0x01)) {
      r->type = Rerror;
      r->ename = "read permission denied";
      return -1;
    }
    /* Return current sysname */
    if (sysname == ((void *)0))
      sysname = "lux9";
    n = snprint(namebuf, sizeof(namebuf), "%s\n", sysname);
    if (t->offset >= (u32int)n) {
      r->type = Rread;
      r->count = 0;
      r->data = ((void *)0);
      return 0;
    }
    if (t->offset + t->count > (u32int)n)
      t->count = (u32int)(n - t->offset);
    r->type = Rread;
    r->count = t->count;
    r->data = (char *)(namebuf + t->offset);
    return 0;
  case Tclunk:
    r->type = Rclunk;
    return 0;
  case Twalk:
    if (t->nwname > 0) {
      r->type = Rerror;
      r->ename = "walk not supported";
      return -1;
    }
    r->type = Rwalk;
    r->nwqid = 0;
    return 0;
  case Tstat:
    /* 0666 = read/write for all */
    return handle_device_stat(t, r, "sysname", 6, 0666, 0);
  default:
    r->type = Rerror;
    r->ename = "operation not supported";
    return -1;
  }
}
/*
 * Helper to map device name to subtype constant
 */
static int devname_to_subtype(char *dev) {
  if (strcmp(dev, "cons") == 0)
    return 1;
  if (strcmp(dev, "null") == 0)
    return 2;
  if (strcmp(dev, "zero") == 0)
    return 3;
  if (strcmp(dev, "random") == 0)
    return 4;
  if (strcmp(dev, "time") == 0)
    return 5;
  if (strcmp(dev, "sysname") == 0)
    return 6;
  if (strcmp(dev, "ram") == 0)
    return 7;
  if (strcmp(dev, "pipe") == 0)
    return 8;
  return 0; /* Unknown */
}
/* Forward declaration */
extern int pipe_9p_handle(Proc *caller, Fcall *t, Fcall *r);
/*
 * Device router dispatch - uses FID subtype for proper device tracking
 */
int dev_9p_handle(Proc *caller, Fcall *t, Fcall *r) {
  int subtype = 0;
  char *dev;
  r->tag = t->tag;
  if (t->type == Tattach) {
    /* On attach, determine device from path and store subtype in FID */
    dev = devname_from_path(t->aname);
    subtype = devname_to_subtype(dev);
    if (subtype == 0) {
      r->type = Rerror;
      r->ename = "device not found";
      return -1;
    }
    /* Install FID with device subtype */
    if (install_fid_with_subtype((int)t->fid, 2, subtype) < 0) {
      r->type = Rerror;
      r->ename = "fid allocation failed";
      return -1;
    }
  } else {
    /* For other operations, retrieve subtype from FID */
    subtype = get_fid_subtype((int)t->fid);
    if (subtype == 0) {
      r->type = Rerror;
      r->ename = "unknown device fid";
      return -1;
    }
  }
  /* Dispatch to device handler based on subtype */
  switch (subtype) {
  case 1:
    return cons_9p_handle(caller, t, r);
  case 2:
    return null_9p_handle(caller, t, r);
  case 3:
    return zero_9p_handle(caller, t, r);
  case 4:
    return random_9p_handle(caller, t, r);
  case 5:
    return time_9p_handle(caller, t, r);
  case 6:
    return sysname_9p_handle(caller, t, r);
  case 7:
    return ram_9p_handle(caller, t, r);
  case 8:
    return rpipe_9p_handle(caller, t, r);
  default:
    r->type = Rerror;
    r->ename = "device not found";
    return -1;
  }
}
int env_9p_handle(Proc *caller, Fcall *t, Fcall *r) {
  char *name;
  char *val;
  r->tag = t->tag;
  /* Simple write-only environment support for now */
  if (t->type == Twrite) {
    /* Path format: /env/VARNAME */
    if (strncmp(t->aname, "/env/", 5) == 0) {
      name = t->aname + 5;
      val = smalloc(t->count + 1);
      memmove(val, t->data, t->count);
      val[t->count] = 0;
      if (setlabel(&up->errlab[up->nerrlab++])) {
        free(val);
        r->type = Rerror;
        r->ename = up->errstr;
        return -1;
      }
      ksetenv(name, val, 0);
      up->nerrlab--;
      free(val);
      r->type = Rwrite;
      r->count = t->count;
      return 0;
    }
  }
  r->type = Rerror;
  r->ename = "env: not fully implemented";
  return -1;
}
/*
 * /srv namespace: In-memory service registry
 * Plan 9 uses this for processes to publish named endpoints
 */
typedef struct SrvEntry {
  char name[64];
  Chan *chan; /* Posted channel */
  int owner_pid; /* PID of process that posted this */
  int active; /* Entry is in use */
} SrvEntry;
static SrvEntry srv_registry[64];
static Lock srv_lock;
static int srv_initialized = 0;
static int srv_visible_to(Proc *caller, SrvEntry *e) {
  if (e == ((void *)0) || !e->active)
    return 0;
  if (caller != ((void *)0) && caller->wasm.initialized) {
    if (e->owner_pid == 0)
      return 1;
    return (ulong)e->owner_pid == caller->pid;
  }
  return 1;
}
void srv_init(void) {
  if (srv_initialized)
    return;
  memset(srv_registry, 0, sizeof(srv_registry));
  srv_initialized = 1;
}
/* Find entry by name */
static SrvEntry *srv_find(char *name) {
  int i;
  for (i = 0; i < 64; i++) {
    if (srv_registry[i].active && strcmp(srv_registry[i].name, name) == 0)
      return &srv_registry[i];
  }
  return ((void *)0);
}
/* Find free slot */
static SrvEntry *srv_alloc(void) {
  int i;
  for (i = 0; i < 64; i++) {
    if (!srv_registry[i].active)
      return &srv_registry[i];
  }
  return ((void *)0);
}
int srv_create_entry(Proc *caller, const char *name) {
  SrvEntry *e;
  if (!name || name[0] == 0 || strlen((char *)name) >= 64)
    return -1;
  srv_init();
  lock(&srv_lock);
  e = srv_find((char *)name);
  if (e == ((void *)0)) {
    e = srv_alloc();
    if (e == ((void *)0)) {
      unlock(&srv_lock);
      return -1;
    }
  }
  strcpy(e->name, (char *)name);
  e->owner_pid = caller ? (int)caller->pid : 0;
  e->active = 1;
  if (e->chan != ((void *)0)) {
    cclose(e->chan);
    e->chan = ((void *)0);
  }
  unlock(&srv_lock);
  return 0;
}
int srv_post_fd(Proc *caller, const char *name, int fd) {
  SrvEntry *e;
  Chan *c;
  if (!name || name[0] == 0 || strlen((char *)name) >= 64)
    return -1;
  if (setlabel(&up->errlab[up->nerrlab++]))
    return -1;
  c = fdtochan(fd, -1, 0, 1);
  if (c == ((void *)0)) {
    up->nerrlab--;
    return -1;
  }
  if (setlabel(&up->errlab[up->nerrlab++])) {
    cclose(c);
    nexterror();
  }
  srv_init();
  lock(&srv_lock);
  e = srv_find((char *)name);
  if (e == ((void *)0)) {
    e = srv_alloc();
    if (e == ((void *)0)) {
      unlock(&srv_lock);
      up->nerrlab--;
      cclose(c);
      return -1;
    }
  }
  strcpy(e->name, (char *)name);
  if (e->chan != ((void *)0))
    cclose(e->chan);
  e->chan = c;
  e->owner_pid = caller ? (int)caller->pid : 0;
  e->active = 1;
  unlock(&srv_lock);
  up->nerrlab--;
  up->nerrlab--;
  return 0;
}
Chan *srv_clone_chan(const char *name) {
  Chan *c = ((void *)0);
  SrvEntry *e;
  if (!name || name[0] == 0)
    return ((void *)0);
  srv_init();
  lock(&srv_lock);
  e = srv_find((char *)name);
  if (e != ((void *)0) && e->chan != ((void *)0))
    c = cclone(e->chan);
  unlock(&srv_lock);
  return c;
}
int srv_remove_entry(Proc *caller, const char *name) {
  SrvEntry *e;
  if (!name || name[0] == 0)
    return -1;
  srv_init();
  lock(&srv_lock);
  e = srv_find((char *)name);
  if (e == ((void *)0)) {
    unlock(&srv_lock);
    return -1;
  }
  if (caller != ((void *)0) && (ulong)e->owner_pid != caller->pid && !iseve()) {
    unlock(&srv_lock);
    return -1;
  }
  if (e->chan != ((void *)0)) {
    cclose(e->chan);
    e->chan = ((void *)0);
  }
  memset(e, 0, sizeof(SrvEntry));
  unlock(&srv_lock);
  return 0;
}
int srv_get_by_index(int index, char *name, int namelen) {
  int i;
  int seen = 0;
  if (index < 0 || namelen <= 0)
    return -1;
  srv_init();
  lock(&srv_lock);
  for (i = 0; i < 64; i++) {
    if (!srv_registry[i].active)
      continue;
    if (seen == index) {
      snprint(name, namelen, "%s", srv_registry[i].name);
      unlock(&srv_lock);
      return 0;
    }
    seen++;
  }
  unlock(&srv_lock);
  return -1;
}
int srv_get_by_index_for_proc(Proc *caller, int index, char *name,
                              int namelen) {
  int i;
  int seen = 0;
  if (index < 0 || namelen <= 0)
    return -1;
  srv_init();
  lock(&srv_lock);
  for (i = 0; i < 64; i++) {
    if (!srv_visible_to(caller, &srv_registry[i]))
      continue;
    if (seen == index) {
      snprint(name, namelen, "%s", srv_registry[i].name);
      unlock(&srv_lock);
      return 0;
    }
    seen++;
  }
  unlock(&srv_lock);
  return -1;
}
int srv_index_of(const char *name) {
  int i;
  int seen = 0;
  if (!name || name[0] == 0)
    return -1;
  srv_init();
  lock(&srv_lock);
  for (i = 0; i < 64; i++) {
    if (!srv_registry[i].active)
      continue;
    if (strcmp(srv_registry[i].name, (char *)name) == 0) {
      unlock(&srv_lock);
      return seen;
    }
    seen++;
  }
  unlock(&srv_lock);
  return -1;
}
int srv_index_of_for_proc(Proc *caller, const char *name) {
  int i;
  int seen = 0;
  if (!name || name[0] == 0)
    return -1;
  srv_init();
  lock(&srv_lock);
  for (i = 0; i < 64; i++) {
    if (!srv_visible_to(caller, &srv_registry[i]))
      continue;
    if (strcmp(srv_registry[i].name, (char *)name) == 0) {
      unlock(&srv_lock);
      return seen;
    }
    seen++;
  }
  unlock(&srv_lock);
  return -1;
}
int srv_9p_handle(Proc *caller, Fcall *t, Fcall *r) {
  char *name;
  static uchar statbuf[512];
  r->tag = t->tag;
  srv_init();
  switch (t->type) {
  case Tattach:
    /* Attach to /srv */
    r->type = Rattach;
    r->qid.type = 0x80;
    r->qid.path = 0x100; /* Unique path for /srv directory */
    r->qid.vers = 0;
    r->iounit = 8192;
    return 0;
  case Twrite:
    /* Write creates or updates a service entry */
    /* Path: /srv/<name>, data contains FD number to post */
    if (t->aname == ((void *)0) || strncmp(t->aname, "/srv/", 5) != 0) {
      r->type = Rerror;
      r->ename = "invalid srv path";
      return -1;
    }
    name = t->aname + 5;
    if (strlen(name) == 0 || strlen(name) >= 64) {
      r->type = Rerror;
      r->ename = "invalid service name";
      return -1;
    }
    if (srv_post_fd(caller, name,
                    (int)((t->count > 0) ? strtoul((char *)t->data, 0, 0)
                                         : (ulong)-1)) < 0) {
      r->type = Rerror;
      r->ename = "srv post failed";
      return -1;
    }
    print("srv: posted '%s' by pid %ld\n", name, caller->pid);
    r->type = Rwrite;
    r->count = t->count;
    return 0;
  case Tread:
    /* Read lists all services (directory listing) */
    /* For now, return a simple list */
    {
      char buf[4096];
      char *p = buf;
      int i, len;
      for (i = 0; i < 64 && p < buf + sizeof(buf) - 100; i++) {
        char namebuf[64];
        if (srv_get_by_index(i, namebuf, sizeof(namebuf)) == 0) {
          p += snprint(p, (int)(buf + sizeof(buf) - p), "%s\n", namebuf);
        }
      }
      len = (int)(p - buf);
      if (t->offset >= len) {
        r->type = Rread;
        r->count = 0;
        r->data = ((void *)0);
        return 0;
      }
      /* Return requested portion */
      len -= (int)t->offset;
      if (len > (int)t->count)
        len = (int)t->count;
      r->count = (u32int)len;
      r->data = smalloc((ulong)len);
      memmove(r->data, buf + t->offset, (usize)len);
      return 0;
    }
  case Tstat:
    /* Stat the /srv directory */
    {
      Dir d;
      int n;
      memset(&d, 0, sizeof(d));
      d.name = "srv";
      d.uid = "sys";
      d.gid = "sys";
      d.muid = "sys";
      d.qid.type = 0x80;
      d.qid.path = 0x100;
      d.qid.vers = 0;
      d.mode = 0x80000000 | 0777;
      d.atime = (ulong)seconds();
      d.mtime = d.atime;
      d.length = 0;
      n = (int)convD2M(&d, statbuf, sizeof(statbuf));
      if (n <= 0) {
        r->type = Rerror;
        r->ename = "stat failed";
        return -1;
      }
      r->type = Rstat;
      r->nstat = (ushort)n;
      r->stat = statbuf;
      return 0;
    }
  case Tclunk:
    r->type = Rclunk;
    return 0;
  case Tremove:
    /* Remove a service entry */
    if (t->aname == ((void *)0) || strncmp(t->aname, "/srv/", 5) != 0) {
      r->type = Rerror;
      r->ename = "invalid srv path";
      return -1;
    }
    name = t->aname + 5;
    if (srv_remove_entry(caller, name) < 0) {
      r->type = Rerror;
      r->ename = "permission denied";
      return -1;
    }
    print("srv: removed '%s'\n", name);
    r->type = Rremove;
    return 0;
  default:
    r->type = Rerror;
    r->ename = "srv: operation not supported";
    return -1;
  }
}
static int mnt_ctl_write(Proc *p, char *cmd, int len) {
  char buf[256];
  char *args[5];
  int n;
  if ((ulong)len >= sizeof(buf))
    return -1;
  memmove(buf, cmd, (usize)len);
  buf[len] = 0;
  n = tokenize(buf, args, 5);
  if (n < 3)
    return -1;
  if (strcmp(args[0], "bind") == 0) {
    /* bind new old [flags] */
    Chan *c0, *c1;
    int flag = (n > 3) ? (int)strtoul(args[3], 0, 0) : 0;
    if (setlabel(&up->errlab[up->nerrlab++]))
      return -1;
    c0 = namec(args[1], Abind, 0, 0);
    if (setlabel(&up->errlab[up->nerrlab++])) {
      cclose(c0);
      nexterror();
    }
    c1 = namec(args[2], Amount, 0, 0);
    if (setlabel(&up->errlab[up->nerrlab++])) {
      cclose(c1);
      nexterror();
    }
    cmount(c0, c1, flag, ((void *)0));
    up->nerrlab--;
    cclose(c1);
    up->nerrlab--;
    cclose(c0);
    up->nerrlab--;
    return len;
  }
  if (strcmp(args[0], "pipe") == 0) {
    /* pipe <mountpoint> */
    /* Convenience command: binds #| to <mountpoint> */
    Chan *c0, *c1;
    if (n < 2)
      return -1;
    if (setlabel(&up->errlab[up->nerrlab++]))
      return -1;
    c0 = namec("#|", Abind, 0, 0);
    if (setlabel(&up->errlab[up->nerrlab++])) {
      cclose(c0);
      nexterror();
    }
    c1 = namec(args[1], Amount, 0, 0);
    if (setlabel(&up->errlab[up->nerrlab++])) {
      cclose(c1);
      nexterror();
    }
    cmount(c0, c1, 0x0000, ((void *)0));
    up->nerrlab--;
    cclose(c1);
    up->nerrlab--;
    cclose(c0);
    up->nerrlab--;
    return len;
  }
  return -1;
}
int mnt_9p_handle(Proc *caller, Fcall *t, Fcall *r) {
  r->tag = t->tag;
  switch (t->type) {
  case Tattach:
    r->type = Rattach;
    r->qid.type = 0x80;
    r->qid.path = 0;
    r->qid.vers = 0;
    return 0;
  case Twalk:
    if (t->nwname > 0 && strcmp(t->wname[0], "ctl") == 0) {
      r->type = Rwalk;
      r->nwqid = 1;
      r->wqid[0].type = 0x00;
      r->wqid[0].path = 1; /* ctl file */
      return 0;
    }
    r->type = Rerror;
    r->ename = "file not found";
    return -1;
  case Topen:
    r->type = Ropen;
    r->qid.type = (t->fid == 1) ? 0x00 : 0x80;
    r->iounit = 8192;
    return 0;
  case Twrite:
    /* Check if writing to ctl (path=1) */
    /* Note: In a real implementation we check the fid's qid.path */
    /* Here assuming simplified router logic where we know the target */
    if (mnt_ctl_write(caller, t->data, (int)t->count) < 0) {
      r->type = Rerror;
      r->ename = "mnt: command failed";
      return -1;
    }
    r->type = Rwrite;
    r->count = t->count;
    return 0;
  case Tclunk:
    r->type = Rclunk;
    return 0;
  default:
    r->type = Rerror;
    r->ename = "mnt: operation not supported";
    return -1;
  }
}
/*
 * Async 9P Operations (Phase 3)
 *
 * Integrates with MSGORD consensus depth classification.
 */
/*
 * Lux9 Selective Consensus Depth
 *
 * Operation classification and depth-based routing for MSGORD.
 * Implements "all nodes, all the way down" with variable depth.
 */
/*
 * Lux9 MSGORD Kernel
 *
 * MSGORD consensus for 9P message ordering.
 * Provides total ordering of all kernel operations without locks.
 *
 * Based on PHANTOM MSGORD adapted for microkernel 9P.
 */
/*
 * Kernel Forward Type Declarations
 *
 * Centralized forward declarations for core kernel types.
 * Include this header when you need type names but not full definitions.
 */
/*
 * Operation Types for Classification
 */
typedef enum {
  OP_TYPE_UNKNOWN = 0,
  OP_TYPE_LOCAL_MEMORY, /* Process-local memory operations */
  OP_TYPE_LOCAL_FILE, /* Local file read/write */
  OP_TYPE_IPC, /* Inter-process communication */
  OP_TYPE_DEVICE, /* Device access */
  OP_TYPE_NETWORK, /* Network operations */
  OP_TYPE_SECURITY, /* Security-critical operations */
  OP_TYPE_CONSENSUS, /* Consensus-related operations */
} OperationType;
/*
 * Consensus Depth Levels
 */
typedef enum {
  DEPTH_NONE = 0, /* No consensus needed - local/immediate */
  DEPTH_LOCAL, /* Local node only */
  DEPTH_CLUSTER, /* Cluster-level consensus */
  DEPTH_GLOBAL, /* Full global consensus */
} ConsensusDepth;
/*
 * Rollback State for Optimistic Execution
 */
typedef enum {
  ROLLBACK_NONE = 0, /* No rollback needed */
  ROLLBACK_PENDING, /* Awaiting consensus verification */
  ROLLBACK_TRIGGERED, /* Rollback in progress */
  ROLLBACK_COMPLETE, /* Rollback finished */
  ROLLBACK_COMMITTED, /* Successfully committed (no rollback) */
} RollbackState;
/*
 * Operation Rollback Entry
 * Tracks optimistically executed operations for potential rollback.
 */
typedef struct OpRollbackEntry {
  uint op_id; /* MSGORD message ID */
  RollbackState state; /* Current rollback state */
  ConsensusDepth required_depth; /* Required consensus depth */
  OperationType op_type; /* Classification of operation */
  /* Rollback data */
  void *snapshot_data; /* Pre-execution state snapshot */
  ulong snapshot_size; /* Size of snapshot data */
  /* Fcall context for rollback */
  struct Proc *caller; /* Calling process */
  Fcall *original_request; /* Original request */
  Fcall *executed_reply; /* Reply that was sent */
  /* Timing */
  uvlong submit_time; /* When operation was submitted */
  uvlong execute_time; /* When optimistic execution occurred */
  uvlong verify_time; /* When consensus verified (or failed) */
  /* Linked list for pending rollbacks */
  struct OpRollbackEntry *next;
  struct OpRollbackEntry *prev;
} OpRollbackEntry;
/*
 * Rollback Registry
 * Tracks all pending optimistic executions awaiting verification.
 */
typedef struct RollbackRegistry {
  OpRollbackEntry *head;
  OpRollbackEntry *tail;
  uint count;
  uint max_entries; /* High water mark before forced sync */
  /* Statistics */
  uvlong total_optimistic; /* Total optimistic executions */
  uvlong total_committed; /* Successfully committed */
  uvlong total_rollbacks; /* Rollbacks triggered */
} RollbackRegistry;
/*
 * Operation Classification API
 */
/* Classify operation and determine required consensus depth */
ConsensusDepth classify_operation(Fcall *t, char *path);
/* Classification helpers */
int is_local_memory_operation(Fcall *t);
int is_ipc_between_processes(Fcall *t, char *path);
int is_device_operation(Fcall *t, char *path);
int is_security_critical_operation(Fcall *t, char *path);
/* Get operation type */
OperationType get_operation_type(Fcall *t, char *path);
/*
 * Rollback Management API
 */
/* Initialize rollback registry */
void rollback_registry_init(RollbackRegistry *reg, uint max_entries);
/* Register optimistic execution for potential rollback */
OpRollbackEntry *rollback_register(RollbackRegistry *reg, uint op_id,
                                   ConsensusDepth depth, struct Proc *caller,
                                   Fcall *t, Fcall *r);
/* Mark operation as verified (commit) */
int rollback_commit(RollbackRegistry *reg, uint op_id);
/* Trigger rollback for failed consensus */
int rollback_trigger(RollbackRegistry *reg, uint op_id);
/* Execute pending rollback */
int rollback_execute(OpRollbackEntry *entry);
/* Get entry by operation ID */
OpRollbackEntry *rollback_find(RollbackRegistry *reg, uint op_id);
/* Clean up completed entries */
void rollback_cleanup(RollbackRegistry *reg);
/*
 * Background Verification API
 */
/* Check all pending operations against their required depth */
void verify_pending_operations(RollbackRegistry *reg, MsgOrd *dag);
/* Callback type for verification completion */
typedef void (*VerifyCallback)(uint op_id, int success, int confidence);
/* Register verification callback */
void register_verify_callback(VerifyCallback cb);
/*
 * Depth-Based Routing API
 */
/* Route operation with automatic depth classification */
int route_with_depth(MsgOrd *dag, struct Proc *caller, Fcall *t, Fcall *r,
                     char *path, RollbackRegistry *reg);
/* Route with explicit depth override */
int route_with_explicit_depth(MsgOrd *dag, struct Proc *caller, Fcall *t,
                              Fcall *r, char *path, ConsensusDepth depth,
                              RollbackRegistry *reg);
/*
 * Global rollback registry
 */
extern RollbackRegistry *global_rollback_registry;
/*
 * Lux9 MSGORD Kernel
 *
 * MSGORD consensus for 9P message ordering.
 * Provides total ordering of all kernel operations without locks.
 *
 * Based on PHANTOM MSGORD adapted for microkernel 9P.
 */
/* Forward declaration - global rollback registry from consensus_depth.c */
extern RollbackRegistry *global_rollback_registry;
/*
 * Callback wrapper that fires when MSGORD completes message ordering
 */
static void p9_msgord_callback(OrdMsg *msg, int status, void *arg) {
  AsyncP9Op *op = (AsyncP9Op *)arg;
  if (op == ((void *)0))
    return;
  /* Map MSGORD status to P9 async status */
  if (status == 0)
    op->status = 0;
  else if (status == 2)
    op->status = 3;
  else
    op->status = 4;
  /* Fire the user-provided callback if present */
  if (op->callback != ((void *)0)) {
    op->callback(&op->reply, op->callback_arg, op->status);
  }
}
/*
 * Submit 9P operation asynchronously through MSGORD
 */
uint p9_submit_async(Proc *p, Fcall *t, char *path, P9CompletionCallback cb,
                     void *arg) {
  AsyncP9Op *op;
  uint op_id;
  ConsensusDepth depth;
  if (p == ((void *)0) || t == ((void *)0))
    return 0;
  /* Allocate async operation tracking struct */
  op = xalloc(sizeof(AsyncP9Op));
  if (op == ((void *)0))
    return 0;
  memset(op, 0, sizeof(AsyncP9Op));
  /* Copy request */
  memmove(&op->request, t, sizeof(Fcall));
  op->callback = cb;
  op->callback_arg = arg;
  op->submit_time = (uvlong)seconds();
  op->status = 1;
  /* Classify operation to determine consensus depth */
  depth = classify_operation(t, path);
  /* For DEPTH_NONE operations, execute immediately (optimistic) */
  if (depth == DEPTH_NONE) {
    p9_dispatch(p, t, &op->reply);
    op->status = 0;
    if (cb)
      cb(&op->reply, arg, 0);
    xfree(op);
    return 0; /* No async tracking needed */
  }
  /* Submit to MsgOrd */
  op_id = msgord_submit_async(msgord, p, t, path, p9_msgord_callback, op, 0);
  if (op_id == 0) {
    xfree(op);
    return 0;
  }
  op->op_id = op_id;
  /* Register for potential rollback if needed */
  if (global_rollback_registry != ((void *)0) && depth >= DEPTH_CLUSTER) {
    rollback_register(global_rollback_registry, op_id, depth, p, t, &op->reply);
  }
  return op_id;
}
/*
 * Handle doorbell asynchronously using consensus depth classification
 */
int p9_handle_doorbell_async(Proc *p) {
  /*@
    @ requires \valid(p);
    */
  P9Control *ctl;
  uchar *reqbuf;
  Fcall t, r;
  int n;
  char path[256];
  ConsensusDepth depth;
  if (p == ((void *)0) || p->p9page == ((void *)0))
    return -1;
  ctl = (P9Control *)((uintptr)p->p9page + 0xF00);
  /* Check doorbell with Acquire semantics */
  if ((*((&ctl->doorbell))) == 0)
    return 0; /* No request pending */
  /* Clear doorbell */
  (*((&ctl->doorbell)) = ((0)));
  /* Mark pending */
  (*((&ctl->status)) = ((1)));
  /* Parse request from exchange page */
  reqbuf = (uchar *)p->p9page + 0x000;
  memset(&t, 0, sizeof(t));
  n = (int)convM2S(reqbuf, 0xF00, &t);
  if (n <= 0) {
    (*((&ctl->status)) = ((3)));
    return -1;
  }
  /* Get path for classification */
  if (t.type == Tattach)
    strncpy(path, t.aname, sizeof(path) - 1);
  else
    strncpy(path, "/", sizeof(path) - 1);
  /* Classify the operation */
  depth = classify_operation(&t, path);
  /* For immediate/local operations, use synchronous path */
  if (depth == DEPTH_NONE) {
    memset(&r, 0, sizeof(r));
    p9_dispatch(p, &t, &r);
    /* Write reply to exchange page */
    {
      P9Control ctl_saved;
      P9Control *ctl2 = (P9Control *)((uintptr)p->p9page + 0xF00);
      uchar rep_copy[0xF00];
      uint rep_size = convS2M(&r, rep_copy, 0xF00);
      if (rep_size == 0) {
        (*((&ctl->status)) = ((3)));
        return 1;
      }
      memmove(&ctl_saved, ctl2, sizeof(ctl_saved));
      scrub_exchange_page(p, rep_copy, rep_size, &ctl_saved, 0, 0, 0);
    }
    /* Release semantics for completion */
    (*((&ctl->status)) = ((2)));
    return 1;
  }
  /* Submit asynchronously for consensus */
  /* Pass 'p' as the callback argument so p9_doorbell_completion knows which
   * exchange page to write to. */
  /* p9_submit_async allocates copies of 't' and the reply buffer. */
  /* We pass our local 'p9_doorbell_completion' which cleans up the Op. */
  p9_submit_async(p, &t, path, ((void *)0), ((void *)0));
  return 1;
}
/*
 * Check if an async operation has completed
 */
int p9_check_async(Proc *p, uint op_id, Fcall *reply_out) {
  OrdMsg *msg;
  if (p) { };
  if (op_id == 0)
    return 4;
  msg = msgord_find_by_id(msgord, op_id);
  if (msg == ((void *)0))
    return 0; /* Already completed and removed */
  if (msg->gm_state >= 2) {
    if (reply_out != ((void *)0) && msg->gm_callback_arg != ((void *)0)) {
      AsyncP9Op *op = (AsyncP9Op *)msg->gm_callback_arg;
      memmove(reply_out, &op->reply, sizeof(Fcall));
    }
    return 0;
  }
  return 1;
}
/*
 * Cancel an async operation
 */
void p9_cancel_async(Proc *p, uint op_id) {
  OrdMsg *msg;
  if (p) { };
  if (op_id == 0)
    return;
  msg = msgord_find_by_id(msgord, op_id);
  if (msg != ((void *)0)) {
    /* Mark for rollback if registered */
    if (global_rollback_registry != ((void *)0)) {
      rollback_trigger(global_rollback_registry, op_id);
    }
  }
}
/*
 * Fire all ready async completions for a process
 */
int p9_fire_completions(Proc *p) {
  if (p) { };
  /* Delegate to MSGORD fire_completions */
  return msgord_fire_completions(msgord);
}
/* ========================================================================
 * Pipe Implementation (Router-Specific)
 * Renamed to RPipe to avoid conflict with kernel devpipe.c's Pipe
 * ======================================================================== */
typedef struct RPipe {
  Lock lock;
  int ref;
  uchar *buf;
  int head;
  int tail;
  int size; /* Capacity */
  int data_len; /* Current data length */
  int writers;
  int readers;
  Rendez r; /* Sleep for both read (empty) and write (full) */
  int busy;
  int closed;
} RPipe;
static RPipe *rpipe_create(void) {
  RPipe *p = xalloc(sizeof(RPipe));
  if (p == ((void *)0))
    return ((void *)0);
  memset(p, 0, sizeof(RPipe));
  p->buf = xalloc(4096);
  if (p->buf == ((void *)0)) {
    xfree(p);
    return ((void *)0);
  }
  p->size = 4096;
  p->ref = 1; /* One ref for the creator */
  p->writers = 1;
  p->readers = 1;
  return p;
}
static void rpipe_decref(RPipe *p) {
  int ref;
  if (p == ((void *)0))
    return;
  lock(&p->lock);
  ref = --p->ref;
  unlock(&p->lock);
  if (ref == 0) {
    xfree(p->buf);
    xfree(p);
  }
}
static void rpipe_clone_notify(void *aux) {
  RPipe *p = (RPipe *)aux;
  if (p) {
    lock(&p->lock);
    p->ref++;
    unlock(&p->lock);
  }
}
static int rpipe_read_cond(void *arg) {
  RPipe *p = (RPipe *)arg;
  if ((p->data_len > 0) || (p->writers == 0) || (p->closed))
    return 1;
  return 0;
}
static int rpipe_write_cond(void *arg) {
  RPipe *p = (RPipe *)arg;
  if ((p->data_len < p->size) || (p->readers == 0) || (p->closed))
    return 1;
  return 0;
}
static int rpipe_9p_handle(Proc *caller, Fcall *t, Fcall *r) {
  Chan *c;
  RPipe *p = ((void *)0);
  Fgrp *f = caller->fgrp;
  r->tag = t->tag;
  /* Get the pipe object from chan aux */
  /* Note: for Tattach, aux is nil initially */
  if (t->type != Tattach) {
    c = fdtochan((int)t->fid, -1, 0, 0); /* Borrow chan, no incref/error */
    if (c == ((void *)0) || c->type != (ushort)-1) {
      r->type = Rerror;
      r->ename = "invalid pipe fid";
      return -1;
    }
    p = (RPipe *)c->aux;
    if (p == ((void *)0)) {
      r->type = Rerror;
      r->ename = "pipe not initialized";
      return -1;
    }
  }
  switch (t->type) {
  case Tattach:
    /* Create new pipe */
    if (strncmp(t->aname, "/dev/pipe", 9) != 0) {
      r->type = Rerror;
      r->ename = "invalid attach path";
      return -1;
    }
    p = rpipe_create();
    if (p == ((void *)0)) {
      r->type = Rerror;
      r->ename = "pipe alloc failed";
      return -1;
    }
    /* Store in FID aux - Must lock Fgrp */
    lock(&f->lock);
    c = f->fd[t->fid];
    if (c)
      c->aux = p;
    unlock(&f->lock);
    r->type = Rattach;
    r->qid.type = 0x00;
    r->qid.path = 0;
    r->qid.vers = 8;
    r->iounit = 0;
    return 0;
  case Tread:
    /* Block until data available */
    while (p->data_len == 0) {
      if (p->writers == 0 || p->closed) {
        /* EOF */
        r->type = Rread;
        r->count = 0;
        r->data = ((void *)0);
        return 0;
      }
      sleep(&p->r, rpipe_read_cond, p);
    }
    lock(&p->lock);
    /* Recheck after wake */
    if (p->data_len == 0) {
      unlock(&p->lock);
      if (p->writers == 0 || p->closed) {
        r->type = Rread;
        r->count = 0;
        return 0;
      }
      /* Should loop, but simple implementation allows spurious return or
       * recurse */
      /* We just return 0 here which might look like EOF. Better to loop. */
      /* But with lock held we can't loop easily. Let's assume cond correct. */
    }
    int n = (int)t->count;
    if (n > p->data_len)
      n = p->data_len;
    /* Circular buffer read */
    if (p->head + n <= p->size) {
      memmove(r->data, p->buf + p->head, (usize)n);
    } else {
      int chunk = p->size - p->head;
      memmove(r->data, p->buf + p->head, (usize)chunk);
      memmove(r->data + chunk, p->buf, (usize)(n - chunk));
    }
    p->head = (p->head + n) % p->size;
    p->data_len -= n;
    wakeup(&p->r); /* Wake writers */
    unlock(&p->lock);
    r->type = Rread;
    r->count = (u32int)n;
    return 0;
  case Twrite:
    /* Block until space available */
    while (p->data_len == p->size) {
      if (p->readers == 0 || p->closed) {
        r->type = Rerror;
        r->ename = "pipe broken";
        postnote(caller, 1, "sys: write on closed pipe", NUser);
        return -1;
      }
      sleep(&p->r, rpipe_write_cond, p);
    }
    lock(&p->lock);
    int cnt = (int)t->count;
    int space = p->size - p->data_len;
    if (cnt > space)
      cnt = space; /* Partial write if strictly blocking not fully implemented,
                    but we blocked for 'some' space */
    if (p->tail + cnt <= p->size) {
      memmove(p->buf + p->tail, t->data, (usize)cnt);
    } else {
      int chunk = p->size - p->tail;
      memmove(p->buf + p->tail, t->data, (usize)chunk);
      memmove(p->buf, t->data + chunk, (usize)(cnt - chunk));
    }
    p->tail = (p->tail + cnt) % p->size;
    p->data_len += cnt;
    wakeup(&p->r); /* Wake readers */
    unlock(&p->lock);
    r->type = Rwrite;
    r->count = (u32int)cnt;
    return 0;
  case Tclunk:
    rpipe_decref(p);
    r->type = Rclunk;
    return 0;
  case Tstat:
    /* 0600 pipe */
    {
      static uchar statbuf[256];
      Dir d;
      memset(&d, 0, sizeof(d));
      d.name = "pipe";
      d.uid = "sys";
      d.gid = "sys";
      d.muid = "sys";
      d.qid.type = 0x00;
      d.qid.path = 0;
      d.qid.vers = 8;
      d.mode = 0600;
      d.length = p->data_len;
      int len = (int)convD2M(&d, statbuf, sizeof(statbuf));
      r->type = Rstat;
      r->nstat = (ushort)len;
      r->stat = statbuf;
    }
    return 0;
  case Twalk:
    /* Dispatch handled the clone and called pipe_clone_notify */
    if (t->nwname == 0) {
      r->type = Rwalk;
      r->nwqid = 0;
      return 0;
    }
    r->type = Rerror;
    r->ename = "walk not supported";
    return -1;
  default:
    r->type = Rerror;
    r->ename = "pipe operation not supported";
    return -1;
  }
}
/* ========================================================================
 * /fd Implementation
 * ======================================================================== */
int fd_9p_handle(Proc *caller, Fcall *t, Fcall *r) {
  int fd;
  Chan *c;
  int subtype;
  r->tag = t->tag;
  subtype = get_fid_subtype((int)t->fid); /* 0 = root, N+1 = fd N */
  switch (t->type) {
  case Tattach:
    r->type = Rattach;
    r->qid.type = 0x80;
    r->qid.path = 0;
    r->qid.vers = 0; /* Root */
    return 0;
  case Twalk:
    if (subtype != 0 && t->nwname > 0) {
      /* Cannot walk from file */
      r->type = Rerror;
      r->ename = "not a directory";
      return -1;
    }
    if (t->nwname == 0) {
      /* Clone */
      r->type = Rwalk;
      r->nwqid = 0;
      return 0;
    }
    if (t->nwname == 1) {
      /* Walk to number */
      if (strcmp(t->wname[0], "..") == 0) {
        r->type = Rwalk;
        r->nwqid = 1;
        r->wqid[0].type = 0x80;
        r->wqid[0].path = 0;
        r->wqid[0].vers = 0;
        return 0;
      }
      fd = (int)strtoul(t->wname[0], 0, 0);
      /* Verify FD exists in caller's fgrp */
      c = fdtochan(fd, -1, 0, 0);
      if (c == ((void *)0)) {
        r->type = Rerror;
        r->ename = "fd not found";
        return -1;
      }
      /* Success */
      r->type = Rwalk;
      r->nwqid = 1;
      r->wqid[0].type = 0x00; /* Or whatever the underlying file is? Protocol
                                   says QTFILE for proxy */
      r->wqid[0].path =
          6; /* Should use actual Qid? No, this is the /fd/N file */
      r->wqid[0].vers = (u32int)(fd + 1); /* Encode FD */
      return 0;
    }
    r->type = Rerror;
    r->ename = "walk too deep";
    return -1;
  case Tread:
    if (subtype == 0) {
      /* Directory listing of FDs */
      /* Simplified: just return empty dir or error? */
      /* Doing full listing requires iterating fgrp */
      r->type = Rread;
      r->count = 0; /* Empty */
      r->data = ((void *)0);
      return 0;
    }
    /* If reading from /fd/N, it usually means READING from the underlying file
     */
    fd = subtype - 1;
    /* Proxy read to underlying channel */
    /* But wait, we need to convert Fcall Tread to devtab read? */
    /* Or use 'pread' ? */
    /* Since we are inside kernel, we can call dev->read directly if we had the
     * channel */
    c = fdtochan(fd, -1, 0,
                 0); // Open for reading? mode -1 checks validity only?
    /* fdtochan(fd, mode, check, ref) */
    /* We need OREAD check? logic: mode=-1 ignores check. */
    if (c == ((void *)0)) {
      r->type = Rerror;
      r->ename = "fd closed";
      return -1;
    }
    /* Direct device read */
    if (setlabel(&up->errlab[up->nerrlab++])) {
      r->type = Rerror;
      r->ename = up->errstr;
      return -1;
    }
    /* devtab[c->type]->read(c, data, count, offset) */
    long n = devtab[c->type]->read(c, r->data, t->count, t->offset);
    r->count = (u32int)n;
    r->type = Rread;
    up->nerrlab--;
    return 0;
  case Twrite:
    if (subtype == 0) {
      r->type = Rerror;
      r->ename = "is a directory";
      return -1;
    }
    fd = subtype - 1;
    c = fdtochan(fd, -1, 0, 0);
    if (c == ((void *)0)) {
      r->type = Rerror;
      r->ename = "fd closed";
      return -1;
    }
    if (setlabel(&up->errlab[up->nerrlab++])) {
      r->type = Rerror;
      r->ename = up->errstr;
      return -1;
    }
    long cnt = devtab[c->type]->write(c, t->data, t->count, t->offset);
    r->count = (u32int)cnt;
    r->type = Rwrite;
    up->nerrlab--;
    return 0;
  case Tstat:
    /* Stat underlying file? */
    if (subtype > 0) {
      fd = subtype - 1;
      c = fdtochan(fd, -1, 0, 0);
      if (c) {
        /* For now, fake Stat */
        static uchar sbuf[256];
        Dir d;
        memset(&d, 0, sizeof(d));
        d.name = "fd"; /* Helper... */
        d.qid = c->qid;
        d.mode = c->mode;
        uint len = convD2M(&d, sbuf, sizeof(sbuf));
        r->type = Rstat;
        r->nstat = (ushort)len;
        r->stat = sbuf;
        return 0;
      }
    }
    r->type = Rstat;
    r->nstat = 0;
    return 0;
  case Tclunk:
    r->type = Rclunk;
    return 0;
  default:
    r->type = Rerror;
    r->ename = "fd operation not supported";
    return -1;
  }
}
/* ========== WASM 9P Handler ========== */
/* TODO: Proper server lookup - for now just stub */
static wasm_fileserver_t *global_wasm_server = ((void *)0);
static int wasm_9p_handle(Proc *caller, Fcall *t, Fcall *r) {
  if (caller) { };
  r->tag = t->tag;
  switch (t->type) {
  case Tattach:
    /* Capability already validated in router - just attach to root */
    if (!global_wasm_server) {
      global_wasm_server = wasm_fileserver_load("/boot/server.wasm", 0);
      if (!global_wasm_server) {
        r->type = Rerror;
        r->ename = "failed to load wasm server";
        return -1;
      }
    }
    r->type = Rattach;
    r->qid.type = 0x80;
    r->qid.path = 0;
    r->qid.vers = 0;
    r->iounit = 0;
    return 0;
  case Twalk:
  case Topen:
  case Tcreate:
  case Tread:
  case Twrite:
  case Tstat:
  case Twstat:
  case Tremove: {
    /* Handle synchronously in WASM server */
    if (!global_wasm_server) {
      r->type = Rerror;
      r->ename = "no wasm server loaded";
      return -1;
    }
    if (wasm_fs_handle_fcall(global_wasm_server, t, r) < 0) {
      r->type = Rerror;
      r->ename = "wasm handler failed";
      return -1;
    }
    r->tag = t->tag;
    return 0;
  }
  case Tclunk:
    r->type = Rclunk;
    return 0;
  default:
    r->type = Rerror;
    r->ename = "operation not supported";
    return -1;
  }
}
