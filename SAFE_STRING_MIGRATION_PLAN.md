# Safe String Migration Plan

The "Invalid infinite range" verification errors in Frama-C are caused by the kernel's use of unbounded C string functions (`strcpy`, `sprintf`, etc.) and `char *` pointers without explicit length tracking.

## Strategy: "Bounded by Convention"

Instead of replacing `char *` with a struct (which would break the entire Plan 9 libc API compatibility), we will enforce bounds via **API Choice** and **ACSL Contracts**.

### 1. Banned Functions
The following functions are **forbidden** in new code and must be replaced in failing verification targets:
*   `strcpy(dst, src)` -> **USE** `strecpy(dst, dst+len, src)` or `utfecpy`
*   `strcat(dst, src)` -> **USE** `strecpy` (pointer arithmetic)
*   `sprintf(dst, fmt, ...)` -> **USE** `snprint(dst, len, fmt, ...)`
*   `vsprintf` -> **USE** `vsnprint`

### 2. The `strecpy` Pattern
Plan 9 provides `strecpy(char *s, char *e, char *p)`, which copies `p` to `s` stopping before `e`. This naturally maps to Frama-C's bounded validity checks.

**ACSL Contract for `strecpy`:**
```c
/*@
  @ requires \valid(s + (0 .. (e - s - 1)));
  @ requires valid_string(p);
  @ assigns s[0 .. (e - s - 1)];
  @ ensures \valid(\result);
  @*/
extern char *strecpy(char *s, char *e, char *p);
```

### 3. Verification Fix Workflow
For every file failing with "Invalid infinite range":
1.  Identify the buffer causing the issue (usually a local `char buf[N]`).
2.  Locate references to `strcpy` or `sprintf` targeting it.
3.  Replace with `strecpy(buf, buf+sizeof(buf), src)` or `snprint(buf, sizeof(buf), ...)`.
4.  If the function takes a `char *` argument without a length, **add a length argument** (refactor) or add an ACSL precondition `requires \valid(s + (0..ACSL_MAXSTR))`.

### 4. Implementation Steps
1.  [ ] Add rigorous ACSL contracts to `strecpy` and `snprint` in `portlib.h` / `libc.h`.
2.  [ ] Refactor `kernel/9front-port/dev.c` (first fail target) to use `strecpy`.
3.  [ ] Refactor `kernel/9front-port/devtpm.c` to use `strecpy`.
4.  [ ] Refactor `kernel/wasm/wasm_runtime/wasm3/m3_core.c` (interpreter loop string handling).

This approach treats `char *` + `len` (or `end`) as the logical "bstring".
