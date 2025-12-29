/*
 * Exchange device - 9P interface for page exchange operations
 * Provides Singularity-style exchange heap semantics at page granularity
 *
 * Extended with channel pool architecture:
 * #X/clone - allocate new channel
 * #X/N/ctl - channel control
 * #X/N/ring - ring buffer control page (mmap)
 * #X/N/pool - exchange page pool (read=alloc, write=free)
 * #X/N/stats - channel statistics
 * #X/N/peers - connected peers
 */

#include "u.h"
#include "portlib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "pageown.h"
#include "exchange.h"
#include "blind_ledger.h"
#include "pebble.h"
#include "uuid.h"
#include "9p_router.h"  /* For EXCHANGE_PAGE_ADDR */
#include <error.h>

enum {
	/* Top level */
	Qtopdir		= 0,	/* top level directory */
	Qclone		= 1,	/* allocate new channel */
	Qglobalctl	= 2,	/* global control/stats */

	/* Channel files (path = (chan_id << 8) | subfile) */
	Qchandir	= 3,	/* channel N directory */
	Qctl		= 4,	/* channel control */
	Qring		= 5,	/* ring buffer control page */
	Qpool		= 6,	/* exchange page pool */
	Qstats		= 7,	/* channel stats */
	Qpeers		= 8,	/* peer list */

	/* Legacy */
	Qexchange	= 9,	/* old exchange interface (deprecated) */
	Qstat		= 10,	/* old statistics */
};

#define CHANQID(id, subfile)	(((id) << 8) | (subfile))
#define CHANID(qid)		((qid) >> 8)
#define SUBFILE(qid)		((qid) & 0xFF)
#define ISCHANDIR(qid)		(SUBFILE(qid) == Qchandir)

#define MAX_EXCHANGE_CHANNELS	256
#define DEFAULT_POOL_SIZE	16

/* Ring Buffer Control Page Structure (4KB, mmap'd to userspace) */
typedef struct RingControl {
	u32int magic;			/* 0x52494E47 "RING" */
	u32int version;			/* 1 */

	/* Submission Ring (User → Kernel) */
	struct {
		u32int head;		/* Consumer index (kernel) */
		u32int tail;		/* Producer index (user) */
		u32int mask;		/* Ring size - 1 */
		u32int flags;
		uuid_t uuids[120];	/* 120 UUIDv8 entries (16 bytes each = 1920 bytes) */
	} submission;

	/* Completion Ring (Kernel → User) */
	struct {
		u32int head;		/* Consumer index (user) */
		u32int tail;		/* Producer index (kernel) */
		u32int mask;
		u32int flags;
		uuid_t uuids[120];	/* 120 UUIDv8 entries (1920 bytes) */
	} completion;

	/* Security */
	uuid_t session_uuid;		/* UUIDv8 session identifier */

	/* Padding to 4KB: 8 + 16 + 1920 + 16 + 1920 + 16 = 3896 bytes, need 200 bytes padding */
	u8int reserved[200];
} RingControl;

/* Exchange Channel - per-process communication endpoint */
typedef struct ExchangeChannel {
	Ref ref;
	Lock lock;
	Proc *owner;			/* Owning process */
	int chan_id;			/* Channel ID */

	/* Ring Buffer Control Page (mmap'd to userspace) */
	RingControl *ring_kaddr;	/* Kernel mapping */
	UserCapability ring_cap;	/* Capability for ring page */

	/* Exchange Page Pool */
	UserCapability *pool_caps;	/* Array of capabilities */
	uint pool_size;			/* Total pool size */
	uint pool_head;			/* Free list head */
	uint pool_tail;			/* Free list tail */
	Lock pool_lock;			/* Pool allocation lock */

	/* Channel Mode */
	enum {
		CHAN_KERNEL,		/* Kernel syscall endpoint */
		CHAN_PEER,		/* Peer IPC endpoint */
	} mode;

	/* Peer Connection (if mode == CHAN_PEER) */
	struct ExchangeChannel *peer;	/* Connected peer channel */

	/* Statistics */
	u64int messages_sent;
	u64int messages_recv;
	u64int bytes_sent;
	u64int bytes_recv;
	u64int pool_allocs;
	u64int pool_frees;

	/* TOCTOU Protection */
	u64int last_seqno;		/* Replay protection */
	uuid_t session_uuid;		/* Per-channel UUIDv8 session ID */
} ExchangeChannel;

