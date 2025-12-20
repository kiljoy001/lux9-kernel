// 9pe_crash_safety.c - Crash safety and deadlock prevention for 9P.e
// Handles cases where translators crash during async/stream operations

#include <u.h>
#include <libc.h>
#include <fcall.h>
#include <thread.h>
#include <9p.h>

// Enhanced request tracking for crash safety
typedef struct AsyncReq {
    u32int tag;
    u64int job_id;
    int pid;                    // Process serving this request
    u64int timeout_ns;          // When to give up
    struct AsyncReq *next;
    Req *original_req;          // Original 9P request
    int state;                  // PENDING, PROCESSING, COMPLETED, FAILED
} AsyncReq;

typedef struct StreamState {
    int active;
    int pid;                    // Streaming process
    u64int last_heartbeat;      // Last activity timestamp
    Channel *cleanup_chan;      // Cleanup coordination
} StreamState;

typedef struct CrashSafeServer {
    Srv base_srv;
    
    // Crash detection
    AsyncReq *pending_async;    // Async operations in flight
    StreamState *streams;       // Active streams
    Channel *death_chan;        // Process death notifications
    
    // Deadlock prevention
    u64int max_timeout_ns;      // Kill hung operations
    int max_pending;            // Limit concurrent ops
    
    // Recovery state
    void (*recovery_handler)(void *ctx);
    void *recovery_ctx;
    
} CrashSafeServer;

// Request states
enum {
    REQ_PENDING = 0,
    REQ_PROCESSING,
    REQ_COMPLETED,
    REQ_FAILED,
    REQ_TIMEOUT
};

static CrashSafeServer *crash_srv = nil;

// Deadlock Prevention: Timeout handler
void
timeout_monitor(void *arg)
{
    CrashSafeServer *cs = arg;
    u64int now;
    AsyncReq *req, *prev, *next;
    
    while(1) {
        sleep(1000);  // Check every second
        now = nsec();
        
        // Check for timed out operations
        prev = nil;
        for(req = cs->pending_async; req; req = next) {
            next = req->next;
            
            if(now > req->timeout_ns) {
                print("9P.e: request tag=%d job=%lld timed out\\n", 
                      req->tag, req->job_id);
                
                // Remove from pending list
                if(prev) prev->next = next;
                else cs->pending_async = next;
                
                // Send error response
                if(req->original_req) {
                    respond(req->original_req, "operation timed out");
                }
                
                req->state = REQ_TIMEOUT;
                free(req);
            } else {
                prev = req;
            }
        }
        
        // Check stream heartbeats
        for(int i = 0; i < 64; i++) {  // Max 64 streams
            if(cs->streams[i].active && 
               (now - cs->streams[i].last_heartbeat) > 5000000000LL) {  // 5s
                
                print("9P.e: stream %d heartbeat timeout, cleaning up\\n", i);
                cs->streams[i].active = 0;
                
                // Signal cleanup
                if(cs->streams[i].cleanup_chan) {
                    nbsend(cs->streams[i].cleanup_chan, nil);
                }
            }
        }
    }
}

// Crash Detection: Process death monitor
void
death_monitor(void *arg)
{
    CrashSafeServer *cs = arg;
    int dead_pid;
    AsyncReq *req, *prev, *next;
    
    while(1) {
        // Wait for death notification
        recv(cs->death_chan, &dead_pid);
        
        print("9P.e: process %d died, cleaning up\\n", dead_pid);
        
        // Clean up async operations from dead process
        prev = nil;
        for(req = cs->pending_async; req; req = next) {
            next = req->next;
            
            if(req->pid == dead_pid) {
                print("9P.e: cleaning up dead async request tag=%d\\n", req->tag);
                
                // Remove from list
                if(prev) prev->next = next;
                else cs->pending_async = next;
                
                // Send error response
                if(req->original_req) {
                    respond(req->original_req, "server process died");
                }
                
                req->state = REQ_FAILED;
                free(req);
            } else {
                prev = req;
            }
        }
        
        // Clean up streams from dead process
        for(int i = 0; i < 64; i++) {
            if(cs->streams[i].active && cs->streams[i].pid == dead_pid) {
                print("9P.e: cleaning up dead stream %d\\n", i);
                cs->streams[i].active = 0;
            }
        }
        
        // Run recovery handler if set
        if(cs->recovery_handler) {
            cs->recovery_handler(cs->recovery_ctx);
        }
    }
}

