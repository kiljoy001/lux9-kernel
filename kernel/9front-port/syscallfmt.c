/*
 * Print functions for system call tracing.
 */
#include "dat.h"
#include "fns.h"
#include "mem.h"
#include "portlib.h"
#include "u.h"

#include <sys.h>

// WE ARE OVERRUNNING SOMEHOW
static void fmtrwdata(Fmt *f, char *a, int n, char *suffix) {
  int i;
  char *t;

  if (a == nil) {
    fmtprint(f, "0x0%s", suffix);
    return;
  }
  validaddr((uintptr)a, n, 0);
  t = smalloc(n + 1);
  t[n] = 0;
  for (i = 0; i < n; i++)
    if (a[i] > 0x20 && a[i] < 0x7f) /* printable ascii? */
      t[i] = a[i];
    else
      t[i] = '.';

  fmtprint(f, " %#p/\"%s\"%s", a, t, suffix);
  free(t);
}

static void fmtuserstring(Fmt *f, char *a, char *suffix) {
  char *t, *e;
  int n;

  if (a == nil) {
    fmtprint(f, "0/\"\"%s", suffix);
    return;
  }
  validaddr((uintptr)a, 1, 0);
  n = 1 << 16;
  e = vmemchr(a, 0, n);
  if (e != nil)
    n = e - a;
  t = smalloc(n + 1);
  memmove(t, a, n);
  t[n] = 0;
  fmtprint(f, "%#p/\"%s\"%s", a, t, suffix);
  free(t);
}