/* Global channel table */
static ExchangeChannel *channels[MAX_EXCHANGE_CHANNELS];
static Lock channels_lock;

/* Legacy exchange control */
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
	"clone",	{Qclone, 0},		0,		0666,
	"ctl",		{Qglobalctl, 0},	0,		0666,
	"exchange",	{Qexchange, 0},		0,		0666,	/* Legacy */
	"stat",		{Qstat, 0},		0,		0444,	/* Legacy */
};

Dirtab chandir[] = {
	".",		{Qchandir, 0, QTDIR},	0,		DMDIR|0555,
	"ctl",		{Qctl, 0},		0,		0666,
	"ring",		{Qring, 0},		0,		0666,
	"pool",		{Qpool, 0},		0,		0666,
	"stats",	{Qstats, 0},		0,		0444,
	"peers",	{Qpeers, 0},		0,		0444,
};

/* Helper: compare two UserCapability structs */
static int
capability_equal(const ExchangeHandle *a, const ExchangeHandle *b)
{
	return memcmp(a->hash, b->hash, BLIND_LEDGER_CAP_SIZE) == 0 &&
	       a->size == b->size &&
	       a->type == b->type &&
	       a->perms == b->perms;
}

/* Helper: check if UserCapability is zero/invalid */
static int
capability_is_zero(const ExchangeHandle *cap)
{
	static const u8int zero_hash[BLIND_LEDGER_CAP_SIZE] = {0};
	return memcmp(cap->hash, zero_hash, BLIND_LEDGER_CAP_SIZE) == 0 &&
	       cap->size == 0 &&
	       cap->type == 0 &&
	       cap->perms == 0;
}

/* Channel Management Helpers */

static ExchangeChannel*
channel_alloc(Proc *owner)
{
	ExchangeChannel *ch;
	int i;

	ch = smalloc(sizeof(ExchangeChannel));
	if(ch == nil)
		return nil;

	memset(ch, 0, sizeof(ExchangeChannel));
	ch->owner = owner;
	ch->mode = CHAN_KERNEL;
	ch->pool_size = DEFAULT_POOL_SIZE;
	ch->ref.ref = 1;

	/* Allocate ring buffer control page */
	void *ring_pa;
	u8int vault_secret[32];
	BlindLedgerError err;

	ring_pa = xspanalloc(4096, BY2PG, 0);
	if(ring_pa == nil){
		free(ch);
		return nil;
	}

	/* Generate vault secret and mint capability */
	ledger_generate_secret(vault_secret);
	err = ledger_mint(&ch->ring_cap, (uintptr)ring_pa, 4096, owner,
		CAP_PERM_READ | CAP_PERM_WRITE, vault_secret);
	if(err != BLIND_LEDGER_OK){
		free(ch);
		return nil;
	}

	ch->ring_kaddr = (RingControl*)ring_pa;
	memset(ch->ring_kaddr, 0, 4096);
	ch->ring_kaddr->magic = 0x52494E47;
	ch->ring_kaddr->version = 1;
	ch->ring_kaddr->submission.mask = 119;  /* 120 - 1 */
	ch->ring_kaddr->completion.mask = 119;

	/* Generate session UUID */
	uuid_new_v8(&ch->ring_kaddr->session_uuid);
	uuid_copy(&ch->session_uuid, &ch->ring_kaddr->session_uuid);

	/* Allocate pool capability array */
	ch->pool_caps = smalloc(ch->pool_size * sizeof(UserCapability));
	if(ch->pool_caps == nil){
		free(ch);
		return nil;
	}
	ch->pool_head = 0;
	ch->pool_tail = 0;

	/* Find free channel slot */
	lock(&channels_lock);
	for(i = 0; i < MAX_EXCHANGE_CHANNELS; i++){
		if(channels[i] == nil){
			channels[i] = ch;
			ch->chan_id = i;
			unlock(&channels_lock);
			return ch;
		}
	}
	unlock(&channels_lock);

	/* No free slots */
	free(ch->pool_caps);
	free(ch);
	return nil;
}