// Safe async read wrapper
void
safe_async_read(Req *r)
{
    AsyncReq *areq;
    u64int now = nsec();
    
    // Deadlock prevention: check pending count
    int pending_count = 0;
    for(AsyncReq *req = crash_srv->pending_async; req; req = req->next) {
        pending_count++;
    }
    
    if(pending_count >= crash_srv->max_pending) {
        respond(r, "too many pending operations");
        return;
    }
    
    // Create async request tracking
    areq = malloc(sizeof(*areq));
    if(areq == nil) {
        respond(r, "out of memory");
        return;
    }
    
    areq->tag = r->ifcall.tag;
    areq->job_id = now;  // Use timestamp as job ID
    areq->pid = getpid(); 
    areq->timeout_ns = now + crash_srv->max_timeout_ns;
    areq->original_req = r;
    areq->state = REQ_PENDING;
    
    // Add to tracking list
    areq->next = crash_srv->pending_async;
    crash_srv->pending_async = areq;
    
    areq->state = REQ_PROCESSING;
    
    // Now do the actual async operation
    // This would call the translator's async_read function
    print("9P.e: starting async read job=%lld\\n", areq->job_id);
    
    // For demo, simulate immediate completion
    areq->state = REQ_COMPLETED;
    
    char response[64];
    snprint(response, sizeof(response), "async_job_id=%lld\\n", areq->job_id);
    readstr(r, response);
    respond(r, nil);
    
    // Remove from tracking (normally done by completion handler)
    AsyncReq **pp = &crash_srv->pending_async;
    while(*pp && *pp != areq) pp = &(*pp)->next;
    if(*pp) *pp = areq->next;
    free(areq);
}

// Safe streaming with heartbeat
int
safe_stream_setup(int direction, u32int chunk_size)
{
    // Find available stream slot
    int slot = -1;
    for(int i = 0; i < 64; i++) {
        if(!crash_srv->streams[i].active) {
            slot = i;
            break;
        }
    }
    
    if(slot == -1) return -1;  // No slots available
    
    crash_srv->streams[slot].active = 1;
    crash_srv->streams[slot].pid = getpid();
    crash_srv->streams[slot].last_heartbeat = nsec();
    crash_srv->streams[slot].cleanup_chan = chancreate(sizeof(void*), 1);
    
    print("9P.e: stream %d setup for PID %d\\n", slot, getpid());
    return slot;
}

void
stream_heartbeat(int stream_id)
{
    if(stream_id >= 0 && stream_id < 64 && crash_srv->streams[stream_id].active) {
        crash_srv->streams[stream_id].last_heartbeat = nsec();
    }
}

// Initialize crash-safe server
CrashSafeServer*
crash_safe_server_new(void)
{
    CrashSafeServer *cs = malloc(sizeof(*cs));
    if(cs == nil) return nil;
    
    memset(cs, 0, sizeof(*cs));
    
    cs->pending_async = nil;
    cs->streams = malloc(64 * sizeof(StreamState));
    memset(cs->streams, 0, 64 * sizeof(StreamState));
    
    cs->death_chan = chancreate(sizeof(int), 16);
    cs->max_timeout_ns = 30000000000LL;  // 30 seconds
    cs->max_pending = 100;               // Max 100 concurrent async ops
    
    // Start monitoring threads
    proccreate(timeout_monitor, cs, 8192);
    proccreate(death_monitor, cs, 8192);
    
    return cs;
}

// Register process death (called by kernel when process dies)
void
notify_process_death(int pid)
{
    if(crash_srv && crash_srv->death_chan) {
        nbsend(crash_srv->death_chan, &pid);
    }
}

// Enhanced 9P server with crash safety
void
serve_9pe_safe(Srv *srv, char *srvname, char *mtpt)
{
    // Initialize crash safety
    crash_srv = crash_safe_server_new();
    if(crash_srv == nil) {
        fprint(2, "failed to initialize crash safety\\n");
        exits("crash_init");
    }
    
    // Copy base server
    memcpy(&crash_srv->base_srv, srv, sizeof(Srv));
    
    // Wrap operations with safety
    crash_srv->base_srv.read = srv->read;  // Keep original for sync ops
    // async_read would use safe_async_read
    
    print("9P.e crash-safe server: max_pending=%d, timeout=%lld ns\\n",
          crash_srv->max_pending, crash_srv->max_timeout_ns);
    
    threadpostmountsrv(&crash_srv->base_srv, srvname, mtpt, MREPL);
}

// Example usage in translator
void
example_translator_with_safety(void)
{
    Srv srv = {0};
    srv.read = safe_async_read;  // Use crash-safe version
    
    serve_9pe_safe(&srv, "example", nil);
}