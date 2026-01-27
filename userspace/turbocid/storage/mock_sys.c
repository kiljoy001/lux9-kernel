#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <sys/stat.h>

/* Do NOT include string.h to avoid conflicts with our manual definitions */

typedef unsigned char u8int;
typedef unsigned long long u64int;

/* Manual implementation of string/memory functions expected by Lux9 code */

void *memset(void *dst, int c, unsigned long n) {
    unsigned char *d = (unsigned char *)dst;
    while (n--) *d++ = (unsigned char)c;
    return dst;
}

void *memmove(void *dst, const void *src, unsigned long n) {
    unsigned char *d = (unsigned char *)dst;
    const unsigned char *s = (const unsigned char *)src;
    if (d < s) {
        while (n--) *d++ = *s++;
    } else {
        d += n;
        s += n;
        while (n--) *--d = *--s;
    }
    return dst;
}

int memcmp(const void *s1, const void *s2, unsigned long n) {
    const unsigned char *p1 = (const unsigned char *)s1;
    const unsigned char *p2 = (const unsigned char *)s2;
    for (unsigned long i = 0; i < n; i++) {
        if (p1[i] != p2[i]) return p1[i] - p2[i];
    }
    return 0;
}

unsigned long strlen(const char *s) {
    unsigned long len = 0;
    while (*s++) len++;
    return len;
}

/* Mock System Calls */

int sys_open(char *path, int mode) {
    int flags = 0;
    if (mode == 0) flags = O_RDONLY;
    else if (mode == 1) flags = O_WRONLY;
    else if (mode == 2) flags = O_RDWR;
    if (mode & 16) flags |= O_TRUNC;
    
    return open(path, flags, 0644);
}

int sys_create(char *path, int mode, unsigned int perm) {
    int flags = O_CREAT | O_TRUNC;
    if ((mode & 3) == 0) flags |= O_RDONLY;
    else if ((mode & 3) == 1) flags |= O_WRONLY;
    else if ((mode & 3) == 2) flags |= O_RDWR;
    
    return open(path, flags, perm);
}

int sys_close(int fd) {
    return close(fd);
}

long sys_read(int fd, void *buf, long n) {
    return read(fd, buf, n);
}

long sys_write(int fd, void *buf, long n) {
    return write(fd, buf, n);
}

long sys_pread(int fd, void *buf, long n, long offset) {
    return pread(fd, buf, n, offset);
}

long sys_pwrite(int fd, void *buf, long n, long offset) {
    return pwrite(fd, buf, n, offset);
}

void sys_exit(char *msg) {
    if (msg) printf("sys_exit: %s\n", msg);
    exit(msg ? 1 : 0);
}

unsigned long long sys_nsec(void) {
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    return (unsigned long long)ts.tv_sec * 1000000000ULL + ts.tv_nsec;
}

/* Crypto Mocks */

void crypto_blake2b(u8int *hash, unsigned long hash_size, const u8int *message, unsigned long message_size) {
    /* Deterministic mock hash */
    memset(hash, 0, hash_size);
    for (unsigned long i = 0; i < hash_size; i++) {
        if (message_size > 0) {
            hash[i] = message[i % message_size] ^ (i & 0xFF);
        } else {
            hash[i] = i & 0xFF;
        }
    }
}

void crypto_blake2b_init(void *ctx, unsigned long long outlen) {}
void crypto_blake2b_update(void *ctx, const u8int *in, unsigned long long inlen) {}
void crypto_blake2b_final(void *ctx, u8int *out) {}