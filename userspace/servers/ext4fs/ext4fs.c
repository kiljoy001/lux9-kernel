/* ext4fs - 9P server for ext4 filesystems using libext2fs */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>
#include <sys/stat.h>
#include <ext2fs/ext2fs.h>
#include "9p.h"

/* lib9p function prototypes */
uint convD2M(Stat*, uint8_t*, uint);
uint sizeD2M(Stat*);

#define MAXMSG		8192
#define MAXFID		1024

typedef struct Fid Fid;
struct Fid {
	uint32_t	fid;
	ext2_ino_t	ino;
	int		omode;
	Fid		*next;
};

static ext2_filsys fs;
static Fid *fids[MAXFID];
static char msgbuf[MAXMSG];
static uint32_t msize = MAXMSG;

/* FID management */
static Fid*
getfid(uint32_t fid)
{
	Fid *f;
	uint32_t h = fid % MAXFID;

	for(f = fids[h]; f; f = f->next)
		if(f->fid == fid)
			return f;
	return NULL;
}

static Fid*
newfid(uint32_t fid, ext2_ino_t ino)
{
	Fid *f;
	uint32_t h = fid % MAXFID;

	f = malloc(sizeof(Fid));
	if(f == NULL)
		return NULL;

	f->fid = fid;
	f->ino = ino;
	f->omode = -1;
	f->next = fids[h];
	fids[h] = f;
	return f;
}

static void
delfid(uint32_t fid)
{
	Fid *f, **fp;
	uint32_t h = fid % MAXFID;

	for(fp = &fids[h]; (f = *fp) != NULL; fp = &f->next) {
		if(f->fid == fid) {
			*fp = f->next;
			free(f);
			return;
		}
	}
}

/* Convert ext2 inode to 9P Qid */
static void
ino2qid(ext2_ino_t ino, struct ext2_inode *inode, Qid *qid)
{
	qid->path = ino;
	qid->vers = inode->i_generation;
	qid->type = QTFILE;
	if(LINUX_S_ISDIR(inode->i_mode))
		qid->type = QTDIR;
}

/* Convert ext2 inode to 9P Stat */
static int
ino2stat(ext2_ino_t ino, Stat *st)
{
	struct ext2_inode inode;
	char namebuf[256];
	errcode_t err;

	err = ext2fs_read_inode(fs, ino, &inode);
	if(err)
		return -1;

	memset(st, 0, sizeof(*st));
	st->type = 0;
	st->dev = 0;
	ino2qid(ino, &inode, &st->qid);

	st->mode = inode.i_mode & 0777;
	if(LINUX_S_ISDIR(inode.i_mode))
		st->mode |= DMDIR;

	st->atime = inode.i_atime;
	st->mtime = inode.i_mtime;
	st->length = EXT2_I_SIZE(&inode);

	snprintf(namebuf, sizeof(namebuf), "%u", ino);
	st->name = strdup(namebuf);
	st->uid = strdup("sys");
	st->gid = strdup("sys");
	st->muid = strdup("sys");

	return 0;
}

/* 9P message handlers */
static void
rversion(Fcall *tx, Fcall *rx)
{
	rx->type = Rversion;
	rx->tag = tx->tag;

	if(tx->version.msize < msize)
		msize = tx->version.msize;
	rx->version.msize = msize;
	rx->version.version = "9P2000";
}

static void
rattach(Fcall *tx, Fcall *rx)
{
	Fid *f;
	struct ext2_inode inode;

	f = newfid(tx->fid, EXT2_ROOT_INO);
	if(f == NULL) {
		rx->type = Rerror;
		rx->tag = tx->tag;
		rx->error.ename = "out of memory";
		return;
	}

	ext2fs_read_inode(fs, EXT2_ROOT_INO, &inode);

	rx->type = Rattach;
	rx->tag = tx->tag;
	ino2qid(EXT2_ROOT_INO, &inode, &rx->rattach.qid);
}

