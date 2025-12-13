# GCC-Compatible Libc for Lux9

## Design Goal

**Users write standard C code:**
```c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char **argv) {
    printf("Hello, world\n");
    return 0;
}
```

**It compiles with GCC and uses Plan 9 syscalls underneath.**

## Architecture

```
┌─────────────────────────────────────────┐
│ User Code (Standard C)                   │
│   #include <stdio.h>                     │
│   printf("hello\n");                     │
└────────────────┬────────────────────────┘
                 │
┌────────────────▼────────────────────────┐
│ Standard C API Layer                     │
│   stdio.h, stdlib.h, string.h           │
│   printf() → calls write()               │
│   malloc() → calls brk()                 │
└────────────────┬────────────────────────┘
                 │
┌────────────────▼────────────────────────┐
│ Plan 9 Syscall Layer                     │
│   open(), read(), write(), brk()         │
│   Uses Plan 9 syscall numbers            │
└────────────────┬────────────────────────┘
                 │
┌────────────────▼────────────────────────┐
│ Lux9 Kernel                              │
│   Processes syscalls                     │
└──────────────────────────────────────────┘
```

## Implementation Strategy

### Approach: Hybrid Libc

**Base:** 9front libc (syscalls, Plan 9 semantics)
**Top layer:** Standard C headers (stdio.h, stdlib.h, etc.)
**Result:** Users write standard C, get Plan 9 underneath

### Layer 1: Plan 9 Syscalls (From 9front)

```c
// syscall/open.c
int p9_open(char *path, int mode) {
    return syscall(OPEN, path, mode);
}

// syscall/read.c
long p9_read(int fd, void *buf, long n) {
    return syscall(READ, fd, buf, n);
}

// syscall/write.c
long p9_write(int fd, void *buf, long n) {
    return syscall(WRITE, fd, buf, n);
}
```

### Layer 2: Standard C API (New)

```c
// stdio/fopen.c
#include <stdio.h>

FILE *fopen(const char *path, const char *mode) {
    int flags = parse_mode(mode);  // "r" → O_RDONLY, "w" → O_WRONLY
    int fd = p9_open((char*)path, flags);
    if (fd < 0)
        return NULL;
    return fdopen(fd, mode);
}

// stdio/fprintf.c
int fprintf(FILE *stream, const char *fmt, ...) {
    va_list ap;
    char buf[4096];
    int n;

    va_start(ap, fmt);
    n = vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);

    return p9_write(stream->fd, buf, n);
}

// stdio/printf.c
int printf(const char *fmt, ...) {
    va_list ap;
    char buf[4096];
    int n;

    va_start(ap, fmt);
    n = vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);

    return p9_write(1, buf, n);  // fd 1 = stdout
}

// stdlib/malloc.c
void *malloc(size_t size) {
    // Use brk() syscall underneath
    return p9_brk(size);
}

// stdlib/exit.c
void exit(int status) {
    p9_exits(status == 0 ? NULL : "error");
}

// string/strlen.c (can reuse 9front's implementation)
size_t strlen(const char *s) {
    const char *p = s;
    while (*p) p++;
    return p - s;
}
```

### Layer 3: Standard C Headers

```c
// include/stdio.h
#ifndef _STDIO_H
#define _STDIO_H

#include <stddef.h>
#include <stdarg.h>

typedef struct FILE {
    int fd;
    int flags;
    char *buf;
    size_t bufsize;
} FILE;

extern FILE *stdin;
extern FILE *stdout;
extern FILE *stderr;

FILE *fopen(const char *path, const char *mode);
int fclose(FILE *stream);
int fprintf(FILE *stream, const char *fmt, ...);
int printf(const char *fmt, ...);
int sprintf(char *str, const char *fmt, ...);
int snprintf(char *str, size_t size, const char *fmt, ...);
size_t fread(void *ptr, size_t size, size_t nmemb, FILE *stream);
size_t fwrite(const void *ptr, size_t size, size_t nmemb, FILE *stream);
int fgetc(FILE *stream);
int fputc(int c, FILE *stream);
char *fgets(char *s, int size, FILE *stream);
int fputs(const char *s, FILE *stream);

#define EOF (-1)

#endif
```

```c
// include/stdlib.h
#ifndef _STDLIB_H
#define _STDLIB_H

#include <stddef.h>

void *malloc(size_t size);
void *calloc(size_t nmemb, size_t size);
void *realloc(void *ptr, size_t size);
void free(void *ptr);

void exit(int status);
void abort(void);

int atoi(const char *nptr);
long atol(const char *nptr);
long long atoll(const char *nptr);

char *getenv(const char *name);
int putenv(char *string);

#endif
```

