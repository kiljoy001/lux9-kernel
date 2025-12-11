/*
 * Pure 9P Syscall Elimination Layer
 *
 * Replaces traditional syscalls with direct 9P protocol dispatch.
 * All I/O operations route through the 9P router with GHOSTDAG ordering.
 *
 * Phase 6: Complete syscall removal
 */

#include "u.h"
#include "portlib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "ureg.h"
#include "9p_router.h"
#include <error.h>

/* Legacy syscall numbers for translation */
enum {
	RFORK = 19,
	OPEN = 14,
	READ = 15,
	WRITE = 20,
	CLOSE = 4,
	EXEC = 7,
	EXITS = 8,
	PIPE = 21,
	CREATE = 22,
	FD2PATH = 23,
	SEEK = 39,
	STAT = 42,
	FSTAT = 43,
};

/*
 * syscall_to_9p - Pure 9P dispatch replacing dosyscall()
 *
 * Called from trap handler when VectorSYSCALL fires.
 * Translates legacy syscall arguments into 9P Fcall messages.
 * Routes through p9_dispatch() with Pebble security and GHOSTDAG ordering.
 *
 * Returns: result in ureg->ax, -1 on error
 */
void
syscall_to_9p(Ureg *ureg)
{
	Fcall t, r;
	ulong scallnr;
	uintptr *args;
	long result;

	scallnr = ureg->bp;  /* RARG - syscall number */
	args = (uintptr*)(ureg->sp + BY2WD);  /* Skip return address slot */

	memset(&t, 0, sizeof(t));
	memset(&r, 0, sizeof(r));

	/* Default error response */
	result = -1;

	switch(scallnr) {
	case OPEN:
		/* OPEN(path, mode) -> Tattach(aname=path) */
		t.type = Tattach;
		t.aname = (char*)args[0];
		t.fid = up->fid_counter++;  /* Allocate fid */
		t.afid = NOFID;
		t.uname = up->user;

		if(p9_dispatch(up, &t, &r) < 0 || r.type == Rerror)
			break;

		result = t.fid;  /* Return fid as fd */
		break;

	case READ:
		/* READ(fd, buf, count) -> Tread(fid, offset, count) */
		t.type = Tread;
		t.fid = (u32int)args[0];
		t.offset = up->fid_offsets[t.fid];  /* Track offset per-fid */
		t.count = (u32int)args[2];

		/* Allocate response buffer */
		r.data = smalloc(t.count);
		if(r.data == nil)
			error(Enomem);

		if(waserror()){
			free(r.data);
			nexterror();
		}

		if(p9_dispatch(up, &t, &r) < 0 || r.type == Rerror){
			poperror();
			free(r.data);
			break;
		}

		/* Copy data to userspace */
		memmove((void*)args[1], r.data, r.count);
		up->fid_offsets[t.fid] += r.count;
		result = r.count;

		poperror();
		free(r.data);
		break;

	case WRITE:
		/* WRITE(fd, buf, count) -> Twrite(fid, offset, count, data) */
		t.type = Twrite;
		t.fid = (u32int)args[0];
		t.offset = up->fid_offsets[t.fid];
		t.count = (u32int)args[2];
		t.data = (char*)args[1];

		if(p9_dispatch(up, &t, &r) < 0 || r.type == Rerror)
			break;

		up->fid_offsets[t.fid] += r.count;
		result = r.count;
		break;

	case CLOSE:
		/* CLOSE(fd) -> Tclunk(fid) */
		t.type = Tclunk;
		t.fid = (u32int)args[0];

		if(p9_dispatch(up, &t, &r) < 0 || r.type == Rerror)
			break;

		result = 0;
		break;

	case EXEC:
		/* EXEC(path, argv) -> Write to /proc/self/ctl */
		t.type = Tattach;
		t.aname = "/proc/self/ctl";
		t.fid = up->fid_counter++;
		t.afid = NOFID;
		t.uname = up->user;

		if(p9_dispatch(up, &t, &r) < 0 || r.type == Rerror)
			break;

		/* Write "exec <path>" command */
		{
			char cmd[256];
			snprint(cmd, sizeof(cmd), "exec %s", (char*)args[0]);

			memset(&t, 0, sizeof(t));
			t.type = Twrite;
			t.fid = r.qid.path;  /* Use fid from attach */
			t.offset = 0;
			t.count = strlen(cmd);
			t.data = cmd;

			if(p9_dispatch(up, &t, &r) < 0 || r.type == Rerror)
				break;
		}

		result = 0;
		break;

	case EXITS:
		/* EXITS(msg) -> Write to /proc/self/ctl */
		t.type = Tattach;
		t.aname = "/proc/self/ctl";
		t.fid = up->fid_counter++;
		t.afid = NOFID;
		t.uname = up->user;

		if(p9_dispatch(up, &t, &r) < 0 || r.type == Rerror)
			break;

		/* Write "exit" command */
		{
			memset(&t, 0, sizeof(t));
			t.type = Twrite;
			t.fid = r.qid.path;
			t.offset = 0;
			t.count = 4;
			t.data = "exit";

			p9_dispatch(up, &t, &r);
		}

		pexit((char*)args[0], 1);
		/* NOTREACHED */
		break;

	case RFORK:
		/* RFORK(flags) -> Write to /proc/self/ctl */
		t.type = Tattach;
		t.aname = "/proc/self/ctl";
		t.fid = up->fid_counter++;
		t.afid = NOFID;
		t.uname = up->user;

		if(p9_dispatch(up, &t, &r) < 0 || r.type == Rerror)
			break;

		/* Write "rfork <flags>" command */
		{
			char cmd[64];
			snprint(cmd, sizeof(cmd), "rfork %lud", args[0]);

			memset(&t, 0, sizeof(t));
			t.type = Twrite;
			t.fid = r.qid.path;
			t.offset = 0;
			t.count = strlen(cmd);
			t.data = cmd;

			if(p9_dispatch(up, &t, &r) < 0 || r.type == Rerror)
				break;

			/* Result is new PID in response */
			result = 0;  /* TODO: parse PID from response */
		}
		break;

	case PIPE:
		/* PIPE(fds) -> Tattach("/dev/pipe") */
		t.type = Tattach;
		t.aname = "/dev/pipe";
		t.fid = up->fid_counter++;
		t.afid = NOFID;
		t.uname = up->user;

		if(p9_dispatch(up, &t, &r) < 0 || r.type == Rerror)
			break;

		/* TODO: Allocate two fids for read/write ends */
		result = 0;
		break;

	default:
		print("syscall_to_9p: unsupported syscall %lud\n", scallnr);
		result = -1;
		break;
	}

	ureg->ax = result;
}
