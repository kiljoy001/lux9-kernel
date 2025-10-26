/*
 * SD/FIS support functions for ATA/SATA drivers
 * Minimal implementations for Lux9 kernel
 */

#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "../port/error.h"
#include "../port/sd.h"
#include <fis.h>

/*
 * sdsetsense - Set SCSI sense data
 */
int
sdsetsense(SDreq *r, int status, int key, int asc, int ascq)
{
	int len;

	r->status = status;
	if(status == SDcheck && !(r->flags & SDnosense)){
		/* Build basic sense data */
		len = sizeof(r->sense)-1;
		memset(r->sense, 0, len);
		r->sense[0] = 0x70;	/* Current errors, fixed format */
		r->sense[2] = key;
		r->sense[7] = len-7;	/* Additional length */
		r->sense[12] = asc;
		r->sense[13] = ascq;
		r->flags |= SDvalidsense;
		return SDcheck;
	}
	return status;
}

/*
 * pflag - Print feature flags
 */
char*
pflag(char *s, char *e, Sfis *f)
{
	uint i;
	char *names[] = {
		"lba", "llba", "smart", "power", "nop", "atapi",
		"atapi16", "data8", "sct",
	};

	for(i = 0; i < Dnflag && i < nelem(names); i++){
		if(f->feat & (1<<i))
			s = seprint(s, e, " %s", names[i]);
	}
	s = seprint(s, e, "\n");
	return s;
}

/*
 * sigtofis - Convert signature to FIS
 */
void
sigtofis(Sfis *f, uchar *c)
{
	uint sig;

	sig = f->sig;
	c[Flba0] = sig;
	c[Flba8] = sig >> 8;
	c[Flba16] = sig >> 16;
	c[Flba24] = sig >> 24;
}

/*
 * setfissig - Set FIS signature
 */
void
setfissig(Sfis *f, uint sig)
{
	f->sig = sig;
}

/*
 * skelfis - Create skeleton FIS
 */
void
skelfis(uchar *c)
{
	memset(c, 0, Fissize);
	c[Ftype] = H2dev;
	c[Fflags] = 0;
}

/*
 * identifyfis - Build IDENTIFY DEVICE FIS
 */
int
identifyfis(Sfis *f, uchar *c)
{
	USED(f);
	skelfis(c);
	c[Fflags] = Fiscmd;
	c[Fcmd] = 0xEC;		/* IDENTIFY DEVICE */
	c[Fdev] = Ataobs;
	return 0;
}

/*
 * featfis - Build SET FEATURES FIS
 */
int
featfis(Sfis *m, uchar *c, uchar feat)
{
	USED(m);
	skelfis(c);
	c[Fflags] = Fiscmd;
	c[Fcmd] = 0xEF;		/* SET FEATURES */
	c[Ffeat] = feat;
	c[Fdev] = Ataobs;
	return 0;
}

/*
 * flushcachefis - Build FLUSH CACHE FIS
 */
int
flushcachefis(Sfis *m, uchar *c)
{
	int llba;

	llba = m->feat & Dllba;
	skelfis(c);
	c[Fflags] = Fiscmd;
	c[Fcmd] = llba ? 0xEA : 0xE7;	/* FLUSH CACHE [EXT] */
	c[Fdev] = Ataobs;
	return 0;
}

/*
 * nopfis - Build NOP FIS
 */
int
nopfis(Sfis *m, uchar *c, int srst)
{
	USED(m);
	skelfis(c);
	if(srst){
		c[Fflags] = 0;
		c[Fcontrol] = 4;	/* SRST */
	}else{
		c[Fflags] = Fiscmd;
		c[Fcmd] = 0;		/* NOP */
	}
	c[Fdev] = Ataobs;
	return 0;
}

/*
 * txmodefis - Build transfer mode SET FEATURES FIS
 */
int
txmodefis(Sfis *f, uchar *c, uchar mode)
{
	int um;

	/* Find highest UDMA mode */
	um = 7;
	while(um >= 0){
		if(f->udma & (1<<um))
			break;
		um--;
	}

	if(um >= 0 && mode == 0xFF)
		mode = 0x40 | um;	/* UDMA mode */
	else if(mode == 0xFF)
		return -1;

	skelfis(c);
	c[Fflags] = Fiscmd;
	c[Fcmd] = 0xEF;		/* SET FEATURES */
	c[Ffeat] = 3;		/* Set transfer mode */
	c[Fsc] = mode;
	c[Fdev] = Ataobs;
	return 0;
}

/*
 * rwfis - Build read/write FIS
 */
