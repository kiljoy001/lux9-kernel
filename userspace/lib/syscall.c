/*
 * Syscall bridge - Plan 9 style syscalls for Lux9
 * Based on 9front libc implementation
 */
#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>

#include "../../../kernel/include/sys.h"
#include "syscall.h"

/**
 * Invoke a system call with a single argument.
 * @param n Syscall number to invoke.
 * @param a1 First argument passed to the syscall.
 * @returns The raw syscall result as a `long`.
 */
static inline long syscall1(long n, long a1) {
    return __syscall(n, a1, 0, 0, 0, 0, 0);
}

/**
 * Invoke a syscall with two arguments.
 * @param n Syscall number.
 * @param a1 First syscall argument.
 * @param a2 Second syscall argument.
 * @returns The syscall result as a signed long.
 */
static inline long syscall2(long n, long a1, long a2) {
    return __syscall(n, a1, a2, 0, 0, 0, 0);
}

/**
 * Invoke a system call identified by its number with three arguments.
 * @param n Syscall number to invoke.
 * @param a1 First argument passed to the syscall.
 * @param a2 Second argument passed to the syscall.
 * @param a3 Third argument passed to the syscall.
 * @returns Result of the syscall as a `long`; negative values indicate failure.
 */
static inline long syscall3(long n, long a1, long a2, long a3) {
    return __syscall(n, a1, a2, a3, 0, 0, 0);
}

/**
 * Invoke the syscall identified by `n` with four arguments.
 *
 * The remaining syscall argument slots are zero-padded.
 *
 * @param n Syscall number to invoke.
 * @param a1 First syscall argument.
 * @param a2 Second syscall argument.
 * @param a3 Third syscall argument.
 * @param a4 Fourth syscall argument.
 * @returns The raw syscall result as a `long`.
 */
static inline long syscall4(long n, long a1, long a2, long a3, long a4) {
    return __syscall(n, a1, a2, a3, a4, 0, 0);
}

/**
 * Invoke a kernel syscall using five arguments.
 * @param n Syscall number.
 * @param a1 First syscall argument.
 * @param a2 Second syscall argument.
 * @param a3 Third syscall argument.
 * @param a4 Fourth syscall argument.
 * @param a5 Fifth syscall argument.
 * @returns The syscall result; a negative value indicates failure.
 */
static inline long syscall5(long n, long a1, long a2, long a3, long a4, long a5) {
    return __syscall(n, a1, a2, a3, a4, a5, 0);
}

/**
 * Invoke a system call with six arguments.
 *
 * @param n System call number.
 * @param a1 First argument to the system call.
 * @param a2 Second argument to the system call.
 * @param a3 Third argument to the system call.
 * @param a4 Fourth argument to the system call.
 * @param a5 Fifth argument to the system call.
 * @param a6 Sixth argument to the system call.
 * @returns The syscall result as `long`; negative values indicate an error. 
 */
static inline long syscall6(long n, long a1, long a2, long a3, long a4, long a5, long a6) {
    return __syscall(n, a1, a2, a3, a4, a5, a6);
}

/**
 * Create a new process by duplicating the calling process.
 * @returns Child PID to the parent process, `0` in the child process, `-1` on error.
 */
int fork(void) {
    return (int)syscall1(RFORK, 0);
}

/**
 * Execute the program at the given path, supplying argv as its argument vector.
 *
 * Converts the provided argv to the Plan 9-compatible format and invokes the EXEC syscall to replace
 * the current process image with the program at path.
 *
 * @param path Path to the executable; must not be NULL.
 * @param argv NULL-terminated argument vector to pass to the new program; may be NULL.
 * @returns The raw syscall result cast to int; `-1` on error (e.g., NULL path or allocation failure).
 */
int exec(const char *path, char *const argv[]) {
    if(path == NULL)
        return -1;
    return (int)syscall2(EXEC, (long)path, (long)argv);
}

/**
 * Terminate the calling process with the given exit status.
 *
 * This function does not return.
 *
 * @param status Exit status code passed to the kernel.
 */
void exit(int status) {
    syscall1(EXITS, status);
    /* Should not return */
    for(;;);
}

/**
 * Waits for a child process to change state, optionally storing its status.
 *
 * @param status Pointer to an int that will receive the child's exit status; if nil, no status is stored.
 * @returns The child's process ID on success, `-1` on error.
 */
int wait(int *status) {
    if(status == NULL)
        return syscall1(_WAIT, 0);
    return (int)syscall2(_WAIT, (long)status, 0);
}

