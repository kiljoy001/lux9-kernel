# Libc Status for Lux9

## Current Situation

### ❌ No Complete Plan 9 Libc Yet

**What exists:**
- `userspace/lib/syscall.c` - Partial syscall wrappers (fork, exec, open, read, write, etc.)
- `userspace/lib/syscall_amd64.S` - Assembly syscall stub
- `kernel/libc9/` - Kernel's internal libc (NOT for userspace)

**What's missing:**
- Complete Plan 9 libc for userspace programs
- Standard library functions (string.h, stdio.h equivalents)
- Proper syscall wrappers for all Plan 9 syscalls
- Headers (libc.h, u.h, etc.)

### ⚠️ Current Workaround

**Current userspace programs use host libc:**
```c
// userspace/bin/init.c
#include <stdio.h>     // Host system's libc!
#include <stdlib.h>    // Host system's libc!
#include <string.h>    // Host system's libc!

// Then manually extern the syscalls
extern int fork(void);
extern int exec(const char *path, char *const argv[]);
```

**Problems with this:**
1. ❌ POSIX semantics, not Plan 9 semantics
2. ❌ Wrong syscall numbers
3. ❌ Can't use Plan 9-specific features properly
4. ❌ Bloated binaries (pulls in glibc/musl)

## What We Need: Plan 9 Libc

### ✅ Source Available in 9front

Location: `/home/scott/Repo/9front/sys/src/libc/`

**Structure:**
```
9front/sys/src/libc/
├── amd64/          # AMD64-specific (assembly, low-level)
│   ├── main9.s     # Program entry point
│   ├── memcpy.s    # Optimized memory functions
│   ├── setjmp.s    # Context switching
│   └── ...
├── 9sys/           # System call wrappers
│   ├── fork.c      # fork() wrapper
│   ├── open.c      # open() wrapper
│   ├── read.c      # read() wrapper
│   ├── getenv.c    # Environment variable access
│   └── ... (100+ functions)
├── 9syscall/       # Syscall definitions
│   └── sys.h       # Syscall numbers
├── port/           # Portable C functions
│   ├── atoi.c      # String to integer
│   ├── strcpy.c    # String copy
│   ├── strlen.c    # String length
│   └── ... (300+ functions)
├── fmt/            # Printf-style formatting
│   ├── dofmt.c     # Core formatter
│   ├── fmtprint.c  # Print to buffer
│   └── ...
└── ...
```

**Total:** ~500 source files providing complete Plan 9 C library

### Key Components Needed

**1. Syscall Layer**
```c
// 9syscall/sys.h defines syscall numbers
#define OPEN   14
#define READ   15
#define WRITE  20
// ... matches kernel/9front-pc64/trap.c!

// Assembly stub (amd64/syscall.s or similar)
TEXT syscall(SB), 1, $-4
    MOVQ    RARG, BP       // syscall number in BP
    MOVQ    $0, AX         // syscall instruction
    SYSCALL
    RET

// C wrappers (9sys/open.c)
int open(char *name, int mode) {
    return syscall(OPEN, name, mode);
}
```

**2. Standard Library Functions**
```c
// port/strlen.c
int strlen(char *s) {
    char *p = s;
    while(*p) p++;
    return p - s;
}

// port/strcpy.c
char* strcpy(char *s1, char *s2) {
    char *os1 = s1;
    while(*s1++ = *s2++)
        ;
    return os1;
}
```

**3. Printf-style Formatting**
```c
// fmt/print.c
int print(char *fmt, ...) {
    // Format and write to stdout (fd 1)
}

// fmt/fprint.c
int fprint(int fd, char *fmt, ...) {
    // Format and write to fd
}
```

**4. Program Startup**
```asm
// amd64/main9.s
TEXT _main(SB), 1, $(2*8+ERRMAX)
    // Set up argc, argv
    // Call main()
    // Call exits() with return value
```

## Porting Strategy

### Option 1: Import 9front Libc Wholesale (Recommended)

**Advantages:**
- ✅ Complete, battle-tested library
- ✅ Full Plan 9 semantics
- ✅ All syscalls properly wrapped
- ✅ Includes fmt, string, etc.

**Steps:**
1. Copy `/home/scott/Repo/9front/sys/src/libc/` to `userspace/libc/`
2. Adapt syscall numbers to match Lux9 (already match!)
3. Build as static library (`libc.a`)
4. Link userspace programs against it

**Work estimate:** 2-3 days
- Day 1: Set up build system, compile amd64 + port
- Day 2: Fix build issues, create libc.a
- Day 3: Test with simple programs

### Option 2: Minimal Libc (Quick Start)

**Start with just what's needed:**
- Syscall stubs (open, read, write, close, fork, exec, exits, pipe)
- Basic string functions (strlen, strcpy, strcmp)
- Basic memory (memcpy, memset, malloc/free via brk)
- Basic I/O (print, fprint)

**Work estimate:** 1-2 days
**Good for:** Getting rc shell working quickly

