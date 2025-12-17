// hello_9p.c - Hurd hello translator converted to 9P
// Extracted from GNU Hurd's hello.c, Mach removed, 9P added

#include <u.h>
#include <libc.h>
#include <fcall.h>
#include <thread.h>
#include <9p.h>

// EXTRACTED: Core translator state (Hurd logic without Mach)
struct HelloTranslator {
    char *contents;
    size_t contents_len;
    int allow_read;
    int allow_write;
};

// EXTRACTED: Per-connection state (was struct open in Hurd)
struct HelloFile {
    off_t offset;
    int mode;
};

// Global translator instance
static struct HelloTranslator hello_trans = {
    .contents = "Hello, world!\n",
    .contents_len = 14,
    .allow_read = 1,
    .allow_write = 0,
};

// CONVERTED: Hurd's trivfs_modify_stat → 9P stat
void
hello_stat(Req *r)
{
    Dir d = {0};
    
    d.qid.type = 0;  // QTFILE
    d.qid.vers = 1;
    d.qid.path = 1;
    
    d.mode = 0444;  // Read-only file
    d.length = hello_trans.contents_len;
    d.name = "hello";
    d.uid = "translator";
    d.gid = "translator";
    d.muid = "";
    
    r->d = d;
    respond(r, nil);
}

// CONVERTED: Hurd's trivfs_S_io_read → 9P read
void
hello_read(Req *r)
{
    struct HelloFile *f = r->fid->aux;
    off_t offset = r->ifcall.offset;
    u32int count = r->ifcall.count;
    
    // PRESERVED: Original Hurd logic for offset handling
    if(offset == -1)  // Use file offset if -1 passed
        offset = f->offset;
    
    // PRESERVED: Original Hurd bounds checking
    if(offset > hello_trans.contents_len)
        offset = hello_trans.contents_len;
    if(offset + count > hello_trans.contents_len)
        count = hello_trans.contents_len - offset;
    
    // CONVERTED: Mach mmap → 9P readbuf
    if(count > 0) {
        readbuf(r, hello_trans.contents + offset, count);
        f->offset = offset + count;  // Update file offset
    } else {
        r->ofcall.count = 0;
    }
    
    respond(r, nil);
}

// CONVERTED: Hurd's trivfs open hook → 9P open
void
hello_open(Req *r)
{
    struct HelloFile *f;
    
    // Check permissions (preserved from Hurd)
    if((r->ifcall.mode & 3) != OREAD) {
        respond(r, "permission denied");
        return;
    }
    
    // CONVERTED: malloc hook → 9P aux data
    f = malloc(sizeof(*f));
    if(f == nil) {
        respond(r, "out of memory");
        return;
    }
    
    // PRESERVED: Initialize offset (same as Hurd)
    f->offset = 0;
    f->mode = r->ifcall.mode;
    r->fid->aux = f;
    
    respond(r, nil);
}

// NEW: Handle file close (cleanup aux data)
void
hello_destroyfid(Fid *fid)
{
    if(fid->aux)
        free(fid->aux);
}

// CONVERTED: Hurd write operation (was trivfs_S_io_write) 
void
hello_write(Req *r)
{
    // PRESERVED: Hurd's write denial logic
    if(!hello_trans.allow_write) {
        respond(r, "file is read-only");
        return;
    }
    
    // If write was allowed, we'd implement it here
    r->ofcall.count = r->ifcall.count;
    respond(r, nil);
}

// 9P service table - maps to Hurd's trivfs operations
Srv hello_srv = {
    .open = hello_open,
    .read = hello_read,
    .write = hello_write,
    .stat = hello_stat,
    .destroyfid = hello_destroyfid,
};

void
usage(void)
{
    fprint(2, "usage: hello_9p [-s srvname] [-m mountpoint]\n");
    exits("usage");
}

void
threadmain(int argc, char **argv)
{
    char *srvname = nil;
    char *mtpt = nil;
    
    ARGBEGIN{
    case 's':
        srvname = EARGF(usage());
        break;
    case 'm':
        mtpt = EARGF(usage());
        break;
    default:
        usage();
    }ARGEND
    
    // Default service name
    if(srvname == nil)
        srvname = "hello";
    
    print("hello translator: serving %s\n", srvname);
    
    // CONVERTED: Hurd's trivfs_startup → 9P threadpostmountsrv
    threadpostmountsrv(&hello_srv, srvname, mtpt, MREPL);
}