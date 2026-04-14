# Userspace Printing Investigation Report

## Executive Summary
Userspace programs (especially WASM applications) cannot print to the console because:
1. The `/dev/cons` device write handler in `devcons_minimal.c` is using `print()` which may not be functional
2. The framebuffer console initialization has an early return that prevents proper setup
3. The UART console output is not being properly hooked into the WASM fd_write path

## Detailed Analysis

### 1. WASM Userspace Printing Path

**Userspace Code** (`userspace/hello.c:24`):
- Calls `fd_write(1, &iov, 1, &nwritten)` to write to stdout (fd 1)
- This is a WASI import that gets handled by kernel shim

**Kernel Shim** (`kernel/wasm/wasi_lux9_shim.c:963-1045`):
```c
m3ApiRawFunction(wasi_snapshot_preview1_fd_write) {
  ...
  if (fd == 1 || fd == 2) {
    DPRINT("%.*s", buf_len, (char *)buf);  // Line 1026 - DEBUG ONLY
    if (ctx->fds[fd].lux9_fid >= 0) {
      kwrite(ctx->fds[fd].lux9_fid, buf, buf_len);  // Line 1029
    }
    total_written += buf_len;
  }
  ...
}
```

**CRITICAL ISSUE #1**: Line 1026 uses `DPRINT` which is conditional on `boot_verbose`. If this flag is 0, userspace writes never produce any output, even though `kwrite()` is called.

### 2. The kwrite() Path

**Kernel Write** (`kernel/9front-port/sysfile.c:794`):
```c
static long write(int fd, void *buf, long len, vlong *offp, int check) {
  ...
  c = fdtochan(fd, OWRITE, 1, 1);
  ...
  m = devtab[c->type]->write(c, buf, n, off);  // Line 835
  ...
}
```

**Console Write** (`kernel/9front-port/devcons_minimal.c:183-226`):
```c
static long conswrite(Chan *c, void *va, long n, vlong offset) {
  ...
  case Qcons:
    {
      char tmp[128];
      long left = n;
      char *ptr = p;
      while (left > 0) {
        int chunk = (left < sizeof(tmp) - 1) ? left : sizeof(tmp) - 1;
        memmove(tmp, ptr, chunk);
        tmp[chunk] = 0;
        print("%s", tmp);  // Line 220 - Uses kernel print()
        ptr += chunk;
        left -= chunk;
      }
    }
  ...
}
```

**CRITICAL ISSUE #2**: `conswrite()` calls `print()` which depends on `screenputs` being properly initialized. If the framebuffer console isn't working, this may fail silently.

### 3. Framebuffer Console Initialization

**Initialization** (`kernel/9front-pc64/fbconsole.c:378-446`):
```c
void fbconsoleinit(void) {
  ...
  if (!saved_fb_info.valid) {
    uartputs("fbconsole: no saved framebuffer info\n", 38);
    return;  // EARLY RETURN - No framebuffer!
  }
  ...
  uartputs("fbconsole: PRE-HOOK\n", 21);
  screenputs = fbconsolescreenputs;  // Line 444
  uartputs("fbconsole: screenputs hooked to framebuffer console\n", 52);
}
```

**CRITICAL ISSUE #3**: If `saved_fb_info.valid` is 0 (no framebuffer detected), `fbconsoleinit()` returns early and never sets `screenputs`. This means `print()` in `conswrite()` has nothing to hook to.

### 4. Main Boot Sequence

**Main Boot** (`kernel/9front-pc64/main.c:351-356`):
```c
boot_log("main_after_cr3: calling fbconsoleinit ENTER\n");
fbconsoleinit();
console_ready = 1; /* Console is now initialized */
/* DEBUG: Force UART output until we are sure it works */
console_ready = 0;  // DISABLED!
boot_log("DEBUG: fbconsoleinit RETURNED\n");
```

**CRITICAL ISSUE #4**: `console_ready` is set to 1, then immediately set back to 0. This disables console output in `print()`.

### 5. Kernel print() Implementation

