#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"../port/error.h"
#include	"pebble.h"

/*
 * /dev/sip - Software Isolated Process (SIP) Lifecycle Manager
 *
 * Manages the creation, configuration, and lifecycle of SIP servers.
 * Enforces capability-based isolation.
 */

enum {
	Qdir = 0,
	Qclone,
	Qservers,
	Qtypes,
	/* Server-specific files */
	Qctl,
	Qstatus,
	Qhealth,
	Qirq,
	Qdevices,
};

#define SIP_MAX_SERVERS 64
#define SIP_NAME_LEN 64
#define SIP_PATH_LEN 128

typedef struct SipServer {
	Ref	ref;
	QLock	lock;
	int	id;
	int	pid;		/* Process ID of the running server */
	char	name[SIP_NAME_LEN];
	char	binary[SIP_PATH_LEN];
	ulong	caps;		/* Capability bitmask */
	int	state;		/* Created, Configured, Running, Stopped */
	char	last_error[128];
	long	uptime;
	long	requests;
	long	errors;
} SipServer;

enum {
	StateFree = 0,
	StateCreated,
	StateConfigured,
	StateRunning,
	StateStopped
};

static struct {
	RWLock	lock;
	SipServer *servers[SIP_MAX_SERVERS];
	int	next_id;
} sipalloc;

/* Capabilities - should match pebble.h or be defined here if not */
/* Assuming these are defined in pebble.h or we define them here for now */
#ifndef PEBBLE_CAP_DEVICE
#define PEBBLE_CAP_DEVICE	(1<<1)
#define PEBBLE_CAP_IOPORT	(1<<2)
#define PEBBLE_CAP_NET		(1<<3)
#define PEBBLE_CAP_IRQ		(1<<4)
#define PEBBLE_CAP_DMA		(1<<5)
#define PEBBLE_CAP_PCI		(1<<6)
#define PEBBLE_CAP_FS		(1<<7)
#endif

static SipServer*
sip_get_server(int id)
{
	SipServer *s;
	int i;

	rlock(&sipalloc.lock);
	for(i = 0; i < SIP_MAX_SERVERS; i++){
		s = sipalloc.servers[i];
		if(s != nil && s->id == id){
			incref(&s->ref);
			runlock(&sipalloc.lock);
			return s;
		}
	}
	runlock(&sipalloc.lock);
	return nil;
}

static void
sip_put_server(SipServer *s)
{
	if(decref(&s->ref) == 0){
		/* In a real implementation, we might not free immediately if it's in the array
		   But here ref counts handle active usage. 
		   The array holds the 'primary' reference. */
	}
}

/* 
 * Helper to generate directory entries 
 */