static void
rwalk(Fcall *tx, Fcall *rx)
{
	Fid *f, *nf;
	ext2_ino_t ino;
	struct ext2_inode inode;
	int i;
	errcode_t err;

	f = getfid(tx->fid);
	if(f == NULL) {
		rx->type = Rerror;
		rx->tag = tx->tag;
		rx->error.ename = "unknown fid";
		return;
	}

	ino = f->ino;

	/* Walk through path elements */
	for(i = 0; i < tx->twalk.nwname; i++) {
		err = ext2fs_read_inode(fs, ino, &inode);
		if(err || !LINUX_S_ISDIR(inode.i_mode)) {
			break;
		}

		/* Lookup name in directory */
		err = ext2fs_namei(fs, EXT2_ROOT_INO, ino, tx->twalk.wname[i], &ino);
		if(err)
			break;

		err = ext2fs_read_inode(fs, ino, &inode);
		if(err)
			break;

		ino2qid(ino, &inode, &rx->rwalk.wqid[i]);
	}

	rx->rwalk.nwqid = i;

	if(i == tx->twalk.nwname) {
		/* Success - create new fid if needed */
		if(tx->fid != tx->twalk.newfid) {
			nf = newfid(tx->twalk.newfid, ino);
			if(nf == NULL) {
				rx->type = Rerror;
				rx->tag = tx->tag;
				rx->error.ename = "out of memory";
				return;
			}
		} else {
			f->ino = ino;
		}
		rx->type = Rwalk;
		rx->tag = tx->tag;
	} else if(i == 0) {
		/* No progress */
		rx->type = Rerror;
		rx->tag = tx->tag;
		rx->error.ename = "file not found";
	} else {
		/* Partial walk is okay */
		rx->type = Rwalk;
		rx->tag = tx->tag;
	}
}

static void
ropen(Fcall *tx, Fcall *rx)
{
	Fid *f;
	struct ext2_inode inode;

	f = getfid(tx->fid);
	if(f == NULL) {
		rx->type = Rerror;
		rx->tag = tx->tag;
		rx->error.ename = "unknown fid";
		return;
	}

	ext2fs_read_inode(fs, f->ino, &inode);
	f->omode = tx->topen.mode;

	rx->type = Ropen;
	rx->tag = tx->tag;
	ino2qid(f->ino, &inode, &rx->ropen.qid);
	rx->ropen.iounit = fs->blocksize;
}

/* Directory reading helper */
struct dirread_ctx {
	uint8_t *buf;
	uint32_t size;
	uint32_t pos;
	uint64_t offset;
	uint64_t current;
};

static int
dirread_callback(struct ext2_dir_entry *dirent, int offset,
                 int blocksize, char *buf, void *priv)
{
	struct dirread_ctx *ctx = priv;
	Stat st;
	uint8_t statbuf[512];
	uint n;
	ext2_ino_t ino;

	(void)offset;
	(void)blocksize;
	(void)buf;

	ino = dirent->inode;
	if(ino == 0)
		return 0;

	/* Skip until we reach requested offset */
	if(ctx->current < ctx->offset) {
		ctx->current++;
		return 0;
	}

	/* Convert to stat */
	if(ino2stat(ino, &st) < 0)
		return 0;

	/* Override name with directory entry name */
	free(st.name);
	st.name = strndup(dirent->name, dirent->name_len & 0xFF);

	/* Marshal stat */
	n = convD2M(&st, statbuf, sizeof(statbuf));

	free(st.name);
	free(st.uid);
	free(st.gid);
	free(st.muid);

	if(n == 0 || ctx->pos + n > ctx->size)
		return DIRENT_ABORT;

	memcpy(ctx->buf + ctx->pos, statbuf, n);
	ctx->pos += n;
	ctx->current++;

	return 0;
}