int
rwfis(Sfis *f, uchar *c, int write, int nsect, uvlong lba)
{
	int llba, cmd;

	llba = f->feat & Dllba;
	skelfis(c);
	c[Fflags] = Fiscmd;

	if(write){
		if(llba)
			cmd = 0x35;	/* WRITE DMA EXT */
		else
			cmd = 0xCA;	/* WRITE DMA */
	}else{
		if(llba)
			cmd = 0x25;	/* READ DMA EXT */
		else
			cmd = 0xC8;	/* READ DMA */
	}
	c[Fcmd] = cmd;

	/* LBA and sector count */
	c[Flba0] = lba;
	c[Flba8] = lba >> 8;
	c[Flba16] = lba >> 16;
	c[Flba24] = lba >> 24;
	c[Fsc] = nsect;
	c[Fdev] = Ataobs | (llba ? Atalba : ((lba>>24) & 0xF));

	if(llba){
		c[Flba32] = lba >> 32;
		c[Flba40] = lba >> 40;
		c[Fsc8] = nsect >> 8;
	}

	return 0;
}

/*
 * fisrw - Parse read/write FIS
 */
uvlong
fisrw(Sfis *f, uchar *c, int *nsect)
{
	uvlong lba;
	int llba;

	USED(f);
	llba = c[Fdev] & Atalba;

	lba = c[Flba0] | (c[Flba8]<<8) | (c[Flba16]<<16) | (c[Flba24]<<24);
	if(llba)
		lba |= (uvlong)c[Flba32]<<32 | (uvlong)c[Flba40]<<40;

	if(nsect)
		*nsect = c[Fsc] | (llba ? c[Fsc8]<<8 : 0);

	return lba;
}

/*
 * atapirwfis - Build ATAPI read/write FIS
 */
int
atapirwfis(Sfis *f, uchar *c, uchar *cmd, int len, int write)
{
	USED(f, cmd, len, write);
	/* ATAPI not implemented yet */
	return -1;
}

/*
 * idfeat - Extract features from IDENTIFY data
 */
vlong
idfeat(Sfis *f, ushort *id)
{
	vlong s;

	f->feat = 0;
	f->udma = 0;

	/* Check for LBA support */
	if(id[49] & (1<<9))
		f->feat |= Dlba;

	/* Check for LBA48 support */
	if(id[83] & (1<<10)){
		f->feat |= Dllba;
		s = id[100] | ((vlong)id[101]<<16) | ((vlong)id[102]<<32) | ((vlong)id[103]<<48);
	}else if(f->feat & Dlba){
		s = id[60] | (id[61]<<16);
	}else{
		/* CHS mode */
		f->c = id[1];
		f->h = id[3];
		f->s = id[6];
		s = f->c * f->h * f->s;
	}

	/* UDMA modes */
	if(id[53] & (1<<2))
		f->udma = id[88];

	/* Other features */
	if(id[83] & (1<<10))
		f->feat |= Dpower;
	if(id[82] & (1<<0))
		f->feat |= Dsmart;

	return s;
}

/*
 * idmove - Extract and clean ID string
 */
void
idmove(char *s, ushort *id, int len)
{
	int i;
	uchar *p, *e;

	p = (uchar*)s;
	for(i = 0; i < len/2; i++){
		p[2*i] = id[i] >> 8;
		p[2*i+1] = id[i];
	}

	/* Remove trailing spaces */
	e = p + len;
	while(e > p && (e[-1] == ' ' || e[-1] == '\0'))
		e--;
	*e = '\0';
}

/*
 * idwwn - Extract World Wide Name
 */
uvlong
idwwn(Sfis *f, ushort *id)
{
	USED(f);
	if((id[108] >> 12) != 5)
		return 0;
	return (uvlong)id[108]<<48 | (uvlong)id[109]<<32 | (uvlong)id[110]<<16 | id[111];
}

/*
 * idss - Get sector size
 */
int
idss(Sfis *f, ushort *id)
{
	USED(f);
	if((id[106] & 0xC000) != 0x4000)
		return 512;
	if((id[106] & (1<<12)) == 0)
		return 512;
	return id[117] | (id[118]<<16);
}

/*
 * idpuis - Check power-up in standby
 */
int
idpuis(ushort *id)
{
	return (id[83] & (1<<5)) != 0 ? Pspinup : 0;
}

/*
 * id16, id32, id64 - Extract ID values
 */
ushort
id16(ushort *id, int offset)
{
	return id[offset];
}

uint
id32(ushort *id, int offset)
{
	return id[offset] | (id[offset+1]<<16);
}

uvlong
id64(ushort *id, int offset)
{
	return (uvlong)id[offset] | ((uvlong)id[offset+1]<<16) |
	       ((uvlong)id[offset+2]<<32) | ((uvlong)id[offset+3]<<48);
}

/*
 * fistosig - Convert FIS to signature
 */
uint
fistosig(uchar *c)
{
	return c[Flba0] | (c[Flba8]<<8) | (c[Flba16]<<16) | (c[Flba24]<<24);
}