static void
channel_free(ExchangeChannel *ch)
{
	if(ch == nil)
		return;

	lock(&channels_lock);
	if(ch->chan_id >= 0 && ch->chan_id < MAX_EXCHANGE_CHANNELS)
		channels[ch->chan_id] = nil;
	unlock(&channels_lock);

	if(ch->pool_caps)
		free(ch->pool_caps);
	free(ch);
}

static ExchangeChannel*
channel_get(int chan_id)
{
	ExchangeChannel *ch;

	if(chan_id < 0 || chan_id >= MAX_EXCHANGE_CHANNELS)
		return nil;

	lock(&channels_lock);
	ch = channels[chan_id];
	if(ch != nil)
		incref(&ch->ref);
	unlock(&channels_lock);

	return ch;
}

static void
channel_put(ExchangeChannel *ch)
{
	if(ch == nil)
		return;

	if(decref(&ch->ref) == 0)
		channel_free(ch);
}

/* Pool Management */
static int
pool_alloc_page(ExchangeChannel *ch, UserCapability *out_cap)
{
	void *pa;
	BlindLedgerError err;
	u8int vault_secret[32];

	if(ch == nil || out_cap == nil)
		return -1;

	lock(&ch->pool_lock);

	/* Check if pool is full */
	if(((ch->pool_tail + 1) & (ch->pool_size - 1)) == ch->pool_head){
		unlock(&ch->pool_lock);
		return -1;  /* Pool full */
	}

	/* Allocate page directly (Phase 1 - simple allocation) */
	pa = xspanalloc(BY2PG, BY2PG, 0);
	if(pa == nil){
		unlock(&ch->pool_lock);
		return -1;
	}

	/* Generate vault secret */
	ledger_generate_secret(vault_secret);

	/* Mint capability for the page */
	err = ledger_mint(out_cap, (uintptr)pa, BY2PG, ch->owner,
		CAP_PERM_READ | CAP_PERM_WRITE, vault_secret);
	if(err != BLIND_LEDGER_OK){
		unlock(&ch->pool_lock);
		return -1;
	}

	/* Add to pool */
	ch->pool_caps[ch->pool_tail] = *out_cap;
	ch->pool_tail = (ch->pool_tail + 1) & (ch->pool_size - 1);
	ch->pool_allocs++;

	unlock(&ch->pool_lock);
	return 0;
}

static int
pool_get_page(ExchangeChannel *ch, UserCapability *out_cap)
{
	if(ch == nil || out_cap == nil)
		return -1;

	lock(&ch->pool_lock);

	/* Check if pool is empty */
	if(ch->pool_head == ch->pool_tail){
		unlock(&ch->pool_lock);
		return -1;  /* Pool empty */
	}

	/* Get capability from pool */
	*out_cap = ch->pool_caps[ch->pool_head];
	ch->pool_head = (ch->pool_head + 1) & (ch->pool_size - 1);

	unlock(&ch->pool_lock);
	return 0;
}

static int
pool_return_page(ExchangeChannel *ch, const UserCapability *cap)
{
	if(ch == nil || cap == nil)
		return -1;

	lock(&ch->pool_lock);

	/* Check if pool is full */
	if(((ch->pool_tail + 1) & (ch->pool_size - 1)) == ch->pool_head){
		unlock(&ch->pool_lock);
		return -1;  /* Pool full */
	}

	/* Return to pool */
	ch->pool_caps[ch->pool_tail] = *cap;
	ch->pool_tail = (ch->pool_tail + 1) & (ch->pool_size - 1);
	ch->pool_frees++;

	unlock(&ch->pool_lock);
	return 0;
}

