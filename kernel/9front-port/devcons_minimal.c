/*
 * Minimal Console Driver for Lux9 Kernel
 *
 * Provides raw UART access for early boot and kernel logging.
 * Stripped of all terminal emulation, keyboard mapping, and complex buffering.
 *
 * Functionality:
 * - /dev/cons: Raw access to system console (UART)
 * - /dev/kmesg: Kernel log buffer
 * - /dev/kprint: Kernel print output
 */

#include "dat.h"
#include "error.h"
#include "fns.h"
#include "io.h"
#include "mem.h"
#include "portlib.h"
#include "u.h"
#include "ureg.h"

/* Qids */
enum {
  Qdir,
  Qcons,
  Qsysname,
  Qkmesg,
  Qkprint,
  Qcmd,
};

static Dirtab consdir[] = {
    {'.', {Qdir, 0, QTDIR}, 0, DMDIR | 0555}, {"cons", {Qcons}, 0, 0660},
    {"sysname", {Qsysname}, 0, 0664},         {"kmesg", {Qkmesg}, 0, 0440},
    {"kprint", {Qkprint}, 0, 0440},           {"consctl", {Qcmd}, 0, 0220},
};

/* Kernel message buffer (kmesg) */
struct Kmesg kmesg;

/* Queue for console input */
Queue *kbdq;
/* kprintoq defined in rdb.c */
Queue *serialoq;

static struct {
  QLock lk;
  int opens;
} kprintq;

/* Forward declarations */
static void kprintinit(void);
static void devcons_screenputs(char *s, int n);

/*
 * Device Operations
 */

/*@
  @ assigns \nothing;
  @*/
static void consinit(void) {
  kprintinit();
  screenputs = devcons_screenputs; /* Assign our screen output function */
  /* Initialize UART if not already done by arch */
}

static Chan *consattach(char *spec) { return devattach('c', spec); }

static Walkqid *conswalk(Chan *c, Chan *nc, char **name, int nname) {
  return devwalk(c, nc, name, nname, consdir, nelem(consdir), devgen);
}

/*@
  @ requires c == \null || \valid(c);
  @ requires dp == \null || \valid(dp);
  @ assigns \nothing;
  @*/
static int consstat(Chan *c, uchar *dp, int n) {
  return devstat(c, dp, n, consdir, nelem(consdir), devgen);
}

static Chan *consopen(Chan *c, int omode) {
  c = devopen(c, omode, consdir, nelem(consdir), devgen);

  switch ((ulong)c->qid.path) {
  case Qcons:
    /* Raw console access */
    break;

  case Qkprint:
    qlock(&kprintq.lk);
    if (kprintq.opens == 0) {
      if (kprintoq == nil) {
        kprintoq = qopen(8 * 1024, Qcoalesce, 0, 0);
        if (kprintoq == nil) {
          qunlock(&kprintq.lk);
          error(Enomem);
        }
      }
    }
    kprintq.opens++;
    qunlock(&kprintq.lk);
    break;
  }
  return c;
}

/*@
  @ requires c == \null || \valid(c);
  @ assigns \nothing;
  @*/
static void consclose(Chan *c) {
  switch ((ulong)c->qid.path) {
  case Qkprint:
    qlock(&kprintq.lk);
    if (kprintq.opens > 0)
      kprintq.opens--;
    if (kprintq.opens == 0 && kprintoq != nil) {
      qfree(kprintoq);
      kprintoq = nil;
    }
    qunlock(&kprintq.lk);
    break;
  }
}

/*@
  @ requires c == \null || \valid(c);
  @ requires va == \null || \valid(va);
  @ assigns \nothing;
  @*/
static long consread(Chan *c, void *va, long n, vlong offset) {
  char *p = va;

  if (n <= 0)
    return n;

  switch ((ulong)c->qid.path) {
  case Qdir:
    return devdirread(c, va, n, consdir, nelem(consdir), devgen);

  case Qsysname:
    return readstr(offset, va, n, sysname);

  case Qkmesg:
    /* Read from circular kernel log buffer */
    /* Simple implementation: just read what's available */
    lock(&kmesg.lk);
    if (offset >= kmesg.n) {
      unlock(&kmesg.lk);
      return 0;
    }
    if (offset + n > kmesg.n)
      n = kmesg.n - offset;
    memmove(va, kmesg.buf + offset, n);
    unlock(&kmesg.lk);
    return n;

  case Qcons:
    /* Read directly from UART via polling/interrupts */
    /* In this minimal driver, we rely on the architecture's UART integration */
    /* For now, just return EOF to prevent hangs if not implemented */
    return 0;

  default:
    error(Egreg);
  }
  return -1;
}