static void
rread(Fcall *tx, Fcall *rx)
{
	Fid *f;
	ext2_file_t file;
	struct ext2_inode inode;
	unsigned int got;
	errcode_t err;

	f = getfid(tx->fid);
	if(f == NULL) {
		rx->type = Rerror;
		rx->tag = tx->tag;
		rx->error.ename = "unknown fid";
		return;
	}

	/* Check if reading a directory */
	err = ext2fs_read_inode(fs, f->ino, &inode);
	if(err) {
		rx->type = Rerror;
		rx->tag = tx->tag;
		rx->error.ename = "cannot read inode";
		return;
	}

	if(LINUX_S_ISDIR(inode.i_mode)) {
		/* Directory read - return directory entries as stats */
		struct dirread_ctx ctx;

		ctx.buf = malloc(tx->tread.count);
		if(ctx.buf == NULL) {
			rx->type = Rerror;
			rx->tag = tx->tag;
			rx->error.ename = "out of memory";
			return;
		}

		ctx.size = tx->tread.count;
		ctx.pos = 0;
		ctx.offset = tx->tread.offset;
		ctx.current = 0;

		err = ext2fs_dir_iterate(fs, f->ino, 0, NULL, dirread_callback, &ctx);
		if(err) {
			free(ctx.buf);
			rx->type = Rerror;
			rx->tag = tx->tag;
			rx->error.ename = "directory read error";
			return;
		}

		rx->type = Rread;
		rx->tag = tx->tag;
		rx->rread.count = ctx.pos;
		rx->rread.data = ctx.buf;
		return;
	}

	/* Regular file read */
	err = ext2fs_file_open(fs, f->ino, 0, &file);
	if(err) {
		rx->type = Rerror;
		rx->tag = tx->tag;
		rx->error.ename = "cannot open file";
		return;
	}

	/* Seek and read */
	ext2fs_file_llseek(file, tx->tread.offset, EXT2_SEEK_SET, NULL);

	rx->rread.data = malloc(tx->tread.count);
	if(rx->rread.data == NULL) {
		ext2fs_file_close(file);
		rx->type = Rerror;
		rx->tag = tx->tag;
		rx->error.ename = "out of memory";
		return;
	}

	err = ext2fs_file_read(file, rx->rread.data, tx->tread.count, &got);
	ext2fs_file_close(file);

	if(err) {
		free(rx->rread.data);
		rx->type = Rerror;
		rx->tag = tx->tag;
		rx->error.ename = "read error";
		return;
	}

	rx->type = Rread;
	rx->tag = tx->tag;
	rx->rread.count = got;
}

static void
rwrite(Fcall *tx, Fcall *rx)
{
	Fid *f;
	ext2_file_t file;
	unsigned int written;
	errcode_t err;

	f = getfid(tx->fid);
	if(f == NULL) {
		rx->type = Rerror;
		rx->tag = tx->tag;
		rx->error.ename = "unknown fid";
		return;
	}

	if(f->omode != OWRITE && f->omode != ORDWR) {
		rx->type = Rerror;
		rx->tag = tx->tag;
		rx->error.ename = "file not open for writing";
		return;
	}

	/* Open ext2 file for writing */
	err = ext2fs_file_open(fs, f->ino, EXT2_FILE_WRITE, &file);
	if(err) {
		rx->type = Rerror;
		rx->tag = tx->tag;
		rx->error.ename = "cannot open file";
		return;
	}

	/* Seek and write */
	ext2fs_file_llseek(file, tx->twrite.offset, EXT2_SEEK_SET, NULL);
	err = ext2fs_file_write(file, tx->twrite.data, tx->twrite.count, &written);

	if(err == 0)
		err = ext2fs_file_flush(file);

	ext2fs_file_close(file);

	if(err) {
		rx->type = Rerror;
		rx->tag = tx->tag;
		rx->error.ename = "write error";
		return;
	}

	rx->type = Rwrite;
	rx->tag = tx->tag;
	rx->rwrite.count = written;
}

