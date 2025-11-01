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

	if(u->nefile >= nelem(u->efile))
		return -1;

	f = &u->efile[u->nefile++];
	f->name = name;
	f->perm = perm;
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
	USED(name, ifc);
	/* Simple stub - return non-nil to indicate success */
	return (void*)1;
}

/*
 * sdadddevs - Add SD devices to the system
 */
void
sdadddevs(SDev *sdev)
{
	USED(sdev);
	/* Device registration stub */
	/* In full implementation, this would register with devsd */
}

/*
 * LED functions - stubs for drive activity LEDs
 */
void
ledr(void *p, int on)
{
	USED(p, on);
	/* LED read activity - not implemented */
}

void
ledw(void *p, int on)
{
	USED(p, on);
	/* LED write activity - not implemented */
}
