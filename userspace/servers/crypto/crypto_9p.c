/* 9P filesystem implementation for crypto service */
#include <u.h>
#include <libc.h>
#include <fcall.h>
#include "crypto_abstraction.h"

/* External algorithm declarations */
extern CryptoRegistry crypto_registry;

/* File system structure */
#define CRYPTO_ROOT_QID    ((Qid){0, 0, QTDIR})
#define CRYPTO_HASH_QID    ((Qid){1, 0, QTDIR})
#define CRYPTO_SIGN_QID    ((Qid){2, 0, QTDIR})

/* FID state management */
typedef struct CryptoFid CryptoFid;
struct CryptoFid {
    uint32_t fid;
    Qid qid;
    int open;           /* Is the fid open? */
    CryptoContext *ctx; /* Crypto context for operations */
    CryptoAlgorithm *algo; /* Algorithm associated with this fid */
    uint8_t *result;    /* Result buffer */
    size_t result_len;  /* Length of result */
    int finalized;      /* Has the operation been finalized? */
    CryptoFid *next;
};

static CryptoFid *fid_list = nil;

/* QID generation */
static Qid
make_qid(uint64_t path, uchar type)
{
    Qid q;
    q.path = path;
    q.vers = 0;
    q.type = type;
    return q;
}

/* FID management */
static CryptoFid*
get_fid(uint32_t fid)
{
    CryptoFid *f;
    
    for(f = fid_list; f; f = f->next)
        if(f->fid == fid)
            return f;
    return nil;
}

static CryptoFid*
new_fid(uint32_t fid)
{
    CryptoFid *f;
    
    f = malloc(sizeof(CryptoFid));
    if(f == nil)
        return nil;
        
    memset(f, 0, sizeof(CryptoFid));
    f->fid = fid;
    f->next = fid_list;
    fid_list = f;
    
    return f;
}

static void
free_fid(CryptoFid *f)
{
    CryptoFid **l;
    
    if(f == nil)
        return;
        
    /* Remove from list */
    for(l = &fid_list; *l; l = &(*l)->next) {
        if(*l == f) {
            *l = f->next;
            break;
        }
    }
    
    /* Clean up crypto context */
    if(f->ctx) {
        if(f->algo && f->algo->cleanup)
            f->algo->cleanup(f->ctx);
        free(f->ctx);
    }
    
    /* Clean up result buffer */
    if(f->result)
        free(f->result);
        
    free(f);
}

/* File system operations */
static int
is_dir(Qid qid)
{
    return (qid.type & QTDIR) != 0;
}

static char*
crypto_walk(Req *r)
{
    char *name;
    Qid qid;
    int i;
    
    for(i = 0; i < r->ifcall.nwname; i++) {
        name = r->ifcall.wname[i];
        
        /* First level - root directory */
        if(r->fid->qid.path == CRYPTO_ROOT_QID.path) {
            if(strcmp(name, "hash") == 0) {
                qid = CRYPTO_HASH_QID;
            } else if(strcmp(name, "sign") == 0) {
                qid = CRYPTO_SIGN_QID;
            } else {
                break; /* Not found */
            }
        }
        /* Second level - hash directory */
        else if(r->fid->qid.path == CRYPTO_HASH_QID.path) {
            /* Look for registered hash algorithms */
            int j;
            for(j = 0; j < crypto_registry.count; j++) {
                if(crypto_registry.algorithms[j]->category == CRYPTO_CAT_HASH &&
                   strcmp(crypto_registry.algorithms[j]->name, name) == 0) {
                    qid = make_qid(100 + j, QTFILE); /* Algorithm file */
                    break;
                }
            }
            if(j >= crypto_registry.count)
                break; /* Not found */
        }
        /* Second level - sign directory */
        else if(r->fid->qid.path == CRYPTO_SIGN_QID.path) {
            /* Look for registered signature algorithms */
            int j;
            for(j = 0; j < crypto_registry.count; j++) {
                if(crypto_registry.algorithms[j]->category == CRYPTO_CAT_SIGN &&
                   strcmp(crypto_registry.algorithms[j]->name, name) == 0) {
                    qid = make_qid(200 + j, QTFILE); /* Algorithm file */
                    break;
                }
            }
            if(j >= crypto_registry.count)
                break; /* Not found */
        }
        else {
            break; /* Not found */
        }
        
        r->ofcall.wqid[i] = qid;
        r->fid->qid = qid;
    }
    
    r->ofcall.nwqid = i;
    if(i == 0)
        return "file not found";
    return nil;
}

static char*
crypto_open(Req *r)
{
    CryptoFid *f;
    int mode = r->ifcall.mode;
    
    /* Can't open directories for I/O */
    if(is_dir(r->fid->qid))
        return "can't open directory for I/O";
        
    /* Check permissions */
    if((mode & OTMP) != 0)
        return "temporary files not supported";
        
    /* Create FID state */
    f = new_fid(r->fid->fid);
    if(f == nil)
        return "out of memory";
        
    f->qid = r->fid->qid;
    f->open = 1;
    
    /* For algorithm files, create crypto context */
    if(r->fid->qid.path >= 100) {
        int algo_idx = r->fid->qid.path - 100;
        if(algo_idx < crypto_registry.count) {
            f->algo = crypto_registry.algorithms[algo_idx];
            
            /* Create crypto context */
            f->ctx = malloc(sizeof(CryptoContext));
            if(f->ctx == nil) {
                free_fid(f);
                return "out of memory";
            }
            memset(f->ctx, 0, sizeof(CryptoContext));
            
            /* Initialize algorithm */
            if(f->algo->init && f->algo->init(f->ctx) != 0) {
                free_fid(f);
                return "failed to initialize algorithm";
            }
        }
    }
    
    r->fid->aux = f;
    r->ofcall.qid = r->fid->qid;
    r->ofcall.iounit = 8192;
    
    return nil;
}

