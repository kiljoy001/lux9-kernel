#include "../inc/server9p.h"
#include "../src/lux_internal.h"

/* Default hash size for FIDs */
#define FID_HASH_SIZE 64

/* Helper: pebble_malloc wrapper */
static void *srv_malloc(ulong size) {
    void *p;
    if (pebble_alloc(size, &p) < 0) return nil;
    return p;
}

static void srv_free(void *p) {
    pebble_free(p);
}

/* Helper: Create a Qid */
void mkqid(Qid *q, u64int path, u32int vers, u8int type) {
    q->path = path;
    q->vers = vers;
    q->type = type;
}

/* Internal: Get FID from hash */
static Fid *get_fid(Srv *s, u32int fid) {
    if (!s->fidhash) return nil;
    Fid *f = s->fidhash[fid % s->fidhashsize];
    while (f) {
        if (f->fid == fid) return f;
        f = f->next;
    }
    return nil;
}

/* Internal: Add FID to hash */
/* static Fid *alloc_fid(Srv *s, u32int fid) {
    Fid *f = srv_malloc(sizeof(Fid));
    if (!f) return nil;
    memset(f, 0, sizeof(Fid));
    f->fid = fid;
    f->omode = -1; 
    
    int bucket = fid % s->fidhashsize;
    f->next = s->fidhash[bucket];
    s->fidhash[bucket] = f;
    return f;
} */

/* Internal: Remove FID from hash */
/* static void free_fid(Srv *s, Fid *f) {
    int bucket = f->fid % s->fidhashsize;
    Fid **prev = &s->fidhash[bucket];
    while (*prev) {
        if (*prev == f) {
            *prev = f->next;
            srv_free(f);
            return;
        }
        prev = &(*prev)->next;
    }
} */

/* Respond to a request */
void srv_respond(Req *r, char *error) {
    uchar buf[8192];
    uint n;

    if (error) {
        r->ofcall.type = Rerror;
        r->ofcall.ename = error;
    } else {
        r->ofcall.type = r->ifcall.type + 1;
        r->ofcall.tag = r->ifcall.tag;
    }
    
    /* Marshall the response */
    n = convS2M(&r->ofcall, buf, sizeof(buf));
    if (n > 0) {
        /* Write response to the output channel */
        r->responding = 1; 
    }
}

/* Main Server Loop */
void srv_loop(Srv *s, int fd_in, int fd_out) {
    /* Allocate FID hash if needed */
    if (!s->fidhash) {
        s->fidhashsize = FID_HASH_SIZE;
        s->fidhash = srv_malloc(sizeof(Fid*) * s->fidhashsize);
        memset(s->fidhash, 0, sizeof(Fid*) * s->fidhashsize);
    }

    uchar buf[8192];
    uint n;
    
    while (1) {
        /* 1. Read message size (4 bytes) */
        n = sys_read(fd_in, buf, 4);
        if (n != 4) break;
        
        u32int size = GBIT32(buf);
        if (size > sizeof(buf)) break; /* Too big */
        
        /* 2. Read rest of message */
        n = sys_read(fd_in, buf + 4, size - 4);
        if (n != size - 4) break;
        
        /* 3. Unmarshall */
        Req r;
        memset(&r, 0, sizeof(Req));
        r.srv = s;
        if (convM2S(buf, size, &r.ifcall) != size) {
            continue; /* Framing error */
        }
        
        /* 4. Dispatch */
        r.fid = get_fid(s, r.ifcall.fid);
        
        switch (r.ifcall.type) {
            case Tversion:
                if (r.ifcall.msize > 8192) r.ofcall.msize = 8192;
                else r.ofcall.msize = r.ifcall.msize;
                r.ofcall.version = "9P2000";
                srv_respond(&r, nil);
                break;
            case Tauth:
                srv_respond(&r, "authentication not required");
                break;
            case Tattach:
                if (s->attach) s->attach(&r);
                else srv_respond(&r, "not implemented");
                break;
            /* ... Add other cases ... */
            default:
                srv_respond(&r, "unknown message");
        }
        
        /* 5. Write Response */
        if (r.responding) {
            n = convS2M(&r.ofcall, buf, sizeof(buf));
            sys_write(fd_out, buf, n);
        }
    }
}

void srv_init(Srv *s) {
    memset(s, 0, sizeof(Srv));
}
