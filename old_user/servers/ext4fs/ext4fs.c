/* ext4fs - 9P server for ext4 filesystems using libext2fs */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>
#include <inttypes.h>
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
	ext2_ino_t	parent;
	char		*name;
	int		omode;
	Fid		*next;
};

static ext2_filsys fs;
static Fid *fids[MAXFID];
static char msgbuf[MAXMSG];
static uint32_t msize = MAXMSG;

static int
read_full(int fd, void *buf, size_t len)
{
        size_t off = 0;

        while(off < len) {
                ssize_t n = read(fd, (uint8_t*)buf + off, len - off);
                if(n <= 0)
                        return -1;
                off += n;
        }

        return 0;
}

static int
write_full(int fd, const void *buf, size_t len)
{
        size_t off = 0;

        while(off < len) {
                ssize_t n = write(fd, (const uint8_t*)buf + off, len - off);
                if(n <= 0)
                        return -1;
                off += n;
        }

        return 0;
}

static void
seterror(Fcall *rx, uint16_t tag, const char *msg)
{
        char *dup;

        dup = strdup(msg);
        if(dup == NULL){
                dup = malloc(1);
                if(dup != NULL)
                        dup[0] = '\0';
        }

        rx->type = Rerror;
        rx->tag = tag;
        rx->error.ename = dup;
}

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
        f->parent = 0;
        f->name = NULL;
        f->omode = -1;
        f->next = fids[h];
        fids[h] = f;
        return f;
}

static int
fid_set_path(Fid *f, ext2_ino_t parent, const char *name)
{
        char *dup = NULL;

        if(name != NULL) {
                dup = strdup(name);
                if(dup == NULL)
                        return -1;
        }

        free(f->name);
        f->name = dup;
        f->parent = parent;
        return 0;
}

static int
fid_clone(Fid *dst, const Fid *src)
{
        if(fid_set_path(dst, src->parent, src->name) < 0)
                return -1;

        dst->ino = src->ino;
        dst->omode = src->omode;
        return 0;
}

