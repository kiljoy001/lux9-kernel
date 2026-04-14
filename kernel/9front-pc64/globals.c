/* Global kernel variables */
#include "dat.h"
#include "error.h"
#include "fns.h"
#include "io.h"
#include "mem.h"
#include "9p_router.h"
#include "pci.h"
#include "u.h"
#include "ureg.h"
#include <lib.h>

/* Memory constants defined in memory_9front.c */
extern u64int MemMin; /* set by bootargsinit() */

extern uvlong rdtsc(void);
extern void uartputs(char *, int);

/* Format flags - from libc.h */
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

/* Format function declarations */
extern int _charfmt(Fmt *);
extern int _runefmt(Fmt *);
extern int _ifmt(Fmt *);
extern int _strfmt(Fmt *);
extern int _runesfmt(Fmt *);
extern int _percentfmt(Fmt *);
extern int _countfmt(Fmt *);
extern int _flagfmt(Fmt *);
extern int _badfmt(Fmt *);

/* Per-CPU and per-process globals */
Mach *m = nil;
Proc *up = nil;

/* HHDM offset - stored in early boot, guaranteed to survive CR3 switch */
uintptr saved_limine_hhdm_offset = 0;
uintptr hhdm_base = 0;

/* HHDM base for the hhdm.h interface - initialized from Limine offset */

/* Global kernel data structures */
struct Swapalloc swapalloc;
/* kmesg is defined in devcons_minimal.c */
struct Active active;
Mach *machp[MAXMACH];

/* System name - used by devcons and 9p_router */
char *sysname = "lux9";

/* Global function pointers */
void (*consdebug)(void) = nil;
void (*hwrandbuf)(void *, ulong) = nil;
void (*kproftimer)(uintptr) = nil;
void (*screenputs)(char *, int) = nil;

int (*sd_inb)(int) = nil;
void (*sd_outb)(int, int) = nil;
ulong (*sd_inl)(int) = nil;
void (*sd_outl)(int, ulong) = nil;
void (*sd_insb)(int, void *, int) = nil;
void (*sd_inss)(int, void *, int) = nil;
void (*sd_outsb)(int, void *, int) = nil;
void (*sd_outss)(int, void *, int) = nil;
Pcidev *(*sd_pcimatch)(Pcidev *prev, int vid, int did) = pcimatch;
void (*sd_microdelay)(int) = nil;

/* libc9 formatting support */
/*@
  @ requires f == \null || \valid(f);
  @ assigns \nothing;
  @*/
int _fmtFdFlush(Fmt *f) {
  /* Write buffered format output to file descriptor */
  if (f == nil || f->start == nil)
    return 0;

  int n = f->to - f->start;
  if (n > 0 && f->farg != nil) {
    /* f->farg typically contains the FD as a pointer */
    /* In kernel context, use write syscall or kwrite */
    /* For now, output to console via print */
    char *buf = f->start;
    char save = buf[n];
    buf[n] = 0;
    print("%s", buf);
    buf[n] = save;
  }

  /* Reset buffer */
  f->to = f->start;
  return 0;
}

/* Get return address of caller */
/*@
  @ requires v == \null || \valid(v);
  @ assigns \nothing;
  @*/
uintptr getcallerpc(void *v) {
  (void)v;
  return (uintptr)__builtin_return_address(1);
}

/* Error strings */
char Etoolong[] = "name too long";

/*@
  @ requires v == \null || \valid(v);
  @ assigns \nothing;
  @*/
int needpages(void *v) {
  int noswap;

  (void)v;
  noswap = (up != nil && up->noswap);
  return !(palloc.freecount > swapalloc.highwater ||
           (noswap && palloc.freecount > 0));
}

char *configfile = "";

/* Device table - array of device drivers */
extern Dev rootdevtab;
extern Dev archdevtab;
extern Dev procdevtab;
extern Dev exchdevtab;
extern Dev memdevtab;
extern Dev irqdevtab;
extern Dev dmadevtab;
extern Dev pcidevtab;
extern Dev consdevtab;

extern Dev sipdevtab;
extern Dev pebbledevtab;
extern Dev ringdevtab;
extern Dev tpmdevtab;
extern Dev consensusdevtab;
extern Dev symdevtab;
extern Dev wasmdevtab;
extern Dev distressdevtab;
extern Dev pipedevtab;
extern Dev mntdevtab;
extern Dev srvdevtab;

