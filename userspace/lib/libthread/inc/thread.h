#ifndef _LIBTHREAD_H_
#define _LIBTHREAD_H_

#include <lux.h>

/* Threading */
typedef struct Thread Thread;
typedef void (*ThreadFn)(void *arg);

int threadcreate(ThreadFn fn, void *arg, uint stacksize);
void threadexit(void);
int threadpid(void);
void thread_yield(void);

/* Locks */
typedef struct Lock {
    int val;
} Lock;

void lock(Lock *l);
void unlock(Lock *l);
int canlock(Lock *l);

/* Channels */
typedef struct Channel Channel;

Channel* chancreate(int elsize, int bufsize);
int chansend(Channel *c, void *v);
int chanrecv(Channel *c, void *v);
void chanfree(Channel *c);

/* Rendezvous (Low level) */
void rendezvous(uintptr tag, uintptr val);

#endif