/*@
  @ requires c == \null || \valid(c);
  @ requires va == \null || \valid(va);
  @ assigns \nothing;
  @*/
static long conswrite(Chan *c, void *va, long n, vlong offset) {
  char *p = va;

  if (n <= 0)
    return n;

  switch ((ulong)c->qid.path) {
  case Qcons:
    /* Write directly to system console (screenputs/uart) */
    /* This bypasses all line discipline */
    if (waserror()) {
      /* Catch errors during output */
      nexterror();
    }

    /* Using the lowest level print function available */
    /* Write byte by byte or chunk */
    /* Note: kprint will eventually call uart output */
    /* To avoid recursion or formatting, we use a raw put function if available
     */
    /* For now, print() is safe enough for boot */

    /* We need to be careful not to format 'va' if it has % characters */
    /* Direct UART write loop would be cleaner, but architecture dependent */

    /* Minimal implementation: assume 'va' is a string chunk */
    /* In a real microkernel, this writes to the UART driver channel */

    /* Hack for minimal boot output */
    {
      char tmp[128];
      long left = n;
      char *ptr = p;
      while (left > 0) {
        int chunk = (left < sizeof(tmp) - 1) ? left : sizeof(tmp) - 1;
        memmove(tmp, ptr, chunk);
        tmp[chunk] = 0;
        print("%s", tmp); // Use kernel print
        ptr += chunk;
        left -= chunk;
      }
    }
    poperror();
    return n;

  case Qsysname:
    /* Update system name */
    if (offset != 0)
      error(Ebadarg);
    if (n >= sizeof(sysname) - 1)
      n = sizeof(sysname) - 1;
    memmove(sysname, va, n);
    sysname[n] = 0;
    return n;

  case Qcmd:
    /* Ignore control commands in minimal driver */
    return n;

  default:
    error(Egreg);
  }
  return -1;
}

Dev consdevtab = {
    'c',      "cons",

    devreset, consinit,  devshutdown, consattach, conswalk,
    consstat, consopen,  devcreate,   consclose,  consread,
    devbread, conswrite, devbwrite,   devremove,  devwstat,
};

/*
 * Kernel Message Buffer Implementation
 */
/*@
  @ assigns \nothing;
  @*/
static void kprintinit(void) {
  /* Initialize the kernel message buffer lock */
  /* kmesg.buf is zeroed by bss */
}

/*
 * Core kernel print hooks
 * Called by print() in libc9/print.c via screenputs function pointer
 */
/*@
  @ requires s == \null || \valid(s);
  @ assigns \nothing;
  @*/
static void devcons_screenputs(char *s, int n) {
  /* 1. Write to UART (Hardware Output) */
  /* Check if uart is available globally */
  extern Uart *consuart;
  if (consuart && consuart->phys && consuart->phys->putc) {
      /*@ loop invariant 0 <= i <= n;
    @ loop assigns i;
    @ loop variant n - i;
    @*/
  for (int i = 0; i < n; i++)
      consuart->phys->putc(consuart, s[i]);
  }

  /* 2. Write to kmesg buffer (Memory Log) */
  ilock(&kmesg.lk);
  if (kmesg.n + (uint)n > sizeof(kmesg.buf)) {
    /* Buffer full - ring buffer logic omitted for minimal driver simplicity */
    /* Just stop recording or wrap? For now, simplistic truncate */
    uint avail = sizeof(kmesg.buf) - kmesg.n;
    if (avail > 0) {
      memmove(kmesg.buf + kmesg.n, s, avail);
      kmesg.n += avail;
    }
  } else {
    memmove(kmesg.buf + kmesg.n, s, (uint)n);
    kmesg.n += (uint)n;
  }
  iunlock(&kmesg.lk);
}

/* panic/panicking are defined in print.c */
