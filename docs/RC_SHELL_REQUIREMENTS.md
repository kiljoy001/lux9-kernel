# RC Shell Requirements for Lux9

## Overview
The rc shell is Plan 9's standard shell. Getting it working in Lux9 requires both kernel support and userspace components.

## Current Status

### ✅ Syscalls Already Implemented
Based on `kernel/9front-port/sysfile.c` and `sysproc.c`:

**File Operations:**
- `open`, `close`, `read`, `write` ✅
- `create`, `remove` ✅
- `stat`, `fstat`, `wstat`, `fwstat` ✅
- `dup`, `pipe` ✅
- `seek`, `pread`, `pwrite` ✅
- `fd2path` ✅

**Namespace:**
- `bind`, `mount`, `unmount` ✅
- `chdir` ✅

**Process Management:**
- `rfork` (Plan 9's fork) ✅
- `exec` ✅
- `exits` (Plan 9's exit) ✅
- `await` (Plan 9's wait) ✅

**Memory:**
- `brk` (memory allocation) ✅
- `segattach`, `segdetach`, `segbrk`, `segfree` ✅

**Synchronization:**
- `sleep`, `alarm` ✅
- `rendezvous` ✅
- `semacquire`, `semrelease` ✅

**Error Handling:**
- `errstr` ✅

**Pebble System:**
- `pebbleblackalloc`, `pebbleblackfree` ✅
- `pebblewhiteissue`, `pebblewhiteverify` ✅
- `pebblebluediscard`, `pebbleredcopy` ✅

### ✅ RC Shell Source Available
Located at: `/home/scott/Repo/9front/sys/src/cmd/rc/`

Files:
- `code.c` - Bytecode compiler
- `exec.c` - Command execution
- `lex.c` - Lexical analyzer
- `plan9.c` - Plan 9 specific code
- `havefork.c` - Fork-based execution
- `glob.c` - Glob pattern matching
- `io.c` - I/O handling
- And more...

## What's Needed to Get RC Working

### 1. **Console Device** (Critical)
RC needs stdin/stdout/stderr (file descriptors 0, 1, 2).

**Status:** Check if `/dev/cons` exists
```bash
# Need to verify:
ls /dev/cons     # Console device
ls /dev/consctl  # Console control
```

**What's needed:**
- Console device driver (`devcons.c` - already exists in kernel!)
- Proper initialization of fds 0, 1, 2 for init process
- Terminal I/O support

### 2. **Basic Filesystem Namespace**
RC expects certain paths to exist:

**Required:**
```
/dev/cons        # Console
/dev/null        # Null device
/env/            # Environment variables (devenv.c exists!)
/bin/rc          # The shell itself
/bin/...         # Basic utilities
```

**Status:**
- `devcons.c` exists in `kernel/9front-port/` ✅
- `devenv.c` exists in `kernel/9front-port/` ✅
- Need to check if they're initialized

### 3. **Build RC Binary**
Need to cross-compile rc for Lux9.

**Steps:**
1. Copy rc source from 9front to `userspace/bin/rc/`
2. Adapt Makefile for Lux9 build system
3. Link against Lux9 libc
4. Build static binary

**Dependencies:**
- Lux9 libc (exists in `kernel/libc9/`)
- Need userspace libc headers
- Syscall stubs

### 4. **Init Process Setup**
The init process needs to:

**Current init.c:** Check `/home/scott/Repo/lux9-kernel/userspace/bin/init.c`

**What init should do:**
```c
int main() {
    // 1. Set up namespace
    bind("#c", "/dev", MAFTER);  // Console device
    bind("#e", "/env", MAFTER);  // Environment

    // 2. Open console for stdio
    int consfd = open("/dev/cons", O_RDWR);
    dup(consfd, 0);  // stdin
    dup(consfd, 1);  // stdout
    dup(consfd, 2);  // stderr
    close(consfd);

    // 3. Set up environment
    putenv("path", "/bin");

    // 4. Exec rc shell
    char *argv[] = {"/bin/rc", "-i", nil};
    exec("/bin/rc", argv);

    // 5. Fallback if exec fails
    exits("init: can't exec rc");
}
```

### 5. **Basic Utilities**
RC needs basic commands to be useful:

**Priority 1 (Essential):**
- `echo` - Print text
- `cat` - Concatenate files
- `ls` - List directory
- `bind` - Namespace manipulation
- `mount` - Mount filesystems

**Priority 2 (Useful):**
- `cp`, `mv`, `rm` - File operations
- `mkdir`, `cd` - Directory operations
- `grep`, `sed` - Text processing

**Where to get them:**
- Build from 9front sources: `/home/scott/Repo/9front/sys/src/cmd/`
- Start with simplest ones (echo, cat)

### 6. **Device Drivers Registration**
Ensure required device drivers are registered in kernel:

**Check in kernel init:**
```c
// kernel/9front-pc64/main.c or similar
void main() {
    ...
    // Register device drivers
    devinit();  // Should register:
                // - devcons (console)
                // - devenv (environment)
                // - devroot (root filesystem)
                // - devproc (#p - process info)
    ...
}
```

**Verify:**
```bash
# Check kernel device table
grep "devcons" kernel/9front-pc64/main.c
grep "devenv" kernel/9front-pc64/main.c
```

## Step-by-Step Implementation Plan

### Phase 1: Verify Kernel Support (Day 1)
1. ✅ Verify syscalls are implemented
2. Check if devcons is registered and working
3. Check if devenv is registered
4. Test basic syscalls from userspace

### Phase 2: Build RC Shell (Day 2-3)
1. Copy rc source to `userspace/bin/rc/`
2. Create Makefile for Lux9
3. Build against Lux9 libc
4. Create minimal test binary

### Phase 3: Fix Init Process (Day 3)
1. Update `userspace/bin/init.c`
2. Set up console fds (0, 1, 2)
3. Bind device drivers to namespace
4. Exec rc shell

### Phase 4: Build Basic Utilities (Day 4-5)
1. Build `echo`, `cat`, `ls`
2. Test basic commands
3. Add more utilities as needed

### Phase 5: Testing (Day 6)
1. Boot kernel with new init
2. Verify rc prompt appears
3. Test basic commands
4. Test pipes, redirection
5. Test rc scripts

## Minimal Working Example

### What You Need:
1. **Kernel** with devcons, devenv registered ✅ (likely already done)
2. **Init** that sets up stdio and execs rc
3. **RC binary** compiled for Lux9
4. **One command** (e.g., echo) to test with

### Quick Test:
```bash
# In rc shell:
echo hello world
# Should print: hello world

# Test pipe:
echo hello | cat
# Should print: hello

# Test environment:
echo $path
# Should print: /bin
```

## Potential Issues

### Issue 1: Console Not Working
**Symptom:** RC starts but no input/output
**Fix:** Verify devcons is bound to /dev and fds 0,1,2 are open

### Issue 2: Exec Fails
**Symptom:** Init can't exec rc
**Fix:**
- Verify /bin/rc exists
- Check file permissions
- Verify exec syscall works

### Issue 3: Missing Symbols
**Symptom:** RC fails to link
**Fix:**
- Implement missing libc functions
- Add syscall stubs

### Issue 4: Namespace Empty
**Symptom:** RC can't find /dev, /bin
**Fix:**
- Verify bind/mount in init
- Check device driver registration

## Resources

**9front RC Source:**
`/home/scott/Repo/9front/sys/src/cmd/rc/`

**Lux9 Syscalls:**
- `kernel/9front-port/sysfile.c`
- `kernel/9front-port/sysproc.c`

**Device Drivers:**
- `kernel/9front-port/devcons.c` (console)
- `kernel/9front-port/devenv.c` (environment)
- `kernel/9front-port/devroot.c` (root namespace)

**Current Init:**
`userspace/bin/init.c`

## Expected Outcome

Once complete, you should have:
```
Lux9 kernel booting...
[init] Setting up namespace
[init] Opening console
[init] Exec /bin/rc
; █  ← RC shell prompt!

; echo hello world
hello world
; ls /crypto
hash/
aead/
sign/
rng/
; cat /crypto/hash/sha256
<writes data, reads hash>
```

## Summary

**Good News:** ✅
- All required syscalls are implemented
- Device drivers exist (devcons, devenv)
- RC source is available

**What's Needed:** 🔧
1. Build rc binary (~1 day)
2. Fix init to set up stdio (~2 hours)
3. Build 1-2 basic utilities (~4 hours)
4. Test and debug (~1 day)

**Total Estimate:** 2-3 days to get a working rc shell!

The kernel infrastructure is ready - we just need to build the userspace components.