Dev *devtab[] = {
    &rootdevtab, &archdevtab,      &consdevtab,
    &procdevtab,  &exchdevtab,     &memdevtab,
    &sipdevtab,   &pebbledevtab,  &ringdevtab,
	    &irqdevtab,   &dmadevtab,      &pcidevtab,
	    &tpmdevtab,   &consensusdevtab,
	    /* &symdevtab, */
	    &wasmdevtab,  &distressdevtab, &pipedevtab,
	    &mntdevtab,   &srvdevtab,      nil,
	};

/* Additional stubs for console/device support */
int cpuserver = 0;

#define SEP(x) ((x) == '/' || (x) == 0)

char *cleanname(char *name) {
  char *p, *q, *dotdot;
  int rooted, erasedprefix;

  rooted = name[0] == '/';
  erasedprefix = 0;

  /*
   * invariants:
   *	p points at beginning of path element we're considering.
   *	q points just past the last path element we wrote (no slash).
   *	dotdot points just past the point where .. cannot backtrack
   *		any further (no slash).
   */
  p = q = dotdot = name + rooted;
  while (*p) {
    if (p[0] == '/') /* null element */
      p++;
    else if (p[0] == '.' && SEP(p[1])) {
      if (p == name)
        erasedprefix = 1;
      p += 1; /* don't count the separator in case it is nul */
    } else if (p[0] == '.' && p[1] == '.' && SEP(p[2])) {
      p += 2;
      if (q > dotdot) { /* can backtrack */
        while (--q > dotdot && *q != '/')
          ;
      } else if (!rooted) { /* /.. is / but ./../ is .. */
        if (q != name)
          *q++ = '/';
        *q++ = '.';
        *q++ = '.';
        dotdot = q;
      }
      if (q == name)
        erasedprefix = 1; /* erased entire path via dotdot */
    } else {              /* real path element */
      if (q != name + rooted)
        *q++ = '/';
      while ((*q = *p) != '/' && *q != 0)
        p++, q++;
    }
  }
  if (q == name) /* empty string is really ``.'' */
    *q++ = '.';
  *q = '\0';
  if (erasedprefix && name[0] == '#') {
    /* this was not a #x device path originally - make it not one now */
    memmove(name + 2, name, strlen(name) + 1);
    name[0] = '.';
    name[1] = '/';
  }
  return name;
}

/* cycles pointer initialised in devarch/mp code */

extern char end[]; /* End of kernel - defined by linker */

/* Memory address conversion functions provided by mmu.c */

/* Random number - must match portlib.h signature */
/*@
  @ assigns \nothing;
  @*/
int nrand(int n) {
  /* Simple LCG */
  static ulong seed = 1;
  seed = seed * 1103515245 + 12345;
  return (int)((seed / 65536) % (ulong)n);
}

/* Error strings */
char Ecmdargs[] = "invalid command arguments";

/* Platform macro SET */
void SET(void *x) { (void)x; }

/* qsort implementation */
static int (*qsort_cmp)(void *, void *);

/* Architecture globals */
extern int cpuserver;
char *conffile = "";

/* Memory pool reset */

/* VMX (virtualization) stubs */
/* Now implemented in devvmx.c */
// void vmxshutdown(void) {}
// void vmxprocrestore(Proc *p) { (void)p; }

/* RAM page allocation - provided by memory_9front.c */

/* CPU identification provided by devarch.c */

/* Clock synchronization provided by mp.c */

/* NVRAM access */
/*@
  @ assigns \nothing;
  @*/
uchar nvramread(int addr) {
  outb(0x70, addr & 0x7F);
  return (uchar)inb(0x71);
}
/*@
  @ assigns \nothing;
  @*/
void nvramwrite(int addr, uchar val) {
  outb(0x70, addr & 0x7F);
  outb(0x71, val);
}

enum {
  I8042Data = 0x60,
  I8042Status = 0x64,
  I8042Outbusy = 0x02,
  I8042Cmd = 0x64,
};

static int i8042outready(void) {
  int tries;

  for (tries = 0; (inb(I8042Status) & I8042Outbusy) != 0; tries++) {
    if (tries > 500)
      return -1;
    delay(2);
  }
  return 0;
}

