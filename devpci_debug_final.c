/* Frama-C Missing Types - types excluded by #ifndef __FRAMAC__ in Plan 9 headers */

/* Types excluded by #ifndef __FRAMAC__ in portlib.h */
typedef struct Fmt Fmt;
typedef int (*Fmts)(Fmt *);
struct Fmt {
    unsigned char runes;
    void *start;
    void *to;
    void *stop;
    int (*flush)(Fmt *);
    void *farg;
    int nfmt;
    __builtin_va_list args;
    int r;
    int width;
    int prec;
    unsigned long flags;
};

typedef struct Qid Qid;
struct Qid {
    unsigned long long path;
    unsigned long vers;
    unsigned char type;
};

typedef struct Dir Dir;
struct Dir {
    unsigned short type;
    unsigned int dev;
    Qid qid;
    unsigned long mode;
    unsigned long atime;
    unsigned long mtime;
    long long length;
    char *name;
    char *uid;
    char *gid;
    char *muid;
};

typedef struct Waitmsg Waitmsg;
struct Waitmsg {
    int pid;
    unsigned long time[3];
    char msg[128]; /* ERRMAX */
};
#include "kernel/include/framac_stubs.h"
typedef __builtin_va_list va_list;
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
 * /dev/pci - PCI device enumeration for SIP drivers
 *
 * Provides 9P-based access to PCI configuration space for userspace
 * device drivers to enumerate and configure PCI devices.
 *
 * Filesystem:
 *   /dev/pci/
 *   ├── ctl           - Control and status
 *   ├── bus           - List of all PCI devices
 *   ├── 0000:00:00.0/ - Device directory (bus:dev:func)
 *   │   ├── config    - Raw config space (256 bytes)
 *   │   ├── ctl       - Device control
 *   │   └── raw       - Alias for config
 *   ├── 0000:00:1f.2/ - Another device (e.g., AHCI)
 *   └── ...
 *
 * Usage:
 *   // List all devices
 *   cat /dev/pci/bus
 *
 *   // Read AHCI config
 *   cat /dev/pci/0000:00:1f.2/config
 *
 * Architecture Integration:
 *   - Uses existing pcimatch() for enumeration
 *   - Uses pcicfgr*() for config space access
 *   - Exports BARs, IRQ, vendor/device IDs
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
/*@
  @ predicate valid_string(char *s) =
  @   \exists integer n; 0 <= n < ACSL_MAXSTR &&
  @     \valid_read(s + (0..n)) &&
  @     s[n] == '\0' &&
  @     (\forall integer i; 0 <= i < n ==> s[i] != '\0');
  */
/*@
  @ predicate valid_string_or_null(char *s) =
  @   s == \null || valid_string(s);
  */
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
/* Define va_list.  */
/* Define the standard macros for the user,
   if this invocation was from the user program.  */
/* Define va_list, if desired, from va_list. */
/* We deliberately do not define va_list when called from
   stdio.h, because ANSI C says that stdio.h is not supposed to define
   va_list.  stdio.h needs to have access to that data type, 
   but must not use that name.  It should use the name va_list,
   which is safe because it is reserved for the implementation.  */
/* The macro _VA_LIST_ is the same thing used by this file in Ultrix.
   But on BSD NET2 we must not test or define or undef it.
   (Note that the comments in NET 2's ansi.h
   are incorrect for _VA_LIST_--see stdio.h!)  */
/* The macro _VA_LIST_DEFINED is used in Windows NT 3.5  */
/* The macro _VA_LIST is used in SCO Unix 3.2.  */
/* The macro _VA_LIST_T_H is used in the Bull dpx2  */
/* The macro __va_list__ is used by BeOS.  */
typedef unsigned int Rune;
/*
 * mem routines
 */
extern void *memccpy(void *, const void *, int, usize);
/*@ requires \valid((char*)s + (0..n-1));
  @ terminates \true;
  @ assigns ((char*)s)[0..n-1];
  @ ensures \result == s;
  */
extern void *memset(void *s, int c, usize n);
extern int memcmp(const void *, const void *, usize);
/*@ terminates \true;
  @ assigns \result \from dst;
  @ ensures \result == dst;
  */
extern void *memmove(void *dst, const void *src, usize n);
extern void *memchr(const void *, int, usize);
/*
 * string routines
 */
extern char *strcat(char *, const char *);
extern char *strchr(const char *, int);
extern char *strrchr(const char *, int);
/*@ requires s1 == \null || valid_string(s1);
  @ requires s2 == \null || valid_string(s2);
  @ terminates \true;
  @ assigns \nothing;
  */