void syscallfmt(ulong syscallno, uintptr pc, ulong *list) {
  long l;
  Fmt fmt;
  void *v;
  vlong vl;
  uintptr p;
  int i[2], len;
  char *a, **argv;

  fmtstrinit(&fmt);
  fmtprint(&fmt, "%uld %s ", up->pid, up->text);

  if (syscallno >= nsyscall)
    fmtprint(&fmt, " %uld ", syscallno);
  else
    fmtprint(&fmt, "%s ", sysctab[syscallno] ? sysctab[syscallno] : "huh?");

  fmtprint(&fmt, "%p ", pc);
  switch (syscallno) {
  case SYSR1:
    p = *(uintptr *)list;
    list++;
    fmtprint(&fmt, "%#p", p);
    break;
  case _ERRSTR: /* deprecated */
  case CHDIR:
  case EXITS:
  case REMOVE:
    a = *(char **)list;
    list++;
    fmtuserstring(&fmt, a, "");
    break;
  case BIND:
    a = *(char **)list;
    list++;
    fmtuserstring(&fmt, a, " ");
    a = *(char **)list;
    list++;
    fmtuserstring(&fmt, a, " ");
    i[0] = *(int *)list;
    list++;
    fmtprint(&fmt, "%#ux", i[0]);
    break;
  case CLOSE:
  case NOTED:
    i[0] = *(int *)list;
    list++;
    fmtprint(&fmt, "%d", i[0]);
    break;
  case DUP:
    i[0] = *(int *)list;
    list++;
    i[1] = *(int *)list;
    list++;
    fmtprint(&fmt, "%d %d", i[0], i[1]);
    break;
  case ALARM:
    l = *(ulong *)list;
    list++;
    fmtprint(&fmt, "%#lud ", l);
    break;
  case EXEC:
    a = *(char **)list;
    list++;
    fmtuserstring(&fmt, a, "");
    argv = *(char ***)list;
    list++;
    evenaddr((uintptr)argv);
    for (;;) {
      validaddr((uintptr)argv, sizeof(char **), 0);
      a = *(char **)argv;
      if (a == nil)
        break;
      fmtprint(&fmt, " ");
      fmtuserstring(&fmt, a, "");
      argv++;
    }
    break;
  case _FSESSION: /* deprecated */
  case _FSTAT:    /* deprecated */
  case _FWSTAT:   /* obsolete */
    i[0] = *(int *)list;
    list++;
    a = *(char **)list;
    list++;
    fmtprint(&fmt, "%d %#p", i[0], a);
    break;
  case FAUTH:
    i[0] = *(int *)list;
    list++;
    a = *(char **)list;
    list++;
    fmtprint(&fmt, "%d", i[0]);
    fmtuserstring(&fmt, a, "");
    break;
  case SEGBRK:
  case RENDEZVOUS:
    v = *(void **)list;
    list++;
    fmtprint(&fmt, "%#p ", v);
    v = *(void **)list;
    list++;
    fmtprint(&fmt, "%#p", v);
    break;
  case _MOUNT: /* deprecated */
    i[0] = *(int *)list;
    list++;
    fmtprint(&fmt, "%d ", i[0]);
    a = *(char **)list;
    list++;
    fmtuserstring(&fmt, a, " ");
    i[0] = *(int *)list;
    list++;
    fmtprint(&fmt, "%#ux ", i[0]);
    a = *(char **)list;
    list++;
    fmtuserstring(&fmt, a, "");
    break;
  case OPEN:
    a = *(char **)list;
    list++;
    fmtuserstring(&fmt, a, " ");
    i[0] = *(int *)list;
    list++;
    fmtprint(&fmt, "%#ux", i[0]);
    break;
  case OSEEK: /* deprecated */
    i[0] = *(int *)list;
    list++;
    l = *(long *)list;
    list++;
    i[1] = *(int *)list;
    list++;
    fmtprint(&fmt, "%d %ld %d", i[0], l, i[1]);
    break;
  case SLEEP:
    l = *(long *)list;
    list++;
    fmtprint(&fmt, "%ld", l);
    break;
  case _STAT:  /* obsolete */
  case _WSTAT: /* obsolete */
    a = *(char **)list;
    list++;
    fmtuserstring(&fmt, a, " ");
    a = *(char **)list;
    list++;
    fmtprint(&fmt, "%#p", a);
    break;
  case RFORK:
    i[0] = *(int *)list;
    list++;
    fmtprint(&fmt, "%#ux", i[0]);
    break;
  case PIPE:
  case BRK_:
    v = *(int **)list;
    list++;
    fmtprint(&fmt, "%#p", v);
    break;
  case CREATE:
    a = *(char **)list;
    list++;
    fmtuserstring(&fmt, a, " ");
    i[0] = *(int *)list;
    list++;
    i[1] = *(int *)list;
    list++;
    fmtprint(&fmt, "%#ux %#ux", i[0], i[1]);
    break;
  case FD2PATH:
  case FSTAT:
  case FWSTAT:
    i[0] = *(int *)list;
    list++;
    a = *(char **)list;
    list++;
    l = *(ulong *)list;
    list++;
    fmtprint(&fmt, "%d %#p %lud", i[0], a, l);
    break;
  case NOTIFY:
  case SEGDETACH:
  case _WAIT: /* deprecated */
    v = *(void **)list;
    list++;
    fmtprint(&fmt, "%#p", v);
    break;
  case SEGATTACH:
    i[0] = *(int *)list;
    list++;
    fmtprint(&fmt, "%d ", i[0]);
    a = *(char **)list;
    list++;
    fmtuserstring(&fmt, a, " ");
    /*FALLTHROUGH*/
  case SEGFREE:
  case SEGFLUSH:
    v = *(void **)list;
    list++;
    l = *(ulong *)list;
    list++;
    fmtprint(&fmt, "%#p %lud", v, l);
    break;
  case UNMOUNT:
    a = *(char **)list;
    list++;
    fmtuserstring(&fmt, a, " ");
    a = *(char **)list;
    list++;
    fmtuserstring(&fmt, a, "");
    break;
  case SEMACQUIRE:
  case SEMRELEASE:
    v = *(int **)list;
    list++;
    i[0] = *(int *)list;
    list++;
    fmtprint(&fmt, "%#p %d", v, i[0]);
    break;
  case TSEMACQUIRE:
    v = *(int **)list;
    list++;
    l = *(ulong *)list;
    list++;
    fmtprint(&fmt, "%#p %ld", v, l);
    break;
  case SEEK:
    v = *(vlong **)list;
    list++;
    i[0] = *(int *)list;
    list++;
    vl = *(vlong *)list;
    list++;
    i[1] = *(int *)list;
    list++;
    fmtprint(&fmt, "%#p %d %#llux %d", v, i[0], vl, i[1]);
    break;
  case FVERSION:
    i[0] = *(int *)list;
    list++;
    i[1] = *(int *)list;
    list++;
    fmtprint(&fmt, "%d %d ", i[0], i[1]);
    a = *(char **)list;
    list++;
    fmtuserstring(&fmt, a, " ");
    l = *(ulong *)list;
    list++;
    fmtprint(&fmt, "%lud", l);
    break;
  case WSTAT:
  case STAT:
    a = *(char **)list;
    list++;
    fmtuserstring(&fmt, a, " ");
    /*FALLTHROUGH*/
  case ERRSTR:
  case AWAIT:
    a = *(char **)list;
    list++;
    l = *(ulong *)list;
    list++;
    fmtprint(&fmt, "%#p %lud", a, l);
    break;
  case MOUNT:
    i[0] = *(int *)list;
    list++;
    i[1] = *(int *)list;
    list++;
    fmtprint(&fmt, "%d %d ", i[0], i[1]);
    a = *(char **)list;
    list++;
    fmtuserstring(&fmt, a, " ");
    i[0] = *(int *)list;
    list++;
    fmtprint(&fmt, "%#ux ", i[0]);
    a = *(char **)list;
    list++;
    fmtuserstring(&fmt, a, "");
    break;
  case _READ: /* deprecated */
  case PREAD:
    i[0] = *(int *)list;
    list++;
    v = *(void **)list;
    list++;
    l = *(long *)list;
    list++;
    fmtprint(&fmt, "%d %#p %ld", i[0], v, l);
    if (syscallno == PREAD) {
      vl = *(vlong *)list;
      list++;
      fmtprint(&fmt, " %lld", vl);
    }
    break;
  case _WRITE: /* deprecated */
  case PWRITE:
    i[0] = *(int *)list;
    list++;
    v = *(void **)list;
    list++;
    l = *(long *)list;
    list++;
    fmtprint(&fmt, "%d ", i[0]);
    len = MIN(l, 64);
    fmtrwdata(&fmt, v, len, " ");
    fmtprint(&fmt, "%ld", l);
    if (syscallno == PWRITE) {
      vl = *(vlong *)list;
      list++;
      fmtprint(&fmt, " %lld", vl);
    }
    break;
  case _NSEC:
    if (sizeof(uintptr) == sizeof(vlong))
      break;
    v = *(vlong **)list;
    list++;
    fmtprint(&fmt, "%#p", v);
    break;
  }

  a = fmtstrflush(&fmt);
  qlock(&up->debug);
  free(up->syscalltrace);
  up->syscalltrace = a;
  qunlock(&up->debug);
}

