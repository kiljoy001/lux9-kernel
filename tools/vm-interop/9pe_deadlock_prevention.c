/*
 * 9P.e Crash-Safe Server Implementation
 * Prevents OS deadlocks when translators/servers crash during operations
 * Part of the deadbeef/libre compute network integration
 */

#include <u.h>
#include <libc.h>
#include <fcall.h>
#include <thread.h>
#include <9p.h>
#include <auth.h>

enum {
	MaxPendingReq = 256,
	DefaultTimeout = 30000, /* 30 seconds in ms */
	HeartbeatInterval = 5000, /* 5 seconds */
	MaxRetries = 3,
};

typedef struct PendingReq PendingReq;
typedef struct ProcessState ProcessState;
typedef struct SafeServer SafeServer;

struct PendingReq {
	Req *r;
	ulong tag;
	uvlong starttime;
	int pid;
	char *operation;
	PendingReq *next;
};

struct ProcessState {
	int pid;
	uvlong lastheartbeat;
	int crashed;
	char *name;
	ProcessState *next;
};

struct SafeServer {
	Srv *srv;
	PendingReq *pending;
	ProcessState *processes;
	Channel *timeoutch;
	Channel *heartbeatch;
	QLock pendinglock;
	QLock processlock;
	int timeout_ms;
	int debug;
};

SafeServer *safesrv;

/* Time utilities */
uvlong
nstime(void)
{
	return nsec();
}

/* Process monitoring */
int
process_alive(int pid)
{
	char path[64];
	int fd;

	snprint(path, sizeof(path), "/proc/%d/status", pid);
	fd = open(path, OREAD);
	if(fd < 0)
		return 0;
	close(fd);
	return 1;
}

void
update_heartbeat(SafeServer *s, int pid)
{
	ProcessState *ps;

	qlock(&s->processlock);
	for(ps = s->processes; ps; ps = ps->next) {
		if(ps->pid == pid) {
			ps->lastheartbeat = nstime();
			ps->crashed = 0;
			break;
		}
	}
	qunlock(&s->processlock);
}

void
mark_process_crashed(SafeServer *s, int pid)
{
	ProcessState *ps;

	qlock(&s->processlock);
	for(ps = s->processes; ps; ps = ps->next) {
		if(ps->pid == pid) {
			ps->crashed = 1;
			if(s->debug)
				fprint(2, "9pe: marked process %d as crashed\n", pid);
			break;
		}
	}
	qunlock(&s->processlock);
}

void
register_process(SafeServer *s, int pid, char *name)
{
	ProcessState *ps;

	ps = emalloc9p(sizeof(ProcessState));
	ps->pid = pid;
	ps->lastheartbeat = nstime();
	ps->crashed = 0;
	ps->name = estrdup9p(name);

	qlock(&s->processlock);
	ps->next = s->processes;
	s->processes = ps;
	qunlock(&s->processlock);

	if(s->debug)
		fprint(2, "9pe: registered process %d (%s)\n", pid, name);
}

/* Request tracking */
void
track_request(SafeServer *s, Req *r, char *operation)
{
	PendingReq *pr;

	pr = emalloc9p(sizeof(PendingReq));
	pr->r = r;
	pr->tag = r->tag;
	pr->starttime = nstime();
	pr->pid = getpid();
	pr->operation = estrdup9p(operation);

	qlock(&s->pendinglock);
	pr->next = s->pending;
	s->pending = pr;
	qunlock(&s->pendinglock);

	if(s->debug)
		fprint(2, "9pe: tracking %s request tag=%uld from pid=%d\n",
		       operation, r->tag, pr->pid);
}

void
untrack_request(SafeServer *s, Req *r)
{
	PendingReq *pr, **ppr;

	qlock(&s->pendinglock);
	for(ppr = &s->pending; *ppr; ppr = &(*ppr)->next) {
		pr = *ppr;
		if(pr->r == r || pr->tag == r->tag) {
			*ppr = pr->next;
			if(s->debug)
				fprint(2, "9pe: untracked %s request tag=%uld\n",
				       pr->operation, pr->tag);
			free(pr->operation);
			free(pr);
			break;
		}
	}
	qunlock(&s->pendinglock);
}

/* Timeout and cleanup */
void
cleanup_crashed_requests(SafeServer *s)
{
	PendingReq *pr, **ppr;
	uvlong now;
	int cleaned = 0;

	now = nstime();
	qlock(&s->pendinglock);

	ppr = &s->pending;
	while(*ppr) {
		pr = *ppr;

		/* Check if process crashed or timeout exceeded */
		if(!process_alive(pr->pid) ||
		   (now - pr->starttime) > (uvlong)s->timeout_ms * 1000000ULL) {

			if(s->debug) {
				if(!process_alive(pr->pid))
					fprint(2, "9pe: cleaning up request from dead process %d\n", pr->pid);
				else
					fprint(2, "9pe: timing out %s request tag=%uld after %llud ms\n",
					       pr->operation, pr->tag,
					       (now - pr->starttime) / 1000000ULL);
			}

			/* Respond with error to prevent client hanging */
			if(pr->r && pr->r->responded == 0) {
				respond(pr->r, "process died or timed out");
			}

			*ppr = pr->next;
			free(pr->operation);
			free(pr);
			cleaned++;
		} else {
			ppr = &pr->next;
		}
	}
	qunlock(&s->pendinglock);

	if(cleaned > 0 && s->debug)
		fprint(2, "9pe: cleaned up %d crashed/timed out requests\n", cleaned);
}

