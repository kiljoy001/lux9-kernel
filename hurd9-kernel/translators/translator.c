// translator.c - Framework implementation

#include "translator.h"

// Generic 9P operations for translators
void
translator_stat(Req *r)
{
    Translator9P *t = r->srv->aux;
    TranslatorFile *f = r->fid->aux;
    Dir d = {0};
    
    if(f == nil) {
        respond(r, "bad file");
        return;
    }
    
    d.qid.type = f->qtype;
    d.qid.vers = 1;
    d.qid.path = (uintptr)f;
    d.mode = f->mode;
    d.length = 0;  // Will be set by specific translator
    d.name = f->name;
    d.uid = "translator";
    d.gid = "translator";
    d.muid = "";
    
    r->d = d;
    
    // Let specific translator modify
    if(t->stat)
        t->stat(r);
    else
        respond(r, nil);
}

void
translator_open(Req *r)
{
    Translator9P *t = r->srv->aux;
    TranslatorFile *f;
    
    // Create file handle
    f = malloc(sizeof(*f));
    if(f == nil) {
        respond(r, "out of memory");
        return;
    }
    
    f->name = "default";
    f->mode = 0444;  // Default read-only
    f->qtype = 0;    // QTFILE
    f->offset = 0;
    
    r->fid->aux = f;
    
    // Let specific translator handle
    if(t->open)
        t->open(r);
    else
        respond(r, nil);
}

void
translator_clunk(Req *r)
{
    if(r->fid->aux) {
        free(r->fid->aux);
        r->fid->aux = nil;
    }
    respond(r, nil);
}

// Stack one translator on another
int
translator_stack(Translator9P *above, Translator9P *below)
{
    if(above == nil || below == nil)
        return -1;
        
    above->below = below;
    below->above = above;
    
    return 0;
}

// Serve translator
void
serve_translator(Translator9P *t, char *srvname, char *mtpt)
{
    Srv s = {0};
    
    // Set up 9P operations
    s.open = t->open ?: translator_open;
    s.read = t->read;
    s.write = t->write; 
    s.stat = t->stat ?: translator_stat;
    s.create = t->create;
    s.remove = t->remove;
    s.wstat = t->wstat;
    s.destroyfid = t->destroyfid;
    
    // Store translator in srv
    s.aux = t;
    
    print("%s translator serving %s\n", t->name, srvname);
    
    threadpostmountsrv(&s, srvname, mtpt, MREPL);
}

// Simple translator implementation
void
simple_read(Req *r)
{
    SimpleTranslator *st = (SimpleTranslator*)r->srv->aux;
    TranslatorFile *f = r->fid->aux;
    off_t offset = r->ifcall.offset;
    u32int count = r->ifcall.count;
    
    // Use file offset if -1
    if(offset == -1)
        offset = f->offset;
    
    // Bounds check
    if(offset > st->content_len)
        offset = st->content_len;
    if(offset + count > st->content_len)
        count = st->content_len - offset;
    
    if(count > 0) {
        readbuf(r, st->content + offset, count);
        f->offset = offset + count;
    } else {
        r->ofcall.count = 0;
    }
    
    respond(r, nil);
}

void
simple_write(Req *r)
{
    SimpleTranslator *st = (SimpleTranslator*)r->srv->aux;
    
    if(!st->writable) {
        respond(r, "file is read-only");
        return;
    }
    
    // For writable simple translators, just accept the write
    r->ofcall.count = r->ifcall.count;
    respond(r, nil);
}

void
simple_stat(Req *r)
{
    SimpleTranslator *st = (SimpleTranslator*)r->srv->aux;
    
    // Call generic stat first
    translator_stat(r);
    
    // Set content length
    if(r->d.type == 0)  // QTFILE
        r->d.length = st->content_len;
}

