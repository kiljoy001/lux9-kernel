/*
 * cmdexec_9p - Command executor with TRUE synthetic files via lib9p
 * Enhanced with blockchain JSON API for network-accessible command execution
 * - 9P filesystem at /n/cmdexec (local access)
 * - TCP JSON API on port 9999 (blockchain integration)
 */

#include <u.h>
#include <libc.h>
#include <bio.h>
#include <fcall.h>
#include <thread.h>
#include <9p.h>
#include <ip.h>

#define RINGSIZE 256  /* Power of 2 for fast modulo */
#define MAXCMD   8192
#define MAXOUT   65536
#define BLOCKCHAIN_PORT 9999  /* TCP port for blockchain JSON API */

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
static int blockchain_fd = -1;  /* TCP server socket for blockchain API */

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

	/* Clean the command string - remove quotes and whitespace */
	char *cleaned = strdup(cmd);
	int len = strlen(cleaned);

	/* Strip trailing whitespace */
	while(len > 0 && (cleaned[len-1] == '\n' || cleaned[len-1] == ' ' || cleaned[len-1] == '\t')) {
		cleaned[--len] = '\0';
	}

	/* Strip surrounding quotes if present */
	if(len >= 2 && cleaned[0] == '"' && cleaned[len-1] == '"') {
		cleaned[len-1] = '\0';
		cleaned = cleaned + 1;  /* Skip first quote */
	}

	/* Execute using -c with proper path setup */
	switch(rfork(RFPROC|RFFDG|RFREND|RFNOTEG|RFENVG)){
	case -1:
		close(p[0]);
		close(p[1]);
		free(cleaned);
		return strdup("rfork failed");

	case 0:  /* child */
		close(p[0]);
		dup(p[1], 1);
		dup(p[1], 2);
		close(p[1]);

		/* Debug what we're executing */
		fprint(1, "DEBUG: About to execute: [%s]\n", cleaned);

		/* Try multiple approaches - don't set path, use inherited */
		fprint(1, "DEBUG: Using inherited environment\n");

		/* Try the command with explicit path prefix */
		if(strcmp(cleaned, "date") == 0) {
			fprint(1, "DEBUG: About to call execl /bin/date\n");
			execl("/bin/date", "date", nil);
			fprint(1, "DEBUG: execl /bin/date returned (failed)\n");
		}

		fprint(1, "DEBUG: About to try rc -c approach\n");
		char *av[4];
		av[0] = "rc";
		av[1] = "-c";
		av[2] = cleaned;
		av[3] = nil;
		fprint(1, "DEBUG: About to call exec /bin/rc\n");
		exec("/bin/rc", av);
		fprint(1, "DEBUG: exec /bin/rc returned (failed)\n");

		fprint(1, "DEBUG: All exec attempts failed - about to exit\n");
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

		free(cleaned);
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
		snprint(status, sizeof(status),
			"===CMD: %s\n"
			"===TIME: %ld\n"
			"===OUTPUT:\n%s\n"
			"===END===\n",
			cmd, time(0), output);
		
		/* Store in output ring */
		ring_put(&outring, status);
		
		free(cmd);
		free(output);
	}
}

/* Blockchain JSON API handler */
static void
blockchain_server(void*)
{
	char adir[40], ldir[40];
	int client_fd;
	char buffer[4096];
	int n;

	/* Create TCP server for blockchain API */
	blockchain_fd = announce("tcp!*!9999", adir);
	if (blockchain_fd < 0) {
		fprint(2, "Failed to create blockchain server on port 9999\n");
		return;
	}

	fprint(2, "🌐 Blockchain JSON API listening on port 9999\n");
	fprint(2, "📝 Usage: echo '{\"event_type\":\"deploy_vm\"}' | nc <vm_ip> 9999\n");

	for(;;) {
		client_fd = listen(adir, ldir);
		if (client_fd < 0) {
			sleep(1000);
			continue;
		}

		/* Read JSON command */
		n = read(client_fd, buffer, sizeof(buffer) - 1);
		if (n > 0) {
			buffer[n] = '\0';

			/* Simple JSON parsing for blockchain events */
			/* TODO SECURITY: This is UNSAFE - no auth, no validation, no command restrictions! */
			/* MUST add: authentication, command whitelist, input validation, rate limiting */
			char *cmd;
			if (strstr(buffer, "deploy_vm")) {
				cmd = "vmx -i /n/vminterop/alpine-virt-3.19.0-x86_64.iso -M 512M";
			} else if (strstr(buffer, "status_check")) {
				cmd = "ps aux | grep vmx";
			} else {
				cmd = "echo 'SECURITY: Unknown/blocked blockchain event'";
			}

			/* Execute command and return JSON response */
			char *output = runcmd(cmd);
			cmdcount++;
			ring_put(&outring, output);

			/* Send JSON response */
			char response[8192];
			snprint(response, sizeof(response),
				"{\"status\":\"success\",\"output\":\"%s\",\"timestamp\":%ld}\n",
				output, time(0));
			write(client_fd, response, strlen(response));

			free(output);
		}

		close(client_fd);
	}
}

/* Start executor thread and blockchain server after filesystem is ready */
static void
fsstart(Srv*)
{
	threadcreate(executor, nil, 32*1024);
	threadcreate(blockchain_server, nil, 64*1024);
}

static Srv fs = {
	.read = fsread,
	.write = fswrite,
	.start = fsstart,
};

static void
usage(void)
{
	fprint(2, "usage: %s [-m mountpoint]\n", argv0);
	threadexits("usage");
}

void
threadmain(int argc, char *argv[])
{
	char *mtpt = "/n/cmdexec";
	char *user;
	Tree *tree;

	ARGBEGIN{
	case 'm':
		mtpt = EARGF(usage());
		break;
	default:
		usage();
	}ARGEND

	starttime = time(0);
	user = getuser();

	/* Create synthetic filesystem */
	tree = alloctree(user, user, DMDIR|0777, nil);

	/* Create synthetic files */
	cmdfile = createfile(tree->root, "cmd", user, 0666, nil);
	statusfile = createfile(tree->root, "status", user, 0444, nil);
	outputfile = createfile(tree->root, "output", user, 0444, nil);
	execfile = createfile(tree->root, "exec", user, 0666, nil);
	
	fs.tree = tree;

	/* Mount and serve - this starts the filesystem and executor thread */
	fprint(2, "cmdexec starting, will mount at %s\n", mtpt);
	threadpostmountsrv(&fs, "cmdexec", mtpt, MREPL);
}

/*
 * USAGE:
 * 
 * Start the server:
 *   cmdexec_9p
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