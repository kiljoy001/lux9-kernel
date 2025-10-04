/* 9P2000 Protocol Definitions */
#ifndef _9P_H_
#define _9P_H_

#include <stdint.h>

/* 9P2000 message types */
enum {
	Tversion =	100,
	Rversion,
	Tauth =		102,
	Rauth,
	Tattach =	104,
	Rattach,
	Terror =	106,	/* illegal */
	Rerror,
	Tflush =	108,
	Rflush,
	Twalk =		110,
	Rwalk,
	Topen =		112,
	Ropen,
	Tcreate =	114,
	Rcreate,
	Tread =		116,
	Rread,
	Twrite =	118,
	Rwrite,
	Tclunk =	120,
	Rclunk,
	Tremove =	122,
	Rremove,
	Tstat =		124,
	Rstat,
	Twstat =	126,
	Rwstat,
	Tmax,
};

/* Qid types */
enum {
	QTDIR =		0x80,
	QTAPPEND =	0x40,
	QTEXCL =	0x20,
	QTMOUNT =	0x10,
	QTAUTH =	0x08,
	QTTMP =		0x04,
	QTSYMLINK =	0x02,
	QTFILE =	0x00,
};

/* Open/Create modes */
enum {
	OREAD =		0,
	OWRITE =	1,
	ORDWR =		2,
	OEXEC =		3,
	OTRUNC =	0x10,
	OCEXEC =	0x20,
	ORCLOSE =	0x40,
	OEXCL =		0x1000,
};

/* Permissions */
enum {
	DMDIR =		0x80000000,
	DMAPPEND =	0x40000000,
	DMEXCL =	0x20000000,
	DMMONT =	0x10000000,
	DMAUTH =	0x08000000,
	DMTMP =		0x04000000,
	DMSYMLINK =	0x02000000,
	DMDEVICE =	0x00800000,
	DMNAMEDPIPE =	0x00200000,
	DMSOCKET =	0x00100000,
	DMSETUID =	0x00080000,
	DMSETGID =	0x00040000,
	DMSETVTX =	0x00010000,
};

#define NOTAG		(uint16_t)~0U
#define NOFID		(uint32_t)~0U
#define IOHDRSZ		24

typedef struct Qid Qid;
typedef struct Stat Stat;
typedef struct Fcall Fcall;

struct Qid {
	uint8_t		type;
	uint32_t	vers;
	uint64_t	path;
};

struct Stat {
	uint16_t	type;
	uint32_t	dev;
	Qid		qid;
	uint32_t	mode;
	uint32_t	atime;
	uint32_t	mtime;
	uint64_t	length;
	char		*name;
	char		*uid;
	char		*gid;
	char		*muid;
};

struct Fcall {
	uint8_t		type;
	uint16_t	tag;
	uint32_t	fid;
	union {
		struct {
			uint32_t	msize;
			char		*version;
		} version;
		struct {
			uint32_t	afid;
			char		*uname;
			char		*aname;
		} tattach;
		struct {
			Qid		qid;
		} rattach;
		struct {
			char		*ename;
		} error;
		struct {
			uint32_t	newfid;
			uint16_t	nwname;
			char		*wname[16];
		} twalk;
		struct {
			uint16_t	nwqid;
			Qid		wqid[16];
		} rwalk;
		struct {
			uint8_t		mode;
		} topen;
		struct {
			char		*name;
			uint32_t	perm;
			uint8_t		mode;
		} tcreate;
		struct {
			Qid		qid;
			uint32_t	iounit;
		} ropen;
		struct {
			uint64_t	offset;
			uint32_t	count;
		} tread;
		struct {
			uint32_t	count;
			uint8_t		*data;
		} rread;
		struct {
			uint64_t	offset;
			uint32_t	count;
			uint8_t		*data;
		} twrite;
		struct {
			uint32_t	count;
		} rwrite;
		struct {
			uint16_t	nstat;
			uint8_t		*stat;
		} stat;
	};
};

/* Function prototypes */
int	convM2S(uint8_t*, uint, Fcall*);
int	convS2M(Fcall*, uint8_t*, uint);
int	sizeS2M(Fcall*);
void	freefcall(Fcall*);

#endif /* _9P_H_ */
