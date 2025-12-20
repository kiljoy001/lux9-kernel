#include "u.h"
#include "portlib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include <error.h>
#include "9p_router.h"

/*
 * SIP Device Driver (#X)
 * Exposes the exchange page mechanism as a device.
 */

enum {
    Qdir,
    Qexchange,
};

static Dirtab siptab[] = {
    {".", {Qdir, 0, QTDIR}, 0, 0555},
    {"exchange", {Qexchange, 0, 0}, 0, 0600},
};

static Chan*
sipattach(char *spec)
{
    return devattach('X', spec);
}

static Walkqid*
sipwalk(Chan *c, Chan *nc, char **name, int nname)
{
    return devwalk(c, nc, name, nname, siptab, nelem(siptab), devgen);
}

static int
sipstat(Chan *c, uchar *db, int n)
{
    return devstat(c, db, n, siptab, nelem(siptab), devgen);
}

static Chan*
sipopen(Chan *c, int omode)
{
    return devopen(c, omode, siptab, nelem(siptab), devgen);
}

static void
sipclose(Chan *c)
{
    /* Nothing to cleanup */
}

static long
sipread(Chan *c, void *a, long n, vlong offset)
{
    /* Read from the exchange page memory */
    if ((ulong)c->qid.path == Qexchange) {
        if (up->p9page == nil) error("no exchange page");
        if (offset >= P9_PAGE_SIZE) return 0;
        if (offset + n > P9_PAGE_SIZE) n = P9_PAGE_SIZE - offset;
        
        memmove(a, (char*)up->p9page + offset, n);
        return n;
    }
    return devread(c, a, n, offset, siptab, nelem(siptab), devgen);
}

static long
sipwrite(Chan *c, void *a, long n, vlong offset)
{
    if ((ulong)c->qid.path == Qexchange) {
        if (up->p9page == nil) error("no exchange page");
        if (offset >= P9_PAGE_SIZE) return 0;
        if (offset + n > P9_PAGE_SIZE) n = P9_PAGE_SIZE - offset;
        
        memmove((char*)up->p9page + offset, a, n);
        return n;
    }
    error(Eperm);
    return -1;
}

Dev sipdevtab = {
    'X',
    "sip",
    devreset,
    devinit,
    devshutdown,
    sipattach,
    sipwalk,
    sipstat,
    sipopen,
    devcreate,
    sipclose,
    sipread,
    devbread,
    sipwrite,
    devbwrite,
    devremove,
    devwstat,
};