static void
exchinit(void)
{
	int i;

	/* Initialize channel table */
	for(i = 0; i < MAX_EXCHANGE_CHANNELS; i++)
		channels[i] = nil;

	/* Initialize legacy exchange control structure */
	exchctl.nprepared = 0;

	print("exchange: 9P device initialized (pooled channels enabled)\n");
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
	int i, j, chan_id;
	Walkqid *wq;
	char buf[32];

	/* Handle walks into channel directories */
	if(c->qid.path == Qtopdir && nname > 0){
		/* Check if walking into a channel directory (numeric name) */
		chan_id = atoi(name[0]);
		if(chan_id >= 0 && chan_id < MAX_EXCHANGE_CHANNELS &&
		   name[0][0] >= '0' && name[0][0] <= '9'){
			/* Verify channel exists */
			ExchangeChannel *ch = channel_get(chan_id);
			if(ch != nil){
				channel_put(ch);
				/* Walk into channel directory */
				mkqid(&nc->qid, CHANQID(chan_id, Qchandir), 0, QTDIR);
				nc->dev = c->dev;

				if(nname == 1)
					return devwalk(c, nc, name+1, 0, chandir, nelem(chandir), devgen);

				/* Multi-element walk: continue into channel */
				wq = devwalk(c, nc, name+1, nname-1, chandir, nelem(chandir), devgen);
				if(wq != nil)
					wq->clone = nc;
				return wq;
			}
		}
	}

	/* Handle walks within channel directories */
	if(SUBFILE(c->qid.path) == Qchandir){
		chan_id = CHANID(c->qid.path);
		return devwalk(c, nc, name, nname, chandir, nelem(chandir), devgen);
	}

	/* Default: top-level walk */
	return devwalk(c, nc, name, nname, exchdir, nelem(exchdir), devgen);
}

static int
exchstat(Chan *c, uchar *dp, int n)
{
	int subfile;

	subfile = SUBFILE(c->qid.path);

	/* Stats for channel files */
	if(subfile >= Qchandir && subfile <= Qpeers)
		return devstat(c, dp, n, chandir, nelem(chandir), devgen);

	/* Stats for top-level files */
	return devstat(c, dp, n, exchdir, nelem(exchdir), devgen);
}

static Chan*
exchopen(Chan *c, int omode)
{
	int subfile;
	ExchangeChannel *ch;

	omode &= 3;	/* mask off exec bit */
	subfile = SUBFILE(c->qid.path);

	switch(subfile){
	case Qtopdir:
	case Qchandir:
		if(omode != OREAD)
			error(Eperm);
		break;

	case Qclone:
		/* Clone: allocate new channel */
		ch = channel_alloc(up);
		if(ch == nil)
			error(Enomem);
		/* Store channel in c->aux for read */
		c->aux = ch;
		break;

	case Qctl:
	case Qpool:
	case Qglobalctl:
		/* Allow read/write */
		break;

	case Qring:
		/* Ring buffer: will be mmap'd */
		break;

	case Qstats:
	case Qpeers:
	case Qstat:
		/* Read-only */
		if(omode != OREAD)
			error(Eperm);
		break;

	case Qexchange:
		/* Legacy: allow read/write */
		break;
	}

	/* Use appropriate directory table */
	if(subfile >= Qchandir && subfile <= Qpeers)
		c = devopen(c, omode, chandir, nelem(chandir), devgen);
	else
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
	ExchangeChannel *ch;

	/* If clone was opened, release the channel */
	if(SUBFILE(c->qid.path) == Qclone && c->aux != nil){
		ch = c->aux;
		channel_put(ch);
		c->aux = nil;
	}
}

