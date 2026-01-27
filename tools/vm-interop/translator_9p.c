/*
 * translator_9p - Hurd-style persistent translators for Plan 9
 * Unifies Hurd translator persistence with Plan 9's 9P elegance
 */

#include <u.h>
#include <libc.h>
#include <fcall.h>
#include <thread.h>
#include <9p.h>

typedef struct Translator Translator;
struct Translator {
	char *path;      /* Mount point */
	char *cmd;       /* Command to run */
	char *args;      /* Arguments */
	int persistent;  /* Survives unmount */
	int stackable;   /* Can stack on top */
	Translator *next;
};

/* Translator registry - persists across reboots */
#define TRANS_DB "/lib/translators/registry"

/*
 * Key Innovation: Persistent synthetic files that survive unmount
 * Unlike traditional Plan 9 srv, these reattach automatically
 */

static void
settrans(char *path, char *translator, char *args, int persist)
{
	Dir *d;
	char buf[256];
	int fd;
	
	/* Verify path exists */
	d = dirstat(path);
	if(d == nil){
		/* Create synthetic node if it doesn't exist */
		fd = create(path, OREAD, DMDIR|0777);
		if(fd >= 0)
			close(fd);
	} else
		free(d);
	
	/* Record translator in persistent registry */
	if(persist){
		snprint(buf, sizeof(buf), "%s %s %s\n", 
			path, translator, args ? args : "");
		
		fd = open(TRANS_DB, OWRITE);
		if(fd < 0)
			fd = create(TRANS_DB, OWRITE, 0666);
		seek(fd, 0, 2);  /* Append */
		write(fd, buf, strlen(buf));
		close(fd);
	}
	
	/* Launch the translator */
	switch(rfork(RFPROC|RFFDG|RFNOWAIT)){
	case -1:
		sysfatal("fork: %r");
	case 0:
		execl(translator, translator, "-m", path, args, nil);
		exits("exec");
	}
}

/* 
 * Example: FTP translator like Hurd
 * settrans("/n/ftp", "/bin/ftpfs", "ftp.gnu.org", 1)
 * Now /n/ftp permanently shows ftp.gnu.org
 */

static void
ftpfs_main(void)
{
	Srv s;
	
	s.read = ftpfs_read;
	s.write = ftpfs_write;
	s.stat = ftpfs_stat;
	s.walk = ftpfs_walk;
	
	postmountsrv(&s, nil, "/n/ftp", MREPL);
}

/*
 * Innovation: Stackable translators
 * Mount an encryption translator on top of network translator
 */

static void
stack_translator(char *base, char *translator, char *args)
{
	int fd[2];
	
	if(pipe(fd) < 0)
		sysfatal("pipe: %r");
		
	switch(rfork(RFPROC|RFFDG)){
	case -1:
		sysfatal("fork: %r");
	case 0:
		/* Child: run translator with input from base */
		dup(fd[0], 0);
		close(fd[0]);
		close(fd[1]);
		execl(translator, translator, args, nil);
		exits("exec");
	default:
		/* Parent: connect base to translator */
		close(fd[0]);
		/* Write base's output to translator's input */
		pump(base, fd[1]);
	}
}

/*
 * Blockchain Gateway Translator
 * This is why cmdexec is critical - it's the bridge
 */

static void
blockchain_translator(char *mountpoint)
{
	File *root, *cmd, *status, *blocks;
	Srv s;
	
	/* Create synthetic filesystem for blockchain */
	root = createfile(nil, "blockchain", nil, DMDIR|0777, nil);
	cmd = createfile(root, "command", nil, 0666, nil);
	status = createfile(root, "status", nil, 0444, nil);
	blocks = createfile(root, "blocks", nil, 0444, nil);
	
	/* When someone writes to /n/blockchain/command */
	cmd->write = func(Req *r){
		/* Forward to cmdexec gateway */
		int fd = open("/n/interop/command.txt", OWRITE);
		write(fd, r->ifcall.data, r->ifcall.count);
		close(fd);
		
		/* Wake cmdexec */
		fd = create("/n/interop/cmdexec.wake", OWRITE, 0666);
		close(fd);
		
		respond(r, nil);
	};
	
	/* When someone reads /n/blockchain/status */
	status->read = func(Req *r){
		char buf[256];
		int fd = open("/n/interop/cmdexec.status", OREAD);
		int n = read(fd, buf, sizeof(buf));
		close(fd);
		readbuf(r, buf, n);
		respond(r, nil);
	};
	
	/* Synthetic blockchain data */
	blocks->read = func(Req *r){
		char *data = fetch_blockchain_state();
		readstr(r, data);
		free(data);
		respond(r, nil);
	};
	
	s.tree = alloctree(nil, nil, DMDIR|0777, nil);
	s.tree->root = root;
	
	threadpostmountsrv(&s, nil, mountpoint, MREPL);
}

/*
 * Boot-time translator restoration
 * This makes translators survive reboots like Hurd
 */

static void
restore_translators(void)
{
	Biobuf *b;
	char *line, *path, *trans, *args;
	
	b = Bopen(TRANS_DB, OREAD);
	if(b == nil)
		return;
		
	while((line = Brdstr(b, '\n', 1)) != nil){
		path = line;
		trans = strchr(line, ' ');
		if(trans == nil)
			continue;
		*trans++ = '\0';
		args = strchr(trans, ' ');
		if(args != nil)
			*args++ = '\0';
			
		settrans(path, trans, args, 0);
		free(line);
	}
	Bterm(b);
}

void
main(int argc, char *argv[])
{
	if(argc < 2){
		fprint(2, "usage: translator path translator [args]\n");
		exits("usage");
	}
	
	if(strcmp(argv[1], "-restore") == 0){
		restore_translators();
		exits(nil);
	}
	
	if(strcmp(argv[1], "-blockchain") == 0){
		blockchain_translator("/n/blockchain");
		exits(nil);
	}
	
	settrans(argv[1], argv[2], argc > 3 ? argv[3] : nil, 1);
	exits(nil);
}