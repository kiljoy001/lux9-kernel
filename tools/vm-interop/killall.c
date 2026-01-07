#include <u.h>
#include <libc.h>
#include <bio.h>

void
usage(void)
{
	fprint(2, "usage: killall [-s signal] pattern\n");
	fprint(2, "signals: kill, interrupt, hangup, alarm, exit\n");
	exits("usage");
}

void
killproc(char *pid, char *signal)
{
	char note[256];
	int fd;
	
	snprint(note, sizeof(note), "/proc/%s/note", pid);
	fd = open(note, OWRITE);
	if(fd < 0){
		fprint(2, "killall: cannot open %s: %r\n", note);
		return;
	}
	
	if(write(fd, signal, strlen(signal)) < 0)
		fprint(2, "killall: cannot kill %s: %r\n", pid);
	else
		print("killed %s\n", pid);
	
	close(fd);
}

void
main(int argc, char *argv[])
{
	char *signal = "kill";
	char *pattern;
	Biobuf *bp;
	char *line;
	char *fields[10];
	int nf;
	
	ARGBEGIN{
	case 's':
		signal = EARGF(usage());
		break;
	default:
		usage();
	}ARGEND
	
	if(argc != 1)
		usage();
	
	pattern = argv[0];
	
	/* Run ps and parse output */
	bp = Bopen("/bin/ps", OREAD);
	if(bp == nil){
		fprint(2, "killall: cannot run ps: %r\n");
		exits("ps");
	}
	
	/* Skip header line */
	Brdline(bp, '\n');
	
	while((line = Brdline(bp, '\n')) != nil){
		line[Blinelen(bp)-1] = '\0';
		
		/* Check if line contains pattern */
		if(strstr(line, pattern) == nil)
			continue;
		
		/* Skip if this is the killall process itself */
		if(strstr(line, "killall") != nil)
			continue;
		
		/* Parse fields to get PID (second field) */
		nf = tokenize(line, fields, nelem(fields));
		if(nf >= 2){
			print("Killing process %s: %s\n", fields[1], line);
			killproc(fields[1], signal);
		}
	}
	
	Bterm(bp);
	exits(nil);
}