static void
rstat(Fcall *tx, Fcall *rx)
{
	Fid *f;
	Stat st;
	uint8_t *statbuf;
	uint n;

	f = getfid(tx->fid);
	if(f == NULL) {
		rx->type = Rerror;
		rx->tag = tx->tag;
		rx->error.ename = "unknown fid";
		return;
	}

	if(ino2stat(f->ino, &st) < 0) {
		rx->type = Rerror;
		rx->tag = tx->tag;
		rx->error.ename = "cannot stat";
		return;
	}

	/* Marshal stat structure */
	statbuf = malloc(sizeD2M(&st) + 16);
	if(statbuf == NULL) {
		free(st.name);
		free(st.uid);
		free(st.gid);
		free(st.muid);
		rx->type = Rerror;
		rx->tag = tx->tag;
		rx->error.ename = "out of memory";
		return;
	}

	n = convD2M(&st, statbuf, sizeD2M(&st) + 16);
	if(n == 0) {
		free(statbuf);
		free(st.name);
		free(st.uid);
		free(st.gid);
		free(st.muid);
		rx->type = Rerror;
		rx->tag = tx->tag;
		rx->error.ename = "stat marshal error";
		return;
	}

	rx->type = Rstat;
	rx->tag = tx->tag;
	rx->stat.nstat = n;
	rx->stat.stat = statbuf;

	/* Free the string fields - they were copied into statbuf */
	free(st.name);
	free(st.uid);
	free(st.gid);
	free(st.muid);
}

static void
rcreate(Fcall *tx, Fcall *rx)
{
	Fid *f;
	ext2_ino_t newino, parent_ino;
	struct ext2_inode inode, parent_inode;
	errcode_t err;
	int is_dir;

	f = getfid(tx->fid);
	if(f == NULL) {
		rx->type = Rerror;
		rx->tag = tx->tag;
		rx->error.ename = "unknown fid";
		return;
	}

	parent_ino = f->ino;

	/* Parent must be a directory */
	err = ext2fs_read_inode(fs, parent_ino, &parent_inode);
	if(err || !LINUX_S_ISDIR(parent_inode.i_mode)) {
		rx->type = Rerror;
		rx->tag = tx->tag;
		rx->error.ename = "not a directory";
		return;
	}

	is_dir = (tx->tcreate.perm & DMDIR) != 0;

	/* Allocate new inode */
	err = ext2fs_new_inode(fs, parent_ino,
		is_dir ? LINUX_S_IFDIR : LINUX_S_IFREG, 0, &newino);
	if(err) {
		rx->type = Rerror;
		rx->tag = tx->tag;
		rx->error.ename = "cannot allocate inode";
		return;
	}

	/* Initialize inode */
	memset(&inode, 0, sizeof(inode));
	if(is_dir) {
		inode.i_mode = LINUX_S_IFDIR | (tx->tcreate.perm & 0777);
		inode.i_links_count = 2;  /* . and .. */
	} else {
		inode.i_mode = LINUX_S_IFREG | (tx->tcreate.perm & 0777);
		inode.i_links_count = 1;
	}
	inode.i_atime = inode.i_ctime = inode.i_mtime = time(NULL);
	inode.i_size = 0;

	err = ext2fs_write_new_inode(fs, newino, &inode);
	if(err) {
		rx->type = Rerror;
		rx->tag = tx->tag;
		rx->error.ename = "cannot write inode";
		return;
	}

	/* Link into parent directory */
	err = ext2fs_link(fs, parent_ino, tx->tcreate.name, newino,
		is_dir ? EXT2_FT_DIR : EXT2_FT_REG_FILE);
	if(err) {
		rx->type = Rerror;
		rx->tag = tx->tag;
		rx->error.ename = "cannot link file";
		return;
	}

	/* For directories, create . and .. entries */
	if(is_dir) {
		err = ext2fs_mkdir(fs, parent_ino, newino, tx->tcreate.name);
		if(err) {
			ext2fs_unlink(fs, parent_ino, tx->tcreate.name, newino, 0);
			rx->type = Rerror;
			rx->tag = tx->tag;
			rx->error.ename = "cannot create directory";
			return;
		}
		/* ext2fs_mkdir already linked it, so unlink our duplicate */
		ext2fs_unlink(fs, parent_ino, tx->tcreate.name, newino, 0);
	}

	ext2fs_inode_alloc_stats2(fs, newino, 1, is_dir);
	ext2fs_mark_ib_dirty(fs);
	ext2fs_mark_bb_dirty(fs);

	/* Update fid to point to new file */
	f->ino = newino;
	f->omode = tx->tcreate.mode;

	/* Read back the inode for qid */
	ext2fs_read_inode(fs, newino, &inode);

	rx->type = Rcreate;
	rx->tag = tx->tag;
	ino2qid(newino, &inode, &rx->ropen.qid);
	rx->ropen.iounit = fs->blocksize;
}

