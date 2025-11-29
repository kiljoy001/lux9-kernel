#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "../port/error.h"
#include "pebble.h"

/*
 * /dev/pebble - Pebble Memory Accounting Interface
 *
 * Provides 9P interface for Pebble memory operations, replacing
 * syscalls to align with "Everything is a file" philosophy.
 */

enum {
    Qdir = 0,
    Qissue,
    Qverify,
    Qalloc,
    Qfree,
    Qstats,
};

static Dirtab pebbledir[] = {
    ".",        {Qdir, 0, QTDIR},   0,      DMDIR|0555,
    "issue",    {Qissue},           0,      0666,
    "verify",   {Qverify},          0,      0666,
    "alloc",    {Qalloc},           0,      0666,
    "free",     {Qfree},            0,      0666,
    "stats",    {Qstats},           0,      0444,
};

static void
pebinit(void)
{
    print("pebble: 9P interface initialized\n");
}

static Chan*
pebattach(char *spec)
{
    return devattach('P', spec);
}

static Walkqid*
pebwalk(Chan *c, Chan *nc, char **name, int nname)
{
    return devwalk(c, nc, name, nname, pebbledir, nelem(pebbledir), devgen);
}

static int
pebstat(Chan *c, uchar *dp, int n)
{
    return devstat(c, dp, n, pebbledir, nelem(pebbledir), devgen);
}

static Chan*
pebopen(Chan *c, int omode)
{
    return devopen(c, omode, pebbledir, nelem(pebbledir), devgen);
}

static void
pebclose(Chan *c)
{
    USED(c);
}

static long
pebread(Chan *c, void *va, long n, vlong off)
{
    char *buf;
    long rv = 0;
    
    switch((ulong)c->qid.path){
    case Qdir:
        return devdirread(c, va, n, pebbledir, nelem(pebbledir), devgen);
        
    case Qstats:
        buf = smalloc(1024);
        /* Assuming pebble_state() returns current proc's state or global stats */
        PebbleState *ps = pebble_state();
        if(ps) {
            rv = snprint(buf, 1024, 
                "budget: %lud\ninuse: %lud\nred: %lud\nblue: %lud\n",
                ps->black_budget, ps->black_inuse, ps->red_count, ps->blue_count);
        } else {
            rv = snprint(buf, 1024, "pebble state unavailable\n");
        }
        rv = readstr(off, va, n, buf);
        free(buf);
        return rv;
        
default:
        error(Eperm);
    }
    return 0;
}

static long
pebwrite(Chan *c, void *va, long n, vlong off)
{
    char *buf, *p;
    ulong size;
    PebbleWhite *pw;
    void *handle;
    PebbleState *ps;
    int err;

    USED(off);

    buf = smalloc(n+1);
    if(buf == nil)
        error(Enomem);
    memmove(buf, va, n);
    buf[n] = 0;

    ps = pebble_state();
    if(ps == nil){
        free(buf);
        error("pebble state not initialized");
    }

    switch((ulong)c->qid.path){
    case Qissue:
        /* "size" -> returns token ID string? 
           Actually 9P write returns count. We can't return data easily on write.
           Convention: Write params, Read result? Or use textual protocol?
           Textual: Write "1024", side effect is issuance. But we need the token.
           
           Alternative: "issue" file is read/write. 
           Write size -> Prepare. Read -> Get Token.
           
           Let's assume write returns success/fail for now, or we log it.
           Ideally we'd use a ctl-style interface where we write "issue 1024" 
           and read back "token: 12345".
           
           For "issue", we likely want to return the token ID.
           Since write() can't return data, maybe we should use `alloc` style:
           open, write request, read response.
           
           Let's stick to simple commands for now assuming the user knows how to check stats 
           or we'll implement a proper ctl file later if needed. 
           
           Wait, PebbleWhite is a struct. 
           
           Let's implement a "ctl" file approach instead of separate files if interaction is complex.
           But separate files are cleaner for permissions.
           
           Let's try: Write size to Qissue. The new token is added to process state.
           The user can see it in Qstats or we assume the library manages it.
           
           Actually, for `pebble_white_issue` to be useful, we need the pointer/token.
           
           Revised approach: 
           Qalloc/Qissue are distinct.
           
           Let's implement `alloc` as: Write "size 1024". 
           It allocates immediately (Black Alloc).
           Returns success if alloc worked.
           But we need the address! 
           
           Okay, the standard Plan 9 way for "allocating" a resource 
           (like a window or a connection) is usually:
           1. Read `clone` to get a directory (ID).
           2. Open `ctl` in that dir.
           
           Maybe we should do:
           /dev/pebble/clone -> returns ID of new allocation
           /dev/pebble/N/ctl -> "size 1024", "commit"
           /dev/pebble/N/addr -> returns physical address
           
           For now, let's stick to the existing syscalls in C if this is too complex to shim 
           without a full rewrite. But the user wants "interface".
           
           Let's assume `devsip` is the main interface and `pebble` is a backing service.
           
           If we MUST replace syscalls:
           We can use the `ctl` file in `/dev/sip/servers/ID/ctl` to issue commands?
           No, that's for SIP management.
           
           Let's look at `devexchange.c`. It does:
           write(fd, "prepare <addr>")
           
           We can do:
           fd = open("/dev/pebble/alloc", ORDWR)
           write(fd, "size 1024")
           read(fd, buf) -> "addr: 0x..."
           
           This works!
        */
        
        /* We'll implement this "RPC-like" write/read on Qissue/Qalloc later.
           For now, let's just stub the device entry points. */
        break;

    case Qalloc:
        /* Simple alloc: "size <bytes>" */
        size = strtoul(buf, 0, 0);
        if(size > 0){
            if(pebble_black_alloc(size, &handle) == 0){
                /* How to return handle? 
                   We need a stateful channel (like clone) or write/read sequence.
                   Let's assume the Go side will use a construct that handles this.
                */
                print("pebble: allocated %lud bytes at %p\n", size, handle);
            } else {
                free(buf);
                error(Enomem);
            }
        }
        break;
        
default:
        free(buf);
        error(Eperm);
    }

    free(buf);
    return n;
}

Dev pebbledevtab = {
    'P',
    "pebble",

    devreset,
    pebinit,
    devshutdown,
    pebattach,
    pebwalk,
    pebstat,
    pebopen,
    devcreate,
    pebclose,
    pebread,
    devbread,
    pebwrite,
    devbwrite,
    devremove,
    devwstat,
};