void
heartbeat_monitor(void *arg)
{
	SafeServer *s = arg;
	ProcessState *ps, **pps;
	uvlong now;

	for(;;) {
		sleep(HeartbeatInterval);
		now = nstime();

		qlock(&s->processlock);
		pps = &s->processes;
		while(*pps) {
			ps = *pps;

			/* Check if process is still alive */
			if(!process_alive(ps->pid) ||
			   (now - ps->lastheartbeat) > HeartbeatInterval * 2 * 1000000ULL) {

				if(!ps->crashed) {
					mark_process_crashed(s, ps->pid);
					if(s->debug)
						fprint(2, "9pe: process %d (%s) appears to have crashed\n",
						       ps->pid, ps->name);
				}

				/* Remove dead processes after marking as crashed */
				*pps = ps->next;
				free(ps->name);
				free(ps);
			} else {
				pps = &ps->next;
			}
		}
		qunlock(&s->processlock);

		/* Clean up any requests from crashed processes */
		cleanup_crashed_requests(s);
	}
}

void
timeout_monitor(void *arg)
{
	SafeServer *s = arg;

	for(;;) {
		sleep(s->timeout_ms / 4); /* Check 4 times per timeout period */
		cleanup_crashed_requests(s);
	}
}

/* Safe 9P operation wrappers */
void
safe_respond(Req *r, char *error)
{
	if(safesrv)
		untrack_request(safesrv, r);
	respond(r, error);
}

void
safe_readstr(Req *r, char *s)
{
	if(safesrv)
		track_request(safesrv, r, "read");
	readstr(r, s);
	if(safesrv)
		untrack_request(safesrv, r);
}

void
safe_readbuf(Req *r, void *s, long n)
{
	if(safesrv)
		track_request(safesrv, r, "read");
	readbuf(r, s, n);
	if(safesrv)
		untrack_request(safesrv, r);
}

/* 9P.e Enhanced Protocol Support */
void
handle_9pe_batch(Req *r)
{
	/* Batch multiple operations to reduce round trips */
	if(safesrv)
		track_request(safesrv, r, "batch");

	/* Implementation would parse batch requests and execute atomically */
	respond(r, "9P.e batch operations not yet implemented");

	if(safesrv)
		untrack_request(safesrv, r);
}

void
handle_9pe_notify(Req *r)
{
	/* Handle notification subscriptions */
	if(safesrv)
		track_request(safesrv, r, "notify");

	/* Implementation would set up notification channels */
	respond(r, "9P.e notifications not yet implemented");

	if(safesrv)
		untrack_request(safesrv, r);
}

void
handle_9pe_stream(Req *r)
{
	/* Handle streaming data operations */
	if(safesrv)
		track_request(safesrv, r, "stream");

	/* Implementation would handle continuous data streams */
	respond(r, "9P.e streaming not yet implemented");

	if(safesrv)
		untrack_request(safesrv, r);
}

/* Initialize crash-safe server */
SafeServer*
init_safe_server(Srv *srv, int timeout_ms, int debug)
{
	SafeServer *s;

	s = emalloc9p(sizeof(SafeServer));
	s->srv = srv;
	s->pending = nil;
	s->processes = nil;
	s->timeout_ms = timeout_ms ? timeout_ms : DefaultTimeout;
	s->debug = debug;

	s->timeoutch = chancreate(sizeof(int), 0);
	s->heartbeatch = chancreate(sizeof(int), 0);

	/* Start monitoring threads */
	proccreate(timeout_monitor, s, 8192);
	proccreate(heartbeat_monitor, s, 8192);

	/* Register main process */
	register_process(s, getpid(), "9pe-server");

	if(debug)
		fprint(2, "9pe: initialized crash-safe server with %dms timeout\n", s->timeout_ms);

	return s;
}

/* Global initialization for use by translators */
void
enable_crash_safety(Srv *srv, int timeout_ms, int debug)
{
	safesrv = init_safe_server(srv, timeout_ms, debug);
}

void
disable_crash_safety(void)
{
	/* TODO: Clean shutdown of monitoring threads */
	safesrv = nil;
}

/* Translator health check interface */
void
translator_heartbeat(void)
{
	if(safesrv)
		update_heartbeat(safesrv, getpid());
}

int
translator_should_exit(void)
{
	ProcessState *ps;
	int should_exit = 0;

	if(!safesrv)
		return 0;

	qlock(&safesrv->processlock);
	for(ps = safesrv->processes; ps; ps = ps->next) {
		if(ps->pid == getpid() && ps->crashed) {
			should_exit = 1;
			break;
		}
	}
	qunlock(&safesrv->processlock);

	return should_exit;
}