#pragma once

#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>

long __syscall(long n, long a1, long a2, long a3, long a4, long a5, long a6);

int fork(void);
int exec(const char *path, char *const argv[]);
void exit(int status);
int wait(int *status);

int open(const char *path, int flags);
int close(int fd);
ssize_t read(int fd, void *buf, size_t count);
ssize_t write(int fd, const void *buf, size_t count);
int pipe(int fd[2]);

int mount(const char *path, int server_pid, const char *proto);
void *sbrk(intptr_t increment);
unsigned long sleep(unsigned long ms);

void* pebble_issue_white(unsigned long size);
int pebble_black_alloc(unsigned long size, void **handle);
int pebble_black_free(void *handle);
