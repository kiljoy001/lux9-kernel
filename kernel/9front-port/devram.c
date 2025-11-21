#include	"u.h"
#include "portlib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include <error.h>

enum
{
	Qdir = 0,
	Qram,
};

static Dirtab ramdir[] = {
	".",		{Qdir, 0, QTDIR},	0,	DMDIR|0555,
	"ram",		{Qram},			0,	0666,
};

/* Ramdisk data - simple in-memory buffer */
static uchar *ramdisk_data;
static ulong ramdisk_size = 64*1024*1024; /* 64MB default */

static void
ramreset(void)
{
	/* Allocate ramdisk memory */
	ramdisk_data = xalloc(ramdisk_size);
	if(ramdisk_data == nil)
		panic("ramdisk: cannot allocate memory");
	
	/* Initialize to zero */
	memset(ramdisk_data, 0, ramdisk_size);
	
	print("ramdisk: %lud MB allocated at %p\n", ramdisk_size/(1024*1024), ramdisk_data);
}

static void
raminit(void)
{
	/* Nothing to do */
}

static Chan*
ramattach(char *spec)
{
	return devattach('r', spec);
}

static Walkqid*
ramwalk(Chan *c, Chan *nc, char **name, int nname)
{
	return devwalk(c, nc, name, nname, ramdir, nelem(ramdir), devgen);
}

static int
ramstat(Chan *c, uchar *dp, int n)
{
	return devstat(c, dp, n, ramdir, nelem(ramdir), devgen);
}

static Chan*
ramopen(Chan *c, int omode)
{
	c = devopen(c, omode, ramdir, nelem(ramdir), devgen);
	c->offset = 0;
	return c;
}

static void
ramclose(Chan *c)
{
	USED(c);
}

static long
ramread(Chan *c, void *va, long n, vlong off)
{
	switch((ulong)c->qid.path){
	case Qdir:
		return devdirread(c, va, n, ramdir, nelem(ramdir), devgen);

	case Qram:
		/* Bounds checking */
		if(off < 0)
			error(Ebadarg);
		if(off >= ramdisk_size)
			return 0;
		if(off + n > ramdisk_size)
			n = ramdisk_size - off;
		
		/* Copy data from ramdisk */
		memmove(va, ramdisk_data + off, n);
		return n;

	default:
		error(Egreg);
		return 0;
	}
}

static long
ramwrite(Chan *c, void *va, long n, vlong off)
{
	switch((ulong)c->qid.path){
	case Qdir:
		error(Eperm);
		return 0;

	case Qram:
		/* Bounds checking */
		if(off < 0)
			error(Ebadarg);
		if(off >= ramdisk_size)
			error(Eio);
		if(off + n > ramdisk_size)
			n = ramdisk_size - off;
		
		/* Copy data to ramdisk */
		memmove(ramdisk_data + off, va, n);
		return n;

	default:
		error(Egreg);
		return 0;
	}
}

Dev ramdevtab = {
	'r',
	"ram",

	ramreset,
	devinit,
	devshutdown,
	ramattach,
	ramwalk,
	ramstat,
	ramopen,
	devcreate,
	ramclose,
	ramread,
	devbread,
	ramwrite,
	devbwrite,
	devremove,
	devwstat,
};