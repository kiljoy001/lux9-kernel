#include	"u.h"
#include "portlib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include <error.h>
#include "pebble.h"

extern char* getconf(char*);

enum
{
	Qdir = 0,
	Qram,
	Qsecureram,
};

static Dirtab ramdir[] = {
	".",		{Qdir, 0, QTDIR},	0,	DMDIR|0555,
	"ram",		{Qram},			0,	0666,
	"secureram",	{Qsecureram},		0,	0600,
};

/* Ramdisk data - simple in-memory buffer */
static uchar *ramdisk_data;
static ulong ramdisk_size = 64*1024*1024; /* 64MB default */

/* Secure Ramdisk Structure */
typedef struct SecureRamdisk {
	uchar	*data;
	ulong	size;
	int	encrypt;
	uchar	key[32];
	void	*pebble_handle;
} SecureRamdisk;

static SecureRamdisk secure_rd;

/* Simple XOR encryption helper */
static void
secure_crypt(uchar *buf, long n, vlong off, uchar *key)
{
	long i;
	for(i = 0; i < n; i++){
		buf[i] ^= key[(off + i) % 32];
	}
}

static void
ramreset(void)
{
	char *conf;
	void *handle;
	PebbleBlack *pb;
	int i;

	/* 1. Setup Standard Ramdisk */
	ramdisk_data = xalloc(ramdisk_size);
	if(ramdisk_data == nil)
		panic("ramdisk: cannot allocate memory");
	
	memset(ramdisk_data, 0, ramdisk_size);
	print("ramdisk: %lud MB allocated at %p\n", ramdisk_size/(1024*1024), ramdisk_data);

	/* 2. Setup Secure Ramdisk */
	secure_rd.size = 0;
	secure_rd.data = nil;
	secure_rd.encrypt = 1; /* Default to encrypted */

	/* Check kernel config for secure ramdisk size */
	if((conf = getconf("secure.ramdisk.size")) != nil)
		secure_rd.size = strtoul(conf, 0, 0);
	else {
		/* Default to 64MB if not specified, to ensure feature is active */
		secure_rd.size = 64*1024*1024;
	}
	
	/* Parse size suffix (M/G) if needed, but strtoul usually just takes number. 
	   Assuming bytes or we might need simple parsing. 
	   For now assuming standard byte input or relying on strtoul handling basic numbers. 
	   Actually, typical Plan 9 getconf strings might be "64M". strtoul stops at 'M'.
	   Let's add basic suffix handling. */
	if(conf != nil) {
		char *p = conf;
		while(*p >= '0' && *p <= '9') p++;
		if(*p == 'M' || *p == 'm') secure_rd.size *= 1024*1024;
		else if(*p == 'G' || *p == 'g') secure_rd.size *= 1024*1024*1024;
		else if(*p == 'K' || *p == 'k') secure_rd.size *= 1024;
	}

	if(secure_rd.size > 0){
		/* Allocation using xalloc directly to avoid early-boot permission issues with Pebble */
		secure_rd.data = xalloc(secure_rd.size);
		if(secure_rd.data == nil)
			secure_rd.size = 0;
	}

	/* Initialize Encryption Key */
	if(secure_rd.size > 0){
		for(i = 0; i < 32; i++)
			secure_rd.key[i] = nrand(256);
		
		/* Zero out the secure memory initially */
		memset(secure_rd.data, 0, secure_rd.size);
	}
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

	case Qsecureram:
		if(secure_rd.data == nil)
			error(Eio); /* Not initialized */

		/* Bounds checking */
		if(off < 0)
			error(Ebadarg);
		if(off >= secure_rd.size)
			return 0;
		if(off + n > secure_rd.size)
			n = secure_rd.size - off;
		
		/* Copy data from secure ramdisk */
		memmove(va, secure_rd.data + off, n);

		/* Decrypt in place (in the user buffer va) if encryption enabled */
		if(secure_rd.encrypt){
			secure_crypt(va, n, off, secure_rd.key);
		}
		return n;

	default:
		error(Egreg);
		return 0;
	}
}

static long
ramwrite(Chan *c, void *va, long n, vlong off)
{
	uchar *tmpbuf;

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

	case Qsecureram:
		if(secure_rd.data == nil)
			error(Eio);

		/* Bounds checking */
		if(off < 0)
			error(Ebadarg);
		if(off >= secure_rd.size)
			error(Eio);
		if(off + n > secure_rd.size)
			n = secure_rd.size - off;

		/* For write, we need to encrypt BEFORE writing to secure memory.
		 * We shouldn't modify 'va' in place as it belongs to caller/user.
		 * So we need a temp buffer. */
		
		/* Direct write if no encryption */
		if(!secure_rd.encrypt){
			memmove(secure_rd.data + off, va, n);
			return n;
		}

		/* Encrypt via temp buffer */
		tmpbuf = smalloc(n);
		if(tmpbuf == nil)
			error(Enomem);
		
		memmove(tmpbuf, va, n);
		secure_crypt(tmpbuf, n, off, secure_rd.key);
		memmove(secure_rd.data + off, tmpbuf, n);
		free(tmpbuf);
		
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