```c
// include/string.h
#ifndef _STRING_H
#define _STRING_H

#include <stddef.h>

void *memcpy(void *dest, const void *src, size_t n);
void *memmove(void *dest, const void *src, size_t n);
void *memset(void *s, int c, size_t n);
int memcmp(const void *s1, const void *s2, size_t n);
void *memchr(const void *s, int c, size_t n);

char *strcpy(char *dest, const char *src);
char *strncpy(char *dest, const char *src, size_t n);
char *strcat(char *dest, const char *src);
char *strncat(char *dest, const char *src, size_t n);
int strcmp(const char *s1, const char *s2);
int strncmp(const char *s1, const char *s2, size_t n);
size_t strlen(const char *s);
char *strchr(const char *s, int c);
char *strrchr(const char *s, int c);
char *strstr(const char *haystack, const char *needle);

#endif
```

```c
// include/unistd.h
#ifndef _UNISTD_H
#define _UNISTD_H

#include <stddef.h>
#include <sys/types.h>

// File I/O
int open(const char *path, int flags, ...);
int close(int fd);
ssize_t read(int fd, void *buf, size_t count);
ssize_t write(int fd, const void *buf, size_t count);
off_t lseek(int fd, off_t offset, int whence);

// Process
pid_t fork(void);
int execv(const char *path, char *const argv[]);
int execve(const char *path, char *const argv[], char *const envp[]);
pid_t getpid(void);
unsigned int sleep(unsigned int seconds);

// Pipes
int pipe(int pipefd[2]);

// Standard file descriptors
#define STDIN_FILENO  0
#define STDOUT_FILENO 1
#define STDERR_FILENO 2

#endif
```

## Directory Structure

```
userspace/libc/
├── include/              # Standard C headers
│   ├── stdio.h
│   ├── stdlib.h
│   ├── string.h
│   ├── unistd.h
│   ├── stddef.h
│   ├── stdarg.h
│   ├── stdint.h
│   └── sys/
│       └── types.h
│
├── syscall/              # Plan 9 syscalls (from 9front)
│   ├── syscall.s         # Assembly stub
│   ├── open.c
│   ├── read.c
│   ├── write.c
│   ├── close.c
│   ├── fork.c
│   ├── exec.c
│   └── ... (~30 syscalls)
│
├── stdio/                # Standard I/O layer
│   ├── printf.c
│   ├── fprintf.c
│   ├── sprintf.c
│   ├── fopen.c
│   ├── fclose.c
│   ├── fread.c
│   ├── fwrite.c
│   └── ...
│
├── stdlib/               # Standard library
│   ├── malloc.c          # Uses brk()
│   ├── exit.c            # Wraps p9_exits()
│   ├── atoi.c
│   ├── getenv.c
│   └── ...
│
├── string/               # String functions (from 9front)
│   ├── strlen.c
│   ├── strcpy.c
│   ├── strcmp.c
│   ├── memcpy.c
│   ├── memset.c
│   └── ...
│
├── crt/                  # C runtime
│   ├── crt0.s            # Program entry point
│   └── crti.s
│
└── Makefile              # Builds libc.a
```

## Building Programs

### Compilation
```bash
gcc -nostdinc -nostdlib \
    -I userspace/libc/include \
    -c myprogram.c -o myprogram.o
```

### Linking
```bash
gcc -nostdlib \
    -o myprogram \
    userspace/libc/crt/crt0.o \
    myprogram.o \
    userspace/libc/libc.a
```

### Or with wrapper script
```bash
# lux9-gcc wrapper
gcc -nostdinc -I /path/to/libc/include \
    -nostdlib -L /path/to/libc \
    -static \
    "$@" -lc
```

## Example Programs

### Hello World
```c
#include <stdio.h>

int main() {
    printf("Hello, world!\n");
    return 0;
}
```

### File I/O
```c
#include <stdio.h>
#include <stdlib.h>

int main() {
    FILE *f = fopen("/crypto/hash/sha256", "r+");
    if (!f) {
        fprintf(stderr, "Can't open crypto device\n");
        return 1;
    }

    // Write data to hash
    fwrite("hello world", 1, 11, f);

    // Read hash
    char hash[65];
    fread(hash, 1, 64, f);
    hash[64] = 0;

    printf("SHA256: %s\n", hash);
    fclose(f);
    return 0;
}
```

### Process Creation
```c
#include <stdio.h>
#include <unistd.h>

int main() {
    pid_t pid = fork();

    if (pid == 0) {
        // Child
        printf("Child process\n");
        execv("/bin/echo", (char*[]){"/bin/echo", "hello", NULL});
    } else {
        // Parent
        printf("Parent waiting...\n");
        wait(NULL);
    }

    return 0;
}
```