static long
exchread(Chan *c, void *buf, long n, vlong off)
{
	char *p, *e;
	int i, chan_id;
	ExchangeChannel *ch;
	UserCapability cap;

	switch(SUBFILE(c->qid.path)){
	case Qtopdir:
		return devdirread(c, buf, n, exchdir, nelem(exchdir), devgen);

	case Qclone:
		/* Return channel ID allocated during open */
		ch = c->aux;
		if(ch == nil)
			error("no channel allocated");
		p = smalloc(32);
		if(p == nil)
			error(Enomem);
		snprint(p, 32, "%d", ch->chan_id);
		n = readstr(off, buf, n, p);
		free(p);
		return n;

	case Qpool:
		/* Allocate page from pool and return UserCapability */
		chan_id = CHANID(c->qid.path);
		ch = channel_get(chan_id);
		if(ch == nil)
			error("invalid channel");

		if(pool_get_page(ch, &cap) < 0){
			/* Pool empty, allocate new page */
			if(pool_alloc_page(ch, &cap) < 0){
				channel_put(ch);
				error("pool allocation failed");
			}
		}

		if(n < sizeof(UserCapability))
			n = sizeof(UserCapability);
		memmove(buf, &cap, sizeof(UserCapability));
		channel_put(ch);
		return sizeof(UserCapability);

	case Qstats:
		/* Return channel statistics */
		chan_id = CHANID(c->qid.path);
		ch = channel_get(chan_id);
		if(ch == nil)
			error("invalid channel");

		p = smalloc(2048);
		if(p == nil){
			channel_put(ch);
			error(Enomem);
		}
		e = p + 2048;

		lock(&ch->lock);
		seprint(p, e, "Channel %d Statistics\n", ch->chan_id);
		seprint(p+strlen(p), e, "Owner PID: %d\n", ch->owner ? ch->owner->pid : -1);
		seprint(p+strlen(p), e, "Mode: %s\n", ch->mode == CHAN_KERNEL ? "kernel" : "peer");
		seprint(p+strlen(p), e, "Pool size: %ud\n", ch->pool_size);
		seprint(p+strlen(p), e, "Pool used: %ud\n",
			(ch->pool_tail - ch->pool_head) & (ch->pool_size - 1));
		seprint(p+strlen(p), e, "Messages sent: %llud\n", ch->messages_sent);
		seprint(p+strlen(p), e, "Messages recv: %llud\n", ch->messages_recv);
		seprint(p+strlen(p), e, "Bytes sent: %llud\n", ch->bytes_sent);
		seprint(p+strlen(p), e, "Bytes recv: %llud\n", ch->bytes_recv);
		seprint(p+strlen(p), e, "Pool allocs: %llud\n", ch->pool_allocs);
		seprint(p+strlen(p), e, "Pool frees: %llud\n", ch->pool_frees);
		unlock(&ch->lock);

		n = readstr(off, buf, n, p);
		free(p);
		channel_put(ch);
		return n;

	case Qctl:
		/* Return channel configuration */
		chan_id = CHANID(c->qid.path);
		ch = channel_get(chan_id);
		if(ch == nil)
			error("invalid channel");

		p = smalloc(1024);
		if(p == nil){
			channel_put(ch);
			error(Enomem);
		}

		lock(&ch->lock);
		snprint(p, 1024, "mode %s\npoolsize %ud\n",
			ch->mode == CHAN_KERNEL ? "kernel" : "peer",
			ch->pool_size);
		unlock(&ch->lock);

		n = readstr(off, buf, n, p);
		free(p);
		channel_put(ch);
		return n;

	case Qring:
		/* Return ring buffer capability for mapping */
		chan_id = CHANID(c->qid.path);
		ch = channel_get(chan_id);
		if(ch == nil)
			error("invalid channel");

		if(n < sizeof(UserCapability))
			n = sizeof(UserCapability);
		memmove(buf, &ch->ring_cap, sizeof(UserCapability));
		channel_put(ch);
		return sizeof(UserCapability);

	case Qpeers:
		/* Return peer list (if any) */
		chan_id = CHANID(c->qid.path);
		ch = channel_get(chan_id);
		if(ch == nil)
			error("invalid channel");

		p = smalloc(1024);
		if(p == nil){
			channel_put(ch);
			error(Enomem);
		}

		lock(&ch->lock);
		if(ch->peer != nil)
			snprint(p, 1024, "peer: channel %d\n", ch->peer->chan_id);
		else
			snprint(p, 1024, "no peers\n");
		unlock(&ch->lock);

		n = readstr(off, buf, n, p);
		free(p);
		channel_put(ch);
		return n;

	case Qexchange:
		/* Return information about prepared exchanges (legacy) */
		p = smalloc(4096);
		if(p == nil)
			error(Enomem);
		e = p + 4096;
		seprint(p, e, "Page Exchange System\n");
		seprint(p+strlen(p), e, "Prepared pages: %d\n", exchctl.nprepared);
		seprint(p+strlen(p), e, "Index  Owner PID   Original VAddr\n");
		seprint(p+strlen(p), e, "-----  ----------  ---------------\n");
		qlock(&exchctl);
		for(i = 0; i < exchctl.nprepared; i++){
			seprint(p+strlen(p), e, "%-5d  %-10d  0x%016llux\n",
				i,
				exchctl.prepared[i].owner ? exchctl.prepared[i].owner->pid : -1,
				(uvlong)exchctl.prepared[i].original_vaddr);
		}
		qunlock(&exchctl);
		n = readstr(off, buf, n, p);
		free(p);
		return n;

	case Qstat:
		/* Return statistics (legacy) */
		p = smalloc(1024);
		if(p == nil)
			error(Enomem);
		pageown_stats();
		seprint(p, p+1024, "Exchange device statistics\n");
		seprint(p+strlen(p), p+1024, "Total prepared: %d\n", exchctl.nprepared);
		n = readstr(off, buf, n, p);
		free(p);
		return n;

	default:
		/* Channel directory listing */
		if(ISCHANDIR(c->qid.path)){
			chan_id = CHANID(c->qid.path);
			ch = channel_get(chan_id);
			if(ch == nil)
				error("invalid channel");
			n = devdirread(c, buf, n, chandir, nelem(chandir), devgen);
			channel_put(ch);
			return n;
		}
	}

	return 0;
}

