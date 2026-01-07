/*
 * cmdring - Ring buffer command executor for Plan 9
 * Synthetic file implementation with atomic operations
 */

#include <u.h>
#include <libc.h>
#include <bio.h>
#include <fcall.h>
#include <thread.h>
#include <9p.h>

#define RINGSIZE 256  /* Power of 2 for fast modulo */
#define CMDMAX   1024

typedef struct Ring {
	char	*cmds[RINGSIZE];
	uint	head;	/* Next read position */
	uint	tail;	/* Next write position */
	Lock	lock;
} Ring;

static Ring ring;
static File *cmdfile;
static File *statusfile;

/* Atomic ring buffer operations */
static int
ring_empty(void)
{
	return ring.head == ring.tail;
}

static int
ring_full(void)
{
	return ((ring.tail + 1) & (RINGSIZE - 1)) == ring.head;
}

static char*
ring_get(void)
{
	char *cmd;
	
	lock(&ring.lock);
	if(ring_empty()){
		unlock(&ring.lock);
		return nil;
	}
	
	cmd = ring.cmds[ring.head];
	ring.head = (ring.head + 1) & (RINGSIZE - 1);
	unlock(&ring.lock);
	
	return cmd;
}

static int
ring_put(char *cmd)
{
	char *copy;
	
	lock(&ring.lock);
	if(ring_full()){
		unlock(&ring.lock);
		return -1;  /* Ring full */
	}
	
	copy = strdup(cmd);
	if(copy == nil){
		unlock(&ring.lock);
		return -1;
	}
	
	ring.cmds[ring.tail] = copy;
	ring.tail = (ring.tail + 1) & (RINGSIZE - 1);
	unlock(&ring.lock);
	
	return 0;
}

/* Synthetic file: /n/interop/cmdring */
static void
fsread(Req *r)
{
	char *cmd;
	
	if(r->fid->file == cmdfile){
		/* Read next command from ring */
		cmd = ring_get();
		if(cmd != nil){
			readstr(r, cmd);
			free(cmd);
		} else {
			readstr(r, "");  /* Empty ring */
		}
		respond(r, nil);
		return;
	}
	
	if(r->fid->file == statusfile){
		/* Generate status dynamically */
		char buf[128];
		int count;
		
		lock(&ring.lock);
		count = (ring.tail - ring.head) & (RINGSIZE - 1);
		unlock(&ring.lock);
		
		snprint(buf, sizeof(buf), "%d queued %d/%d\n", 
			count, ring.head, ring.tail);
		readstr(r, buf);
		respond(r, nil);
		return;
	}
	
	respond(r, "unknown file");
}

static void
fswrite(Req *r)
{
	char *cmd;
	int n;
	
	if(r->fid->file != cmdfile){
		respond(r, "permission denied");
		return;
	}
	
	/* Extract command from write request */
	n = r->ifcall.count;
	if(n >= CMDMAX){
		respond(r, "command too long");
		return;
	}
	
	cmd = malloc(n + 1);
	if(cmd == nil){
		respond(r, "no memory");
		return;
	}
	
	memmove(cmd, r->ifcall.data, n);
	cmd[n] = '\0';
	
	/* Strip trailing newline if present */
	if(n > 0 && cmd[n-1] == '\n')
		cmd[n-1] = '\0';
	
	/* Add to ring buffer */
	if(ring_put(cmd) < 0){
		free(cmd);
		respond(r, "ring buffer full");
		return;
	}
	
	free(cmd);
	r->ofcall.count = n;
	respond(r, nil);
}

/* Command executor thread */
static void
executor(void*)
{
	char *cmd;
	int p[2];
	Waitmsg *w;
	
	for(;;){
		cmd = ring_get();
		if(cmd == nil){
			sleep(100);  /* Nothing to do */
			continue;
		}
		
		/* Execute command */
		if(pipe(p) < 0){
			free(cmd);
			continue;
		}
		
		switch(fork()){
		case -1:
			close(p[0]);
			close(p[1]);
			break;
			
		case 0:  /* Child */
			close(p[0]);
			dup(p[1], 1);
			dup(p[1], 2);
			close(p[1]);
			execl("/bin/rc", "rc", "-c", cmd, nil);
			exits("exec");
			
		default:  /* Parent */
			close(p[1]);
			/* Could read output here */
			close(p[0]);
			w = wait();
			if(w != nil)
				free(w);
		}
		
		free(cmd);
	}
}

static Srv fs = {
	.read = fsread,
	.write = fswrite,
};

void
main(int argc, char *argv[])
{
	char *mtpt = "/n/cmdring";
	Tree *tree;
	
	ARGBEGIN{
	case 'm':
		mtpt = EARGF(usage());
		break;
	default:
		fprint(2, "usage: %s [-m mountpoint]\n", argv0);
		exits("usage");
	}ARGEND
	
	/* Initialize ring buffer */
	memset(&ring, 0, sizeof(ring));
	
	/* Create synthetic filesystem */
	tree = alloctree(nil, nil, DMDIR|0777, nil);
	cmdfile = createfile(tree->root, "cmd", nil, 0666, nil);
	statusfile = createfile(tree->root, "status", nil, 0444, nil);
	
	fs.tree = tree;
	
	/* Start executor thread */
	threadcreate(executor, nil, 8192);
	
	/* Mount and serve */
	threadpostmountsrv(&fs, nil, mtpt, MREPL);
	threadexits(nil);
}

/*
 * Usage:
 * echo "ls -la" > /n/cmdring/cmd     # Add command
 * cat /n/cmdring/cmd                  # Get next command (removes it)
 * cat /n/cmdring/status               # See queue status
 * 
 * Benefits over text file:
 * - Atomic operations (no race conditions)
 * - FIFO ordering guaranteed
 * - No line parsing needed
 * - No mtime checking
 * - Efficient wraparound
 * - Status monitoring
 */