void sysretfmt(ulong syscallno, ulong *list, uintptr ret, uvlong start,
               uvlong stop) {
  long l;
  void *v;
  Fmt fmt;
  vlong vl;
  int i, len;
  char *a, *errstr;

  fmtstrinit(&fmt);

  errstr = "\"\"";
  switch (syscallno) {
  case EXEC:
  case SEGBRK:
  case SEGATTACH:
  case RENDEZVOUS:
    if ((void *)ret == (void *)-1)
      errstr = up->syserrstr;
    fmtprint(&fmt, " = %#p", (void *)ret);
    break;
  case AWAIT:
    a = *(char **)list;
    list++;
    l = *(ulong *)list;
    list++;
    if (ret > 0) {
      fmtuserstring(&fmt, a, " ");
      fmtprint(&fmt, "%lud = %ld", l, (long)ret);
    } else {
      fmtprint(&fmt, "%#p/\"\" %lud = %ld", a, l, (long)ret);
      errstr = up->syserrstr;
    }
    break;
  case _ERRSTR:
  case ERRSTR:
    a = *(char **)list;
    list++;
    if (syscallno == _ERRSTR)
      l = 64;
    else {
      l = *(ulong *)list;
      list++;
    }
    if (ret > 0) {
      fmtuserstring(&fmt, a, " ");
      fmtprint(&fmt, "%lud = %ld", l, (long)ret);
    } else {
      fmtprint(&fmt, "\"\" %lud = %ld", l, (long)ret);
      errstr = up->syserrstr;
    }
    break;
  case FD2PATH:
    i = *(int *)list;
    list++;
    USED(i);
    a = *(char **)list;
    list++;
    l = *(ulong *)list;
    list++;
    if (ret > 0) {
      fmtuserstring(&fmt, a, " ");
      fmtprint(&fmt, "%lud = %ld", l, (long)ret);
    } else {
      fmtprint(&fmt, "\"\" %lud = %ld", l, (long)ret);
      errstr = up->syserrstr;
    }
    break;
  case _READ:
  case PREAD:
    i = *(int *)list;
    list++;
    USED(i);
    v = *(void **)list;
    list++;
    l = *(long *)list;
    list++;
    if (ret > 0) {
      len = MIN(ret, 64);
      fmtrwdata(&fmt, v, len, "");
    } else {
      fmtprint(&fmt, "/\"\"");
      errstr = up->syserrstr;
    }
    fmtprint(&fmt, " %ld", l);
    if (syscallno == PREAD) {
      vl = *(vlong *)list;
      list++;
      fmtprint(&fmt, " %lld", vl);
    }
    fmtprint(&fmt, " = %ld", (long)ret);
    break;
  case _NSEC:
    if (sizeof(uintptr) == sizeof(vlong)) {
      fmtprint(&fmt, " = %lld", (vlong)ret);
      break;
    }
    /* wet floor */
  case ALARM:
  case _WRITE:
  case PWRITE:
  default:
    if ((long)ret == -1)
      errstr = up->syserrstr;
    fmtprint(&fmt, " = %ld", (long)ret);
    break;
  }
  fmtprint(&fmt, " %s %#llud %#llud\n", errstr, start, stop);

  a = fmtstrflush(&fmt);
  qlock(&up->debug);
  free(up->syscalltrace);
  up->syscalltrace = a;
  qunlock(&up->debug);
}