SimpleTranslator*
simple_translator_new(char *name, char *content, int readable, int writable)
{
    SimpleTranslator *st = malloc(sizeof(*st));
    if(st == nil)
        return nil;
        
    memset(st, 0, sizeof(*st));
    
    // Set up base translator
    st->base.name = strdup(name);
    st->base.read = readable ? simple_read : nil;
    st->base.write = writable ? simple_write : nil;
    st->base.stat = simple_stat;
    
    // Set up content
    st->content = strdup(content);
    st->content_len = strlen(content);
    st->readable = readable;
    st->writable = writable;
    
    return st;
}

// Device translator implementation
void
device_read(Req *r)
{
    DeviceTranslator *dt = (DeviceTranslator*)r->srv->aux;
    TranslatorFile *f = r->fid->aux;
    char *name = f->name;
    
    if(strcmp(name, "ctl") == 0) {
        readstr(r, dt->ctl_content ?: "");
    }
    else if(strcmp(name, "status") == 0) {
        readstr(r, dt->status_content ?: "state=idle\n");
    }
    else if(strcmp(name, "info") == 0) {
        readstr(r, dt->info_content ?: "type=device\n");
    }
    else if(strcmp(name, "data") == 0) {
        if(dt->device_read) {
            void *buf = malloc(r->ifcall.count);
            int n = dt->device_read(buf, r->ifcall.count, r->ifcall.offset);
            if(n > 0) {
                readbuf(r, buf, n);
            } else {
                r->ofcall.count = 0;
            }
            free(buf);
        } else {
            r->ofcall.count = 0;
        }
    }
    else {
        respond(r, "no such file");
        return;
    }
    
    respond(r, nil);
}

void
device_write(Req *r)
{
    DeviceTranslator *dt = (DeviceTranslator*)r->srv->aux;
    TranslatorFile *f = r->fid->aux;
    char *name = f->name;
    
    if(strcmp(name, "ctl") == 0) {
        if(dt->device_ctl) {
            char *response = nil;
            char cmd[256];
            snprint(cmd, sizeof(cmd), "%.*s", 
                   r->ifcall.count, (char*)r->ifcall.data);
            
            int result = dt->device_ctl(cmd, &response);
            if(response) {
                free(dt->ctl_content);
                dt->ctl_content = response;
            }
        }
        r->ofcall.count = r->ifcall.count;
    }
    else if(strcmp(name, "data") == 0) {
        if(dt->device_write) {
            int n = dt->device_write(r->ifcall.data, r->ifcall.count, r->ifcall.offset);
            r->ofcall.count = n;
        } else {
            respond(r, "device not writable");
            return;
        }
    }
    else {
        respond(r, "file not writable");
        return;
    }
    
    respond(r, nil);
}

DeviceTranslator*
device_translator_new(char *name)
{
    DeviceTranslator *dt = malloc(sizeof(*dt));
    if(dt == nil)
        return nil;
        
    memset(dt, 0, sizeof(*dt));
    
    // Set up base translator
    dt->base.name = strdup(name);
    dt->base.read = device_read;
    dt->base.write = device_write;
    
    // Default content
    dt->ctl_content = strdup("");
    dt->status_content = strdup("state=idle\n");
    dt->info_content = smprint("type=%s\nversion=1.0\n", name);
    
    return dt;
}

// Utility functions
int
parse_translator_args(int argc, char **argv, char **srvname, char **mtpt)
{
    int i;
    
    *srvname = nil;
    *mtpt = nil;
    
    for(i = 1; i < argc; i++) {
        if(strcmp(argv[i], "-s") == 0 && i+1 < argc) {
            *srvname = argv[++i];
        }
        else if(strcmp(argv[i], "-m") == 0 && i+1 < argc) {
            *mtpt = argv[++i];
        }
    }
    
    return 0;
}

char*
hurd_error_to_9p(error_t err)
{
    switch(err) {
    case 0: return nil;
    case ENOENT: return "file not found";
    case EACCES: return "permission denied";
    case ENOMEM: return "out of memory";
    case EBADF: return "bad file descriptor";
    case EBUSY: return "device busy";
    case EIO: return "i/o error";
    default: return "error";
    }
}