## Implementation Phases

### Phase 1: Core Functions (Week 1)
**Priority: Get basic programs compiling**

**Headers:**
- stdio.h (basic)
- stdlib.h (basic)
- string.h
- unistd.h (basic)

**Functions (~50):**
- String: strlen, strcpy, strcmp, memcpy, memset
- Stdio: printf, fprintf, sprintf
- Stdlib: malloc, free, exit, atoi
- Unistd: open, close, read, write, fork, exec

**Goal:** Compile and run hello world, basic utilities

### Phase 2: Extended Functions (Week 2)
**Priority: Full stdio, better malloc**

**Add:**
- Complete FILE* operations (fopen, fclose, fread, fwrite)
- Better malloc/free (heap allocator)
- More string functions
- Environment variables

**Goal:** Compile complex programs, rc shell

### Phase 3: POSIX Compatibility (Week 3)
**Priority: Standard compliance**

**Add:**
- errno handling
- Signal handling (map to Plan 9 notes)
- Directory operations (readdir, etc.)
- Time functions

**Goal:** Compile existing POSIX software with minimal changes

## Handling Semantic Differences

### Fork vs Rfork
```c
// unistd/fork.c
pid_t fork(void) {
    // Plan 9 rfork with flags that mimic Unix fork
    return p9_rfork(RFPROC | RFFDG | RFENVG | RFNAMEG);
}
```

### Exit vs Exits
```c
// stdlib/exit.c
void exit(int status) {
    if (status == 0)
        p9_exits(NULL);  // Success
    else
        p9_exits("error");  // Failure
}
```

### Open Flags
```c
// unistd/open.c
int open(const char *path, int flags, ...) {
    // Translate POSIX flags to Plan 9
    int p9flags = 0;

    if (flags & O_RDONLY) p9flags = OREAD;
    if (flags & O_WRONLY) p9flags = OWRITE;
    if (flags & O_RDWR)   p9flags = ORDWR;
    if (flags & O_TRUNC)  p9flags |= OTRUNC;

    return p9_open((char*)path, p9flags);
}
```

## Advantages of This Approach

✅ **Users write standard C** - Familiar API
✅ **GCC compatible** - Normal compilation
✅ **Plan 9 underneath** - Native syscalls, no translation overhead
✅ **Portable** - Can compile existing C programs
✅ **Dual API** - Can use Plan 9 functions directly if wanted
✅ **Minimal size** - Only include what's used

## Comparison to Alternatives

**vs Pure Plan 9 libc:**
- ✅ More familiar to C programmers
- ✅ Can compile existing software
- ⚠️ Some semantic translation needed

**vs glibc/musl:**
- ✅ Much smaller (~100KB vs 10MB)
- ✅ Native Plan 9 syscalls
- ✅ No POSIX baggage
- ⚠️ Not 100% POSIX compatible

**vs newlib:**
- ✅ Designed for our kernel
- ✅ Optimized for Plan 9
- ✅ Simpler to maintain

## Build System

```makefile
# userspace/libc/Makefile

CC = gcc
AR = ar
CFLAGS = -nostdinc -I include -O2 -fno-builtin -ffreestanding

# Source files
SYSCALL_SRC = $(wildcard syscall/*.c)
STDIO_SRC = $(wildcard stdio/*.c)
STDLIB_SRC = $(wildcard stdlib/*.c)
STRING_SRC = $(wildcard string/*.c)

# Object files
SYSCALL_OBJ = $(SYSCALL_SRC:.c=.o)
STDIO_OBJ = $(STDIO_SRC:.c=.o)
STDLIB_OBJ = $(STDLIB_SRC:.c=.o)
STRING_OBJ = $(STRING_SRC:.c=.o)

ALL_OBJ = $(SYSCALL_OBJ) $(STDIO_OBJ) $(STDLIB_OBJ) $(STRING_OBJ)

# Target
libc.a: $(ALL_OBJ)
	$(AR) rcs $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(ALL_OBJ) libc.a
```

## Summary

**Design:** Standard C API → Plan 9 syscalls
**Users write:** Normal C code with stdio.h, stdlib.h
**Compiles with:** GCC (standard toolchain)
**Runs on:** Lux9 kernel with Plan 9 semantics
**Size:** ~100-200KB static library
**Time:** 1-3 weeks to implement

**Result:** Users can write portable C code that runs natively on Lux9!

```c
// Just works!
#include <stdio.h>
#include <stdlib.h>

int main() {
    printf("Hello from Lux9!\n");

    FILE *f = fopen("/crypto/hash/sha256", "w");
    fprintf(f, "hash this");
    // ... crypto magic happens via Plan 9 9P syscalls!

    return 0;
}
```
