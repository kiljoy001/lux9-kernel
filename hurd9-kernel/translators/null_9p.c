// null_9p.c - Hurd null translator converted to 9P
// Extracted from GNU Hurd's null.c, Mach removed, 9P added

#include "translator.h"

// EXTRACTED: Core translator state (Hurd logic without Mach)
struct NullTranslator {
    int allow_read;
    int allow_write;
    int write_error_code;  // 0 = success, ENOSPC = "disk full"
};

// Global translator instance
static struct NullTranslator null_trans = {
    .allow_read = 1,
    .allow_write = 1,
    .write_error_code = 0,  // Default: successful writes to bitbucket
};

// CONVERTED: Hurd's trivfs_modify_stat → 9P stat
void
null_stat(Req *r)
{
    Dir d = {0};
    
    d.qid.type = 0;  // QTFILE - character device acts like file
    d.qid.vers = 1;
    d.qid.path = 2;  // Different from hello
    
    d.mode = 0666;  // Read/write device
    d.length = 0;   // Always 0 size (like /dev/null)
    d.name = "null";
    d.uid = "translator";
    d.gid = "translator";
    d.muid = "";
    
    r->d = d;
    respond(r, nil);
}

// CONVERTED: Hurd's trivfs_S_io_read → 9P read
void
null_read(Req *r)
{
    // PRESERVED: Original Hurd logic - null always returns EOF
    if(!(r->ifcall.mode & O_READ)) {
        respond(r, "bad file descriptor");
        return;
    }
    
    // Always return 0 bytes (EOF)
    r->ofcall.count = 0;
    respond(r, nil);
}

// CONVERTED: Hurd's trivfs_S_io_write → 9P write
void
null_write(Req *r)
{
    // PRESERVED: Original Hurd logic for write handling
    if(!(r->ifcall.mode & O_WRITE)) {
        respond(r, "bad file descriptor");
        return;
    }
    
    // Check if we should simulate disk full
    if(null_trans.write_error_code != 0) {
        respond(r, hurd_error_to_9p(null_trans.write_error_code));
        return;
    }
    
    // PRESERVED: Accept all writes to bitbucket
    r->ofcall.count = r->ifcall.count;
    respond(r, nil);
}

// CONVERTED: Hurd's trivfs open hook → 9P open  
void
null_open(Req *r)
{
    TranslatorFile *f;
    
    // Null accepts both read and write
    if((r->ifcall.mode & 3) > ORDWR) {
        respond(r, "permission denied");
        return;
    }
    
    f = malloc(sizeof(*f));
    if(f == nil) {
        respond(r, "out of memory");
        return;
    }
    
    f->name = "null";
    f->mode = r->ifcall.mode;
    f->qtype = 0;  // QTFILE
    f->offset = 0;
    r->fid->aux = f;
    
    respond(r, nil);
}

// Initialize null translator with options
int
null_init(Translator9P *t, char **argv)
{
    int i;
    
    // Parse command line options
    for(i = 0; argv[i]; i++) {
        if(strcmp(argv[i], "--full") == 0 || strcmp(argv[i], "-f") == 0) {
            null_trans.write_error_code = ENOSPC;
            print("null: simulating full disk\n");
        }
    }
    
    return 0;
}

// Create null translator instance
Translator9P null_translator = {
    .name = "null",
    .open = null_open,
    .read = null_read,
    .write = null_write,
    .stat = null_stat,
    .init = null_init,
    .destroyfid = translator_clunk,  // Use generic cleanup
};

TRANSLATOR_MAIN(&null_translator)