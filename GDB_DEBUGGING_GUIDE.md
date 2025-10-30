# GDB Debugging Guide for Remaining Lock Issue

## Current Status
We've successfully resolved the original Physseg deadlock issue by implementing a dynamic doubly-linked list. However, there's still a lock loop occurring at a different address: `0xffff800000125778` in the `attachimage()` function.

## Debugging Approach

### 1. Prepare QEMU for GDB Debugging

First, let's create a script to launch QEMU with GDB support:

```bash
#!/bin/bash
# save as debug_kernel.sh
cd /home/scott/Repo/lux9-kernel
echo "Launching QEMU with GDB support..."
qemu-system-x86_64 \
    -kernel lux9.elf \
    -s -S \
    -nographic \
    -serial stdio \
    -no-reboot
```

Make it executable:
```bash
chmod +x debug_kernel.sh
```

### 2. Launch QEMU with GDB Stub

In terminal 1:
```bash
cd /home/scott/Repo/lux9-kernel
./debug_kernel.sh
```

The kernel will start but pause at the first instruction (`-S` flag).

### 3. Connect GDB in Terminal 2

In terminal 2:
```bash
cd /home/scott/Repo/lux9-kernel
gdb ./lux9.elf
```

Then in GDB:
```gdb
(gdb) target remote localhost:1234
(gdb) symbol-file ./lux9.elf
```

### 4. Set Breakpoints for Investigation

Set breakpoints at key locations:

```gdb
# Breakpoint at the attachimage function where the lock loop occurs
(gdb) break attachimage

# Breakpoint at the specific line causing the lock (line 477)
(gdb) break kernel/9front-port/segment.c:477

# Breakpoint at imagealloc lock
(gdb) break lock
(gdb) commands
>if $arg0 == &imagealloc
>  printf "Locking imagealloc at %p\n", &imagealloc
>  bt
>end
>continue
>end

# Breakpoint at unlock
(gdb) break unlock
(gdb) commands
>if $arg0 == &imagealloc
>  printf "Unlocking imagealloc at %p\n", &imagealloc
>end
>continue
>end
```

### 5. Run and Investigate

```gdb
(gdb) continue
```

When the breakpoint hits, examine the state:

```gdb
# Check what's happening at the lock
(gdb) info registers
(gdb) bt  # backtrace
(gdb) info locals

# Examine the imagealloc structure
(gdb) p imagealloc
(gdb) p &imagealloc

# Check if the lock is already held
(gdb) p imagealloc.lock.key
(gdb) p imagealloc.lock.pc

# Examine the specific lock address
(gdb) p *(Lock*)0xffff800000125778
```

### 6. Advanced Debugging Techniques

#### Watch the specific lock:
```gdb
(gdb) watch *((ulong*)0xffff800000125778)
```

This will break whenever the lock key changes.

#### Check the calling function:
```gdb
(gdb) info symbol 0xffffffff802c946c
```

This gives you the function name at that address.

#### Examine the lock holder:
```gdb
(gdb) p ((Lock*)0xffff800000125778)->key
(gdb) p ((Lock*)0xffff800000125778)->pc
```

### 7. Specific Investigation Commands

Once you're at the breakpoint in `attachimage()`:

```gdb
# Check the Chan parameter
(gdb) p *c

# Check if we're in a retry loop
(gdb) p i  # the Image variable
(gdb) p imagealloc.nidle
(gdb) p conf.nimage

# Check the hash lookup
(gdb) p c->qid.path
(gdb) p ihash(c->qid.path)

# Check if we're in the retry section
(gdb) p i
(gdb) p newimage(pages)
```

### 8. Memory Analysis

To understand what lock `0xffff800000125778` is:

```gdb
# Check what structure this address belongs to
(gdb) info symbol 0xffff800000125778

# If it's within a structure, check the structure
(gdb) p &imagealloc
(gdb) p sizeof(struct Imagealloc)

# Check nearby memory
(gdb) x/20gx 0xffff800000125700
```

### 9. Process Information

Check which process is holding the lock:

```gdb
# Check current process
(gdb) p up
(gdb) p up->pid
(gdb) p up->text

# Check all processes if needed
(gdb) p *proc
```

### 10. Common Issues to Look For

1. **Double-lock**: Same process trying to lock twice
2. **Interrupt during lock**: ISR trying to acquire same lock
3. **Retry loop**: attachimage going into infinite retry
4. **Memory corruption**: Lock structure being corrupted

### 11. Useful GDB Aliases

Add these to your .gdbinit for convenience:

```
define show_lock
  p *(Lock*)$arg0
end

define show_imagealloc
  p imagealloc
  p imagealloc.nidle
  p imagealloc.pgidle
  p conf.nimage
end

define show_proc
  p up
  p up->pid
  p up->text
end
```

### 12. Step-by-Step Investigation Process

1. **Set breakpoints** as shown above
2. **Run until first hit** of attachimage
3. **Check parameters** - what image are we trying to attach?
4. **Continue** and see if we hit the retry path
5. **Watch the lock** - who's holding it and when
6. **Check for recursion** - same process trying to lock twice
7. **Look for interrupts** - any ISR activity

### 13. Expected Findings

The issue is likely in the `attachimage` retry logic:

```c
retry:
    lock(&imagealloc);
    // ... search logic ...
    if(imagealloc.nidle > conf.nimage || (i = newimage(pages)) == nil) {
        unlock(&imagealloc);
        if(imagealloc.nidle == 0)
            error(Enomem);
        if(imagereclaim(0) == 0)
            freebroken();  // can use the memory
        goto retry;  // ← This might be the problem
    }
```

Check if:
- `imagealloc.nidle` is always > `conf.nimage`
- `newimage(pages)` always returns `nil`
- `imagereclaim(0)` never frees memory
- `freebroken()` doesn't actually free anything

This would cause an infinite loop with the lock held.