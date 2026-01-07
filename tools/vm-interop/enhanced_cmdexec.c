/*
 * enhanced_cmdexec - Ultra-reliable command executor for VM-blockchain gateway
 * Fixes: deadlocks, file detection issues, process monitoring, atomic operations
 */

#include <u.h>
#include <libc.h>
#include <bio.h>

#define CMDFILE "/n/interop/command.txt"
#define OUTFILE "/n/interop/output.txt"
#define LOCKFILE "/tmp/cmdexec.lock"
#define HEARTBEAT "/n/interop/cmdexec.heartbeat"
#define MAXCMD 8192
#define MAXOUT 65536

typedef struct Cmd {
	char *text;
	int num;
	vlong mtime;
	vlong size;  /* Add file size for better change detection */
} Cmd;

Biobuf *outbio;
int debug = 1;
int cmdcount = 0;
vlong lastheartbeat = 0;

void
logmsg(char *fmt, ...)
{
	va_list args;
	char buf[256];
	
	va_start(args, fmt);
	vsnprint(buf, sizeof(buf), fmt, args);
	va_end(args);
	
	if(debug)
		fprint(2, "[cmdexec] %s\n", buf);
	
	if(outbio != nil){
		Bprint(outbio, "===LOG: %s===\n", buf);
		Bflush(outbio);
	}
}

/* Enhanced heartbeat mechanism */
void
writehb(void)
{
	int fd;
	char hb[64];
	vlong now;
	
	now = nsec();
	if(now - lastheartbeat < 1000000000LL)  /* Rate limit to 1Hz */
		return;
		
	snprint(hb, sizeof(hb), "%lld %d\n", now, getpid());
	fd = create(HEARTBEAT, OWRITE, 0666);
	if(fd >= 0){
		write(fd, hb, strlen(hb));
		close(fd);
		lastheartbeat = now;
	}
}

void
writeoutput(char *cmd, char *output, char *status)
{
	char *timestr;
	long t;
	
	t = time(0);
	timestr = ctime(t);
	if(timestr != nil && strlen(timestr) > 0)
		timestr[strlen(timestr)-1] = '\0';
	
	Bprint(outbio, "===CMD: %s\n", cmd);
	Bprint(outbio, "===TIME: %s\n", timestr);
	if(output != nil && *output != 0)
		Bwrite(outbio, output, strlen(output));
	Bprint(outbio, "===STATUS: %s\n", status);
	Bprint(outbio, "===END===\n");
	Bflush(outbio);
	
	logmsg("Executed command %d: %s", ++cmdcount, cmd);
	writehb();
}

int
runcmd(char *cmd)
{
	int p[2];
	Waitmsg *w;
	char *output;
	char status[256];
	int n, total, capacity;
	
	writehb();  /* Heartbeat before command execution */
	
	if(pipe(p) < 0){
		logmsg("pipe failed: %r");
		writeoutput(cmd, "", "pipe failed");
		return -1;
	}
	
	switch(fork()){
	case -1:
		close(p[0]);
		close(p[1]);
		logmsg("fork failed: %r");
		writeoutput(cmd, "", "fork failed");
		return -1;
		
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
			writeoutput(cmd, "", "malloc failed");
			return -1;
		}
		
		total = 0;
		while((n = read(p[0], output + total, capacity - total - 1)) > 0){
			total += n;
			writehb();  /* Heartbeat during long operations */
			if(total >= capacity - 1){
				if(capacity >= MAXOUT)
					break;
				capacity *= 2;
				if(capacity > MAXOUT)
					capacity = MAXOUT;
				output = realloc(output, capacity);
				if(output == nil){
					close(p[0]);
					writeoutput(cmd, "", "realloc failed");
					return -1;
				}
			}
		}
		output[total] = '\0';
		close(p[0]);
		
		w = wait();
		if(w == nil){
			snprint(status, sizeof(status), "wait failed");
		} else if(w->msg[0] != '\0'){
			snprint(status, sizeof(status), "%s", w->msg);
		} else {
			status[0] = '\0';
		}
		if(w != nil)
			free(w);
		
		writeoutput(cmd, output, status);
		free(output);
		return 0;
	}
}

