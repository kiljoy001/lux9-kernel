/*
 * /dev/irq - Interrupt delivery device for SIP drivers
 *
 * Provides 9P-based interrupt delivery to userspace device drivers.
 * Integrates with the existing Vctl interrupt infrastructure.
 *
 * Filesystem:
 *   /dev/irq/
 *   ├── ctl        - Control file for registration
 *   ├── 0          - IRQ 0 events (blocking read)
 *   ├── 1          - IRQ 1 events
 *   ...
 *   └── 255        - IRQ 255 events
 *
 * Usage:
 *   // Register for IRQ
 *   echo "register 11 ahci-driver" > /dev/irq/ctl
 *
 *   // Wait for interrupt (blocks)
 *   read(/dev/irq/11, buf, sizeof(buf))
 *
 * Architecture Integration:
 *   - SIP servers declare CapInterrupt capability
 *   - Kernel checks capability at open time
 *   - IRQ handler wakes up blocked readers
 *   - Uses existing Vctl infrastructure (intrenable/trapenable)
 */

#include	"u.h"
#include "portlib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"io.h"
#include <error.h>

enum
{
	Qdir = 0,
	Qctl,
	Qirqbase = 16,		/* IRQ files start at qid 16 */

	MaxIRQ = 256,
	MaxPending = 64,	/* Max queued interrupts per IRQ */
};

/*
 * Per-IRQ state for userspace delivery
 */
typedef struct IrqState IrqState;
struct IrqState
{
	Lock		lock;
	Rendez		rendez;		/* Wait queue for blocked readers */

	int		irq;		/* IRQ number */
	int		registered;	/* Has a userspace handler */
	Proc		*owner;		/* Process that registered */
	char		name[KNAMELEN];	/* Handler name (e.g., "ahci-driver") */

	uint		pending;	/* Count of pending interrupts */
	uint		delivered;	/* Total delivered */
	uint		dropped;	/* Dropped due to overflow */
};

static IrqState irqstate[MaxIRQ];

/*
 * Capability checking
 */
enum
{
	CapInterrupt = 1<<4,	/* Must match SIP capability */
};

static void
checkcap(ulong required)
{
	/*
	 * TODO: Check up->capabilities & required
	 * For now, allow all access for development
	 */
	USED(required);
}

/*
 * IRQ handler called from kernel interrupt dispatch
 * This is registered via intrenable() and called at interrupt time
 */
static void
irq_userspace_handler(Ureg *ureg, void *arg)
{
	IrqState *is = arg;

	USED(ureg);

	ilock(&is->lock);

	if(!is->registered) {
		iunlock(&is->lock);
		return;
	}

	/* Queue the interrupt */
	if(is->pending < MaxPending)
		is->pending++;
	else
		is->dropped++;

	iunlock(&is->lock);

	/* Wake up blocked readers */
	wakeup(&is->rendez);
}

/*
 * Check if interrupt is available for read
 */
static int
irq_available(void *arg)
{
	IrqState *is = arg;
	return is->pending > 0;
}

/*
 * Generate directory entry for IRQ file
 */
static int
irqgen(Chan *c, char *name, Dirtab *tab, int ntab, int pos, Dir *dp)
{
	Qid qid;
	char buf[32];
	int irq;

	USED(c, tab, ntab);

	if(pos == 0) {
		/* "." */
		devdir(c, (Qid){Qdir, 0, QTDIR}, ".", 0, eve, 0555, dp);
		return 1;
	}
	pos--;

	if(pos == 0) {
		/* "ctl" */
		devdir(c, (Qid){Qctl, 0, 0}, "ctl", 0, eve, 0666, dp);
		return 1;
	}
	pos--;

	/* IRQ files: 0..255 */
	irq = pos;
	if(irq >= MaxIRQ)
		return -1;

	snprint(buf, sizeof(buf), "%d", irq);
	qid.path = Qirqbase + irq;
	qid.vers = 0;
	qid.type = 0;
	devdir(c, qid, buf, 0, eve, 0400, dp);
	return 1;
}

static void
irqreset(void)
{
	int i;

	/* Initialize IRQ state */
	for(i = 0; i < MaxIRQ; i++) {
		irqstate[i].irq = i;
		irqstate[i].registered = 0;
		irqstate[i].owner = nil;
		irqstate[i].pending = 0;
		irqstate[i].delivered = 0;
		irqstate[i].dropped = 0;
		irqstate[i].name[0] = '\0';
	}
}

static Chan*
irqattach(char *spec)
{
	return devattach('I', spec);
}

static Walkqid*
irqwalk(Chan *c, Chan *nc, char **name, int nname)
{
	return devwalk(c, nc, name, nname, nil, 0, irqgen);
}

static int
irqstat(Chan *c, uchar *dp, int n)
{
	return devstat(c, dp, n, nil, 0, irqgen);
}