void i8042reset(void) {
  int i, x;

  *(ushort *)KADDR(0x472) = 0x1234;

  if (i8042outready() == 0) {
    outb(I8042Cmd, 0xFE);
    i8042outready();
  }

  x = 0xDF;
  for (i = 0; i < 5; i++) {
    x ^= 1;
    if (i8042outready() < 0)
      break;
    outb(I8042Cmd, 0xD1);
    if (i8042outready() < 0)
      break;
    outb(I8042Data, x);
    delay(100);
  }
}

/* DMA controller - function pointer (nil = not available) */
void (*i8237alloc)(void) = nil;

/* Memory initialization functions provided by memory_9front.c */

/* NOTE: ramdiskinit() removed - initrd is properly parsed in proc0 via
 * initrd_init() See kernel/9front-port/userinit.c:59-71 and
 * kernel/9front-pc64/initrd.c */

/* Coherence function pointer - implementation in l.S */
extern void coherence_impl(void);
void (*coherence)(void) = coherence_impl;

/* Additional global function pointers and buffers */
void (*fprestore)(FPsave *) = nil;
void (*fpsave)(FPsave *) = nil;

/* Architecture reset */
/* String functions */
char *strrchr(const char *s, int c) {
  const char *last = nil;
  while (*s) {
    if (*s == c)
      last = s;
    s++;
  }
  if (c == '\0')
    return (char *)s;
  return (char *)last;
}

/* Error strings */
char Edirseek[] = "directory seek";
char Eismtpt[] = "is a mount point";
char Enegoff[] = "negative offset";

/* Signal search provided by memory_9front.c */

/* UART console - global pointer */
Uart *consuart = nil;

/*@
  @ assigns \nothing;
  @*/
int uartgetc(void) {
  if (consuart == nil || consuart->phys == nil || consuart->phys->getc == nil)
    return -1;
  return consuart->phys->getc(consuart);
}

/*@
  @ assigns \nothing;
  @*/
void uartputc(int c) {
  if (consuart == nil || consuart->phys == nil || consuart->phys->putc == nil)
    return;
  consuart->phys->putc(consuart, c);
}

/* Checksum provided by memory_9front.c */

/* Delay loop */
void delayloop(int loops) {
  while (loops-- > 0)
    __asm__ __volatile__("pause" ::: "memory");
}

/* Math */
void mul64fract(uvlong *result, uvlong a, uvlong b) { *result = (a * b) >> 32; }

/* UTF-8 */
char *utfecpy(char *to, char *e, char *from) {
  if (to >= e)
    return to;
  while (*from && to < e - 1)
    *to++ = *from++;
  *to = '\0';
  return to;
}

/* UPA (user programmable arrays) provided by memory_9front.c */

/* Stubs for missing console/boot functions */
void printinit(void) {
  prbuf_init();
  if (kmesg.n > 0)
    uartputs(kmesg.buf, (int)kmesg.n);
}

/* Stubs for exit/reboot functions */
void cpushutdown(void) {
  int ms, once;

  once = active.machs[m->machno];
  active.machs[m->machno] = 0;
  active.exiting = 1;

  if (once)
    iprint("cpu%d: exiting\n", m->machno);

  spllo();
  for (ms = 5 * 1000; ms > 0; ms -= TK2MS(2)) {
    delay(TK2MS(2));
    if (memchr(active.machs, 1, MAXMACH) == nil && consactive() == 0)
      break;
  }
}
/* Console output stub */
/*@
  @ requires str == \null || \valid(str);
  @ assigns \nothing;
  @*/
void putstrn(char *str, int n) {
  if (str == nil || n <= 0)
    return;
  if (screenputs != nil)
    screenputs(str, n);
  else
    uartputs(str, n);
}

/* Stubs for missing symbols */
/*@
  @ assigns \nothing;
  @*/
uvlong nsec(void) { return fastticks2ns(fastticks(nil)); }

/*@
  @ requires buf == \null || \valid((uchar*)buf + (0..n-1));
  @ assigns ((uchar*)buf)[0..n-1];
  @*/
void randombytes(void *buf, long n) {
  if (buf == nil || n <= 0)
    return;
  randomread(buf, (ulong)n);
}

/*@
  @ assigns \nothing;
  @*/
int __popcountdi2(long long a) {
  unsigned long long x = (unsigned long long)a;
  int c = 0;
  while (x) {
    c++;
    x &= x - 1;
  }
  return c;
}