/**
 * Open a file at the specified path using the provided flags.
 * @param path Path to the file to open; if NULL the call fails.
 * @param flags Flags that control how the file is opened.
 * @returns File descriptor on success, -1 on failure.
 */
int open(const char *path, int flags) {
    if(path == NULL)
        return -1;
    return (int)syscall3(OPEN, (long)path, flags, 0);
}

/**
 * Close an open file descriptor.
 *
 * @returns `0` on success, a negative value on failure.
 */
int close(int fd) {
    return (int)syscall1(CLOSE, fd);
}

/**
 * Read up to `count` bytes from file descriptor `fd` into `buf`.
 *
 * @param fd File descriptor to read from.
 * @param buf Destination buffer; must be non-NULL.
 * @param count Maximum number of bytes to read; must be greater than zero.
 * @returns Number of bytes actually read. Returns `0` if `buf` is NULL or `count` is `0`. Returns a negative value on error.
 */
ssize_t read(int fd, void *buf, size_t count) {
    if(buf == NULL || count == 0)
        return 0;
    return (ssize_t)syscall3(_READ, fd, (long)buf, count);
}

/**
 * Write data from a buffer to a file descriptor.
 *
 * @param fd File descriptor to write to.
 * @param buf Pointer to the data to write; if `buf` is nil the call returns 0.
 * @param count Number of bytes to write from `buf`; if `count` is 0 the call returns 0.
 * @returns Number of bytes written on success, or a negative value on error; returns 0 when `buf` is nil or `count` is 0.
 */
ssize_t write(int fd, const void *buf, size_t count) {
    if(buf == NULL)
        return 0;
    if(count == 0)
        return 0;
    return (ssize_t)syscall3(_WRITE, fd, (long)buf, count);
}

<<<<<<< HEAD
/**
 * Adjust the program break by the given increment.
 * @param increment Number of bytes to change the program break by; may be negative to reduce it.
 * @returns Pointer to the resulting program break, or `(void*)-1` on failure.
 */
void *sbrk(intptr_t increment) {
    return (void*)syscall1(BRK_, increment);
}

/**
 * Suspend execution for the specified number of milliseconds.
 * @param ms Number of milliseconds to sleep.
 * @returns The amount of time remaining (in milliseconds) if the sleep was interrupted, otherwise 0.
 */
unsigned long sleep(unsigned long ms) {
    return (unsigned long)syscall1(SLEEP, ms);
}

/**
 * Create a pipe and store its file descriptors.
 *
 * @param fd Pointer to an array of two ints where the read end (fd[0]) and write end (fd[1]) will be stored.
 * @returns `0` on success, negative value on failure (syscall error code).
 */
int pipe(int *fd) {
    return (int)syscall1(PIPE, (long)fd);
}

/**
 * Mounts a filesystem at the given path using the specified server process and protocol.
 *
 * @param path Mount point path; must be non-NULL.
 * @param server_pid PID of the server providing the filesystem.
 * @param proto Protocol name string; must be non-NULL.
 * @returns A non-negative mount-specific result on success, `-1` on failure (also returned if `path` or `proto` is NULL).
 */
int mount(const char *path, int server_pid, const char *proto) {
    if(path == NULL || proto == NULL)
        return -1;
    return (int)syscall3(MOUNT, (long)path, server_pid, (long)proto);
}

/**
 * Issue a white pebble buffer of the specified size.
 *
 * @param size Number of bytes to allocate or issue for the white pebble buffer.
 * @returns Pointer returned by the kernel representing the issued white buffer, or NULL on failure.
 */
void* pebble_issue_white(unsigned long size) {
    long result = syscall2(PEBBLE_WHITE_ISSUE, size, 0);
    return (void*)result;
}

/**
 * Allocate a black Pebble buffer of the specified size and store its handle.
 * @param size Number of bytes to allocate.
 * @param handle Pointer to a `void*` that will receive the allocated handle; must not be nil.
 * @returns 0 on success, negative value on failure.
 */
int pebble_black_alloc(unsigned long size, void **handle) {
    if(handle == NULL)
        return -1;
    long result = syscall2(PEBBLE_BLACK_ALLOC, size, (long)handle);
    return (int)result;
}

/**
 * Free a previously allocated Pebble black buffer handle.
 *
 * @param handle Pointer to the handle returned by pebble_black_alloc.
 * @returns 0 on success, negative error code on failure.
 */
int pebble_black_free(void *handle) {
    return (int)syscall1(PEBBLE_BLACK_FREE, (long)handle);
}