static Chan*
irqopen(Chan *c, int omode)
{
	int irq;

	switch((int)c->qid.path){
	case Qdir:
		if(omode != OREAD)
			error(Eperm);
		break;

	case Qctl:
		checkcap(CapInterrupt);
		break;

	default:
		/* IRQ file */
		if(c->qid.path < Qirqbase || c->qid.path >= Qirqbase + MaxIRQ)
			error(Egreg);

		irq = c->qid.path - Qirqbase;

		/* Check if registered */
		if(!irqstate[irq].registered)
			error("IRQ not registered");

		/* Only owner can read */
		if(irqstate[irq].owner != up)
			error(Eperm);

		if(omode != OREAD)
			error(Eperm);
		break;
	}

	c->mode = openmode(omode);
	c->flag |= COPEN;
	c->offset = 0;
	return c;
}

static void
irqclose(Chan *c)
{
	USED(c);
}

static long
irqread(Chan *c, void *va, long n, vlong off)
{
	IrqState *is;
	int irq;
	char buf[256];
	int len, i;

	switch((int)c->qid.path){
	case Qdir:
		return devdirread(c, va, n, nil, 0, irqgen);

	case Qctl:
		/* Return status of all registered IRQs */
		len = 0;
		for(i = 0; i < MaxIRQ; i++) {
			if(irqstate[i].registered) {
				len += snprint(buf + len, sizeof(buf) - len,
					"irq %d: %s pending=%ud delivered=%ud dropped=%ud\n",
					i, irqstate[i].name,
					irqstate[i].pending,
					irqstate[i].delivered,
					irqstate[i].dropped);
			}
		}

		if(off >= len)
			return 0;
		if(off + n > len)
			n = len - off;
		memmove(va, buf + off, n);
		return n;

	default:
		/* IRQ file - block until interrupt */
		if(c->qid.path < Qirqbase || c->qid.path >= Qirqbase + MaxIRQ)
			error(Egreg);

		irq = c->qid.path - Qirqbase;
		is = &irqstate[irq];

		if(!is->registered)
			error("IRQ not registered");

		if(is->owner != up)
			error(Eperm);

		/* Wait for interrupt */
		ilock(&is->lock);
		while(is->pending == 0) {
			iunlock(&is->lock);
			sleep(&is->rendez, irq_available, is);
			ilock(&is->lock);
		}

		/* Consume one interrupt */
		is->pending--;
		is->delivered++;
		iunlock(&is->lock);

		/* Return a simple notification */
		len = snprint(buf, sizeof(buf), "irq %d\n", irq);
		if(n > len)
			n = len;
		memmove(va, buf, n);
		return n;
	}
}

static long
irqwrite(Chan *c, void *va, long n, vlong off)
{
	char *buf, *fields[4];
	int nf, irq;
	IrqState *is;

	USED(off);

	switch((int)c->qid.path){
	case Qdir:
		error(Eperm);
		return 0;

	case Qctl:
		/* Parse command */
		buf = smalloc(n + 1);
		memmove(buf, va, n);
		buf[n] = '\0';

		nf = tokenize(buf, fields, nelem(fields));

		if(nf >= 3 && strcmp(fields[0], "register") == 0) {
			/* register <irq> <name> */
			irq = atoi(fields[1]);
			if(irq < 0 || irq >= MaxIRQ) {
				free(buf);
				error("invalid IRQ number");
			}

			is = &irqstate[irq];

			ilock(&is->lock);
			if(is->registered) {
				iunlock(&is->lock);
				free(buf);
				error("IRQ already registered");
			}

			/* Register the handler */
			is->registered = 1;
			is->owner = up;
			strncpy(is->name, fields[2], KNAMELEN-1);
			is->name[KNAMELEN-1] = '\0';
			is->pending = 0;
			is->delivered = 0;
			is->dropped = 0;
			iunlock(&is->lock);

			/* Hook into kernel interrupt system */
			intrenable(irq, irq_userspace_handler, is, BUSUNKNOWN, is->name);

			print("devirq: registered IRQ %d for %s (pid %d)\n",
				irq, is->name, up->pid);

		} else if(nf >= 2 && strcmp(fields[0], "unregister") == 0) {
			/* unregister <irq> */
			irq = atoi(fields[1]);
			if(irq < 0 || irq >= MaxIRQ) {
				free(buf);
				error("invalid IRQ number");
			}

			is = &irqstate[irq];

			ilock(&is->lock);
			if(!is->registered || is->owner != up) {
				iunlock(&is->lock);
				free(buf);
				error("not owner of IRQ");
			}

			is->registered = 0;
			is->owner = nil;
			iunlock(&is->lock);

			/* TODO: Call intrdisable() when it's implemented */

			print("devirq: unregistered IRQ %d\n", irq);

		} else {
			free(buf);
			error("invalid command");
		}

		free(buf);
		return n;

	default:
		error(Eperm);
		return 0;
	}
}

Dev irqdevtab = {
	'I',
	"irq",

	irqreset,
	devinit,
	devshutdown,
	irqattach,
	irqwalk,
	irqstat,
	irqopen,
	devcreate,
	irqclose,
	irqread,
	devbread,
	irqwrite,
	devbwrite,
	devremove,
	devwstat,
};