### Option 3: Use Musl + Wrappers (Not Recommended)

Keep using host libc but add Plan 9 syscall wrappers.

**Problems:**
- ❌ Mixed semantics (POSIX + Plan 9)
- ❌ Large binaries
- ❌ Not true Plan 9 environment

## Recommended Path

### Phase 1: Minimal Libc (Week 1)
**Goal:** Get rc shell working

**What to port:**
1. **Syscall layer** (`9syscall/` + `amd64/` syscall stub)
   - Define syscall numbers matching Lux9
   - Assembly stub for making syscalls
   - C wrappers for each syscall

2. **Essential string functions** (`port/`)
   - strlen, strcpy, strcmp, strcat
   - memcpy, memset, memmove
   - atoi, utflen

3. **Basic I/O** (`fmt/`)
   - print, fprint (write to fd)
   - snprint (format to buffer)
   - Basic %d, %s, %x formatting

4. **Program startup** (`amd64/main9.s`)
   - Entry point that sets up argc/argv
   - Calls main()
   - Calls exits()

**Files to port:** ~30 files
**Work:** 1-2 days

### Phase 2: Complete Libc (Week 2-3)
**Goal:** Full Plan 9 environment

**Add:**
- All syscall wrappers (~50 functions)
- Complete string library (~100 functions)
- Complete fmt library (~20 functions)
- File path manipulation
- Environment variables
- Error handling

**Files to port:** ~200 files
**Work:** 1-2 weeks

### Phase 3: Optimizations (Later)
- Use optimized assembly implementations
- Add architecture-specific optimizations

## Current Userspace Impact

### Programs That Need Libc

**Existing:**
- `userspace/bin/init` - Needs minimal libc
- `userspace/bin/fscheck` - Needs file I/O
- `userspace/servers/ext4fs` - Uses stdio.h (currently host libc)

**Planned:**
- `/bin/rc` - Needs full Plan 9 libc ⚠️
- `/bin/echo`, `/bin/cat`, etc. - Need basic libc
- Crypto server - Needs stdio, string functions

### Migration Plan

1. **Build minimal Plan 9 libc**
2. **Port init** - Recompile against Plan 9 libc
3. **Build rc** - Now has proper libc!
4. **Port utilities** - Gradually migrate tools
5. **Complete libc** - Add remaining functions as needed

## Syscall Numbers Verification

**Good news:** 9front syscall numbers already match Lux9!

**9front sys.h:**
```c
#define OPEN    14
#define READ    15
#define WRITE   20
#define RFORK   19
#define EXEC    7
```

**Lux9 trap.c:**
```c
[14] = "OPEN",
[15] = "READ",
[20] = "WRITE",
[19] = "RFORK",
[7] = "EXEC",
```

✅ **Perfect match!** No syscall number translation needed.

## Example: Minimal Libc Structure

```
userspace/libc/
├── include/
│   ├── u.h          # Types (uchar, ulong, etc.)
│   ├── libc.h       # Main header
│   └── fmt.h        # Printf-style formatting
├── syscall/
│   ├── syscall.s    # Assembly stub
│   ├── open.c       # open() wrapper
│   ├── read.c       # read() wrapper
│   ├── write.c      # write() wrapper
│   └── ... (~30 syscalls)
├── string/
│   ├── strlen.c
│   ├── strcpy.c
│   ├── strcmp.c
│   └── ... (~20 functions)
├── fmt/
│   ├── print.c
│   ├── fprint.c
│   └── dofmt.c
├── mem/
│   ├── memcpy.c
│   ├── memset.c
│   └── malloc.c     # Uses brk() syscall
└── Makefile         # Builds libc.a
```

## Building Programs with Plan 9 Libc

**Before (current):**
```bash
gcc -o init init.c  # Links against glibc
```

**After (Plan 9 libc):**
```bash
gcc -nostdlib \
    -I userspace/libc/include \
    -o init \
    userspace/libc/amd64/main9.o \
    init.o \
    userspace/libc/libc.a
```

**Program structure:**
```c
// init.c
#include <u.h>      // Plan 9 types
#include <libc.h>   // Plan 9 libc

void main(int argc, char *argv[]) {  // Note: void main, not int!
    print("Hello from Plan 9 libc\n");
    exits(0);  // exits(), not exit()!
}
```

## Summary

**Current Status:**
- ❌ No Plan 9 libc for userspace
- ⚠️ Using host libc as workaround
- ✅ Syscall numbers already match 9front

**What's Available:**
- ✅ Complete 9front libc source code
- ✅ All needed functions (~500 files)
- ✅ AMD64 support
- ✅ Syscalls match kernel

**Recommendation:**
1. **Week 1:** Port minimal libc (~30 files)
   - Get rc shell working
   - Basic utilities (echo, cat)

2. **Week 2-3:** Complete libc port
   - Full Plan 9 environment
   - All utilities work

3. **Later:** Optimize and extend

**Effort:** 1-3 weeks for full Plan 9 libc

The infrastructure is ready - just need to port the code!