static void
rremove(Fcall *tx, Fcall *rx)
{
	Fid *f;
	errcode_t err;

	f = getfid(tx->fid);
	if(f == NULL) {
		rx->type = Rerror;
		rx->tag = tx->tag;
		rx->error.ename = "unknown fid";
		return;
	}

	/* Unlink the inode */
	err = ext2fs_unlink(fs, EXT2_ROOT_INO, NULL, f->ino, 0);
	if(err) {
		rx->type = Rerror;
		rx->tag = tx->tag;
		rx->error.ename = "cannot remove file";
		return;
	}

	delfid(tx->fid);

	rx->type = Rremove;
	rx->tag = tx->tag;
}

static void
rclunk(Fcall *tx, Fcall *rx)
{
	delfid(tx->fid);
	rx->type = Rclunk;
	rx->tag = tx->tag;
}

/* Main server loop */
static void
serve(void)
{
	Fcall tx, rx;
	int n, m;

	for(;;) {
		/* Read message */
		n = read(0, msgbuf, 4);
		if(n != 4)
			break;

		m = msgbuf[0] | (msgbuf[1]<<8) | (msgbuf[2]<<16) | (msgbuf[3]<<24);
		if(m > msize)
			break;

		n = read(0, msgbuf+4, m-4);
		if(n != m-4)
			break;

		/* Parse message */
		if(convM2S((uint8_t*)msgbuf, m, &tx) <= 0)
			continue;

		/* Dispatch */
		memset(&rx, 0, sizeof(rx));
		switch(tx.type) {
		case Tversion:	rversion(&tx, &rx); break;
		case Tattach:	rattach(&tx, &rx); break;
		case Twalk:	rwalk(&tx, &rx); break;
		case Topen:	ropen(&tx, &rx); break;
		case Tread:	rread(&tx, &rx); break;
		case Twrite:	rwrite(&tx, &rx); break;
		case Tcreate:	rcreate(&tx, &rx); break;
		case Tremove:	rremove(&tx, &rx); break;
		case Tstat:	rstat(&tx, &rx); break;
		case Tclunk:	rclunk(&tx, &rx); break;
		default:
			rx.type = Rerror;
			rx.tag = tx.tag;
			rx.error.ename = "unknown message";
			break;
		}

		/* Send reply */
		m = convS2M(&rx, (uint8_t*)msgbuf, msize);
		if(m > 0)
			write(1, msgbuf, m);

		freefcall(&tx);
		freefcall(&rx);
	}
}

int
main(int argc, char *argv[])
{
	errcode_t err;

	if(argc != 2) {
		fprintf(stderr, "usage: %s device\n", argv[0]);
		return 1;
	}

	/* Open ext4 filesystem with write support */
	err = ext2fs_open(argv[1], EXT2_FLAG_RW, 0, 0, unix_io_manager, &fs);
	if(err) {
		fprintf(stderr, "cannot open %s: %s\n", argv[1], error_message(err));
		return 1;
	}

	/* Serve 9P on stdin/stdout */
	serve();

	ext2fs_close(fs);
	return 0;
}