extern int strcmp(const char *s1, const char *s2);
extern char *strcpy(char *, const char *);
extern char *strecpy(char *, char *, const char *);
extern char *strncat(char *, const char *, long);
extern char *strncpy(char *, const char *, long);
extern int strncmp(const char *, const char *, long);
/*@ requires s == \null || valid_string(s);
  @ terminates \true;
  @ assigns \nothing;
  @ ensures \result >= 0;
  */
extern long strlen(const char *s);
extern char *strstr(const char *, const char *);
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
typedef int (*Fmts)(Fmt *);
/*@ assigns \result \from fmt; */
extern int print(char *fmt, ...);
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
extern double strtod(char *, char **);
/*@
  @ assigns \result, *endptr;
  @ ensures \valid(endptr) ==> \valid(*endptr);
  */
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
/* For Frama-C: don't alias bprint to print to avoid declaration conflicts */
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
/*@
  @ predicate pebble_state_valid(PebbleState *ps) =
  @   \valid(ps) &&
  @   ps->colorless_bank + ps->black_inuse + ps->blue_inuse + ps->red_inuse <=
  pebble_total_system_tokens &&
  @   ps->white_pending <= ps->colorless_bank;
  */
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
  uintptr p9uaddr; /* user VA for exchange page (per-process slot) */
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
    u32int permissions; /* Active WASM capability permissions bitmask */
    void *cap_table; /* Capability handle table (wasm_cap_table_t) */
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
  uintptr *pml4; /* pml4 base for this processor (va) */
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
  @ requires valid_string(e);
  @ terminates \true;
  @ assigns \nothing;
  @ ensures \false;
  */
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
/*@ requires name == \null || valid_string(name);
  @ assigns \result \from name[0..];
  @ ensures \result == \null || valid_string(\result);
  */
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
  */
void ilock(Lock *l);
_Noreturn void interrupted(void);
/*@ requires l != \null;
  @ terminates \true;
  @ assigns *l;
  */
void iunlock(Lock *l);
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
/*@ requires l != \null;
  @ terminates \true;
  @ assigns *l;
  */
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
void *malloc(ulong size);
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
/*@ requires fmt != \null;
  @ terminates \true;
  @ assigns \nothing;
  @ ensures \false;
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
/*@ requires s == \null || valid_string(s); */
int postnote(Proc *, int, char *s, int);
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
/*@ requires l != \null;
  @ terminates \true;
  @ assigns *l;
  */
void unlock(Lock *l);
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
/*@ terminates \true;
    allocates \result;
    assigns \result \from size;
    ensures \result == \null || \valid((char*)\result + (0..size-1));
*/
void *xalloc(ulong size);
/*@ terminates \true;
    allocates \result;
    assigns \result \from size;
    ensures \result == \null || \valid((char*)\result + (0..size-1));
*/
void *xalloc_raw(ulong size);
/*@ terminates \true;
    allocates \result;
    assigns \result \from size;
    ensures \result == \null || \valid((char*)\result + (0..size-1));
*/
void *xallocz(ulong size, int zero);
/*@ terminates \true;
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
    allocates \result;
    assigns \result \from size, zero;
    ensures \result == \null || \valid((char*)\result + (0..size-1));
*/
void *xallocz_driver(ulong size, int zero);
/*@ terminates \true;
    allocates \result;
    assigns \result \from size;
    ensures \result == \null || \valid((char*)\result + (0..size-1));
*/
void *smalloc_driver(ulong size);
void xfree_driver(void *p);
/*@ terminates \true;
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
ushort ins(int port); /* Input from I/O port */
void outs(int port, ushort value); /* Output to I/O port */
/* Memory and page ownership functions */
uintptr cankaddr(uintptr); /* Check if address in kernel address space - matches
                              arch signature */
int pageown_acquire(Proc *, uintptr, u64int); /* Acquire page ownership */
int pageown_release(Proc *, uintptr); /* Release page ownership */
void pageown_cleanup_process(Proc *); /* Clean up page ownership for process */
/* Architecture-specific process functions - declarations handled in
 * arch-specific fns.h */
