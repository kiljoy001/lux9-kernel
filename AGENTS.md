# AGENTS.md - Coding Guidelines for Lux9 Kernel

## Build Commands
- `make` - Build the kernel (lux9.elf)
- `make iso` - Create bootable ISO image
- `make run` - Test in QEMU
- `make clean` - Clean build artifacts
- `make test-build` - Test compilation of first file
- `make count` - Show line counts
- `make list` - Show source files to compile

## Testing Commands
- `make test-build` - Quick compilation test
- Individual tests in userspace/test_*.sh
- No unit test framework currently; integration tests use shell scripts
- To run a single test: `./userspace/test_ext4fs_simple.sh` (no sudo) or `./userspace/test_ext4fs.sh` (requires sudo)
- For debugging with GDB: `qemu-system-x86_64 -s -S -cdrom lux9.iso` then `gdb lux9.elf`
- To verify Coq proofs: `cd proofs && make`

## Code Style Guidelines

### Headers and Imports
- Plan 9 C style: `#include <u.h>` then `#include <libc.h>`
- Kernel headers: `#include "mem.h"`, `#include "dat.h"`, `#include "fns.h"`
- No stdio.h/stddef.h - use Plan 9 equivalents
- Headers in alphabetical order within each group
- All kernel files should include the standard header sequence

### Types
- Use Plan 9 types: uchar, ushort, uint, ulong, uvlong, vlong
- Use `nil` instead of NULL
- Function return types on separate line
- Use `USED(x)` macro to mark intentionally unused parameters
- String functions return `long` not `int`

### Function Style
```c
int                    // Return type on own line
functionname(char *s, int n)
{
    // body
    return 0;
}
```

### Formatting
- 8-space tabs (no spaces)
- No space after function names: `func(arg)` not `func (arg)`
- Opening brace on same line as function/struct
- Pointer declarations: `char *s` not `char* s`
- Line length: Prefer under 80 characters but up to 100 is acceptable
- Space after commas and semicolons: `func(a, b);` not `func(a,b);`
- Space around operators: `a + b` not `a+b`

### Naming Conventions
- snake_case for functions and variables
- UPPERCASE for constants/macros
- Struct tags: `typedef struct Name Name;`
- Global variables prefixed with `extern` in headers
- Static functions for internal use
- Function parameters should use descriptive names

### Error Handling
- Check return values explicitly
- Use `sysfatal("error: %r")` for fatal errors (%r prints errno)
- Return -1 for errors, 0 for success where applicable
- Use `waserror()` and `poperror()` for exception-like handling
- Log errors with `print()` before returning error codes
- Always handle file operation errors appropriately

### Memory Management
- Use `malloc/free` from Plan 9 libc
- Always check malloc return value against `nil`
- Use `realloc` for dynamic arrays
- Clean up allocated memory in error paths
- Use `smalloc()` for small fixed-size allocations
- Use `nelem(array)` macro to get array length

### Strings and I/O
- Use `print()`, `fprint()`, `sprint()` instead of printf variants
- String functions return `long` not `int`
- Use `strchr(s, 0)` for strlen implementation
- Check buffer bounds when using sprint functions
- Use `seprint` for safe string formatting with buffer end pointers
- Use `utfecpy` for safe UTF-8 string copying

### Kernel-Specific Conventions
- Page management: Use `newpage()`, `segpage()`, `pmap()` for memory mapping
- Locking: Use `qlock(&s->qlock)` not `qlock(s)`
- Debug output: Use single character markers with `outb` for boot tracing
- Memory addresses: USTKTOP, UTZERO for user space; KZERO, KTZERO for kernel space
- Interrupt handling: Use `splhi()`, `spllo()`, `splx()` for interrupt control
- Process management: Use `up` for current process, `p->pid` for process ID
- Device operations: Follow Plan 9 device driver patterns
- 9P protocol: Use standard 9P structures and functions for file operations