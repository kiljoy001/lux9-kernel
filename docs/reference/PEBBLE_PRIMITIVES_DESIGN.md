# Pebble Primitives Design for Lux 9

## 1. High-Level Design Overview

### Conceptual Mapping

| Pebble Primitive | Role in Pebble Model                     | Lux 9 Mapping / Analogy                                                                                  |
|------------------|-------------------------------------------|-----------------------------------------------------------------------------------------------------------|
| **Black-only**   | Non-clonable resource; must stay in kernel | Capability-controlled memory allocation; similar to `/dev/dma` or kernel heap blocks held in `Proc`      |
| **Black–White**  | White token from user validated to black  | Capability verification at syscall boundary; akin to `kenter()` or `pageown` borrow checker              |
| **Red–Blue**     | Copy-on-write shadow; red (safe) of blue  | Snapshot/duplicate semantics; similar to `forkret` return path or TLB shootdown-handled context copying  |

### Data-Flow Diagram (Open Path)

```
user open() syscall
        |
        v
 +--------------+
 | white token? |---- no --> existing path (traditional open)
 +--------------+
        | yes
        v
white → pebble_white_verify()
        |
        v
black handle created → tracked in Proc
        |
        v
pebble_black_alloc() if new buffer needed
        |
        v
normal open processing using black handle
        |
        v
return → sysret → red-blue check (if copy required)
```

### Code Locations

| Concern                        | Candidate Files                                                         |
|--------------------------------|--------------------------------------------------------------------------|
| Kernel globals & API glue      | `kernel/9front-pc64/globals.c`, `kernel/include/fns.h`, `kernel/include/dat.h` |
| Syscall/verification hook      | `kernel/9front-pc64/trap.c` (`syscall`, `kenter`), `kernel/9front-port/sysproc.c` |
| Assembly return path (red-blue)| `kernel/9front-pc64/l.S` (`sysretq`, `forkret`)                          |
| Capability bookkeeping         | `kernel/9front-port/proc.c` (augment `Proc` struct)                      |
| Config flag & boot             | `kernel/9front-pc64/main.c`, `kernel/include/conf.h`                    |
| Documentation                  | `docs/PEBBLE_IMPLEMENTATION.md`, `docs/PEBBLE_API.md`                    |

---

## 2. API Specification

### Kernel Entry Points / Syscalls

```c
int pebble_white_issue(ulong size, PebbleWhite **tokenp);
int pebble_black_alloc(uintptr size, void **handle);
int pebble_black_free(void *handle);
int pebble_white_verify(void *white_cap, void **black_cap);
int pebble_red_copy(void *blue_obj, void **red_copy);
int pebble_blue_discard(void *blue_obj);
```

### Common Structures / Bookkeeping

- Extend `Proc`:

  ```c
  typedef struct PebbleState {
      ulong black_budget;      /* remaining bytes/process */
      ulong black_inuse;       /* current allocated */
      ulong white_verified;    /* count of active white→black conversions */
      ulong red_count;         /* number of live red shadows */
      ulong blue_count;        /* live blue objects */
      void *pebble_list;       /* optional list head  */
  } PebbleState;
  ```

- `pebble_enabled` global flag (boot option).

### Invariants & Error Codes

| Function                     | Purpose                                      | Key Invariants                                                                                   | Errors (return -1, set `up->errstr`)              |
|-----------------------------|----------------------------------------------|---------------------------------------------------------------------------------------------------|--------------------------------------------------|
| `pebble_white_issue`        | Mint white token with size quota             | - Only allowed when Pebble enabled<br>- Fails if token ring full                                  | `Eperm` (disabled)<br>`Eagain` (token exhaustion) |
| `pebble_black_alloc`        | Allocate kernel-managed opaque buffer        | - Budget not exceeded<br>- Requires verified white→black sequence or kernel-only caller          | `Eperm` (no capability)<br>`Eagain` (budget)<br>`Enomem` |
| `pebble_black_free`         | Release black buffer                         | - Handle must belong to caller<br>- If blue copy exists, must wait for red copy before free       | `Eperm` (foreign handle)<br>`Ebusy` (red-blue unmet) |
| `pebble_white_verify`       | Validate white cap, produce black cap        | - White token must be freshly minted<br>- Only once; duplicates rejected                          | `Eperm` (invalid token)<br>`Ebadarg` (null pointers) |
| `pebble_red_copy`           | Produce safe copy from blue object           | - Blue must exist<br>- Red copy increments `red_count`                                           | `Eperm` (no blue)<br>`Enomem`                     |
| `pebble_blue_discard`       | Drop speculative blue obj                    | - Allowed only if matching red copy exists or caller holds special capability                    | `Ebusy` (no red)<br>`Eperm`                       |

Each routine should emit debug logs (`print("PEBBLE ...")`) when `pebble_debug` flag is set.