static char*
crypto_create(Req *r)
{
    return "create not supported";
}

static char*
crypto_read(Req *r)
{
    CryptoFid *f;
    uint8_t *buf = r->ofcall.data;
    uint32_t count = r->ifcall.count;
    uint64_t offset = r->ifcall.offset;
    
    f = r->fid->aux;
    if(f == nil)
        return "invalid fid";
        
    /* For algorithm files, return the result if available */
    if(r->fid->qid.path >= 100) {
        if(f->algo == nil)
            return "no algorithm associated";
            
        /* If not finalized yet, finalize now */
        if(!f->finalized && f->ctx) {
            f->result = malloc(f->algo->output_bytes);
            if(f->result == nil)
                return "out of memory";
                
            if(f->algo->final(f->ctx, f->result, &f->result_len) != 0) {
                free(f->result);
                f->result = nil;
                return "failed to finalize operation";
            }
            
            f->finalized = 1;
        }
        
        /* Return result data */
        if(f->result && offset < f->result_len) {
            uint32_t n = f->result_len - offset;
            if(n > count)
                n = count;
            memmove(buf, f->result + offset, n);
            r->ofcall.count = n;
        } else {
            r->ofcall.count = 0;
        }
    } else {
        /* Directory reads not implemented in this simplified version */
        r->ofcall.count = 0;
    }
    
    return nil;
}

static char*
crypto_write(Req *r)
{
    CryptoFid *f;
    uint8_t *buf = r->ifcall.data;
    uint32_t count = r->ifcall.count;
    
    f = r->fid->aux;
    if(f == nil)
        return "invalid fid";
        
    /* For algorithm files, feed data to the crypto context */
    if(r->fid->qid.path >= 100) {
        if(f->algo == nil)
            return "no algorithm associated";
            
        if(f->finalized)
            return "operation already finalized";
            
        if(f->ctx && f->algo->update) {
            if(f->algo->update(f->ctx, buf, count) != 0)
                return "failed to update algorithm";
        }
        
        r->ofcall.count = count;
    } else {
        return "write not supported on this file";
    }
    
    return nil;
}

static char*
crypto_clunk(Req *r)
{
    CryptoFid *f;
    
    f = r->fid->aux;
    if(f) {
        /* Clean up FID state */
        free_fid(f);
        r->fid->aux = nil;
    }
    
    return nil;
}

static char*
crypto_remove(Req *r)
{
    return "remove not supported";
}

static char*
crypto_stat(Req *r)
{
    Stat *st = &r->ofcall.stat;
    
    /* Set up basic stat structure */
    st->type = 0;
    st->dev = 0;
    st->qid = r->fid->qid;
    st->mode = 0755;
    st->atime = time(nil);
    st->mtime = st->atime;
    st->length = 0;
    st->name = strdup("unknown");
    st->uid = strdup("crypto");
    st->gid = strdup("crypto");
    st->muid = strdup("crypto");
    
    /* Customize based on file */
    if(r->fid->qid.path == CRYPTO_ROOT_QID.path) {
        st->mode |= DMDIR;
        free(st->name);
        st->name = strdup("/");
    } else if(r->fid->qid.path == CRYPTO_HASH_QID.path) {
        st->mode |= DMDIR;
        free(st->name);
        st->name = strdup("hash");
    } else if(r->fid->qid.path == CRYPTO_SIGN_QID.path) {
        st->mode |= DMDIR;
        free(st->name);
        st->name = strdup("sign");
    } else if(r->fid->qid.path >= 100) {
        /* Algorithm file */
        int algo_idx = r->fid->qid.path - 100;
        if(algo_idx < crypto_registry.count) {
            free(st->name);
            st->name = strdup(crypto_registry.algorithms[algo_idx]->name);
            st->length = crypto_registry.algorithms[algo_idx]->output_bytes;
        }
    }
    
    return nil;
}

static char*
crypto_wstat(Req *r)
{
    return "wstat not supported";
}

/* 9P service setup */
Srv crypto_srv = {
    .attach = nil,      /* Use default */
    .walk = crypto_walk,
    .open = crypto_open,
    .create = crypto_create,
    .read = crypto_read,
    .write = crypto_write,
    .clunk = crypto_clunk,
    .remove = crypto_remove,
    .stat = crypto_stat,
    .wstat = crypto_wstat,
};

/* Start the 9P server */
int
crypto_server_start(void)
{
    /* Set up the root directory */
    crypto_srv.tree = nil; /* No persistent tree, we handle everything manually */
    
    /* Start serving on standard input/output */
    crypto_srv.infd = 0;  /* stdin */
    crypto_srv.outfd = 1; /* stdout */
    crypto_srv.msize = 8192;
    
    /* Serve requests */
    srv(&crypto_srv);
    
    return 0;
}