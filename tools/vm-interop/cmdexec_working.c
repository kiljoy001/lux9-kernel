/*
 * cmdexec - Command executor with synthetic files via lib9p
 * Based on the working ramfs.c pattern from 9front
 */

#include <u.h>
#include <libc.h>
#include <fcall.h>
#include <thread.h>
#include <9p.h>

#define RINGSIZE 256  /* Power of 2 for fast modulo */
#define MAXCMD   8192
#define MAXOUT   65536

/* Ring buffer for commands */
typedef struct Ring {
	char	*cmds[RINGSIZE];
	char	*outputs[RINGSIZE];
	int	head;
	int	tail;
	Lock	lock;
} Ring;

typedef struct Cmdfile {
	char *data;
	int ndata;
} Cmdfile;

static Ring cmdring;
static Ring outring;
static int cmdcount = 0;
static long starttime;

/* Ring buffer operations */
static int
ring_empty(Ring *r)
{
	return r->head == r->tail;
}

static int
ring_full(Ring *r)
{
	return ((r->tail + 1) & (RINGSIZE - 1)) == r->head;
}

static char*
ring_get(Ring *r)
{
	char *s;
	
	lock(&r->lock);
	if(ring_empty(r)){
		unlock(&r->lock);
		return nil;
	}
	
	s = r->cmds[r->head];
	r->head = (r->head + 1) & (RINGSIZE - 1);
	unlock(&r->lock);
	
	return s;
}

static int
ring_put(Ring *r, char *s)
{
	char *copy;
	
	lock(&r->lock);
	if(ring_full(r)){
		/* Drop oldest */
		free(r->cmds[r->head]);
		r->head = (r->head + 1) & (RINGSIZE - 1);
	}
	
	copy = strdup(s);
	if(copy == nil){
		unlock(&r->lock);
		return -1;
	}
	
	r->cmds[r->tail] = copy;
	r->tail = (r->tail + 1) & (RINGSIZE - 1);
	unlock(&r->lock);
	
	return 0;
}

static int
ring_count(Ring *r)
{
	int n;
	lock(&r->lock);
	n = (r->tail - r->head) & (RINGSIZE - 1);
	unlock(&r->lock);
	return n;
}

/* Execute command and capture output */
static char*
runcmd(char *cmd)
{
	int p[2];
	Waitmsg *w;
	char *output;
	int n, total, capacity;
	
	if(pipe(p) < 0)
		return strdup("pipe failed");
	
	switch(fork()){
	case -1:
		close(p[0]);
		close(p[1]);
		return strdup("fork failed");
		
	case 0:  /* child */
		close(p[0]);
		dup(p[1], 1);
		dup(p[1], 2);
		close(p[1]);
		execl("/bin/rc", "rc", "-c", cmd, nil);
		exits("exec failed");
		
	default:  /* parent */
		close(p[1]);
		
		capacity = 4096;
		output = malloc(capacity);
		if(output == nil){
			close(p[0]);
			return strdup("malloc failed");
		}
		
		total = 0;
		while((n = read(p[0], output + total, capacity - total - 1)) > 0){
			total += n;
			if(total >= capacity - 1){
				if(capacity >= MAXOUT)
					break;
				capacity *= 2;
				if(capacity > MAXOUT)
					capacity = MAXOUT;
				output = realloc(output, capacity);
				if(output == nil){
					close(p[0]);
					return strdup("realloc failed");
				}
			}
		}
		output[total] = '\0';
		close(p[0]);
		
		w = wait();
		if(w != nil){
			if(w->msg[0] != '\0'){
				/* Append exit status */
				char *tmp = smprint("%s\nstatus: %s", output, w->msg);
				free(output);
				output = tmp;
			}
			free(w);
		}
		
		return output;
	}
}