void procsave(Proc *); /* Save process state */
void procrestore(Proc *); /* Restore process state */
void procsetup(Proc *); /* Setup process state */
void procfork(Proc *); /* Fork process state */
int proc_setup_p9page(Proc *); /* Setup 9P exchange page (deprecated) */
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
struct Chan; /* Forward declaration */
struct M3Function; /* Forward declaration for WASM3 */
int wasm_exec_compile(struct Chan *, struct M3Function **);
void wasm_exec_run(struct M3Function *);
void wasm_runtime_cleanup_process(Proc *);
extern int boot_verbose;
/* Bounded print for formal verification */
/*@ requires \valid_read(fmt);
  @ terminates \true;
  @ assigns \nothing;
  */
int bprint(const char *fmt, ...);
/*@ requires \valid_read(fmt);
  @ terminates \false;
  */
void bpanic(const char *fmt, ...) ;
/* Plan 9 universal header */
extern char Enoerror[128]; /* no error */
extern char Emount[128]; /* inconsistent mount */
extern char Eunmount[128]; /* not mounted */
extern char Eismtpt[128]; /* is a mount point */
extern char Eunion[128]; /* not in union */
extern char Emountrpc[128]; /* mount rpc error */
extern char Eshutdown[128]; /* device shut down */
extern char Enocreate[128]; /* mounted directory forbids creation */
extern char Enonexist[128]; /* file does not exist */
extern char Eexist[128]; /* file already exists */
extern char Ebadsharp[128]; /* unknown device in # filename */
extern char Enotdir[128]; /* not a directory */
extern char Eisdir[128]; /* file is a directory */
extern char Ebadchar[128]; /* bad character in file name */
extern char Efilename[128]; /* file name syntax */
extern char Eperm[128]; /* permission denied */
extern char Ebadusefd[128]; /* inappropriate use of fd */
extern char Ebadarg[128]; /* bad arg in system call */
extern char Einuse[128]; /* device or object already in use */
extern char Eio[128]; /* i/o error */
extern char Etoobig[128]; /* read or write too large */
extern char Etoosmall[128]; /* read or write too small */
extern char Enoport[128]; /* network port not available */
extern char Ehungup[128]; /* i/o on hungup channel */
extern char Ebadctl[128]; /* bad process or channel control request */
extern char Enodev[128]; /* no free devices */
extern char Eprocdied[128]; /* process exited */
extern char Enochild[128]; /* no living children */
extern char Eioload[128]; /* i/o error in demand load */
extern char Enovmem[128]; /* virtual memory allocation failed */
extern char Ebadfd[128]; /* fd out of range or not open */
extern char Enofd[128]; /* no free file descriptors */
extern char Eisstream[128]; /* seek on a stream */
extern char Ebadexec[128]; /* exec header invalid */
extern char Etimedout[128]; /* connection timed out */
extern char Econrefused[128]; /* connection refused */
extern char Econinuse[128]; /* connection in use */
extern char Eintr[128]; /* interrupted */
extern char Enomem[128]; /* kernel allocate failed */
extern char Esoverlap[128]; /* segments overlap */
extern char Emouseset[128]; /* mouse type already set */
extern char Eshort[128]; /* i/o count too small */
extern char Egreg[128]; /* the front fell off */
extern char Ebadspec[128]; /* bad attach specifier */
extern char Enoreg[128]; /* process has no saved registers */
extern char Enoattach[128]; /* mount/attach disallowed */
extern char Eshortstat[128]; /* stat buffer too small */
extern char Ebadstat[128]; /* malformed stat buffer */
extern char Enegoff[128]; /* negative i/o offset */
extern char Ecmdargs[128]; /* wrong #args in control message */
extern char Ebadip[128]; /* bad ip address syntax */
extern char Edirseek[128]; /* seek in directory */
extern char Etoolong[128]; /* name too long */
extern char Echange[128]; /* media or partition has changed */
/* Plan 9 universal header */
enum { /* type 0 & type 1 pre-defined header */
       PciVID = 0x00, /* vendor ID */
       PciDID = 0x02, /* device ID */
       PciPCR = 0x04, /* command */
       PciPSR = 0x06, /* status */
       PciRID = 0x08, /* revision ID */
       PciCCRp = 0x09, /* programming interface class code */
       PciCCRu = 0x0A, /* sub-class code */
       PciCCRb = 0x0B, /* base class code */
       PciCLS = 0x0C, /* cache line size */
       PciLTR = 0x0D, /* latency timer */
       PciHDT = 0x0E, /* header type */
       PciBST = 0x0F, /* BIST */
       PciBAR0 = 0x10, /* base address */
       PciBAR1 = 0x14,
       PciCAP = 0x34, /* capabilities pointer */
       PciINTL = 0x3C, /* interrupt line */
       PciINTP = 0x3D, /* interrupt pin */
};
/* ccrb (base class code) values; controller types */
enum {
  Pcibcpci1 = 0, /* pci 1.0; no class codes defined */
  Pcibcstore = 1, /* mass storage */
  Pcibcnet = 2, /* network */
  Pcibcdisp = 3, /* display */
  Pcibcmmedia = 4, /* multimedia */
  Pcibcmem = 5, /* memory */
  Pcibcbridge = 6, /* bridge */
  Pcibccomm = 7, /* simple comms (e.g., serial) */
  Pcibcbasesys = 8, /* base system */
  Pcibcinput = 9, /* input */
  Pcibcdock = 0xa, /* docking stations */
  Pcibcproc = 0xb, /* processors */
  Pcibcserial = 0xc, /* serial bus (e.g., USB) */
  Pcibcwireless = 0xd, /* wireless */
  Pcibcintell = 0xe, /* intelligent i/o */
  Pcibcsatcom = 0xf, /* satellite comms */
  Pcibccrypto = 0x10, /* encryption/decryption */
  Pcibcdacq = 0x11, /* data acquisition & signal proc. */
};
/* ccru (sub-class code) values; common cases only */
enum {
  /* mass storage */
  Pciscscsi = 0, /* SCSI */
  Pciscide = 1, /* IDE (ATA) */
  /* network */
  Pciscether = 0, /* Ethernet */
  /* display */
  Pciscvga = 0, /* VGA */
  Pciscxga = 1, /* XGA */
  Pcisc3d = 2, /* 3D */
  /* bridges */
  Pcischostpci = 0, /* host/pci */
  Pciscpcicpci = 1, /* pci/pci */
  /* simple comms */
  Pciscserial = 0, /* 16450, etc. */
  Pciscmultiser = 1, /* multiport serial */
  /* serial bus */
  Pciscusb = 3, /* USB */
};
enum { /* type 0 pre-defined header */
       PciCIS = 0x28, /* cardbus CIS pointer */
       PciSVID = 0x2C, /* subsystem vendor ID */
       PciSID = 0x2E, /* subsystem ID */
       PciEBAR0 = 0x30, /* expansion ROM base address */
       PciMGNT = 0x3E, /* burst period length */
       PciMLT = 0x3F, /* maximum latency between bursts */
};
enum { /* type 1 pre-defined header */
       PciPBN = 0x18, /* primary bus number */
       PciSBN = 0x19, /* secondary bus number */
       PciUBN = 0x1A, /* subordinate bus number */
       PciSLTR = 0x1B, /* secondary latency timer */
       PciIBR = 0x1C, /* I/O base */
       PciILR = 0x1D, /* I/O limit */
       PciSPSR = 0x1E, /* secondary status */
       PciMBR = 0x20, /* memory base */
       PciMLR = 0x22, /* memory limit */
       PciPMBR = 0x24, /* prefetchable memory base */
       PciPMLR = 0x26, /* prefetchable memory limit */
       PciPUBR = 0x28, /* prefetchable base upper 32 bits */
       PciPULR = 0x2C, /* prefetchable limit upper 32 bits */
       PciIUBR = 0x30, /* I/O base upper 16 bits */
       PciIULR = 0x32, /* I/O limit upper 16 bits */
       PciEBAR1 = 0x38, /* expansion ROM base address */
       PciBCR = 0x3E, /* bridge control register */
};
enum { /* type 2 pre-defined header */
       PciCBExCA = 0x10,
       PciCBSPSR = 0x16,
       PciCBPBN = 0x18, /* primary bus number */
       PciCBSBN = 0x19, /* secondary bus number */
       PciCBUBN = 0x1A, /* subordinate bus number */
       PciCBSLTR = 0x1B, /* secondary latency timer */
       PciCBMBR0 = 0x1C,
       PciCBMLR0 = 0x20,
       PciCBMBR1 = 0x24,
       PciCBMLR1 = 0x28,
       PciCBIBR0 = 0x2C, /* I/O base */
       PciCBILR0 = 0x30, /* I/O limit */
       PciCBIBR1 = 0x34, /* I/O base */
       PciCBILR1 = 0x38, /* I/O limit */
       PciCBSVID = 0x40, /* subsystem vendor ID */
       PciCBSID = 0x42, /* subsystem ID */
       PciCBLMBAR = 0x44, /* legacy mode base address */
};
/* capabilities */
enum {
  PciCapPMG = 0x01, /* power management */
  PciCapAGP = 0x02,
  PciCapVPD = 0x03, /* vital product data */
  PciCapSID = 0x04, /* slot id */
  PciCapMSI = 0x05,
  PciCapCHS = 0x06, /* compact pci hot swap */
  PciCapPCIX = 0x07,
  PciCapHTC = 0x08, /* hypertransport irq conf */
  PciCapVND = 0x09, /* vendor specific information */
  PciCapPCIe = 0x10,
  PciCapMSIX = 0x11,
  PciCapSATA = 0x12,
  PciCapHSW = 0x0c, /* hot swap */
};
enum {
  /* bar bits */
  Barioaddr = 1 << 0, /* vs. memory addr */
  Barwidthshift = 1,
  Barwidthmask = 3,
  Barwidth32 = 0,
  Barwidth64 = 2,
  Barprefetch = 1 << 3,
};
enum { /* command register */
       IOen = (1 << 0),
       MEMen = (1 << 1),
       MASen = (1 << 2),
       MemWrInv = (1 << 4),
       PErrEn = (1 << 6),
       SErrEn = (1 << 8),
};
typedef struct Pcidev Pcidev;
struct Pcidev {
  int tbdf; /* type+bus+device+function */
  ushort vid; /* vendor ID */
  ushort did; /* device ID */
  ushort pcr;
  uchar rid;
  uchar ccrp;
  uchar ccru;
  uchar ccrb;
  uchar cls;
  uchar ltr;
  uchar intl; /* interrupt line */
  struct {
    uvlong bar; /* base address */
    vlong size;
  } mem[6];
  struct { /* expansion rom bar */
    uvlong bar;
    vlong size;
  } rom;
  struct { /* 32-bit io and memory windows */
    uvlong bar;
    vlong size;
  } ioa, mema;
  struct { /* 64-bit prefechable memory window */
    uvlong bar;
    vlong size;
  } prefa;
  Pcidev *list;
  Pcidev *link; /* next device on this bno */
  Pcidev *parent; /* up a bus */
  Pcidev *bridge; /* down a bus */
  int pmrb; /* power management register block */
  int msi; /* MSI capability register block */
};
enum {
  /* vendor ids */
  Vintel = 0x8086,
  Vmyricom = 0x14c1,
  Vatiamd = 0x1002,
  Vjmicron = 0x197b,
  Vmarvell = 0x1b4b,
  Vnvidia = 0x10de,
  Vrealtek = 0x10ec,
};
enum {
  MaxFNO = 7,
  MaxUBN = 255,
};
extern int pcimaxdno;
extern Pcidev *pciparentdev;
extern void pcidevfree(Pcidev *pcidev);
extern int pcicfgr32(Pcidev *pcidev, int rno);
extern void pcicfgw32(Pcidev *pcidev, int rno, int data);
extern int pcicfgr16(Pcidev *pcidev, int rno);
extern void pcicfgw16(Pcidev *pcidev, int rno, int data);
extern int pcicfgr8(Pcidev *pcidev, int rno);
extern void pcicfgw8(Pcidev *pcidev, int rno, int data);
extern int pciscan(int bno, Pcidev **list, Pcidev *parent);
extern void pcibusmap(Pcidev *root, uvlong *pmema, ulong *pioa, int wrreg);
extern void pcibussize(Pcidev *root, uvlong *msize, ulong *iosize);
extern Pcidev *pcimatch(Pcidev *prev, int vid, int did);
extern Pcidev *pcimatchtbdf(int tbdf);
extern vlong pcibarsize(Pcidev *, int rno);
extern void pcisetbar(Pcidev *, int, uvlong);
extern uchar pciipin(Pcidev *pci, uchar pin);
extern void pcisetioe(Pcidev *p);
extern void pciclrioe(Pcidev *p);
extern void pcisetbme(Pcidev *p);
extern void pciclrbme(Pcidev *p);
extern void pcisetmwi(Pcidev *p);
extern void pciclrmwi(Pcidev *p);
extern int pcicap(Pcidev *p, int cap);
extern int pcihtcap(Pcidev *p, int cap);
extern int pcienumcaps(Pcidev *p, int (*fmatch)(Pcidev *, int, int, int),
                       int arg);
