#include <u.h>
#include <libc.h>

#define CMDFILE "/n/interop/command.txt"
#define OUTFILE "/n/interop/output.txt"
#define STOPFILE "/n/interop/stop_cmdexec"
#define LOCKFILE "/n/interop/cmdexec.lock"
#define MAXOUT 65536

int debug = 1;
Cmd cmd;

typedef struct Cmd Cmd;
struct Cmd {
	char *text;
	int num;
	vlong mtime;
};

void
logmsg(char *fmt, ...)
{
	va_list args;
	char buf[1024];
	int fd;
	
	if(!debug)
		return;
	
	va_start(args, fmt);
	vsnprint(buf, sizeof(buf), fmt, args);
	va_end(args);
	
	fd = open(OUTFILE, OWRITE);
	if(fd >= 0){
		seek(fd, 0, 2);
		fprint(fd, "===LOG: %s===\n", buf);
		close(fd);
	}
}

void
writeoutput(char *command, char *output, char *status)
{
	int fd;
	char *tm;
	
	fd = open(OUTFILE, OWRITE);
	if(fd < 0){
		fd = create(OUTFILE, OWRITE, 0666);
		if(fd < 0){
			if(debug)
				fprint(2, "cannot create %s: %r\n", OUTFILE);
			return;
		}
	}
	seek(fd, 0, 2);
	
	tm = ctime(time(0));
	tm[strlen(tm)-1] = '\0';  /* Remove newline */
	
	fprint(fd, "===CMD: %s\n", command);
	fprint(fd, "===TIME: %s\n", tm);
	
	if(output && *output)
		fprint(fd, "%s", output);
	if(output && strlen(output) > 0 && output[strlen(output)-1] != '\n')
		fprint(fd, "\n");
	
	fprint(fd, "===STATUS: %s\n", status);
	fprint(fd, "===END===\n");
	
	close(fd);
}

int
runcmd(char *cmd)
{
	int p[2];
	Waitmsg *w;
	char status[256];
	char *output;
	int n, total, capacity;
	
	if(pipe(p) < 0){
		writeoutput(cmd, "", "pipe failed");
		return -1;
	}
	
	switch(fork()){
	case -1:
		close(p[0]);
		close(p[1]);
		writeoutput(cmd, "", "fork failed");
		return -1;
	
	case 0:
		/* Child */
		close(p[0]);
		dup(p[1], 1);
		dup(p[1], 2);
		close(p[1]);
		execl("/bin/rc", "rc", "-c", cmd, nil);
		sysfatal("exec failed: %r");
	
	default:
		/* Parent */
		close(p[1]);
		
		/* Dynamic buffer to avoid truncation */
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
			if(total >= capacity - 1){
				if(capacity >= MAXOUT){
					break;  /* Hit max size */
				}
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

Cmd*
readnewcmd(vlong *lastmtime, int *lastline)
{
	Dir *d;
	int fd;
	char *buf, *p, *ep;
	int n, linenum;
	
	/* Check if file has changed or been replaced */
	d = dirstat(CMDFILE);
	if(d == nil)
		return nil;
	
	vlong mtime = d->mtime;
	int length = d->length;
	free(d);
	
	/* Open and read file */
	fd = open(CMDFILE, OREAD);
	if(fd < 0)
		return nil;
	
	buf = malloc(length + 1);
	if(buf == nil){
		close(fd);
		return nil;
	}
	
	n = read(fd, buf, length);
	close(fd);
	if(n <= 0){
		free(buf);
		return nil;
	}
	buf[n] = '\0';
	
	/* Count lines to see if file was replaced */
	int totallines = 0;
	for(p = buf; p < buf + n; p++){
		if(*p == '\n')
			totallines++;
	}
	if(p > buf && p[-1] != '\n')
		totallines++;  /* Last line without newline */
	
	/* If file was replaced (has fewer lines than lastline), reset */
	if(totallines < *lastline || mtime != *lastmtime){
		if(totallines < *lastline){
			logmsg("File replaced (had %d lines, now %d), resetting", *lastline, totallines);
		}
		*lastline = 0;
		*lastmtime = mtime;
	}
	
	/* Parse lines */
	p = buf;
	ep = buf + n;
	linenum = 0;
	
	while(p < ep){
		char *linestart = p;
		
		/* Find end of line */
		while(p < ep && *p != '\n')
			p++;
		
		linenum++;
		
		/* Check if this is a new command line */
		if(linenum > *lastline && linestart < p){
			/* Skip empty lines and whitespace */
			while(linestart < p && (*linestart == ' ' || *linestart == '\t'))
				linestart++;
			
			if(linestart < p){
				/* Found a new command */
				int len = p - linestart;
				cmd.text = malloc(len + 1);
				if(cmd.text != nil){
					memmove(cmd.text, linestart, len);
					cmd.text[len] = '\0';
					cmd.num = linenum;
					cmd.mtime = mtime;
					*lastline = linenum;
					*lastmtime = mtime;
					free(buf);
					return &cmd;
				}
			}
		}
		
		/* Skip newline */
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
}

int
takelock(void)
{
	int fd;
	char pid[32];
	
	/* Try to create lock file */
	fd = create(LOCKFILE, OWRITE|OEXCL, 0666);
	if(fd < 0){
		/* Check if lock holder is still alive */
		fd = open(LOCKFILE, OREAD);
		if(fd >= 0){
			int n = read(fd, pid, sizeof(pid)-1);
			close(fd);
			if(n > 0){
				pid[n] = '\0';
				int lockpid = atoi(pid);
				if(lockpid > 0){
					/* Check if process exists */
					char procfile[64];
					snprint(procfile, sizeof(procfile), "/proc/%d/status", lockpid);
					if(access(procfile, AEXIST) < 0){
						/* Process doesn't exist, remove stale lock */
						logmsg("Removing stale lock from PID %d", lockpid);
						remove(LOCKFILE);
						return takelock();
					}
				}
			}
		}
		return -1;
	}
	
	/* Write our PID to lock file */
	snprint(pid, sizeof(pid), "%d", getpid());
	write(fd, pid, strlen(pid));
	close(fd);
	
	/* Register cleanup on exit */
	atexit(clearlock);
	return 0;
}

void
main(int argc, char *argv[])
{
	vlong lastmtime = 0;
	Cmd *cmd;
	int lastline = 0;
	
	ARGBEGIN{
	case 'd':
		debug = 1;
		break;
	case 'q':
		debug = 0;
		break;
	default:
		fprint(2, "usage: %s [-d] [-q]\n", argv0);
		exits("usage");
	}ARGEND
	
	/* Take lock */
	if(takelock() < 0){
		fprint(2, "cmdexec: another instance is running\n");
		exits("locked");
	}
	
	logmsg("Started, monitoring %s", CMDFILE);
	logmsg("Writing output to %s", OUTFILE);
	
	for(;;){
		/* Check for stop file */
		if(access(STOPFILE, AEXIST) >= 0){
			logmsg("Stop file detected, exiting");
			break;
		}
		
		/* Process ALL new commands, not just one */
		while((cmd = readnewcmd(&lastmtime, &lastline)) != nil){
			logmsg("New command on line %d: %s", cmd->num, cmd->text);
			runcmd(cmd->text);
			logmsg("Executed command %d: %s", cmd->num, cmd->text);
			free(cmd->text);
		}
		
		sleep(1000);  /* 1 second */
	}
	
	exits(nil);
}