/* 9P handlers */
static void
fsread(Req *r)
{
	char buf[8192];
	char *s;
	Cmdfile *cf;
	
	cf = r->fid->file->aux;
	if(cf && cf->data){
		readbuf(r, cf->data, cf->ndata);
		respond(r, nil);
		return;
	}
	
	/* Handle different files based on name */
	if(strcmp(r->fid->file->name, "cmd") == 0){
		/* Read next command from queue */
		s = ring_get(&cmdring);
		if(s != nil){
			readstr(r, s);
			free(s);
		} else {
			readstr(r, "");
		}
		respond(r, nil);
		return;
	}
	
	if(strcmp(r->fid->file->name, "status") == 0){
		/* Generate status dynamically */
		long uptime = time(0) - starttime;
		snprint(buf, sizeof(buf), 
			"pid: %d\n"
			"uptime: %ld seconds\n"
			"commands executed: %d\n"
			"commands queued: %d\n"
			"outputs available: %d\n",
			getpid(), uptime, cmdcount,
			ring_count(&cmdring), ring_count(&outring));
		readstr(r, buf);
		respond(r, nil);
		return;
	}
	
	if(strcmp(r->fid->file->name, "output") == 0){
		/* Read next output from ring */
		s = ring_get(&outring);
		if(s != nil){
			readstr(r, s);
			free(s);
		} else {
			readstr(r, "");
		}
		respond(r, nil);
		return;
	}
	
	respond(r, "unknown file");
}

static void
fswrite(Req *r)
{
	char cmd[MAXCMD];
	int n;
	
	if(strcmp(r->fid->file->name, "cmd") == 0){
		/* Add command to queue */
		n = r->ifcall.count;
		if(n >= MAXCMD)
			n = MAXCMD - 1;
		memmove(cmd, r->ifcall.data, n);
		cmd[n] = '\0';
		
		/* Strip trailing newline */
		if(n > 0 && cmd[n-1] == '\n')
			cmd[n-1] = '\0';
		
		if(ring_put(&cmdring, cmd) < 0){
			respond(r, "ring full");
			return;
		}
		
		r->ofcall.count = r->ifcall.count;
		respond(r, nil);
		return;
	}
	
	if(strcmp(r->fid->file->name, "exec") == 0){
		/* Execute command immediately */
		n = r->ifcall.count;
		if(n >= MAXCMD)
			n = MAXCMD - 1;
		memmove(cmd, r->ifcall.data, n);
		cmd[n] = '\0';
		
		if(n > 0 && cmd[n-1] == '\n')
			cmd[n-1] = '\0';
		
		char *output = runcmd(cmd);
		cmdcount++;
		
		/* Store output */
		ring_put(&outring, output);
		free(output);
		
		r->ofcall.count = r->ifcall.count;
		respond(r, nil);
		return;
	}
	
	respond(r, "permission denied");
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

static void
usage(void)
{
	fprint(2, "usage: cmdexec [-D] [-s srvname] [-m mtpt]\n");
	exits("usage");
}

void
main(int argc, char *argv[])
{
	char *srvname = "cmdexec";
	char *mtpt = "/n/cmdexec";
	
	ARGBEGIN{
	case 'D':
		chatty9p++;
		break;
	case 's':
		srvname = EARGF(usage());
		break;
	case 'm':
		mtpt = EARGF(usage());
		break;
	default:
		usage();
	}ARGEND;
	
	if(argc)
		usage();
	
	starttime = time(0);
	
	/* Create tree exactly like ramfs does */
	fs.tree = alloctree(nil, nil, DMDIR|0777, fsdestroyfile);
	
	/* Create synthetic files */
	createfile(fs.tree->root, "cmd", nil, 0666, nil);
	createfile(fs.tree->root, "status", nil, 0444, nil);
	createfile(fs.tree->root, "output", nil, 0444, nil);
	createfile(fs.tree->root, "exec", nil, 0666, nil);
	
	if(chatty9p)
		fprint(2, "cmdexec: srvname %s mtpt %s\n", srvname, mtpt);
	
	/* This is the exact pattern from ramfs */
	if(srvname || mtpt)
		postmountsrv(&fs, srvname, mtpt, MREPL|MCREATE);
	exits(0);
}