static long
exchwrite(Chan *c, void *vp, long n, vlong off)
{
	char *buf;
	char *fields[4];
	int nf, chan_id;
	uintptr vaddr;
	ExchangeHandle handle;
	int result;
	ExchangeChannel *ch;
	UserCapability cap;

	(void)off;

	buf = smalloc(n+1);
	if(buf == nil)
		error(Enomem);
	memmove(buf, vp, n);
	buf[n] = 0;

	switch(SUBFILE(c->qid.path)){
	case Qctl:
		/* Handle channel control commands */
		chan_id = CHANID(c->qid.path);
		ch = channel_get(chan_id);
		if(ch == nil){
			free(buf);
			error("invalid channel");
		}

		nf = tokenize(buf, fields, nelem(fields));
		if(nf < 1){
			channel_put(ch);
			free(buf);
			error("invalid control command");
		}

		if(strcmp(fields[0], "attach") == 0){
			if(nf < 2){
				channel_put(ch);
				free(buf);
				error("usage: attach kernel|#s/name");
			}
			lock(&ch->lock);
			if(strcmp(fields[1], "kernel") == 0){
				ch->mode = CHAN_KERNEL;
			} else {
				ch->mode = CHAN_PEER;
				/* TODO: Connect to peer endpoint */
			}
			unlock(&ch->lock);
		}
		else if(strcmp(fields[0], "poolsize") == 0){
			if(nf < 2){
				channel_put(ch);
				free(buf);
				error("usage: poolsize N");
			}
			uint new_size = strtoul(fields[1], nil, 0);
			if(new_size < 16 || new_size > 1024){
				channel_put(ch);
				free(buf);
				error("poolsize must be 16-1024");
			}
			lock(&ch->pool_lock);
			/* TODO: Resize pool if needed */
			ch->pool_size = new_size;
			unlock(&ch->pool_lock);
		}
		else if(strcmp(fields[0], "mapring") == 0){
			/* Map ring buffer into process address space
			 * For Phase 2, we store the channel in the process
			 * and userspace will use segattach() to map it.
			 * This command just prepares the channel for mapping.
			 */
			lock(&ch->lock);
			/* Mark channel as ready for mapping */
			ch->owner = up;
			unlock(&ch->lock);

			/* Store channel pointer in process for later segattach */
			/* In full implementation, would use up->exch_channel or similar */
			/* For now, userspace can use segattach(SG_PHYSICAL, "#X/N/ring", ...) */
		}
		else {
			channel_put(ch);
			free(buf);
			error("unknown control command");
		}

		channel_put(ch);
		free(buf);
		return n;

	case Qpool:
		/* Return page to pool */
		if(n < sizeof(UserCapability)){
			free(buf);
			error("invalid capability size");
		}

		chan_id = CHANID(c->qid.path);
		ch = channel_get(chan_id);
		if(ch == nil){
			free(buf);
			error("invalid channel");
		}

		memmove(&cap, vp, sizeof(UserCapability));
		if(pool_return_page(ch, &cap) < 0){
			channel_put(ch);
			free(buf);
			error("pool full");
		}

		channel_put(ch);
		free(buf);
		return n;

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
			if(exchange_prepare(vaddr, &handle) != BLIND_LEDGER_OK){
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
			/* Parse "accept <cap_index> <dest_vaddr> <prot>" */
			char *p = buf+7;
			int cap_idx = strtol(p, &p, 0);
			while(*p == ' ') p++;
			uintptr dest_vaddr = strtoul(p, &p, 0);
			while(*p == ' ') p++;
			int prot = strtol(p, nil, 0);

			qlock(&exchctl);
			if(cap_idx < 0 || cap_idx >= exchctl.nprepared || (dest_vaddr & (BY2PG-1)) != 0){
				qunlock(&exchctl);
				free(buf);
				error("invalid parameters");
			}

			handle = exchctl.prepared[cap_idx].handle;
			result = exchange_accept(&handle, dest_vaddr, prot);
			if(result != EXCHANGE_OK){
				qunlock(&exchctl);
				free(buf);
				error("exchange_accept failed");
			}

			/* Remove from prepared tracking */
			for(int j = cap_idx; j < exchctl.nprepared - 1; j++){
				exchctl.prepared[j] = exchctl.prepared[j+1];
			}
			exchctl.nprepared--;
			qunlock(&exchctl);
			
			free(buf);
			return n;
		}
		else if(strncmp(buf, "cancel ", 7) == 0){
			/* cancel <cap_index> */
			int cap_idx = strtol(buf+7, nil, 0);

			qlock(&exchctl);
			if(cap_idx < 0 || cap_idx >= exchctl.nprepared){
				qunlock(&exchctl);
				free(buf);
				error("invalid capability index");
			}

			handle = exchctl.prepared[cap_idx].handle;
			result = exchange_cancel(&handle);
			if(result != EXCHANGE_OK){
				qunlock(&exchctl);
				free(buf);
				error("exchange_cancel failed");
			}

			/* Remove from prepared tracking */
			for(int j = cap_idx; j < exchctl.nprepared - 1; j++){
				exchctl.prepared[j] = exchctl.prepared[j+1];
			}
			exchctl.nprepared--;
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

/* Phase 3 will implement capability-based mapping
 * For now, Phase 2 exposes capabilities via read(#X/N/ring)
 * and userspace will use those capabilities to map memory
 */

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

/* Kernel boot integration - setup exchange infrastructure for init process
 * Called from proc0() during kernel boot to proactively set up 9P exchange.
 * This eliminates the chicken-egg problem of init needing syscalls to open #X.
 *
 * Returns: ExchangeChannel on success, nil on failure
 */
void*
kernel_setup_init_exchange(Proc *p)
{
	ExchangeChannel *ch;
	UserCapability cap;
	uintptr ring_pa, pool_pa;

	if(p == nil)
		return nil;

	print("BOOT[kernel_setup_init_exchange]: allocating exchange channel for PID %ld\n", p->pid);

	/* Allocate exchange channel using same infrastructure as #X/clone */
	ch = channel_alloc(p);
	if(ch == nil){
		print("BOOT[kernel_setup_init_exchange]: channel_alloc failed\n");
		return nil;
	}

	print("BOOT[kernel_setup_init_exchange]: channel %d allocated, ring at %#p\n",
		ch->chan_id, ch->ring_kaddr);

	/* Allocate initial pool pages (2 pages for request/reply) */
	if(pool_alloc_page(ch, &cap) < 0){
		channel_put(ch);
		print("BOOT[kernel_setup_init_exchange]: pool_alloc_page failed\n");
		return nil;
	}

	/* Get physical addresses from capabilities for mapping */
	/* The capability contains the kernel physical address in the hash
	 * For now, we'll allocate dedicated pages and map them directly */

	/* Allocate 2 exchange pages (request + reply) */
	void *exch_pages = mallocalign(BY2PG * 2, BY2PG, 0, 0);
	if(exch_pages == nil){
		channel_put(ch);
		print("BOOT[kernel_setup_init_exchange]: failed to allocate exchange pages\n");
		return nil;
	}
	memset(exch_pages, 0, BY2PG * 2);

	/* Store exchange pages in p->p9page for doorbell handler */
	p->p9page = exch_pages;

	/* Get physical addresses for borrowchecker tracking */
	uintptr req_pa = PADDR(exch_pages);
	uintptr rep_pa = PADDR(exch_pages) + BY2PG;

	/*
	 * CRITICAL: Userspace claims pages FIRST (per user requirement)
	 * "During init, once the page is setup, USERSPACE (init) claims the page FIRST.
	 *  This is logical: the kernel creates the way to communicate with it,
	 *  the init (an application running on top of the kernel) utilizes the method provided."
	 *
	 * Acquire ownership for init process via borrowchecker.
	 * This ensures exclusive access - only userspace can read/write until syscall doorbell.
	 */
	enum BorrowError berr = borrow_acquire(p, req_pa);
	if(berr != BORROW_OK){
		print("BOOT[kernel_setup_init_exchange]: borrow_acquire(req_page) failed: %d\n", berr);
		channel_put(ch);
		return nil;
	}

	berr = borrow_acquire(p, rep_pa);
	if(berr != BORROW_OK){
		print("BOOT[kernel_setup_init_exchange]: borrow_acquire(rep_page) failed: %d\n", berr);
		borrow_release(p, req_pa);  /* Rollback first page */
		channel_put(ch);
		return nil;
	}

	/* Map exchange pages to userspace at EXCHANGE_PAGE_ADDR
	 * Page 1: Request buffer (0x7FFFFEEFF000)
	 * Page 2: Reply buffer (0x7FFFFEF00000)
	 *
	 * Pages are now owned exclusively by userspace (init process).
	 * Kernel CANNOT access until ownership is transferred via syscall doorbell.
	 */
	userpmap(EXCHANGE_PAGE_ADDR, req_pa, PTEVALID | PTEUSER | PTEWRITE);
	userpmap(EXCHANGE_PAGE_ADDR + BY2PG, rep_pa, PTEVALID | PTEUSER | PTEWRITE);

	print("BOOT[kernel_setup_init_exchange]: userspace claimed exchange pages at %#p (PA req=%#p rep=%#p)\n",
		EXCHANGE_PAGE_ADDR, req_pa, rep_pa);

	/* Map ring buffer control page to userspace (for future use)
	 * The ring buffer provides batched message submission/completion */
	ring_pa = PADDR(ch->ring_kaddr);
	/* Note: Ring mapping will be done when init calls segattach() on #X/N/ring */

	print("BOOT[kernel_setup_init_exchange]: exchange channel ready (id=%d)\n", ch->chan_id);

	return ch;
}