static void
delfid(uint32_t fid)
{
        Fid *f, **fp;
        uint32_t h = fid % MAXFID;

        for(fp = &fids[h]; (f = *fp) != NULL; fp = &f->next) {
                if(f->fid == fid) {
                        *fp = f->next;
                        free(f->name);
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
        char namebuf[32];
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

	snprintf(namebuf, sizeof(namebuf), "%" PRIu64, (uint64_t)ino);
	st->name = strdup(namebuf);
	st->uid = strdup("sys");
	st->gid = strdup("sys");
	st->muid = strdup("sys");

	if(st->name == NULL || st->uid == NULL || st->gid == NULL || st->muid == NULL) {
		free(st->name);
		free(st->uid);
		free(st->gid);
		free(st->muid);
		memset(st, 0, sizeof(*st));
		return -1;
	}

	return 0;
}

/* 9P message handlers */
static void
rversion(Fcall *tx, Fcall *rx)
{
        rx->tag = tx->tag;

        if(tx->version.msize < msize)
                msize = tx->version.msize;
        rx->version.msize = msize;
        rx->version.version = strdup("9P2000");
        if(rx->version.version == NULL) {
                seterror(rx, tx->tag, "out of memory");
                return;
        }

        rx->type = Rversion;
}

static void
rattach(Fcall *tx, Fcall *rx)
{
        Fid *f;
        struct ext2_inode inode;
        errcode_t err;

        f = newfid(tx->fid, EXT2_ROOT_INO);
        if(f == NULL) {
                seterror(rx, tx->tag, "out of memory");
                return;
        }

        err = ext2fs_read_inode(fs, EXT2_ROOT_INO, &inode);
        if(err) {
                delfid(tx->fid);
                seterror(rx, tx->tag, "cannot read root inode");
                return;
        }

        if(fid_set_path(f, 0, NULL) < 0) {
                delfid(tx->fid);
                seterror(rx, tx->tag, "out of memory");
                return;
        }

        rx->type = Rattach;
        rx->tag = tx->tag;
        ino2qid(EXT2_ROOT_INO, &inode, &rx->rattach.qid);
}

static void
rwalk(Fcall *tx, Fcall *rx)
{
	Fid *f, *nf;
	ext2_ino_t ino, parent;
	struct ext2_inode inode;
	int i;
	errcode_t err;

	f = getfid(tx->fid);
	if(f == NULL) {
		seterror(rx, tx->tag, "unknown fid");
		return;
	}

	ino = f->ino;
	parent = f->parent;

	/* Walk through path elements */
	for(i = 0; i < tx->twalk.nwname; i++) {
		ext2_ino_t next;

		err = ext2fs_read_inode(fs, ino, &inode);
		if(err || !LINUX_S_ISDIR(inode.i_mode)) {
			break;
		}

		/* Lookup name in directory */
		err = ext2fs_namei(fs, EXT2_ROOT_INO, ino, tx->twalk.wname[i], &next);
		if(err)
			break;

		err = ext2fs_read_inode(fs, next, &inode);
		if(err)
			break;

		ino2qid(next, &inode, &rx->rwalk.wqid[i]);
		parent = ino;
		ino = next;
	}

	rx->rwalk.nwqid = i;

	if(i == tx->twalk.nwname) {
		/* Success - create new fid if needed */
		if(tx->fid != tx->twalk.newfid) {
			nf = newfid(tx->twalk.newfid, ino);
			if(nf == NULL) {
				seterror(rx, tx->tag, "out of memory");
				return;
			}
			if(tx->twalk.nwname == 0) {
				if(fid_clone(nf, f) < 0) {
					delfid(nf->fid);
					seterror(rx, tx->tag, "out of memory");
					return;
				}
			}
		} else {
			f->ino = ino;
			nf = f;
		}

		if(tx->twalk.nwname > 0) {
			const char *leaf = tx->twalk.wname[tx->twalk.nwname - 1];
			if(fid_set_path(nf, parent, leaf) < 0) {
				if(nf != f)
					delfid(nf->fid);
				seterror(rx, tx->tag, "out of memory");
				return;
			}
		}

		rx->type = Rwalk;
		rx->tag = tx->tag;
	} else if(i == 0) {
		/* No progress */
		seterror(rx, tx->tag, "file not found");
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
                seterror(rx, tx->tag, "unknown fid");
                return;
        }

        if(ext2fs_read_inode(fs, f->ino, &inode)) {
                seterror(rx, tx->tag, "cannot read inode");
                return;
        }
        f->omode = tx->topen.mode & 0x3;

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
        uint64_t next_cookie;
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

        /* Skip entries that end before the requested offset */
        if((uint64_t)(offset + dirent->rec_len) <= ctx->offset)
                return 0;

        /* Convert to stat */
        if(ino2stat(ino, &st) < 0)
                return 0;

        /* Override name with directory entry name */
        free(st.name);
        st.name = strndup(dirent->name, dirent->name_len & 0xFF);
        if(st.name == NULL) {
                free(st.uid);
                free(st.gid);
                free(st.muid);
                return DIRENT_ABORT;
        }

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
        ctx->next_cookie = (uint64_t)offset + dirent->rec_len;

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
                seterror(rx, tx->tag, "unknown fid");
                return;
        }

	/* Check if reading a directory */
        err = ext2fs_read_inode(fs, f->ino, &inode);
        if(err) {
                seterror(rx, tx->tag, "cannot read inode");
                return;
        }

        if(LINUX_S_ISDIR(inode.i_mode)) {
                /* Directory read - return directory entries as stats */
                struct dirread_ctx ctx;

                if(tx->tread.count == 0) {
                        rx->type = Rread;
                        rx->tag = tx->tag;
                        rx->rread.count = 0;
                        rx->rread.data = NULL;
                        return;
                }

                ctx.buf = malloc(tx->tread.count);
                if(ctx.buf == NULL) {
                        seterror(rx, tx->tag, "out of memory");
                        return;
                }

                ctx.size = tx->tread.count;
                ctx.pos = 0;
                ctx.offset = tx->tread.offset;
                ctx.next_cookie = tx->tread.offset;

                err = ext2fs_dir_iterate(fs, f->ino, 0, NULL, dirread_callback, &ctx);
                if(err) {
                        free(ctx.buf);
                        seterror(rx, tx->tag, "directory read error");
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
                seterror(rx, tx->tag, "cannot open file");
                return;
        }

	/* Seek and read */
	ext2fs_file_llseek(file, tx->tread.offset, EXT2_SEEK_SET, NULL);

        if(tx->tread.count == 0) {
                rx->type = Rread;
                rx->tag = tx->tag;
                rx->rread.count = 0;
                rx->rread.data = NULL;
                ext2fs_file_close(file);
                return;
        }

        rx->rread.data = malloc(tx->tread.count);
        if(rx->rread.data == NULL) {
                ext2fs_file_close(file);
                seterror(rx, tx->tag, "out of memory");
                return;
        }

	err = ext2fs_file_read(file, rx->rread.data, tx->tread.count, &got);
	ext2fs_file_close(file);

        if(err) {
                free(rx->rread.data);
                seterror(rx, tx->tag, "read error");
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
                seterror(rx, tx->tag, "unknown fid");
                return;
        }

        if(f->omode != OWRITE && f->omode != ORDWR) {
                seterror(rx, tx->tag, "file not open for writing");
                return;
        }

	/* Open ext2 file for writing */
        err = ext2fs_file_open(fs, f->ino, EXT2_FILE_WRITE, &file);
        if(err) {
                seterror(rx, tx->tag, "cannot open file");
                return;
        }

	/* Seek and write */
	ext2fs_file_llseek(file, tx->twrite.offset, EXT2_SEEK_SET, NULL);
	err = ext2fs_file_write(file, tx->twrite.data, tx->twrite.count, &written);

	if(err == 0)
		err = ext2fs_file_flush(file);

	ext2fs_file_close(file);

        if(err) {
                seterror(rx, tx->tag, "write error");
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
                seterror(rx, tx->tag, "unknown fid");
                return;
        }

        if(ino2stat(f->ino, &st) < 0) {
                seterror(rx, tx->tag, "cannot stat");
                return;
        }

	/* Marshal stat structure */
        statbuf = malloc(sizeD2M(&st) + 16);
        if(statbuf == NULL) {
                free(st.name);
                free(st.uid);
                free(st.gid);
                free(st.muid);
                seterror(rx, tx->tag, "out of memory");
                return;
        }

	n = convD2M(&st, statbuf, sizeD2M(&st) + 16);
        if(n == 0) {
                free(statbuf);
                free(st.name);
                free(st.uid);
                free(st.gid);
                free(st.muid);
                seterror(rx, tx->tag, "stat marshal error");
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
        int stats_recorded = 0;
        int linked = 0;
        const char *errmsg = NULL;

        f = getfid(tx->fid);
        if(f == NULL) {
                seterror(rx, tx->tag, "unknown fid");
                return;
        }

        parent_ino = f->ino;

        /* Parent must be a directory */
        err = ext2fs_read_inode(fs, parent_ino, &parent_inode);
        if(err || !LINUX_S_ISDIR(parent_inode.i_mode)) {
                seterror(rx, tx->tag, "not a directory");
                return;
        }

        is_dir = (tx->tcreate.perm & DMDIR) != 0;

        /* Allocate new inode */
        err = ext2fs_new_inode(fs, parent_ino,
                is_dir ? LINUX_S_IFDIR : LINUX_S_IFREG, 0, &newino);
        if(err) {
                seterror(rx, tx->tag, "cannot allocate inode");
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
                errmsg = "cannot write inode";
                goto error;
        }

        if(is_dir)
                err = ext2fs_mkdir(fs, parent_ino, newino, tx->tcreate.name);
        else
                err = ext2fs_link(fs, parent_ino, tx->tcreate.name, newino, EXT2_FT_REG_FILE);
        if(err) {
                errmsg = is_dir ? "cannot create directory" : "cannot link file";
                goto error;
        }
        linked = 1;

        ext2fs_inode_alloc_stats2(fs, newino, 1, is_dir);
        stats_recorded = 1;
        ext2fs_mark_ib_dirty(fs);
        ext2fs_mark_bb_dirty(fs);

        err = ext2fs_read_inode(fs, newino, &inode);
        if(err) {
                errmsg = "cannot read inode";
                goto error;
        }

        if(fid_set_path(f, parent_ino, tx->tcreate.name) < 0) {
                errmsg = "out of memory";
                goto error;
        }

        f->ino = newino;
        f->omode = tx->tcreate.mode & 0x3;

        rx->type = Rcreate;
        rx->tag = tx->tag;
        ino2qid(newino, &inode, &rx->ropen.qid);
        rx->ropen.iounit = fs->blocksize;
        return;

error:
        if(linked)
                ext2fs_unlink(fs, parent_ino, tx->tcreate.name, newino, 0);
        if(stats_recorded)
                ext2fs_inode_alloc_stats2(fs, newino, -1, is_dir);
        if(errmsg == NULL)
                errmsg = "create failed";
        seterror(rx, tx->tag, errmsg);
}


static void
rremove(Fcall *tx, Fcall *rx)
{
        Fid *f;
        errcode_t err;

        f = getfid(tx->fid);
        if(f == NULL) {
                seterror(rx, tx->tag, "unknown fid");
                return;
        }

        if(f->name == NULL) {
                seterror(rx, tx->tag, "cannot remove root");
                return;
        }

        /* Unlink the inode */
        err = ext2fs_unlink(fs, f->parent, f->name, f->ino, 0);
        if(err) {
                seterror(rx, tx->tag, "cannot remove file");
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
        uint32_t m;

        for(;;) {
                /* Read message */
                if(read_full(0, msgbuf, 4) < 0)
                        return;

                m = (uint32_t)(uint8_t)msgbuf[0]
                        | ((uint32_t)(uint8_t)msgbuf[1] << 8)
                        | ((uint32_t)(uint8_t)msgbuf[2] << 16)
                        | ((uint32_t)(uint8_t)msgbuf[3] << 24);
                if(m > msize || m < 4)
                        return;

                if(read_full(0, msgbuf + 4, m - 4) < 0)
                        return;

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
                        seterror(&rx, tx.tag, "unknown message");
                        break;
                }

                /* Send reply */
                m = convS2M(&rx, (uint8_t*)msgbuf, msize);
                if(m > 0 && write_full(1, msgbuf, m) < 0)
                        return;

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
