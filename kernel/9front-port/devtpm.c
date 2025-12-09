/*
 * TPM 2.0 Device Driver - 9P Interface
 *
 * Provides Plan 9 style filesystem interface to TPM:
 *   /dev/tpm/ctl         - Control file for commands
 *   /dev/tpm/random      - Hardware random number generator
 *   /dev/tpm/pcr0-23     - PCR read (32 bytes) / extend (write 32 bytes)
 *   /dev/tpm/nv          - NVRAM operations via ctl commands
 */

#include "u.h"
#include "portlib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "error.h"
#include "io.h"

/* TPM driver functions (from tpm2_driver.c) */
extern void tpminit(void);
extern int tpm_get_random(u8int *buffer, int len);
extern int tpm20_pcr_extend(u32int pcr_handle, u8int *hash, usize hash_len);
extern int tpm20_pcr_read(u32int pcr_handle, u8int *pcr_value, usize *pcr_len);

/* Qid types */
enum {
	Qdir,
	Qctl,
	Qrandom,
	Qpcr0,      /* PCR files: Qpcr0 through Qpcr23 */
	Qpcr23 = Qpcr0 + 23,
};

static Dirtab tpmdir[] = {
	".",        {Qdir, 0, QTDIR},  0,   DMDIR|0555,
	"ctl",      {Qctl},            0,   0664,
	"random",   {Qrandom},         0,   0444,
	"pcr0",     {Qpcr0},           32,  0664,
	"pcr1",     {Qpcr0+1},         32,  0664,
	"pcr2",     {Qpcr0+2},         32,  0664,
	"pcr3",     {Qpcr0+3},         32,  0664,
	"pcr4",     {Qpcr0+4},         32,  0664,
	"pcr5",     {Qpcr0+5},         32,  0664,
	"pcr6",     {Qpcr0+6},         32,  0664,
	"pcr7",     {Qpcr0+7},         32,  0664,
	"pcr8",     {Qpcr0+8},         32,  0664,
	"pcr9",     {Qpcr0+9},         32,  0664,
	"pcr10",    {Qpcr0+10},        32,  0664,
	"pcr11",    {Qpcr0+11},        32,  0664,
	"pcr12",    {Qpcr0+12},        32,  0664,
	"pcr13",    {Qpcr0+13},        32,  0664,
	"pcr14",    {Qpcr0+14},        32,  0664,
	"pcr15",    {Qpcr0+15},        32,  0664,
	"pcr16",    {Qpcr0+16},        32,  0664,
	"pcr17",    {Qpcr0+17},        32,  0664,
	"pcr18",    {Qpcr0+18},        32,  0664,
	"pcr19",    {Qpcr0+19},        32,  0664,
	"pcr20",    {Qpcr0+20},        32,  0664,
	"pcr21",    {Qpcr0+21},        32,  0664,
	"pcr22",    {Qpcr0+22},        32,  0664,
	"pcr23",    {Qpcr0+23},        32,  0664,
};

static void
tpminit_dev(void)
{
	/* Initialize TPM hardware */
	tpminit();
}

static Chan*
tpmattach(char *spec)
{
	return devattach('Ϯ', spec);
}

static Walkqid*
tpmwalk(Chan *c, Chan *nc, char **name, int nname)
{
	return devwalk(c, nc, name, nname, tpmdir, nelem(tpmdir), devgen);
}

static long
tpmstat(Chan *c, uchar *db, long n)
{
	return devstat(c, db, n, tpmdir, nelem(tpmdir), devgen);
}

static Chan*
tpmopen(Chan *c, int omode)
{
	/* Validate access */
	if(c->qid.path == Qrandom && (omode & (OWRITE|OTRUNC)))
		error(Eperm);

	c->mode = openmode(omode);
	c->flag |= COPEN;
	c->offset = 0;
	return c;
}

static void
tpmclose(Chan *c)
{
	USED(c);
}

static long
tpmread(Chan *c, void *va, long n, vlong off)
{
	u8int buf[512];
	usize len;
	int nr, pcr;

	switch((ulong)c->qid.path) {
	case Qdir:
		return devdirread(c, va, n, tpmdir, nelem(tpmdir), devgen);

	case Qctl:
		return readstr(off, va, n, "tpm2.0\n");

	case Qrandom:
		/* Read hardware random bytes */
		if(n > sizeof(buf))
			n = sizeof(buf);
		nr = tpm_get_random(buf, n);
		if(nr < 0)
			error(Eio);
		memmove(va, buf, nr);
		return nr;

	default:
		/* PCR read */
		if(c->qid.path >= Qpcr0 && c->qid.path <= Qpcr23) {
			pcr = c->qid.path - Qpcr0;
			len = sizeof(buf);
			if(tpm20_pcr_read(pcr, buf, &len) < 0)
				error(Eio);

			if(off >= len)
				return 0;
			if(off + n > len)
				n = len - off;
			memmove(va, buf + off, n);
			return n;
		}
		error(Ebadarg);
	}
	return 0;
}

static long
tpmwrite(Chan *c, void *va, long n, vlong off)
{
	u8int hash[32];
	int pcr;
	Cmdbuf *cb;

	USED(off);

	switch((ulong)c->qid.path) {
	case Qctl:
		/* Parse control commands */
		cb = parsecmd(va, n);
		if(waserror()){
			free(cb);
			nexterror();
		}

		/* Future: NV define/write/delete commands */
		print("TPM ctl: %s\n", cb->f[0]);

		poperror();
		free(cb);
		return n;

	default:
		/* PCR extend */
		if(c->qid.path >= Qpcr0 && c->qid.path <= Qpcr23) {
			pcr = c->qid.path - Qpcr0;

			if(n != 32)
				error("PCR extend requires exactly 32 bytes (SHA256)");

			memmove(hash, va, 32);
			if(tpm20_pcr_extend(pcr, hash, 32) < 0)
				error(Eio);
			return n;
		}
		error(Eperm);
	}
	return 0;
}

Dev tpmdevtab = {
	'Ϯ',
	"tpm",

	devreset,
	tpminit_dev,
	devshutdown,
	tpmattach,
	tpmwalk,
	tpmstat,
	tpmopen,
	devcreate,
	tpmclose,
	tpmread,
	devbread,
	tpmwrite,
	devbwrite,
	devremove,
	devwstat,
};
