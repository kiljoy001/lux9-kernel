#include "../include/lux.h"

/* Wrapper functions for system calls */

int open(const char *path, int mode) {
    return (int)_syscall(SYS_OPEN, path, mode);
}

int close(int fd) {
    return (int)_syscall(SYS_CLOSE, fd);
}

long read(int fd, void *buf, long n) {
    return _syscall(SYS_READ, fd, buf, n);
}

long write(int fd, const void *buf, long n) {
    return _syscall(SYS_WRITE, fd, buf, n);
}

int bind(const char *name, const char *old, int flag) {
    return (int)_syscall(SYS_BIND, name, old, flag);
}

int mount(int fd, int afd, const char *old, int flag, const char *aname) {
    return (int)_syscall(SYS_MOUNT, fd, afd, old, flag, aname);
}

long brk(void *addr) {
    return _syscall(SYS_BRK, addr);
}

void exits(const char *msg) {
    _syscall(SYS_EXITS, msg);
    /* Should not return */
    while(1);
}

int rfork(int flags) {
    return (int)_syscall(SYS_RFORK, flags);
}

int await(char *status, int nstatus) {
    return (int)_syscall(SYS_AWAIT, status, nstatus);
}

/* Utility functions */

void *memset(void *s, int c, ulong n) {
    unsigned char *p = s;
    while(n--)
        *p++ = (unsigned char)c;
    return s;
}

void *memcpy(void *dest, const void *src, ulong n) {
    unsigned char *d = dest;
    const unsigned char *s = src;
    while(n--)
        *d++ = *s++;
    return dest;
}

ulong strlen(const char *s) {
    const char *p = s;
    while(*p)
        p++;
    return p - s;
}

char *strcpy(char *dest, const char *src) {
    char *d = dest;
    while((*d++ = *src++))
        ;
    return dest;
}

int strcmp(const char *s1, const char *s2) {
    while(*s1 && *s1 == *s2) {
        s1++;
        s2++;
    }
    return *(unsigned char*)s1 - *(unsigned char*)s2;
}

/* Minimal print support */
static void putc(int c) {
    char ch = c;
    write(2, &ch, 1);  /* Write to stderr */
}

static void puts(const char *s) {
    while(*s)
        putc(*s++);
}

static void putn(long n) {
    char buf[32];
    int i = 0;
    int neg = 0;
    
    if(n < 0) {
        neg = 1;
        n = -n;
    }
    
    if(n == 0) {
        putc('0');
        return;
    }
    
    while(n > 0) {
        buf[i++] = '0' + (n % 10);
        n /= 10;
    }
    
    if(neg)
        putc('-');
        
    while(i > 0)
        putc(buf[--i]);
}

int print(const char *fmt, ...) {
    const char *p = fmt;
    long *args = (long*)&fmt + 1;
    int argidx = 0;
    int nprinted = 0;
    
    while(*p) {
        if(*p == '%' && p[1]) {
            p++;
            switch(*p) {
                case 's':
                    puts((const char*)args[argidx++]);
                    break;
                case 'd':
                case 'l':
                    putn((long)args[argidx++]);
                    break;
                case 'x': {
                    unsigned long val = (unsigned long)args[argidx++];
                    char hex[17];
                    int i = 0;
                    if(val == 0) {
                        putc('0');
                    } else {
                        while(val > 0) {
                            int digit = val % 16;
                            hex[i++] = digit < 10 ? '0' + digit : 'a' + digit - 10;
                            val /= 16;
                        }
                        while(i > 0)
                            putc(hex[--i]);
                    }
                    break;
                }
                case '%':
                    putc('%');
                    break;
                default:
                    putc('%');
                    putc(*p);
                    break;
            }
            nprinted++;
        } else {
            putc(*p);
        }
        p++;
    }
    
    return nprinted;
}
