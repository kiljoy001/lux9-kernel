#include <thread.h>
#include <lux.h>

/* Using internal liblux definitions if needed, or re-declaring for standalone */
extern int sys_rfork(int flags);
extern void sys_exit(char *msg);
extern int sys_semacquire(int *addr, int block); // If available, else spin
extern void sys_semrelease(int *addr, int count);
extern uintptr sys_rendezvous(uintptr tag, uintptr val);
extern int sys_sleep(long ms);

/* Flags from kernel/include/sys.h or similar */
#define RFPROC      (1<<4)
#define RFMEM       (1<<5)
#define RFNOWAIT    (1<<6)
#define RFCNAMEG    (1<<10)
#define RFCENVG     (1<<11)
#define RFCFDG      (1<<12)
#define RFFDG       (1<<2) 

/* 
 * Threading implementation 
 * 
 * NOTE: We default to RFFDG to give threads their own FD tables, 
 * preventing the race condition seen in resurrection.c.
 * Plan 9 libthread usually shares FDs, but safety first here.
 */

#define DEFAULT_STACK 32768

/* Naked implementation of stack switching if needed, 
 * but for now we let the kernel do it via rfork stack logic if feasible,
 * or we manually alloc stack.
 * sys_rfork copies the current stack pointer. We need to move it.
 */

/* Assembly helper to switch stack and call threadstart */
/* For x86_64 */
void _thread_launcher(void *stack_top, ThreadFn fn, void *arg); 
/* Defined in thread_asm.S */

int threadcreate(ThreadFn fn, void *arg, uint stacksize) {
    (void)stacksize; /* Stack managed by kernel via COW */
    /* 
     * SECURE TASK CREATION (Share-Nothing)
     * 
     * We do NOT use RFMEM. This creates a new process with a 
     * Copy-On-Write view of the parent's memory.
     * 
     * - No shared globals (eliminates data races).
     * - No shared stack/heap (eliminates corruption vectors).
     * - Communication MUST happen via Channels (Pipes).
     * 
     * Flags:
     * RFPROC: New process ID
     * RFFDG:  Copy FD table (so we can share the Pipe FDs created before fork)
     * RFNOWAIT: Decouple lifecycle
     */
    int pid = sys_rfork(RFPROC | RFFDG | RFNOWAIT);
    
    if (pid < 0) {
        return -1;
    }
    
    if (pid == 0) {
        /* Child Task */
        /* We are on a COW copy of the parent's stack. 
           We don't need to manually switch stacks. */
        fn(arg);
        sys_exit(0);
    }
    
    /* Parent */
    return pid;
}

/* Wrapper to match ABI if needed, or just called directly */
void threadstart_wrapper(ThreadFn fn, void *arg) {
    fn(arg);
    sys_exit(0);
}

void threadexit(void) {
    sys_exit(0);
}

void thread_yield(void) {
    sys_sleep(0);
}

/* Simple Spinlock */
void lock(Lock *l) {
    int i = 1;
    while(1) {
        /* xchg */
        __asm__ volatile("xchgl %0, %1" : "+r"(i), "+m"(l->val));
        if (i == 0) return; /* Acquired */
        i = 1;
        sys_sleep(0); /* Yield while waiting */
    }
}

void unlock(Lock *l) {
    int i = 0;
    __asm__ volatile("xchgl %0, %1" : "+r"(i), "+m"(l->val));
}

int canlock(Lock *l) {
    int i = 1;
    __asm__ volatile("xchgl %0, %1" : "+r"(i), "+m"(l->val));
    return (i == 0);
}
