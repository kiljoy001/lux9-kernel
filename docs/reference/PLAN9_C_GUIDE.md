# Yes! This IS Plan 9 C - You Can Learn and Hack!

## What You Have

**100% Authentic 9front libc** - The same C library used in production Plan 9 systems.

### Example: strlen() - Plan 9 Style
```c
#include <u.h>
#include <libc.h>

long
strlen(char *s)
{
    return strchr(s, 0) - s;
}
```

**Notice**:
- Return type on its own line (Plan 9 style!)
- Uses `strchr(s, 0)` to find null terminator
- Elegant pointer arithmetic
- No complexity, just works

## Plan 9 C Differences from Standard C

### 1. Headers
```c
#include <u.h>        // Universal types (uchar, ulong, uvlong, etc.)
#include <libc.h>     // Plan 9 libc
```

**NOT**:
```c
#include <stdio.h>    // NO stdio.h in Plan 9!
#include <stdlib.h>   // NO stdlib.h!
```

### 2. Types (from u.h)
```c
uchar   // unsigned char
ushort  // unsigned short  
uint    // unsigned int
ulong   // unsigned long
uvlong  // unsigned long long
vlong   // long long
```

### 3. Runes (Unicode)
Plan 9 was **Unicode from day 1** (1992!):
```c
Rune r;               // 32-bit Unicode codepoint
char *s = "hello";
chartorune(&r, s);    // Convert UTF-8 to rune
```

### 4. Function Style
```c
int                   // Return type on own line
myfunc(char *s, int n)
{
    // body
}
```

### 5. No stdio - Use print/fprint
```c
print("hello %d\n", 42);           // print to stdout
fprint(2, "error %s\n", msg);      // print to fd 2 (stderr)
sprint(buf, "format %d", n);       // sprintf equivalent
```

### 6. File Operations
```c
int fd = open("/path/file", OREAD);
if(fd < 0)
    sysfatal("open: %r");    // %r prints errno string!

n = read(fd, buf, sizeof buf);
close(fd);
```

### 7. Memory Management
```c
void *p = malloc(100);
if(p == nil)          // nil, not NULL!
    sysfatal("malloc: %r");
    
free(p);
```

## Available in Your Kernel

### Headers (kernel/include/)
- `u.h` - Universal types
- `libc.h` - Full Plan 9 libc API
- `dat.h` - Kernel data structures
- `fns.h` - Function prototypes

### LibC Functions (kernel/libc9/)
Currently minimal, but you can add more from `/home/scott/Repo/9front/sys/src/libc/port/`:
- `strlen.c`
- `strcmp.c`
- `strcpy.c`
- `memset.c`
- `memmove.c`

**Want more?** Just copy from 9front libc:
```bash
cp /home/scott/Repo/9front/sys/src/libc/port/strcat.c kernel/libc9/
cp /home/scott/Repo/9front/sys/src/libc/port/atoi.c kernel/libc9/
# etc...
```

## Learning Resources

### 1. Read 9front Code
```bash
cd /home/scott/Repo/9front/sys/src/

# Simple utilities to learn from:
cat cmd/cat.c         # 57 lines
cat cmd/echo.c        # 22 lines  
cat cmd/ls.c          # Clean, readable

# Kernel code:
cat 9/port/proc.c     # Scheduler
cat 9/port/chan.c     # File operations
```

### 2. The Papers
Plan 9 has **excellent documentation**:
- `/home/scott/Repo/9front/sys/doc/` - Official papers
- "The Use of Name Spaces in Plan 9"
- "Process Sleep and Wakeup on a Shared-memory Multiprocessor"

### 3. man Pages
9front has man pages for everything:
```bash
# In 9front (if you boot it):
man 2 open    # System calls
man 3 print   # Library functions
```

## Example: Writing a Simple Kernel Module

```c
#include <u.h>
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"

void
myinit(void)
{
    print("Hello from my kernel module!\n");
}

int
myfunction(char *s)
{
    int n;
    
    n = strlen(s);
    if(n == 0)
        return -1;
        
    print("Got string: %s (len %d)\n", s, n);
    return 0;
}
```

## Why Plan 9 C is Great for Hacking

1. **Simple**: No hidden complexity, what you see is what you get
2. **Clean**: Function style is readable
3. **Unicode**: Built-in from the start (Runes)
4. **Consistent**: Same style throughout entire system
5. **Small**: Easy to understand the whole thing
6. **9P everywhere**: Everything talks the same protocol

## Your Advantage

You have:
- ✓ Full 9front kernel source (`/home/scott/Repo/9front/`)
- ✓ All libc source to learn from
- ✓ Working kernel code to modify
- ✓ Can copy any libc function you need

**Start hacking!** The entire system uses this style, so once you learn it, you can understand and modify ANYTHING.

## Quick Reference

```c
// Types
uchar, ushort, uint, ulong, uvlong, vlong

// Constants
nil          // NULL pointer
nelem(arr)   // Array length

// Print
print()      // stdout
fprint(fd)   // to file descriptor
sprint(buf)  // to buffer

// Memory
malloc/free/realloc

// String (returns long, not int!)
strlen(s)
strcmp(a, b)
strcpy(dst, src)

// Error handling
if(fd < 0)
    sysfatal("open: %r");  // %r = strerror(errno)
```

Go forth and hack Plan 9 C!