---

## 3. Implementation Sketch (Pseudo-code)

### Core Helpers

```c
static PebbleState*
pebble_state(void)
{
    return &up->pebble;
}

int
pebble_black_alloc(uintptr size, void **handle)
{
    PebbleState *ps;
    void *buf;

    if(!pebble_enabled)
        error("pebble disabled");

    ps = pebble_state();
    if(size == 0 || handle == nil)
        error(Ebadarg);

    ilock(&pebble_lock);
    if(ps->white_verified == 0 || ps->white_pending < size){
        iunlock(&pebble_lock);
        error(Eperm);
    }
    if(ps->black_budget < size){
        iunlock(&pebble_lock);
        error(Eagain);
    }
    ps->white_pending -= size;
    ps->white_verified--;
    ps->black_budget -= size;
    ps->black_inuse += size;
    iunlock(&pebble_lock);

    buf = xalloc(size);
    if(buf == nil){
        ilock(&pebble_lock);
        ps->black_budget += size;
        ps->black_inuse -= size;
        iunlock(&pebble_lock);
        error(Enomem);
    }

    *handle = buf;
    if(pebble_debug)
        print("PEBBLE B alloc %p %lud\n", buf, size);
    return 0;
}
```

Similar pseudo-code for free, verify, red copy:

```c
int
pebble_white_verify(void *white_cap, void **black_cap)
{
    PebbleState *ps;

    if(white_cap == nil || black_cap == nil)
        error(Ebadarg);

    ps = pebble_state();
    lock(&pebble_white_lock);
    if(!valid_white_token(white_cap)){
        unlock(&pebble_white_lock);
        error(Eperm);
    }
    /* transform white→black */
    *black_cap = issue_black_handle(white_cap);
    ps->white_pending += white_cap->size;
    ps->white_verified++;
    unlock(&pebble_white_lock);

    if(pebble_debug)
        print("PEBBLE W verify %p -> %p\n", white_cap, *black_cap);
    return 0;
}
```

Red-blue:

```c
int
pebble_red_copy(void *blue_obj, void **red_copy)
{
    if(blue_obj == nil || red_copy == nil)
        error(Ebadarg);

    lock(&pebble_rb_lock);
    if(!blue_exists(up, blue_obj)){
        unlock(&pebble_rb_lock);
        error(Eperm);
    }
    *red_copy = duplicate_blue(blue_obj);
    mark_red(up, *red_copy);
    unlock(&pebble_rb_lock);

    if(pebble_debug)
        print("PEBBLE RB copy blue %p -> red %p\n", blue_obj, *red_copy);
    return 0;
}

int
pebble_blue_discard(void *blue_obj)
{
    lock(&pebble_rb_lock);
    if(!blue_exists(up, blue_obj)){
        unlock(&pebble_rb_lock);
        error(Eperm);
    }
    if(!has_matching_red(up, blue_obj)){
        unlock(&pebble_rb_lock);
        error(Ebusy);
    }
    destroy_blue(blue_obj);
    unlock(&pebble_rb_lock);
    if(pebble_debug)
        print("PEBBLE RB discard blue %p\n", blue_obj);
    return 0;
}
```

### Sysret / Red-Blue Hook (assembly sketch)

In `kernel/9front-pc64/l.S` after `sysretq` preparation:

```asm
    /* prior to sysretq */
    call    pebble_red_blue_exit   /* new C helper: handles pending blue→red enforcement */
    sysretq
```

`pebble_red_blue_exit`:

```c
void
pebble_red_blue_exit(void)
{
    PebbleState *ps = pebble_state();
    if(ps->blue_count == 0)
        return;
    lock(&pebble_rb_lock);
    /* iterate pending blue objects requiring red copy */
    ensure_red_snapshots(ps);
    unlock(&pebble_rb_lock);
}
```

### Integration with `userureg()/kenter()`

- In `kenter()` (or right after `user = kenter(ureg);` in `trap.c`), call `pebble_auto_verify(up, ureg)`:

```c
if(user && pebble_enabled)
    pebble_auto_verify(up, ureg);
```

`pebble_auto_verify`:

```c
void
pebble_auto_verify(Proc *p, Ureg *ureg)
{
    void *white = find_pending_white(p);
    void *black;

    if(white == nil)
        return;
    if(pebble_white_verify(white, &black) == 0)
        attach_black_to_proc(p, black);
}
```

---

## 4. Testing Strategy

### Test Goals

1. **Black budget enforcement**: Attempt to allocate beyond per-process quota → expect `Eagain`.
2. **White pre-verification**: Use white capability directly (e.g., pass to `pebble_black_alloc` without verification) → expect kernel error (`Eperm`).
3. **Red-blue invariant**: Discard blue object without prior red copy → expect `Ebusy`.

