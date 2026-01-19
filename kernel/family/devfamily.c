/*
 * Family Device Driver (#F)
 * 
 * Provides a unified 9P facade for all device families (PCI, USB, etc.)
 * Exposed as /dev/family/ or #F
 *
 * Implements a "Congruent 9P Router" architecture:
 * - Root directory lists registered families.
 * - Access to family directories is routed to the specific family's 9P server.
 */

#include "u.h"
#include "portlib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "family/family.h"
#include <error.h>

enum {
	Qdir = 0,
	Qctl,
};

#define FAMILY_SHIFT 56
#define FAMILY_MASK 0xFF00000000000000ULL
#define INTERNAL_MASK 0x00FFFFFFFFFFFFFFULL

/* Forward decls */
static int familygen(Chan *c, char *name, Dirtab *tab, int ntab, int pos, Dir *dp);

/*@
  @ assigns \nothing; // Initializes global state managed by family.c
  @ terminates \true;
  @*/
static void
familyinit(void)
{
	family_init();
}

/*@
  @ requires valid_string(spec);
  @ assigns \nothing;
  @ ensures \result == \null || \valid(\result);
  @ terminates \true;
  @*/static Chan*
familyattach(char *spec)
{
	return devattach('F', spec);
}

/*@
  @ requires \valid(c) && \valid(nc);
  @ requires name != \null;
  @ requires nname >= 0;
  @ assigns *family;
  @ ensures \result == \null || \valid(\result);
  @*/static Walkqid*
familywalk(Chan *c, Chan *nc, char **name, int nname)
{
	int type = (c->qid.path & FAMILY_MASK) >> FAMILY_SHIFT;
	
	if(type == 0) {
		/* Global root: use generic devwalk with our generator */
		return devwalk(c, nc, name, nname, nil, 0, familygen);
	}
	
	/* Sub-tree routing: delegate to family driver */
	struct FamilyExchangePage *family = family_lookup(type);
	if(family && family->ops && family->ops->walk) {
		return family->ops->walk(family, c, nc, name, nname);
	}
	
	error(Enonexist);
	return nil;
}

/*@
  @ requires \valid(c);
  @ requires \valid(dp + (0 .. n-1));
  @ assigns dp[0 .. n-1];
  @ ensures \result == -1 || \result > 0;
  @*/
static int
familystat(Chan *c, uchar *dp, int n)
{
	int type = (c->qid.path & FAMILY_MASK) >> FAMILY_SHIFT;
	
	if(type == 0) {
		return devstat(c, dp, n, nil, 0, familygen);
	}
	
	struct FamilyExchangePage *family = family_lookup(type);
	if(family && family->ops && family->ops->stat) {
		return family->ops->stat(family, c, dp, n);
	}
	
	error(Enonexist);
	return -1;
}

/*@
  @ requires \valid(c);
  @ assigns c->mode, c->offset, c->flag;
  @ ensures \result == \null || \result == c;
  @*/
static Chan*
familyopen(Chan *c, int omode)
{
	int type = (c->qid.path & FAMILY_MASK) >> FAMILY_SHIFT;
	
	if(type == 0) {
		return devopen(c, omode, nil, 0, familygen);
	}
	
	struct FamilyExchangePage *family = family_lookup(type);
	if(family && family->ops && family->ops->open) {
		return family->ops->open(family, c, omode);
	}
	
	error(Enonexist);
	return nil;
}

/*@
  @ requires \valid(c);
  @ assigns *family;
  @*/static void
familyclose(Chan *c)
{
	int type = (c->qid.path & FAMILY_MASK) >> FAMILY_SHIFT;
	
	if(type == 0) return;
	
	struct FamilyExchangePage *family = family_lookup(type);
	if(family && family->ops && family->ops->close) {
		family->ops->close(family, c);
	}
}

/*@
  @ requires \valid(c);
  @ requires \valid((char*)va + (0 .. n-1));
  @ assigns ((char*)va)[0 .. n-1];
  @ ensures \result == -1 || \result >= 0;
  @*/
static long
familyread(Chan *c, void *va, long n, vlong off)
{
	int type = (c->qid.path & FAMILY_MASK) >> FAMILY_SHIFT;
	
	if(type == 0) {
		return devdirread(c, va, n, nil, 0, familygen);
	}
	
	struct FamilyExchangePage *family = family_lookup(type);
	if(family && family->ops && family->ops->read) {
		return family->ops->read(family, c, va, n, off);
	}
	
	error(Enonexist);
	return -1;
}

/*@
  @ requires \valid(c);
  @ requires \valid_read((char*)va + (0 .. n-1));
  @ assigns *family;
  @ ensures \result == -1 || \result >= 0;
  @*/static long
familywrite(Chan *c, void *va, long n, vlong off)
{
	int type = (c->qid.path & FAMILY_MASK) >> FAMILY_SHIFT;
	
	if(type == 0) {
		error(Eperm);
	}
	
	struct FamilyExchangePage *family = family_lookup(type);
	if(family && family->ops && family->ops->write) {
		return family->ops->write(family, c, va, n, off);
	}
	
	error(Enonexist);
	return -1;
}

/* Root directory generator */
static int
familygen(Chan *c, char *name, Dirtab *tab, int ntab, int pos, Dir *dp)
{
	Qid qid;
	int i, idx;
	struct FamilyExchangePage *family;

	USED(c, tab, ntab);

	if(pos == 0) {
		/* "." */
		devdir(c, (Qid){Qdir, 0, QTDIR}, ".", 0, eve, 0555, dp);
		return 1;
	}
	pos--;

	if(c->qid.path == Qdir) {
		if(pos == 0) {
			devdir(c, (Qid){Qctl, 0, 0}, "ctl", 0, eve, 0666, dp);
			return 1;
		}
		pos--;

		/* Iterate registered families */
		idx = 0;
		for(i = 1; i < FAMILY_MAX; i++) {
			family = family_lookup(i);
			if(family) {
				if(idx == pos) {
					/* Qid path encodes family type in high bits */
					qid.path = ((uint64_t)i << FAMILY_SHIFT); 
					qid.vers = 0;
					qid.type = QTDIR;
					devdir(c, qid, family->family_name, 0, eve, 0555, dp);
					return 1;
				}
				idx++;
			}
		}
		return -1;
	}
	
	return -1;
}

Dev familydevtab = {
	'F',
	"family",

	devreset,
	familyinit,
	devshutdown,
	familyattach,
	familywalk,
	familystat,
	familyopen,
	devcreate,
	familyclose,
	familyread,
	devbread,
	familywrite,
	devbwrite,
	devremove,
	devwstat,
};