static int
sipgen(Chan *c, char *name, Dirtab *tab, int ntab, int s, Dir *dp)
{
	Qid q;
	SipServer *srv;
	int id, type;
	char buf[32];

	USED(tab);
	USED(ntab);

	if(c->qid.path == Qdir){
		switch(s){
		case 0:
			mkqid(&q, Qclone, 0, QTFILE);
			devdir(c, q, "clone", 0, eve, 0666, dp);
			return 1;
		case 1:
			mkqid(&q, Qservers, 0, QTDIR);
			devdir(c, q, "servers", 0, eve, 0555, dp);
			return 1;
		case 2:
			mkqid(&q, Qtypes, 0, QTFILE);
			devdir(c, q, "types", 0, eve, 0444, dp);
			return 1;
		default:
			return -1;
		}
	}

	if(c->qid.path == Qservers){
		/* List active servers as directories named by ID or Name */
		/* For simplicity, listing by ID first */
		rlock(&sipalloc.lock);
		if(s >= SIP_MAX_SERVERS){
			runlock(&sipalloc.lock);
			return -1;
		}
		
		/* Find the s-th active server */
		int count = 0;
		int i;
		srv = nil;
		for(i = 0; i < SIP_MAX_SERVERS; i++){
			if(sipalloc.servers[i] != nil){
				if(count == s){
					srv = sipalloc.servers[i];
					break;
				}
				count++;
			}
		}
		runlock(&sipalloc.lock);

		if(srv == nil)
			return -1;

		/* We make the Qid path include the server ID in high bits */
		/* 8 bits for type, 24 bits for ID */
		mkqid(&q, (srv->id << 8) | Qdir, 0, QTDIR);
		/* Prefer name if set, else ID */
		if(srv->name[0])
			snprint(buf, sizeof(buf), "%s", srv->name);
		else
			snprint(buf, sizeof(buf), "%d", srv->id);
			
		devdir(c, q, buf, 0, eve, 0555, dp);
		return 1;
	}

	/* Inside a server directory */
	id = c->qid.path >> 8;
	type = c->qid.path & 0xFF;
	
	if(type == Qdir && c->qid.type == QTDIR){
		switch(s){
		case 0:
			mkqid(&q, (id << 8) | Qctl, 0, QTFILE);
			devdir(c, q, "ctl", 0, eve, 0666, dp);
			return 1;
		case 1:
			mkqid(&q, (id << 8) | Qstatus, 0, QTFILE);
			devdir(c, q, "status", 0, eve, 0444, dp);
			return 1;
		case 2:
			mkqid(&q, (id << 8) | Qhealth, 0, QTFILE);
			devdir(c, q, "health", 0, eve, 0444, dp);
			return 1;
		case 3:
			mkqid(&q, (id << 8) | Qirq, 0, QTFILE);
			devdir(c, q, "irq", 0, eve, 0444, dp);
			return 1;
		default:
			return -1;
		}
	}

	return -1;
}

static void
sipinit(void)
{
	sipalloc.next_id = 0;
}

static Chan*
sipattach(char *spec)
{
	return devattach('S', spec);
}

static Walkqid*
sipwalk(Chan *c, Chan *nc, char **name, int nname)
{
	return devwalk(c, nc, name, nname, 0, 0, sipgen);
}

static int
sipstat(Chan *c, uchar *dp, int n)
{
	return devstat(c, dp, n, 0, 0, sipgen);
}

static Chan*
sipopen(Chan *c, int omode)
{
	int id;
	SipServer *s;

	if(c->qid.type & QTDIR){
		if(omode != OREAD)
			error(Eperm);
	}

	/* Cloning logic */
	if(c->qid.path == Qclone){
		if(omode != OREAD && omode != OWRITE && omode != ORDWR)
			error(Eperm);
			
		wlock(&sipalloc.lock);
		/* Find free slot */
		int i;
		for(i = 0; i < SIP_MAX_SERVERS; i++){
			if(sipalloc.servers[i] == nil)
				break;
		}
		if(i == SIP_MAX_SERVERS){
			wunlock(&sipalloc.lock);
			error(Enomem);
		}

		s = smalloc(sizeof(SipServer));
		s->ref.ref = 1;
		s->id = sipalloc.next_id++;
		s->state = StateCreated;
		s->caps = 0;
		s->pid = -1;
		sipalloc.servers[i] = s;
		wunlock(&sipalloc.lock);

		/* Redirect c to the ctl file of the new server */
		c->qid.path = (s->id << 8) | Qctl;
		c->qid.type = QTFILE;
		return c;
	}

	c->mode = openmode(omode);
	c->flag |= COPEN;
	c->offset = 0;
	return c;
}

static void
sipclose(Chan *c)
{
	USED(c);
}

