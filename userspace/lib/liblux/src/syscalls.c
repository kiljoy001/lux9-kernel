#include "lux_internal.h"

extern uint convS2M(Fcall *f, uchar *ap, uint n);
extern uint convM2S(uchar *ap, uint n, Fcall *f);
extern void *memmove(void *dst, const void *src, ulong n);

int lux_call(Fcall *tx, Fcall *rx) {
    uchar *page = (uchar*)EXCHANGE_PAGE_ADDR;
    
    /* 1. Marshal Request */
    int n = convS2M(tx, page + P9_MSG_OFFSET, P9_MSG_SIZE);
    if (n <= 0) return -1;
    
    /* 2. Ring Doorbell */
    _syscall();
    
    /* 3. Unmarshal Reply */
    if (convM2S(page + P9_MSG_OFFSET, P9_MSG_SIZE, rx) <= 0) return -1;
    
    if (rx->type == Rerror) return -1;
    return 0;
}

int sys_open(char *path, int mode) {
    Fcall tx, rx;
    tx.type = Tsysopen;
    tx.tag = 1;
    tx.fid = 0;
    tx.name = path;
    tx.mode = mode;
    
    if (lux_call(&tx, &rx) < 0) return -1;
    return rx.fid;
}

int sys_close(int fd) {
    Fcall tx, rx;
    tx.type = Tsysclose;
    tx.tag = 1;
    tx.fid = fd;
    
    if (lux_call(&tx, &rx) < 0) return -1;
    return 0;
}

long sys_read(int fd, void *buf, long n) {
    Fcall tx, rx;
    tx.type = Tsysread;
    tx.tag = 1;
    tx.fid = fd;
    tx.offset = 0; 
    tx.count = n;
    
    if (lux_call(&tx, &rx) < 0) return -1;
    
    if (rx.count > n) rx.count = n;
    memmove(buf, rx.data, rx.count);
    return rx.count;
}

long sys_write(int fd, void *buf, long n) {
    Fcall tx, rx;
    tx.type = Tsyswrite;
    tx.tag = 1;
    tx.fid = fd;
    tx.offset = 0;
    tx.count = n;
    tx.data = buf;
    
    if (lux_call(&tx, &rx) < 0) return -1;
    return rx.count;
}

long sys_pwrite(int fd, void *buf, long n, long offset) {
    Fcall tx, rx;
    tx.type = Tsyspwrite;
    tx.tag = 1;
    tx.fid = fd;
    tx.offset = offset;
    tx.count = n;
    tx.data = buf;
    
    if (lux_call(&tx, &rx) < 0) return -1;
    return rx.count;
}

void sys_exit(char *msg) {
    Fcall tx, rx;
    tx.type = Tsysexit;
    tx.tag = 1;
    tx.ename = msg;

    lux_call(&tx, &rx);
    while(1);
}

int sys_create(char *path, int mode, uint perm) {
    Fcall tx, rx;
    tx.type = Tsyscreate;
    tx.tag = 1;
    tx.name = path;
    tx.mode = mode;
    tx.perm = perm;

    if (lux_call(&tx, &rx) < 0) return -1;
    return rx.fid;
}

int sys_rfork(int flags) {
    Fcall tx, rx;
    tx.type = Tsyscall;
    tx.tag = 1;
    tx.scallnr = 19; /* SYS_RFORK */
    tx.scount = 8;  /* argc (4 bytes) + flags (4 bytes) */
    uchar buf[8];
    *(uint*)buf = 1; /* argc */
    *(uint*)(buf+4) = flags;
    tx.sdata = buf;

    if (lux_call(&tx, &rx) < 0) return -1;
    return (int)rx.retval;
}

void sys_exec(char *path) {
    Fcall tx, rx;
    tx.type = Texec;
    tx.tag = 1;
    tx.count = 0;
    tx.name = path;

    lux_call(&tx, &rx);
    /* If we return, exec failed */
}

int sys_pipe(int *fds) {
    Fcall tx, rx;
    tx.type = Tsyscall;
    tx.tag = 1;
    tx.scallnr = 21; /* SYS_PIPE */
    tx.scount = 0;
    tx.sdata = 0;

    if (lux_call(&tx, &rx) < 0) return -1;
    if (rx.scount >= 8) {
        fds[0] = *(int*)rx.sdata;
        fds[1] = *(int*)(rx.sdata + 4);
    }
    return 0;
}

long sys_seek(int fd, long offset, int whence) {
    Fcall tx, rx;
    tx.type = Tsyscall;
    tx.tag = 1;
    tx.scallnr = 39; /* SYS_SEEK */
    tx.scount = 20;  /* argc:4 + fd:4 + offset:8 + whence:4 */
    uchar buf[20];
    *(uint*)buf = 3; /* argc */
    *(int*)(buf+4) = fd;
    *(vlong*)(buf+8) = offset;
    *(int*)(buf+16) = whence;
    tx.sdata = buf;

    if (lux_call(&tx, &rx) < 0) return -1;
    return (long)rx.retval;
}

int sys_wait(void) {
    Fcall tx, rx;
    tx.type = Tsyscall;
    tx.tag = 1;
    tx.scallnr = 36; /* SYS_WAIT */
    tx.scount = 0;
    tx.sdata = 0;

    if (lux_call(&tx, &rx) < 0) return -1;
    return (int)rx.retval;
}

long sys_pread(int fd, void *buf, long n, long offset) {
    Fcall tx, rx;
    tx.type = Tsyscall;
    tx.tag = 1;
    tx.scallnr = 50; /* SYS_PREAD */
    tx.scount = 20;  /* argc:4 + fd:4 + offset:8 + n:4 */
    uchar cbuf[20];
    *(uint*)cbuf = 3; /* argc */
    *(int*)(cbuf+4) = fd;
    *(vlong*)(cbuf+8) = offset;
    *(uint*)(cbuf+16) = n;
    tx.sdata = cbuf;

    if (lux_call(&tx, &rx) < 0) return -1;
    if (rx.scount > n) rx.scount = n;
    if (rx.scount > 0)
        memmove(buf, rx.sdata, rx.scount);
    return rx.scount;
}

uvlong sys_nsec(void) {
    Fcall tx, rx;
    tx.type = Tsyscall;
    tx.tag = 1;
    tx.scallnr = 53; /* SYS_NSEC */
    tx.scount = 0;
    tx.sdata = 0;

    if (lux_call(&tx, &rx) < 0) return 0;
    return rx.retval;
}

int sys_stat(char *path, uchar *buf, int nbuf) {
    Fcall tx, rx;
    tx.type = Tsysstat;
    tx.tag = 1;
    tx.name = path;

    if (lux_call(&tx, &rx) < 0) return -1;
    /* rx should contain stat data in rx.stat */
    /* For now, just return success indicator */
    return 0;
}

int sys_wstat(char *path, uchar *buf, int nbuf) {
    Fcall tx, rx;
    tx.type = Tsyscall;
    tx.tag = 1;
    tx.scallnr = 44; /* SYS_WSTAT */
    /* TODO: Implement proper stat buffer handling */
    tx.scount = 0;
    tx.sdata = 0;

    if (lux_call(&tx, &rx) < 0) return -1;
    return 0;
}

int sys_mount(int fd, int afd, char *old, int flags, char *aname) {
    Fcall tx, rx;
    tx.type = Tsyscall;
    tx.tag = 1;
    tx.scallnr = 46; /* SYS_MOUNT */
    /* TODO: Implement proper mount argument encoding */
    tx.scount = 0;
    tx.sdata = 0;

    if (lux_call(&tx, &rx) < 0) return -1;
    return 0;
}