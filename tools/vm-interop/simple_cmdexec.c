/*
 * Minimal cmdexec - Based on vmx/9p.c structure
 */

#include <u.h>
#include <libc.h>
#include <thread.h>
#include <fcall.h>
#include <9p.h>

static File *testfile;

static void
fsread(Req *r)
{
	if(r->fid->file == testfile){
		readstr(r, "Hello from cmdexec!\n");
		respond(r, nil);
		return;
	}
	respond(r, "unknown file");
}

static void
fswrite(Req *r)
{
	if(r->fid->file == testfile){
		r->ofcall.count = r->ifcall.count;
		respond(r, nil);
		return;
	}
	respond(r, "permission denied");
}

static Srv cmdsrv = {
	.read = fsread,
	.write = fswrite,
};

void
threadmain(int argc, char *argv[])
{
	char *uid;
	
	USED(argc);
	USED(argv);
	
	uid = getuser();
	cmdsrv.tree = alloctree(uid, uid, 0770, nil);
	testfile = createfile(cmdsrv.tree->root, "test", uid, 0660, nil);
	
	fprint(2, "simple_cmdexec starting, posting to /srv/cmdexec\n");
	threadpostmountsrv(&cmdsrv, "cmdexec", nil, 0);
}