/*
 * Exchange device - 9P interface for page exchange operations
 * Provides Singularity-style exchange heap semantics at page granularity
 */

#include "u.h"
#include "portlib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "pageown.h"
#include "exchange.h"
#include <error.h>

enum {
	Qtopdir		= 0,	/* top level directory */
	Qexchange	= 1,	/* exchange control interface */
	Qstat		= 2,	/* statistics */
};

typedef struct Exchctl Exchctl;
struct Exchctl {
	QLock qlock;

	/* Prepared pages tracking */
	struct {
		ExchangeHandle handle;
		uintptr original_vaddr;
		Proc *owner;
		ulong time;	/* time when prepared */
	} prepared[1024];
	int nprepared;
};

static Exchctl exchctl;

typedef struct Dirtab Dirtab;
Dirtab exchdir[] = {
	".",		{Qtopdir, 0, QTDIR},	0,		DMDIR|0555,
	"exchange",	{Qexchange, 0},		0,		0666,
	"stat",		{Qstat, 0},		0,		0444,
};

static void
exchinit(void)
{
	/* Initialize exchange control structure */
	exchctl.nprepared = 0;
	print("exchange: 9P device initialized\n");
}

static Chan*
exchattach(char *spec)
{
	Chan *c;

	c = devattach(L'X', spec);
	mkqid(&c->qid, Qtopdir, 0, QTDIR);
	c->dev = 0;
	return c;
}

static Walkqid*
exchwalk(Chan *c, Chan *nc, char **name, int nname)
{
	return devwalk(c, nc, name, nname, exchdir, nelem(exchdir), devgen);
}

static int
exchstat(Chan *c, uchar *dp, int n)
{
	return devstat(c, dp, n, exchdir, nelem(exchdir), devgen);
}

static Chan*
exchopen(Chan *c, int omode)
{
	omode &= 3;	/* mask off exec bit */
	switch((int)c->qid.path){
	case Qtopdir:
		if(omode != OREAD)
			error(Eperm);
		break;
	case Qexchange:
	case Qstat:
		/* Files can be opened for read or write */
		break;
	}
	c = devopen(c, omode, exchdir, nelem(exchdir), devgen);
	return c;
}

static void
exchcreate(Chan *c, char *name, int omode, ulong perm)
{
	(void)c;
	(void)name;
	(void)omode;
	(void)perm;
	error(Eperm);
}

static void
exchclose(Chan *c)
{
	(void)c;
	/* Nothing to do */
}

static long
exchread(Chan *c, void *buf, long n, vlong off)
{
	char *p, *e;
	int i;

	switch((int)c->qid.path){
	case Qtopdir:
		return devdirread(c, buf, n, exchdir, nelem(exchdir), devgen);

	case Qexchange:
		/* Return information about prepared exchanges */
		p = smalloc(4096);
		if(p == nil)
			error(Enomem);
		e = p + 4096;
		seprint(p, e, "Page Exchange System\n");
		seprint(p+strlen(p), e, "Prepared pages: %d\n", exchctl.nprepared);
		seprint(p+strlen(p), e, "Owner PID   Handle           Original VAddr\n");
		seprint(p+strlen(p), e, "----------  ---------------  ---------------\n");
		qlock(&exchctl);
		for(i = 0; i < exchctl.nprepared; i++){
			seprint(p+strlen(p), e, "%-10d  0x%016llux  0x%016llux\n",
				exchctl.prepared[i].owner ? exchctl.prepared[i].owner->pid : -1,
				(uvlong)exchctl.prepared[i].handle,
				(uvlong)exchctl.prepared[i].original_vaddr);
		}
		qunlock(&exchctl);
		n = readstr(off, buf, n, p);
		free(p);
		return n;

	case Qstat:
		/* Return statistics */
		p = smalloc(1024);
		if(p == nil)
			error(Enomem);
		pageown_stats();
		seprint(p, p+1024, "Exchange device statistics\n");
		seprint(p+strlen(p), p+1024, "Total prepared: %d\n", exchctl.nprepared);
		n = readstr(off, buf, n, p);
		free(p);
		return n;
	}

	return 0;
}