extern int pcimsienable(Pcidev *p, uvlong addr, ulong data);
extern int pcimsidisable(Pcidev *p);
extern int pcigetpms(Pcidev *p);
extern int pcisetpms(Pcidev *p, int state);
extern void pcienable(Pcidev *p);
extern void pcidisable(Pcidev *p);
extern void pcihinv(Pcidev *p);
extern void pcireset(void);
enum {
  BusCBUS = 0, /* Corollary CBUS */
  BusCBUSII, /* Corollary CBUS II */
  BusEISA, /* Extended ISA */
  BusFUTURE, /* IEEE Futurebus */
  BusINTERN, /* Internal bus */
  BusISA, /* Industry Standard Architecture */
  BusMBI, /* Multibus I */
  BusMBII, /* Multibus II */
  BusMCA, /* Micro Channel Architecture */
  BusMPI, /* MPI */
  BusMPSA, /* MPSA */
  BusNUBUS, /* Apple Macintosh NuBus */
  BusPCI, /* Peripheral Component Interconnect */
  BusPCMCIA, /* PC Memory Card International Association */
  BusTC, /* DEC TurboChannel */
  BusVL, /* VESA Local bus */
  BusVME, /* VMEbus */
  BusXPRESS, /* Express System Bus */
};
extern int tbdffmt(Fmt *);
enum {
  Qdir = 0,
  Qctl,
  Qbus,
  Qdevbase = 0x1000, /* Device directories start here */
  Qdevconfig = 0x10000, /* Config files */
  Qdevctl = 0x20000, /* Device ctl files */
};
/*
 * PCI device list (cached from enumeration)
 */
