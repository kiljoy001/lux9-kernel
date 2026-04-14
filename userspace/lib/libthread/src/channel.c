#include <thread.h>
#include <lux.h>

extern void *malloc(unsigned long size);
extern void free(void *ptr);

/*
 * Secure Channel Implementation (Pipe-based)
 * 
 * Uses kernel pipes to transfer data between isolated processes.
 * This guarantees:
 * 1. Synchronization (read blocks until data available)
 * 2. Isolation (data is copied, not shared)
 */

struct Channel {
    int elsize;
    int fd[2]; /* fd[0]=read, fd[1]=write */
};

extern int sys_pipe(int *fds);
extern long sys_read(int fd, void *buf, long n);
extern long sys_write(int fd, void *buf, long n);
extern int sys_close(int fd);

Channel* chancreate(int elsize, int bufsize) {
    /* 
     * bufsize is ignored in this Pipe implementation; 
     * pipes have fixed kernel buffering (typically 4KB-64KB).
     */
    Channel *c = malloc(sizeof(Channel));
    if (!c) return 0;
    
    c->elsize = elsize;
    if (sys_pipe(c->fd) < 0) {
        free(c);
        return 0;
    }
    
    return c;
}

void chanfree(Channel *c) {
    if (c) {
        sys_close(c->fd[0]);
        sys_close(c->fd[1]);
        free(c);
    }
}

int chansend(Channel *c, void *v) {
    /* Write to pipe. Kernel blocks if full. */
    long n = sys_write(c->fd[1], v, c->elsize);
    return (n == c->elsize);
}

int chanrecv(Channel *c, void *v) {
    /* Read from pipe. Kernel blocks if empty. */
    long n = sys_read(c->fd[0], v, c->elsize);
    return (n == c->elsize);
}