**Print Function** (`kernel/9front-port/print.c:12-26`):
```c
int print(char *fmt, ...) {
  va_list arg;
  char buf[1024];
  int n;

  va_start(arg, fmt);
  n = vseprint(buf, buf + sizeof(buf), fmt, arg) - buf;
  va_end(arg);

  if (screenputs)
    screenputs(buf, n);  // Line 21 - Only calls if screenputs exists!
  uartputs(buf, n);      // Line 23 - Always calls UART

  return n;
}
```

**GOOD NEWS**: `print()` always calls `uartputs()` on line 23, so UART should work. The issue is that if `conswrite()` never gets called (due to fd mapping problems), this never executes.

### 6. /dev/cons Setup in proc0

**Userinit** (`kernel/9front-port/userinit.c:458-498`):
```c
/* Open /dev/cons for Stdin/Stdout/Stderr (FD 0, 1, 2) */
c = namec("#c/cons", Aopen, OREAD, 0);
...
if (newfd(c, 0) != 0) { ... }  // FD 0 - stdin

c = namec("#c/cons", Aopen, OWRITE, 0);
...
if (newfd(c, 0) != 1) { ... }  // FD 1 - stdout

c = namec("#c/cons", Aopen, OWRITE, 0);
...
if (newfd(c, 0) != 2) { ... }  // FD 2 - stderr

print("BOOT[proc0]: FDs 0,1,2 bound to /dev/cons\n");
```

This correctly sets up file descriptors 0, 1, 2 to point to `/dev/cons`.

## Root Causes Identified

1. **Framebuffer Detection Failure**: If the system doesn't have a framebuffer or it's not properly detected, `fbconsoleinit()` returns early and never sets up `screenputs`. This breaks the `print()` path in `conswrite()`.

2. **Debug-Only Output in WASM**: The WASM fd_write implementation uses `DPRINT` for output which is disabled unless `boot_verbose=1`. The actual `kwrite()` happens but produces no visible output.

3. **Console Ready Flag**: `console_ready` is set to 1 then immediately 0, which might be disabling some output paths.

4. **Missing Direct UART Path**: The `/dev/cons` write handler doesn't directly call `uartputs()` - it uses `print()` which may not be reliable if `screenputs` isn't set.

## Recommendations

### Fix 1: Make `/dev/cons` Write Direct to UART
Modify `conswrite()` in `devcons_minimal.c` to always write to UART directly:
```c
case Qcons:
  /* Write directly to UART */
  for (int i = 0; i < n; i++) {
    uartputc(((char*)va)[i]);
  }
  return n;
```

### Fix 2: Ensure screenputs is Always Set
In `devcons_minimal.c:consinit()`, ensure `screenputs` is always set to `devcons_screenputs`:
```c
static void consinit(void) {
  kprintinit();
  screenputs = devcons_screenputs;  // Always set, don't check for nil
}
```

### Fix 3: Fix DPRINT in wasi_lux9_shim.c
Make the DPRINT unconditional or add a direct UART output:
```c
if (fd == 1 || fd == 2) {
  uartputs(buf, buf_len);  // Direct UART output
  if (ctx->fds[fd].lux9_fid >= 0) {
    kwrite(ctx->fds[fd].lux9_fid, buf, buf_len);
  }
  total_written += buf_len;
}
```

### Fix 4: Check Framebuffer Detection
Verify that framebuffer info is being properly saved and retrieved:
- Check `save_framebuffer_info()` is called before CR3 switch
- Verify `saved_fb_info` is properly populated

## Test Cases

To verify the fix:
1. Build ISO: `make iso -C kernel`
2. Run in QEMU: `qemu-system-x86_64 -cdrom kernel/lux9.iso -display none -serial stdio`
3. Check if WASM hello program outputs "Hello from WASM Integration Test!"

## Conclusion

The userspace printing is broken because:
1. `/dev/cons` write handler depends on framebuffer console which may not initialize
2. WASM fd_write uses debug prints that are disabled by default
3. The UART output path is not directly used by `/dev/cons` writes

The fix requires ensuring UART output is always available as a fallback, even when framebuffer console fails to initialize.
