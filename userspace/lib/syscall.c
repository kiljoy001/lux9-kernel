/*
 * Syscall bridge - Plan 9 style syscalls for Lux9
 */
#include <u.h>
#include <libc.h>
#include "syscall.h"
#include "syscall_ops.h"
#include <sys/types.h>
#include <stdint.h>
#include <sys.h>

int rfork(int flags) {
    return (int)syscall1(RFORK, flags);
}

int exec(char *path, char *argv[]) {
    return (int)syscall2(EXEC, (long)path, (long)argv);
}

void _exits(char *status) {
    syscall1(EXITS, (long)status);
    for(;;);
}

int await(char *s, int n) {
    return (int)syscall2(AWAIT, (long)s, n);
}

int open(char *path, int flags) {
    return (int)syscall3(OPEN, (long)path, flags, 0);
}

int close(int fd) {
    return (int)syscall1(CLOSE, fd);
}

long read(int fd, void *buf, long count) {
    return syscall3(_READ, fd, (long)buf, count);
}

long write(int fd, void *buf, long count) {
    return syscall3(_WRITE, fd, (long)buf, count);
}

int pipe(int *fd) {
    return (int)syscall1(PIPE, (long)fd);
}

int mount(int fd, int afd, char *old, int flag, char *spec) {
    return (int)syscall5(MOUNT, fd, afd, (long)old, flag, (long)spec);
}

int bind(char *new, char *old, int flag) {
    return (int)syscall3(BIND, (long)new, (long)old, flag);
}

void *sbrk(ulong increment) {
    return (void*)syscall1(BRK_, increment);
}

int sleep(long ms) {
    return (int)syscall1(SLEEP, ms);
}

void* pebble_issue_white(ulong size) {
    long result = syscall2(PEBBLE_WHITE_ISSUE, size, 0);
    return (void*)result;
}

int pebble_black_alloc(ulong size, void **handle) {
    if(handle == nil)
        return -1;
    long result = syscall2(PEBBLE_BLACK_ALLOC, size, (long)handle);
    return (int)result;
}

int pebble_black_free(void *handle) {
    return (int)syscall1(PEBBLE_BLACK_FREE, (long)handle);
}

ExchangeHandle exchange_prepare(ulong vaddr) {
    return (ExchangeHandle)syscall1(VMEXCHANGE, vaddr);
}

int exchange_accept(ExchangeHandle handle, ulong dest_vaddr, int prot) {
    return (int)syscall3(VMLEND_SHARED, handle, dest_vaddr, prot);
}