/* Enhanced file change detection using both mtime and size */
Cmd*
readnewcmd(vlong lastmtime, vlong lastsize, int lastline)
{
	Dir *d;
	int fd;
	char *buf, *p, *ep;
	long n;
	static Cmd cmd;
	int linenum;
	
	writehb();  /* Heartbeat during file operations */
	
	d = dirstat(CMDFILE);
	if(d == nil)
		return nil;
	
	/* Enhanced change detection: check both mtime AND size */
	if(d->mtime <= lastmtime && d->length <= lastsize){
		free(d);
		return nil;
	}
	
	cmd.mtime = d->mtime;
	cmd.size = d->length;
	free(d);
	
	fd = open(CMDFILE, OREAD);
	if(fd < 0){
		logmsg("cannot open %s: %r", CMDFILE);
		return nil;
	}
	
	buf = malloc(MAXCMD);
	if(buf == nil){
		close(fd);
		return nil;
	}
	
	n = read(fd, buf, MAXCMD-1);
	close(fd);
	
	if(n <= 0){
		free(buf);
		return nil;
	}
	buf[n] = '\0';
	
	linenum = 0;
	p = buf;
	ep = buf + n;
	
	while(p < ep){
		char *linestart = p;
		
		while(p < ep && *p != '\n')
			p++;
		
		linenum++;
		
		if(linenum > lastline && linestart < p){
			while(linestart < p && (*linestart == ' ' || *linestart == '\t'))
				linestart++;
			
			if(linestart < p){
				int len = p - linestart;
				cmd.text = malloc(len + 1);
				if(cmd.text != nil){
					memmove(cmd.text, linestart, len);
					cmd.text[len] = '\0';
					cmd.num = linenum;
					free(buf);
					return &cmd;
				}
			}
		}
		
		if(p < ep && *p == '\n')
			p++;
	}
	
	free(buf);
	return nil;
}

void
clearlock(void)
{
	remove(LOCKFILE);
	remove(HEARTBEAT);
}

/* Enhanced lock handling with stale detection */
int
takelock(void)
{
	int fd;
	char pid[32];
	Dir *d;
	vlong now;
	
	/* Check for stale locks based on heartbeat */
	d = dirstat(LOCKFILE);
	if(d != nil){
		now = nsec();
		/* If lock is older than 30 seconds, consider it stale */
		if(now - d->mtime > 30000000000LL){
			logmsg("Removing stale lock (>30s old)");
			remove(LOCKFILE);
			remove(HEARTBEAT);
		}
		free(d);
	}
	
	fd = create(LOCKFILE, OWRITE|OEXCL, 0666);
	if(fd < 0){
		fd = open(LOCKFILE, OREAD);
		if(fd >= 0){
			int n = read(fd, pid, sizeof(pid)-1);
			close(fd);
			if(n > 0){
				pid[n] = '\0';
				int lockpid = atoi(pid);
				if(lockpid > 0){
					char procfile[64];
					snprint(procfile, sizeof(procfile), "/proc/%d/status", lockpid);
					if(access(procfile, AEXIST) < 0){
						logmsg("Removing stale lock from dead PID %d", lockpid);
						remove(LOCKFILE);
						remove(HEARTBEAT);
						return takelock();
					}
				}
			}
		}
		return -1;
	}
	
	snprint(pid, sizeof(pid), "%d", getpid());
	write(fd, pid, strlen(pid));
	close(fd);
	
	atexit(clearlock);
	writehb();
	return 0;
}

void
main(int argc, char *argv[])
{
	vlong lastmtime = 0;
	vlong lastsize = 0;
	Cmd *cmd;
	int lastline = 0;
	int iterations = 0;
	
	ARGBEGIN{
	case 'd':
		debug = 1;
		break;
	case 'q':
		debug = 0;
		break;
	default:
		fprint(2, "usage: %s [-dq]\n", argv0);
		exits("usage");
	}ARGEND
	
	if(takelock() < 0){
		fprint(2, "cmdexec: another instance is running\n");
		exits("locked");
	}
	
	outbio = Bopen(OUTFILE, OWRITE);
	if(outbio == nil){
		fprint(2, "cmdexec: cannot open %s: %r\n", OUTFILE);
		exits("open");
	}
	Bseek(outbio, 0, 2);
	
	logmsg("Started, monitoring %s", CMDFILE);
	logmsg("Writing output to %s", OUTFILE);
	writehb();
	
	/* Enhanced main loop with better error recovery */
	for(;;){
		iterations++;
		
		/* Process ALL new commands in batch */
		while((cmd = readnewcmd(lastmtime, lastsize, lastline)) != nil){
			logmsg("New command on line %d: %s", cmd->num, cmd->text);
			runcmd(cmd->text);
			lastline = cmd->num;
			lastmtime = cmd->mtime;
			lastsize = cmd->size;
			if(cmd->text != nil){
				free(cmd->text);
				cmd->text = nil;
			}
		}
		
		/* Periodic heartbeat and health check */
		if(iterations % 10 == 0){
			writehb();
			/* Force flush output buffer */
			if(outbio != nil){
				Bflush(outbio);
			}
		}
		
		if(access("/n/interop/stop_cmdexec", AEXIST) == 0){
			logmsg("Stop file detected, exiting");
			break;
		}
		
		/* Shorter sleep for better responsiveness */
		sleep(200);
	}
	
	Bterm(outbio);
	clearlock();
	exits(nil);
}