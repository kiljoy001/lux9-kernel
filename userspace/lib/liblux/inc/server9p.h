#ifndef _LIBLUX_SERVER9P_H_
#define _LIBLUX_SERVER9P_H_

#include <u.h>
#include <libc.h>
#include <fcall.h>

typedef struct Srv Srv;
typedef struct Req Req;
typedef struct Fid Fid;

/* A 9P Request */
struct Req {
    Fcall ifcall; /* Input 9P message (T-message) */
    Fcall ofcall; /* Output 9P message (R-message) */
    Req *next;    /* For queuing */
    Srv *srv;     /* The server handling this */
    Fid *fid;     /* The FID this request acts upon */
    Fid *newfid;  /* The new FID (for Tclone/Twalk) */
    void *aux;    /* Per-request auxiliary data */
    int responding; /* Set if we are sending a response */
};

/* A 9P File ID (Active file handle) */
struct Fid {
    u32int fid;
    int ref;      /* Reference count */
    int omode;    /* Open mode (-1 if not open) */
    Qid qid;      /* The unique file ID on server */
    void *aux;    /* Per-file auxiliary data (e.g. Inode pointer) */
    Fid *next;    /* Hash bucket link */
};

/* The 9P Server Definition */
struct Srv {
    /* Configuration */
    int debug;
    
    /* Callbacks - NULL means return Rerror "not implemented" */
    void (*attach)(Req *r);
    void (*walk)(Req *r);
    void (*open)(Req *r);
    void (*create)(Req *r);
    void (*read)(Req *r);
    void (*write)(Req *r);
    void (*clunk)(Req *r); /* Close/Clunk */
    void (*remove)(Req *r);
    void (*stat)(Req *r);
    void (*wstat)(Req *r);
    void (*flush)(Req *r);
    
    /* State */
    void *aux;
    /* Private */
    Fid **fidhash; /* Hash table of FIDs */
    int fidhashsize;
};

/* API */
void srv_init(Srv *s);
void srv_loop(Srv *s, int fd_in, int fd_out);
void srv_respond(Req *r, char *error);

/* Helper to initialize Qid */
void mkqid(Qid *q, u64int path, u32int vers, u8int type);

#endif
