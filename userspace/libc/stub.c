#include <u.h>
#include <libc.h>

void
_assert(char *s)
{
	print("assert failed: %s\n", s);
	abort();
}

void
uartputs(char *s, int n)
{
	write(2, s, n);
}

void
early_iprint(char *fmt, ...)
{
	va_list v;
	va_start(v, fmt);
	vfprint(2, fmt, v);
	va_end(v);
}

/* Lock stubs */
void lock(Lock *l) { USED(l); }
void unlock(Lock *l) { USED(l); }
void qlock(QLock *l) { USED(l); }
void qunlock(QLock *l) { USED(l); }

/* Exit stubs */
void
exit(int status)
{
    char buf[16];
    sprint(buf, "%d", status);
    _exits(buf);
}

void
abort(void)
{
    _exits("abort");
}

/* Float stub */
int
_efgfmt(Fmt *f)
{
    return fmtprint(f, "?");
}

/* utfecpy stub */
char*
utfecpy(char *to, char *e, char *from)
{
    if(to >= e) return to;
    char *p = memccpy(to, from, 0, e - to);
    if(p == 0){
        p = e - 1;
        *p = 0;
    } else {
        p--; /* point to terminator */
    }
    return p;
}

/* _fmtFdFlush needed by fmtfd.c */
int
_fmtFdFlush(Fmt *f)
{
    int n;
    if(f->start == 0)
        return 0;
    n = (uintptr)f->to - (uintptr)f->start;
    if(n && write((int)(uintptr)f->farg, f->start, n) != n)
        return 0;
    f->to = f->start;
    return 1;
}
