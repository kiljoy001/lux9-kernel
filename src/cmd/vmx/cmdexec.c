/*
 * cmdexec - Command executor with TRUE synthetic files via lib9p
 * No more text files, no more newline issues, just pure 9P goodness!
 */

#include <u.h>
#include <libc.h>
#include <bio.h>
#include <fcall.h>
#include <thread.h>
#include <9p.h>

#define RINGSIZE 256  /* Power of 2 for fast modulo */
#define MAXCMD   8192
#define MAXOUT   65536

/* Ring buffer for commands */
typedef struct Ring {
	char	*cmds[RINGSIZE];
	char	*outputs[RINGSIZE];  /* Store outputs too */
	int	head;
	int	tail;
	Lock	lock;
} Ring;

static Ring cmdring;
static Ring outring;
static File *root;
static File *cmdfile;
static File *statusfile;
static File *outputfile;
static File *execfile;
static int cmdcount = 0;
static long starttime;
static int debug = 0;

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

static void
logmsg(char *fmt, ...)
{
	va_list args;
	char buf[256];
	
	if(!debug)
		return;
		
	va_start(args, fmt);
	vsnprint(buf, sizeof(buf), fmt, args);
	va_end(args);
	
	fprint(2, "[cmdexec] %s\n", buf);
}

/* Execute command and capture output */
static char*
runcmd(char *cmd)
{
	int p[2];
	Waitmsg *w;
	char *output;
	int n, total, capacity;
	
	logmsg("Executing: %s", cmd);
	
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

/* Synthetic file handlers */
static void
fsread(Req *r)
{
	char buf[8192];
	char *s;
	
	if(r->fid->file == cmdfile){
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
	
	if(r->fid->file == statusfile){
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
	
	if(r->fid->file == outputfile){
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
	
	if(r->fid->file == cmdfile){
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
		
		logmsg("Queued: %s", cmd);
		
		r->ofcall.count = r->ifcall.count;
		respond(r, nil);
		return;
	}
	
	if(r->fid->file == execfile){
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

/* Executor thread - processes queued commands */
static void
executor(void*)
{
	char *cmd;
	char *output;
	char status[256];
	char *timestr;
	long t;
	
	for(;;){
		cmd = ring_get(&cmdring);
		if(cmd == nil){
			sleep(100);
			continue;
		}
		
		/* Execute command */
		output = runcmd(cmd);
		cmdcount++;
		
		/* Format output with metadata */
		t = time(0);
		timestr = ctime(t);
		if(timestr != nil && strlen(timestr) > 0)
			timestr[strlen(timestr)-1] = '\0';
		
		snprint(status, sizeof(status),
			"===CMD: %s\n"
			"===TIME: %s\n"
			"===OUTPUT:\n%s\n"
			"===END===\n",
			cmd, timestr, output);
		
		/* Store in output ring */
		ring_put(&outring, status);
		
		free(cmd);
		free(output);
	}
}

static Srv fs = {
	.read = fsread,
	.write = fswrite,
};

void
threadmain(int argc, char *argv[])
{
	char *mtpt = "/n/cmdexec";
	Tree *tree;
	
	ARGBEGIN{
	case 'm':
		mtpt = ARGF();
		if(mtpt == nil){
			fprint(2, "usage: %s [-d] [-m mountpoint]\n", argv0);
			threadexits("usage");
		}
		break;
	case 'd':
		debug = 1;
		break;
	default:
		fprint(2, "usage: %s [-d] [-m mountpoint]\n", argv0);
		threadexits("usage");
	}ARGEND
	
	starttime = time(0);
	
	/* Create synthetic filesystem */
	tree = alloctree(nil, nil, DMDIR|0777, nil);
	
	/* Create synthetic files */
	cmdfile = createfile(tree->root, "cmd", nil, 0666, nil);
	statusfile = createfile(tree->root, "status", nil, 0444, nil);
	outputfile = createfile(tree->root, "output", nil, 0444, nil);
	execfile = createfile(tree->root, "exec", nil, 0666, nil);
	
	fs.tree = tree;
	
	/* Start executor thread */
	threadcreate(executor, nil, 32*1024);
	
	/* Mount and serve - this is where the magic happens! */
	threadpostmountsrv(&fs, nil, mtpt, MREPL);
	
	fprint(2, "cmdexec mounted at %s\n", mtpt);
	fprint(2, "Files:\n");
	fprint(2, "  %s/cmd     - write to queue, read to dequeue\n", mtpt);
	fprint(2, "  %s/exec    - write to execute immediately\n", mtpt);
	fprint(2, "  %s/status  - read for status\n", mtpt);
	fprint(2, "  %s/output  - read to get command outputs\n", mtpt);
	
	threadexits(nil);
}

/*
 * USAGE:
 * 
 * Start the server:
 *   cmdexec [-d] [-m /n/cmdexec]
 * 
 * Queue commands:
 *   echo "ls -la" > /n/cmdexec/cmd
 *   echo "ps" > /n/cmdexec/cmd
 * 
 * Execute immediately:
 *   echo "date" > /n/cmdexec/exec
 * 
 * Check status:
 *   cat /n/cmdexec/status
 * 
 * Get outputs:
 *   cat /n/cmdexec/output
 * 
 * NO MORE:
 * - Text file parsing
 * - Newline issues  
 * - mtime checking
 * - File locking
 * - Namespace problems
 * 
 * Just pure synthetic 9P files!
 */