typedef struct PCIDev PCIDev;
struct PCIDev {
  Pcidev *pci; /* Pointer to PCI device */
  int busno; /* Bus number */
  int devno; /* Device number */
  int funcno; /* Function number */
  char name[32]; /* "0000:00:1f.2" */
  PCIDev *next;
};
static struct {
  Lock lock;
  PCIDev *devlist;
  int ndevs;
} pcistate;
/*
 * Capability checking
 */
enum {
  CapPCI = 1 << 6, /* PCI device access */
};
static void checkcap(ulong required) {
  if (up == ((void *)0))
    return; /* kernel processes have full access */
  if ((up->capabilities & required) != required)
    lux9_error("insufficient capabilities for PCI access");
}
/*
 * Enumerate all PCI devices
 */
static void pci_enumerate(void) {
  Pcidev *p;
  PCIDev *pd, **tail;
  lock(&pcistate.lock);
  /* Free existing list */
  while (pcistate.devlist != ((void *)0)) {
    pd = pcistate.devlist;
    pcistate.devlist = pd->next;
    free(pd);
  }
  pcistate.ndevs = 0;
  /* Enumerate all PCI devices */
  tail = &pcistate.devlist;
  p = ((void *)0);
  while ((p = pcimatch(p, 0, 0)) != ((void *)0)) {
    pd = smalloc(sizeof(PCIDev));
    pd->pci = p;
    pd->busno = (((p->tbdf) >> 16) & 0xFF);
    pd->devno = (((p->tbdf) >> 11) & 0x1F);
    pd->funcno = (((p->tbdf) >> 8) & 0x07);
    snprint(pd->name, sizeof(pd->name), "%04x:%02x:%02x.%d", 0, pd->busno,
            pd->devno, pd->funcno);
    pd->next = ((void *)0);
    *tail = pd;
    tail = &pd->next;
    pcistate.ndevs++;
  }
  unlock(&pcistate.lock);
}
/*
 * Find PCI device by name
 */
