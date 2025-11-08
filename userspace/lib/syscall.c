/*
 * Syscall bridge - Plan 9 style syscalls for Lux9
 * Based on 9front libc implementation
 */
#include <stdarg.h>
#include <stdint.h>
#include <sys/types.h>

/* Syscall numbers must match kernel definitions */
#include "../../../kernel/include/sys.h"

/* External syscall entry point - implemented in assembly */
extern long __syscall(long n, long a1, long a2, long a3, long a4, long a5, long a6);

/* Syscall wrapper functions */
static inline long syscall1(long n, long a1) {
    return __syscall(n, a1, 0, 0, 0, 0, 0);
}

static inline long syscall2(long n, long a1, long a2) {
    return __syscall(n, a1, a2, 0, 0, 0, 0);
}

static inline long syscall3(long n, long a1, long a2, long a3) {
    return __syscall(n, a1, a2, a3, 0, 0, 0);
}

static inline long syscall4(long n, long a1, long a2, long a3, long a4) {
    return __syscall(n, a1, a2, a3, a4, 0, 0);
}

static inline long syscall5(long n, long a1, long a2, long a3, long a4, long a5) {
    return __syscall(n, a1, a2, a3, a4, a5, 0);
}

static inline long syscall6(long n, long a1, long a2, long a3, long a4, long a5, long a6) {
    return __syscall(n, a1, a2, a3, a4, a5, a6);
}

/* Process management syscalls */
int fork(void) {
    return (int)syscall1(RFORK, 0);
}

int exec(const char *path, char *const argv[]) {
    /* Convert argv to Plan 9 format */
    char **nargp;
    int argc, i;
    uintptr argp, q;
    
    if(path == nil)
        return -1;
    
    /* Count arguments */
    for(argc = 0; argv && argv[argc]; argc++)
        ;
    
    /* Convert to Plan 9 format */
    nargp = malloc((argc + 1) * sizeof(char*));
    if(nargp == nil)
        return -1;
    
    for(i = 0; i < argc; i++)
        nargp[i] = (char*)argv[i];
    nargp[argc] = nil;
    
    long result = syscall2(EXEC, (long)path, (long)nargp);
    free(nargp);
    return (int)result;
}

void exit(int status) {
    syscall1(EXITS, status);
    /* Should not return */
    for(;;);
}

int wait(int *status) {
    if(status == nil)
        return syscall1(_WAIT, 0);
    return (int)syscall2(_WAIT, (long)status, 0);
}

/* File operation syscalls */
int open(const char *path, int flags) {
    if(path == nil)
        return -1;
    return (int)syscall3(OPEN, (long)path, flags, 0);
}

int close(int fd) {
    return (int)syscall1(CLOSE, fd);
}

ssize_t read(int fd, void *buf, size_t count) {
    if(buf == nil || count == 0)
        return 0;
    return (ssize_t)syscall3(_READ, fd, (long)buf, count);
}

ssize_t write(int fd, const void *buf, size_t count) {
    if(buf == nil)
        return 0;
    if(count == 0)
        return 0;
    return (ssize_t)syscall3(_WRITE, fd, (long)buf, count);
}

/* Memory management */
void *sbrk(intptr increment) {
    return (void*)syscall1(BRK_, increment);
}

/* Time and I/O */
unsigned long sleep(unsigned long seconds) {
    return (unsigned long)syscall1(SLEEP, seconds);
}

int pipe(int *fd) {
    return (int)syscall1(PIPE, (long)fd);
}

/* Mount operations */
int mount(const char *path, int server_pid, const char *proto) {
    if(path == nil || proto == nil)
        return -1;
    return (int)syscall3(MOUNT, (long)path, server_pid, (long)proto);
}

/* Pebble system syscalls - wrapped for easier use */
void* pebble_issue_white(unsigned long size) {
    long result = syscall2(PEBBLE_WHITE_ISSUE, size, 0);
    return (void*)result;
}

int pebble_black_alloc(unsigned long size, void **handle) {
    if(handle == nil)
        return -1;
    long result = syscall2(PEBBLE_BLACK_ALLOC, size, (long)handle);
    return (int)result;
}

int pebble_black_free(void *handle) {
    return (int)syscall1(PEBBLE_BLACK_FREE, (long)handle);
}