### Testing Framework

- Extend `shell_scripts/run_test.sh` to accept new `TEST=pebble_*`.
- Use Go/rc program to exercise syscalls via `/proc/syscall` or direct wrappers.
- Monitor serial log (`qemu.log`) for `PEBBLE` debug messages.

### Sample Bash Snippets

**Test 1: Budget**

```bash
cat > /tmp/test_pebble_budget.c <<'EOF'
#include <u.h>
#include <libc.h>
int main(void){
    void *h;
    int i;
    for(i=0; i<5; i++){
        if(pebble_black_alloc(512*1024*1024, &h) < 0){
            fprint(2, "alloc failed: %r\n");
            exits("fail");
        }
    }
    exits(nil);
}
EOF
9c /tmp/test_pebble_budget.c && 9l /tmp/test_pebble_budget.o
./a.out > /tmp/pebble.out 2>&1 || echo expected failure
grep "PEBBLE B alloc" qemu.log
```

Expect final call to fail with `alloc failed: quota exceeded`.

**Test 2: White misuse**

```bash
cat <<'EOF' >/tmp/test_pebble_white.rc
#!/bin/rc
if(! test -e /dev/pebble) {
    echo 'pebble disabled'
    exit fail
}
# Create fake white token but skip verify
pebble_black_alloc 1024 '' >/tmp/result || echo 'error as expected'
grep 'Eperm' /tmp/result
EOF
```

**Test 3: Red-blue**

```bash
cat <<'EOF' >/tmp/test_pebble_red.rc
#!/bin/rc
# Acquire blue object (e.g., open with pebble flags)
echo 'alloc blue' > /dev/pebble/ctl
cat /dev/pebble/blue/0 >[2]/tmp/blue.log
echo 'discard 0' > /dev/pebble/ctl   # should fail until red copy exists
grep 'PEBBLE RB discard denied' qemu.log
EOF
```

Adapt testers to exit with non-zero code on failure and integrate in `run_test.sh`.

---

## 5. Documentation Updates

### `docs/PEBBLE_API.md` (new section)

```
## Pebble Kernel API

### pebble_black_alloc(size uintptr, handle **void)
Allocate an opaque “black” object owned by the kernel.

- Preconditions: caller has verified white capability.
- Returns: 0 on success, -1 on error (`Eagain` if budget exceeded).
- Example (Go):

```
func BlackAlloc(size uint64) (unsafe.Pointer, error) {
    var handle unsafe.Pointer
    r := C.pebble_black_alloc(C.uintptr_t(size), &handle)
    if r < 0 {
        return nil, syscall.Errno(C.geterr())
    }
    return handle, nil
}
```

Similarly document the other syscalls, invariants, and typical sequences (white→black, red-blue closure).

### Placement

- `docs/PEBBLE_IMPLEMENTATION.md`: high-level design, state diagrams.
- Short “Getting Started” in `docs/DEVELOPERS.md` linking to the new doc:
  - Enable flag: add `pebble=1` to kernel cmdline.
  - Build Go wrapper in `userspace/go-lux/pebble`.
  - Run tests via `make test-pebble`.

---

## 6. Migration / Compatibility Notes

- **Backward compatibility**: Existing SIP/9P services untouched. Pebble API only active when `pebble` boot flag present.
- **Kernel flag**: Add parsing in `main.c`:

  ```
  if(findconf("pebble"))
      pebble_enabled = 1;
  ```

- **Initrd**: Ensure new kernel binary and optional `libpebble.a` (if created) copied into initrd image. Update `userspace/Makefile` to build Go wrapper and include any CLI utilities.
- **Userland fallback**: When pebble disabled, syscalls return `Eperm`, allowing old code to continue.

---

## 7. Open Questions / Future Work

1. **Formal Verification**: Need mechanical proof (Coq/Isabelle) that red-blue invariant (no blue discard before red copy) holds across preemption and SMP.
2. **SMP Scaling**: Extend locks to per-core structures or RCU-friendly lists to avoid contention in high-CPU systems.
3. **Integration with borrow checker**: Explore merging pebble tracking with existing `borrowchecker.c` to reduce duplication.
4. **Go bindings**: Decide whether to ship FFI-based wrappers or implement via Plan 9 syscalls interface (`syscall.Syscall`).
5. **Persisted state**: Determine whether pebble handles survive checkpoint/restore or need revalidation.
6. **Auditability**: Add tracing hooks to existing tracing infrastructure (`trace.c`) for compliance logging.
7. **User API**: Consider 9P exposure (`/dev/pebble/...`) for debug/management tools.

---

This document outlines the design, API, implementation sketch, testing approach, documentation plan, compatibility considerations, and future work needed to introduce Pebble primitives into the Lux 9 kernel while keeping the existing architecture intact.
