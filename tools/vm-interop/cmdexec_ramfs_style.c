/*
 * cmdexec - Following ramfs.c pattern exactly
 */

#include <u.h>
#include <libc.h>
#include <fcall.h>
#include <thread.h>
#include <9p.h>

#define MAXCMD 8192

typedef struct Cmdfile Cmdfile;
struct Cmdfile {
	char *data;
	int ndata;
};

static void
fsread(Req *r)
{
	Cmdfile *cf;
	char buf[256];
	
	cf = r->fid->file->aux;
	if(cf == nil){
		/* status file - generate dynamically */
		snprint(buf, sizeof(buf), "cmdexec running\npid: %d\n", getpid());
		readstr(r, buf);
		respond(r, nil);
		return;
	}
	
	/* Regular file with data */
	if(cf->data == nil || cf->ndata == 0){
		r->ofcall.count = 0;
		respond(r, nil);
		return;
	}
	
	if(r->ifcall.offset >= cf->ndata){
		r->ofcall.count = 0;
		respond(r, nil);
		return;
	}
	
	long count = r->ifcall.count;
	if(r->ifcall.offset + count > cf->ndata)
		count = cf->ndata - r->ifcall.offset;
	
	memmove(r->ofcall.data, cf->data + r->ifcall.offset, count);
	r->ofcall.count = count;
	respond(r, nil);
}

static void
fswrite(Req *r)
{
	Cmdfile *cf;
	char cmd[MAXCMD];
	int n;
	
	cf = r->fid->file->aux;
	if(cf == nil){
		respond(r, "permission denied");
		return;
	}
	
	/* Get command */
	n = r->ifcall.count;
	if(n >= MAXCMD)
		n = MAXCMD - 1;
	memmove(cmd, r->ifcall.data, n);
	cmd[n] = '\0';
	
	/* Execute it */
	int p[2];
	if(pipe(p) < 0){
		respond(r, "pipe failed");
		return;
	}
	
	switch(fork()){
	case -1:
		close(p[0]);
		close(p[1]);
		respond(r, "fork failed");
		return;
		
	case 0:  /* child */
		close(p[0]);
		dup(p[1], 1);
		dup(p[1], 2);
		close(p[1]);
		execl("/bin/rc", "rc", "-c", cmd, nil);
		exits("exec failed");
		
	default:  /* parent */
		close(p[1]);
		
		/* Read output */
		free(cf->data);
		cf->data = malloc(8192);
		cf->ndata = read(p[0], cf->data, 8192);
		if(cf->ndata < 0)
			cf->ndata = 0;
		close(p[0]);
		
		wait();
		
		r->ofcall.count = r->ifcall.count;
		respond(r, nil);
	}
}

static void
fsdestroyfile(File *f)
{
	Cmdfile *cf;
	cf = f->aux;
	if(cf){
		free(cf->data);
		free(cf);
	}
}

static Srv fs = {
	.read = fsread,
	.write = fswrite,
};

void
main(int argc, char **argv)
{
	char *srvname = "cmdexec";
	char *mtpt = nil;
	Cmdfile *cf;
	
	ARGBEGIN{
	case 's':
		srvname = EARGF(usage());
		break;
	case 'm':
		mtpt = EARGF(usage());
		break;
	default:
		fprint(2, "usage: %s [-s srvname] [-m mtpt]\n", argv0);
		exits("usage");
	}ARGEND;
	
	fs.tree = alloctree(nil, nil, DMDIR|0777, fsdestroyfile);
	
	/* Create cmd file */
	cf = emalloc9p(sizeof *cf);
	createfile(fs.tree->root, "cmd", nil, 0666, cf);
	
	/* Create output file */
	cf = emalloc9p(sizeof *cf);
	createfile(fs.tree->root, "output", nil, 0444, cf);
	
	/* Create status file with nil aux */
	createfile(fs.tree->root, "status", nil, 0444, nil);
	
	fprint(2, "cmdexec: posting to /srv/%s\n", srvname);
	if(mtpt)
		fprint(2, "cmdexec: mounting on %s\n", mtpt);
	
	postmountsrv(&fs, srvname, mtpt, MREPL|MCREATE);
	exits(0);
}