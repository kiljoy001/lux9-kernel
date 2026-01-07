/*
 * Simple test for lib9p mounting
 */

#include <u.h>
#include <libc.h>
#include <thread.h>
#include <9p.h>

static void
fsread(Req *r)
{
	readstr(r, "Hello from synthetic file!\n");
	respond(r, nil);
}

static Srv fs = {
	.read = fsread,
};

void
threadmain(int argc, char *argv[])
{
	char *mtpt = "/n/test9p";
	Tree *tree;
	
	print("Starting test9p...\n");
	
	tree = alloctree(nil, nil, DMDIR|0777, nil);
	createfile(tree->root, "hello", nil, 0444, nil);
	
	fs.tree = tree;
	
	print("Mounting at %s...\n", mtpt);
	threadpostmountsrv(&fs, nil, mtpt, MREPL);
	
	print("test9p should be mounted at %s\n", mtpt);
	print("Try: cat %s/hello\n", mtpt);
	
	threadexits(nil);
}