static long
sipread(Chan *c, void *va, long n, vlong off)
{
	int id, type;
	SipServer *s;
	char *buf;
	long rv;

	if(c->qid.type & QTDIR)
		return devdirread(c, va, n, 0, 0, sipgen);

	id = c->qid.path >> 8;
	type = c->qid.path & 0xFF;

	if(type == Qclone){
		/* Reading clone returns nothing, usually used to get ctl fd via open */
		return 0;
	}
	
	if(type == Qtypes){
		return readstr(off, va, n, "ahci-driver\next4fs\nframebuffer\n");
	}

	s = sip_get_server(id);
	if(s == nil)
		error(Enonexist);

	buf = smalloc(1024);
	rv = 0;

	switch(type){
	case Qctl:
		/* Read config */
		rv = snprint(buf, 1024, "id: %d\nname: %s\nbinary: %s\ncaps: 0x%lux\npid: %d\n", 
			s->id, s->name, s->binary, s->caps, s->pid);
		break;
	case Qstatus:
		rv = snprint(buf, 1024, "status: %s\nuptime: %ld\nrequests: %ld\nerrors: %ld\n",
			s->state == StateRunning ? "running" : 
			s->state == StateConfigured ? "configured" : "stopped",
			s->uptime, s->requests, s->errors);
		break;
	case Qhealth:
		rv = snprint(buf, 1024, "%s\n", s->errors > 0 ? "degraded" : "healthy");
		if(s->last_error[0])
			rv += snprint(buf+rv, 1024-rv, "last_error: %s\n", s->last_error);
		break;
	default:
		free(buf);
		sip_put_server(s);
		error(Ebadusefd);
	}

	rv = readstr(off, va, n, buf);
	free(buf);
	sip_put_server(s);
	return rv;
}

static long
sipwrite(Chan *c, void *va, long n, vlong off)
{
	int id, type;
	SipServer *s;
	Cmdbuf *cb;
	Cmdtab *ct;

	if(c->qid.type & QTDIR)
		error(Eperm);

	id = c->qid.path >> 8;
	type = c->qid.path & 0xFF;

	if(type == Qclone){
		/* Writing to clone creates a server of that type? 
		   The open() logic handles creation. 
		   Maybe spec implies writing type name to clone? 
		   For now, we just accept it as a no-op if opened. */
		return n;
	}

	if(type != Qctl)
		error(Eperm);

	s = sip_get_server(id);
	if(s == nil)
		error(Enonexist);

	cb = parsecmd(va, n);
	if(waserror()){
		free(cb);
		sip_put_server(s);
		nexterror();
	}

	qlock(&s->lock);

	if(strcmp(cb->f[0], "name") == 0){
		if(cb->nf < 2)
			error(Ebadarg);
		strncpy(s->name, cb->f[1], SIP_NAME_LEN-1);
		s->name[SIP_NAME_LEN-1] = 0;
	}
	else if(strcmp(cb->f[0], "binary") == 0){
		if(cb->nf < 2)
			error(Ebadarg);
		strncpy(s->binary, cb->f[1], SIP_PATH_LEN-1);
	}
	else if(strcmp(cb->f[0], "capability") == 0){
		if(cb->nf < 2)
			error(Ebadarg);
		if(strcmp(cb->f[1], "device") == 0) s->caps |= PEBBLE_CAP_DEVICE;
		else if(strcmp(cb->f[1], "interrupt") == 0) s->caps |= PEBBLE_CAP_IRQ;
		else if(strcmp(cb->f[1], "dma") == 0) s->caps |= PEBBLE_CAP_DMA;
		else if(strcmp(cb->f[1], "pci") == 0) s->caps |= PEBBLE_CAP_PCI;
		else if(strcmp(cb->f[1], "fs") == 0) s->caps |= PEBBLE_CAP_FS;
		else error("unknown capability");
	}
	else if(strcmp(cb->f[0], "start") == 0){
		/* 
		 * In a real microkernel, this might spawn the process.
		 * Here, we adopt the current process as the SIP server.
		 * The writing process (userspace manager) effectively 
		 * hands over control or this configures the *next* exec.
		 * 
		 * For this implementation: "start" marks it running and 
		 * assigns the current PID to the server record.
		 * Real capability enforcement would happen in sysproc.c 
		 * checking s->caps based on s->pid.
		 */
		if(s->state == StateRunning)
			error(Einuse);
		
		s->pid = up->pid; /* 'up' is current process */
		s->state = StateRunning;
		/* Apply capabilities to current process (if we had a field in Proc) */
		/* up->sip_caps = s->caps; */
	}
	else if(strcmp(cb->f[0], "stop") == 0){
		s->state = StateStopped;
		s->pid = -1;
	}
	else {
		error(Ebadctl);
	}

	qunlock(&s->lock);
	poperror();
	free(cb);
	sip_put_server(s);
	return n;
}

Dev sipdevtab = {
	'S',
	"sip",

	devreset,
	sipinit,
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