static PCIDev *pci_find(char *name) {
  PCIDev *pd;
  lock(&pcistate.lock);
  for (pd = pcistate.devlist; pd != ((void *)0); pd = pd->next) {
    if (strcmp(pd->name, name) == 0) {
      unlock(&pcistate.lock);
      return pd;
    }
  }
  unlock(&pcistate.lock);
  return ((void *)0);
}
/*
 * Directory generation
 */
static int pcigen(Chan *c, char *name, Dirtab *tab, int ntab, int pos,
                  Dir *dp) {
  Qid qid;
  PCIDev *pd;
  int n;
  char buf[64];
  if (c, tab, ntab) { };
  if (pos == 0) {
    /* "." */
    devdir(c, (Qid){Qdir, 0, 0x80}, ".", 0, eve, 0555, dp);
    return 1;
  }
  pos--;
  /* Root directory */
  if ((c->qid.path & 0xFFFF0000) == 0) {
    if (pos == 0) {
      devdir(c, (Qid){Qctl, 0, 0}, "ctl", 0, eve, 0444, dp);
      return 1;
    }
    pos--;
    if (pos == 0) {
      devdir(c, (Qid){Qbus, 0, 0}, "bus", 0, eve, 0444, dp);
      return 1;
    }
    pos--;
    /* Device directories */
    n = 0;
    lock(&pcistate.lock);
    for (pd = pcistate.devlist; pd != ((void *)0); pd = pd->next) {
      if (n == pos) {
        qid.path = Qdevbase + n;
        qid.vers = 0;
        qid.type = 0x80;
        unlock(&pcistate.lock);
        devdir(c, qid, pd->name, 0, eve, 0555, dp);
        return 1;
      }
      n++;
    }
    unlock(&pcistate.lock);
    return -1;
  }
  /* Device subdirectory */
  if (c->qid.path >= Qdevbase && c->qid.path < Qdevbase + 0x1000) {
    if (pos == 0) {
      qid.path = Qdevconfig + (c->qid.path - Qdevbase);
      qid.vers = 0;
      qid.type = 0;
      devdir(c, qid, "config", 256, eve, 0444, dp);
      return 1;
    }
    pos--;
    if (pos == 0) {
      qid.path = Qdevconfig + (c->qid.path - Qdevbase);
      qid.vers = 0;
      qid.type = 0;
      devdir(c, qid, "raw", 256, eve, 0444, dp);
      return 1;
    }
    pos--;
    if (pos == 0) {
      qid.path = Qdevctl + (c->qid.path - Qdevbase);
      qid.vers = 0;
      qid.type = 0;
      devdir(c, qid, "ctl", 0, eve, 0666, dp);
      return 1;
    }
    return -1;
  }
  return -1;
}
static void devpcireset(void) { memset(&pcistate, 0, sizeof(pcistate)); }
static Chan *pciattach(char *spec) {
  /* Enumerate PCI devices on first attach */
  if (pcistate.ndevs == 0)
    pci_enumerate();
  return devattach('P', spec);
}
static Walkqid *pciwalk(Chan *c, Chan *nc, char **name, int nname) {
  return devwalk(c, nc, name, nname, ((void *)0), 0, pcigen);
}
static int pcistat(Chan *c, uchar *dp, int n) {
  return devstat(c, dp, n, ((void *)0), 0, pcigen);
}
static Chan *pciopen(Chan *c, int omode) {
  checkcap(CapPCI);
  c = devopen(c, omode, ((void *)0), 0, pcigen);
  c->offset = 0;
  return c;
}
static void pciclose(Chan *c) { if (c) { }; }
/*@ requires c != \null;
    requires va != \null;
    requires (n > 0 ==> \valid((char*)va + (0 .. (integer)n-1))) || (n == 0);
    assigns ((char*)va)[0 .. (integer)n-1] \if n > 0;
*/
static long pciread(Chan *c, void *va, long n, vlong off) {
  char buf[4096];
  int len, devno;
  PCIDev *pd;
  uchar *config;
  int i;
  /* Universal CBS: Check PCI capability for device access */
  if ((c->qid.path >= Qdevconfig) && !((up)->capabilities & ((1 << 7))))
    lux9_error("permission denied");
  switch (c->qid.path) {
  case Qdir:
    return devdirread(c, va, n, ((void *)0), 0, pcigen);
  case Qctl:
    /* Return PCI status */
    len = snprint(buf, sizeof(buf), "devices: %d\n", pcistate.ndevs);
    if (off >= len)
      return 0;
    if (off + n > len)
      n = len - off;
    memmove(va, buf + off, n);
    return n;
  case Qbus:
    /* List all PCI devices */
    len = 0;
    lock(&pcistate.lock);
    for (pd = pcistate.devlist; pd != ((void *)0); pd = pd->next) {
      /* Check if we have enough space for device line */
      int device_len = snprint(
          ((void *)0), 0,
          "%s vendor=0x%04x device=0x%04x class=%02x.%02x.%02x irq=%d\n",
          pd->name, pd->pci->vid, pd->pci->did, pd->pci->ccrb, pd->pci->ccru,
          pd->pci->ccrp, pd->pci->intl);
      /* Stop if we don't have enough space for this device */
      if (len + device_len >= sizeof(buf) - 1)
        break;
      len += snprint(
          buf + len, sizeof(buf) - len,
          "%s vendor=0x%04x device=0x%04x class=%02x.%02x.%02x irq=%d\n",
          pd->name, pd->pci->vid, pd->pci->did, pd->pci->ccrb, pd->pci->ccru,
          pd->pci->ccrp, pd->pci->intl);
      /* Add BAR information */
      for (i = 0; i < (sizeof(pd->pci->mem) / sizeof((pd->pci->mem)[0])); i++) {
        if (pd->pci->mem[i].size > 0) {
          /* Check if we have enough space for BAR line */
          int bar_len = snprint(((void *)0), 0, "  bar%d: addr=0x%llux size=0x%llux\n",
                                i, pd->pci->mem[i].bar, pd->pci->mem[i].size);
          /* Stop if we don't have enough space for this BAR */
          if (len + bar_len >= sizeof(buf) - 1)
            break;
          len += snprint(buf + len, sizeof(buf) - len,
                         "  bar%d: addr=0x%llux size=0x%llux\n", i,
                         pd->pci->mem[i].bar, pd->pci->mem[i].size);
        }
      }
      /* Check if we've run out of buffer space */
      if (len >= sizeof(buf) - 1)
        break;
    }
    unlock(&pcistate.lock);
    if (off >= len)
      return 0;
    if (off + n > len)
      n = len - off;
    memmove(va, buf + off, n);
    return n;
  default:
    /* Config space read */
    if (c->qid.path >= Qdevconfig && c->qid.path < Qdevconfig + 0x1000) {
      devno = c->qid.path - Qdevconfig;
      /* Find device */
      i = 0;
      lock(&pcistate.lock);
      for (pd = pcistate.devlist; pd != ((void *)0); pd = pd->next) {
        if (i == devno) {
          unlock(&pcistate.lock);
          /* Read config space */
          config = smalloc(256);
          for (i = 0; i < 256; i++)
            config[i] = pcicfgr8(pd->pci, i);
          if (off >= 256) {
            free(config);
            return 0;
          }
          if (off + n > 256)
            n = 256 - off;
          memmove(va, config + off, n);
          free(config);
          return n;
        }
        i++;
      }
      unlock(&pcistate.lock);
      lux9_error("device not found");
    }
    /* Device ctl read */
    if (c->qid.path >= Qdevctl && c->qid.path < Qdevctl + 0x1000) {
      devno = c->qid.path - Qdevctl;
      i = 0;
      lock(&pcistate.lock);
      for (pd = pcistate.devlist; pd != ((void *)0); pd = pd->next) {
        if (i == devno) {
          unlock(&pcistate.lock);
          /* Return device info */
          len = 0;
          len += snprint(buf + len, sizeof(buf) - len, "vendor: 0x%04x\n",
                         pd->pci->vid);
          len += snprint(buf + len, sizeof(buf) - len, "device: 0x%04x\n",
                         pd->pci->did);
          len +=
              snprint(buf + len, sizeof(buf) - len, "class: %02x.%02x.%02x\n",
                      pd->pci->ccrb, pd->pci->ccru, pd->pci->ccrp);
          len +=
              snprint(buf + len, sizeof(buf) - len, "irq: %d\n", pd->pci->intl);
          len += snprint(buf + len, sizeof(buf) - len, "tbdf: 0x%08x\n",
                         pd->pci->tbdf);
          for (i = 0; i < (sizeof(pd->pci->mem) / sizeof((pd->pci->mem)[0])); i++) {
            if (pd->pci->mem[i].size > 0) {
              len += snprint(buf + len, sizeof(buf) - len,
                             "bar%d: 0x%llux %llud\n", i, pd->pci->mem[i].bar,
                             pd->pci->mem[i].size);
            }
          }
          if (off >= len)
            return 0;
          if (off + n > len)
            n = len - off;
          memmove(va, buf + off, n);
          return n;
        }
        i++;
      }
      unlock(&pcistate.lock);
      lux9_error("device not found");
    }
    lux9_error(Egreg);
    return 0;
  }
}
/*@ requires c != \null;
    requires va != \null;
    requires (n > 0 ==> \valid_read((char*)va + (0 .. (integer)n-1))) || (n ==
   0); assigns \nothing;
*/
static long pciwrite(Chan *c, void *va, long n, vlong off) {
  if (va, off) { };
  /* Universal CBS: Check PCI capability for write access */
  if ((c->qid.path >= Qdevconfig) && !((up)->capabilities & ((1 << 7))))
    lux9_error("permission denied");
  switch (c->qid.path) {
  case Qdir:
  case Qbus:
    lux9_error(Eperm);
    return 0;
  case Qctl:
    /* Could add rescan command here */
    lux9_error(Eperm);
    return 0;
  default:
    lux9_error(Eperm);
    return 0;
  }
}
Dev pcidevtab = {
    'P', "pci",
    devpcireset, devinit, devshutdown, pciattach, pciwalk,
    pcistat, pciopen, devcreate, pciclose, pciread,
    devbread, pciwrite, devbwrite, devremove, devwstat,
};