static long
exchwrite(Chan *c, void *vp, long n, vlong off)
{
	char *buf;
	char *fields[4];
	int nf;
	uintptr vaddr;
	ExchangeHandle handle;
	int result;

	(void)off;

	buf = smalloc(n+1);
	if(buf == nil)
		error(Enomem);
	memmove(buf, vp, n);
	buf[n] = 0;

	switch((int)c->qid.path){
	case Qexchange:
		/* Parse command: prepare <vaddr> or accept <handle> <dest_vaddr> <prot> or cancel <handle> */
		if(strncmp(buf, "prepare ", 8) == 0){
			/* prepare <vaddr> */
			vaddr = strtoul(buf+8, nil, 0);
			if(vaddr == 0 || (vaddr & (BY2PG-1)) != 0){
				free(buf);
				error("invalid virtual address");
			}
			
			/* Prepare the page for exchange */
			handle = exchange_prepare(vaddr);
			if(handle == 0){
				free(buf);
				error("exchange_prepare failed");
			}
			
			/* Track the prepared page */
			qlock(&exchctl);
			if(exchctl.nprepared < nelem(exchctl.prepared)){
				exchctl.prepared[exchctl.nprepared].handle = handle;
				exchctl.prepared[exchctl.nprepared].original_vaddr = vaddr;
				exchctl.prepared[exchctl.nprepared].owner = up;
				exchctl.prepared[exchctl.nprepared].time = m->ticks;
				exchctl.nprepared++;
			}
			qunlock(&exchctl);
			
			free(buf);
			return n;
		}
		else if(strncmp(buf, "accept ", 7) == 0){
			/* accept <handle> <dest_vaddr> <prot> */
			char *p = buf+7;
			handle = strtoul(p, &p, 0);
			while(*p == ' ') p++;
			uintptr dest_vaddr = strtoul(p, &p, 0);
			while(*p == ' ') p++;
			int prot = strtol(p, nil, 0);
			
			if(handle == 0 || (dest_vaddr & (BY2PG-1)) != 0){
				free(buf);
				error("invalid parameters");
			}
			
			result = exchange_accept(handle, dest_vaddr, prot);
			if(result != EXCHANGE_OK){
				free(buf);
				error("exchange_accept failed");
			}
			
			/* Remove from prepared tracking */
			qlock(&exchctl);
			for(int i = 0; i < exchctl.nprepared; i++){
				if(exchctl.prepared[i].handle == handle){
					/* Shift remaining entries down */
					for(int j = i; j < exchctl.nprepared - 1; j++){
						exchctl.prepared[j] = exchctl.prepared[j+1];
					}
					exchctl.nprepared--;
					break;
				}
			}
			qunlock(&exchctl);
			
			free(buf);
			return n;
		}
		else if(strncmp(buf, "cancel ", 7) == 0){
			/* cancel <handle> */
			handle = strtoul(buf+7, nil, 0);
			if(handle == 0){
				free(buf);
				error("invalid handle");
			}
			
			result = exchange_cancel(handle);
			if(result != EXCHANGE_OK){
				free(buf);
				error("exchange_cancel failed");
			}
			
			/* Remove from prepared tracking */
			qlock(&exchctl);
			for(int i = 0; i < exchctl.nprepared; i++){
				if(exchctl.prepared[i].handle == handle){
					/* Shift remaining entries down */
					for(int j = i; j < exchctl.nprepared - 1; j++){
						exchctl.prepared[j] = exchctl.prepared[j+1];
					}
					exchctl.nprepared--;
					break;
				}
			}
			qunlock(&exchctl);
			
			free(buf);
			return n;
		}
		else{
			free(buf);
			error("unknown command");
		}
		
	default:
		free(buf);
		error(Eperm);
	}

	free(buf);
	return 0;
}

static void
exchremove(Chan *c)
{
	(void)c;
	error(Eperm);
}

static int
exchwstat(Chan *c, uchar *dp, int n)
{
	(void)c;
	(void)dp;
	(void)n;
	error(Eperm);
	return 0;
}

static void exchreset(void);

Dev exchdevtab = {
	.dc = L'X',
	.name = "exchange",
	.reset = exchreset,
	.init = exchinit,
	.shutdown = nil,
	.attach = exchattach,
	.walk = exchwalk,
	.stat = exchstat,
	.open = exchopen,
	.create = exchcreate,
	.close = exchclose,
	.read = exchread,
	.bread = devbread,
	.write = exchwrite,
	.bwrite = devbwrite,
	.remove = exchremove,
	.wstat = exchwstat,
	.power = nil,
	.config = nil,
};

static void
exchreset(void)
{
	/* Nothing to prime yet; hook exists to satisfy chandevreset(). */
}
