/*
 * I/O port helper functions for SD drivers
 */

#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "io.h"
#include "../port/error.h"
#include "../port/sd.h"

static char devletters[] = "0123456789"
	"abcdefghijklmnopqrstuvwxyz"
	"ABCDEFGHIJKLMNOPQRSTUVWXYZ";
static SDev *devs[sizeof devletters - 1];
static QLock devslock;

static void
freeunit(SDunit *u)
{
	int i;

	if(u == nil)
		return;

	for(i = 0; i < u->nefile; i++){
		free(u->efile[i].name);
		u->efile[i].name = nil;
		free(u->efile[i].user);
		u->efile[i].user = nil;
	}

	for(i = 0; i < u->npart; i++){
		free(u->part[i].name);
		u->part[i].name = nil;
		free(u->part[i].user);
		u->part[i].user = nil;
	}
	free(u->part);
	u->part = nil;

	free(u->name);
	u->name = nil;
	free(u->user);
	u->user = nil;
	free(u->ctlperm.name);
	u->ctlperm.name = nil;
	free(u->ctlperm.user);
	u->ctlperm.user = nil;
	free(u->rawperm.name);
	u->rawperm.name = nil;
	free(u->rawperm.user);
	u->rawperm.user = nil;

	free(u);
}

static void
freesdev(SDev *sdev)
{
	int i;

	if(sdev == nil)
		return;

	if(sdev->unit != nil){
		for(i = 0; i < sdev->nunit; i++)
			freeunit(sdev->unit[i]);
	}
	free(sdev->unitflg);
	sdev->unitflg = nil;
	free(sdev->unit);
	sdev->unit = nil;
	free(sdev);
}

static int
sdindex(int idno)
{
	char *p;

	p = strchr(devletters, idno);
	if(p == nil)
		return -1;
	return (int)(p - devletters);
}

static int
sdparseid(char *name)
{
	if(name == nil || *name == 0)
		return -1;
	if(name[0] == 's' && name[1] == 'd' && name[2] != 0)
		return name[2];
	return name[0];
}

/*
 * inss - Input string of words (16-bit)
 */
void
inss(int port, void *buf, int len)
{
	ushort *p = buf;
	int i;

	for(i = 0; i < len; i++)
		p[i] = ins(port);
}

/*
 * outss - Output string of words (16-bit)
 */
void
outss(int port, void *buf, int len)
{
	ushort *p = buf;
	int i;

	for(i = 0; i < len; i++)
		outs(port, p[i]);
}


/*
 * sdaddfile - Add special file to SD unit
 */
int
sdaddfile(SDunit *u, char *name, int perm, char *user, SDrw *r, SDrw *w)
{
	SDfile *f;

	if(u->nefile >= (int)nelem(u->efile))
		return -1;

	f = &u->efile[u->nefile++];
	f->name = name;
	f->perm = (ulong)perm;
	f->user = user;
	f->r = r;
	f->w = w;

	return 0;
}

/*
 * sdannexctlr - Annex a controller to SD system
 */
void*
sdannexctlr(char *name, SDifc *ifc)
{
	void *ctlr;
	SDev *sdev;
	int i;

	if((i = sdindex(sdparseid(name))) < 0)
		error(Enonexist);

	qlock(&devslock);
	sdev = devs[i];
	if(sdev == nil || sdev->ifc != ifc){
		qunlock(&devslock);
		error(Enonexist);
	}
	if(sdev->r.ref != 0){
		qunlock(&devslock);
		error(Einuse);
	}
	devs[i] = nil;
	qunlock(&devslock);

	if(!sdev->enabled && ifc != nil && ifc->enable != nil)
		(*ifc->enable)(sdev);

	ctlr = sdev->ctlr;
	freesdev(sdev);
	return ctlr;
}

/*
 * sdadddevs - Add SD devices to the system
 */
void
sdadddevs(SDev *sdev)
{
	SDev *next;
	int i, j, start;
	ulong unitsz, flagsz;

	for(; sdev != nil; sdev = next){
		next = sdev->next;
		sdev->next = nil;

		unitsz = (ulong)sdev->nunit * sizeof(sdev->unit[0]);
		flagsz = (ulong)sdev->nunit * sizeof(sdev->unitflg[0]);
		if(sdev->nunit > 0){
			sdev->unit = malloc(unitsz);
			sdev->unitflg = malloc(flagsz);
			if(sdev->unit == nil || sdev->unitflg == nil){
				print("sdadddevs: out of memory\n");
				if(sdev->ifc != nil && sdev->ifc->clear != nil)
					(*sdev->ifc->clear)(sdev);
				freesdev(sdev);
				continue;
			}
			memset(sdev->unit, 0, unitsz);
			memset(sdev->unitflg, 0, flagsz);
		}else{
			sdev->unit = nil;
			sdev->unitflg = nil;
		}

		start = sdindex(sdev->idno);
		if(start < 0)
			start = 0;

		qlock(&devslock);
		for(i = 0; i < (int)nelem(devs); i++){
			j = (start + i) % (int)nelem(devs);
			if(devs[j] == nil){
				sdev->idno = devletters[j];
				devs[j] = sdev;
				snprint(sdev->name, sizeof sdev->name, "sd%c", devletters[j]);
				break;
			}
		}
		qunlock(&devslock);

		if(i == (int)nelem(devs)){
			print("sdadddevs: out of device letters\n");
			if(sdev->ifc != nil && sdev->ifc->clear != nil)
				(*sdev->ifc->clear)(sdev);
			freesdev(sdev);
		}
	}
}

/*
 * LED functions - stubs for drive activity LEDs
 */
void
ledr(void *p, int on)
{
	SDio *io;

	io = p;
	if(io == nil || io->led == nil)
		return;
	(*io->led)(io, on);
}

void
ledw(void *p, int on)
{
	SDio *io;

	io = p;
	if(io == nil || io->led == nil)
		return;
	(*io->led)(io, on);
}
