/*
 * SCSI emulation for ATA/SATA devices
 * Minimal implementation for Lux9 kernel
 */

#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "../port/error.h"
#include "../port/sd.h"

/*
 * scsiverify - Verify SCSI unit is ready
 */
int
scsiverify(SDunit *u)
{
	USED(u);
	/* For ATA devices, always return ready */
	return 1;
}

/*
 * scsionline - Bring SCSI unit online
 */
int
scsionline(SDunit *u)
{
	uchar *p;

	/* Set up basic SCSI inquiry data for ATAPI/CDROM */
	p = u->inquiry;
	memset(p, 0, sizeof(u->inquiry));
	p[0] = 0x05;		/* CD-ROM device */
	p[1] = 0x80;		/* Removable */
	p[2] = 0x02;		/* SCSI-2 */
	p[3] = 0x02;		/* Response data format */
	p[4] = sizeof(u->inquiry) - 4;	/* Additional length */

	return 0;
}

/*
 * scsibio - SCSI block I/O
 */
long
scsibio(SDunit *u, int lun, int write, void *data, long nb, uvlong bno)
{
	USED(u, lun, write, data, nb, bno);
	/* Not implemented - ATA devices use ataio directly */
	return -1;
}

/*
 * sdfakescsi - Convert ATA request to fake SCSI
 */
int
sdfakescsi(SDreq *r)
{
	uchar *cmd;

	cmd = r->cmd;
	r->clen = 0;

	/* Simple SCSI command emulation */
	switch(cmd[0]){
	case 0x00:	/* TEST UNIT READY */
		return SDok;
	case 0x12:	/* INQUIRY */
		if(r->data && r->dlen >= 36){
			memmove(r->data, r->unit->inquiry,
				r->dlen < sizeof(r->unit->inquiry) ? r->dlen : sizeof(r->unit->inquiry));
			r->rlen = r->dlen;
		}
		return SDok;
	case 0x1A:	/* MODE SENSE(6) */
	case 0x5A:	/* MODE SENSE(10) */
		/* Return minimal mode page */
		if(r->data && r->dlen > 0){
			memset(r->data, 0, r->dlen);
			r->rlen = r->dlen;
		}
		return SDok;
	}

	return SDcheck;
}

/*
 * sdfakescsirw - Parse fake SCSI read/write command
 */
int
sdfakescsirw(SDreq *r, uvlong *llba, int *nsecs, int *rwp)
{
	uchar *cmd;
	uvlong lba;
	int ns, rw;

	cmd = r->cmd;
	rw = r->write;

	switch(cmd[0]){
	case 0x08:	/* READ(6) */
	case 0x0A:	/* WRITE(6) */
		lba = ((cmd[1]&0x1F)<<16) | (cmd[2]<<8) | cmd[3];
		ns = cmd[4];
		if(ns == 0)
			ns = 256;
		break;
	case 0x28:	/* READ(10) */
	case 0x2A:	/* WRITE(10) */
		lba = (cmd[2]<<24) | (cmd[3]<<16) | (cmd[4]<<8) | cmd[5];
		ns = (cmd[7]<<8) | cmd[8];
		break;
	case 0x88:	/* READ(16) */
	case 0x8A:	/* WRITE(16) */
		lba = (uvlong)cmd[2]<<56 | (uvlong)cmd[3]<<48 |
		      (uvlong)cmd[4]<<40 | (uvlong)cmd[5]<<32 |
		      (uvlong)cmd[6]<<24 | (uvlong)cmd[7]<<16 |
		      (uvlong)cmd[8]<<8 | cmd[9];
		ns = (cmd[10]<<24) | (cmd[11]<<16) | (cmd[12]<<8) | cmd[13];
		break;
	default:
		return -1;
	}

	if(llba)
		*llba = lba;
	if(nsecs)
		*nsecs = ns;
	if(rwp)
		*rwp = rw;

	return 0;
}
