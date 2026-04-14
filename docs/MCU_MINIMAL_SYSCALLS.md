# Minimal Kernel Syscalls for MCU Microkernel

## Current State: 38+ Syscalls in Lux9
- File ops: OPEN, CLOSE, READ, WRITE, CREATE, SEEK, STAT, REMOVE (8)
- Process: FORK, RFORK, EXIT, WAIT, BRK, EXEC, SEGATTACH (7)
- IPC: PIPE, EXCHANGE_* (12+ syscalls)
- WASM: COMPILE, EXECUTE, DESTROY (3)
- Pebble: ALLOC, FREE, etc (6+)
- Misc: NSEC, MOUNT, etc. (2+)

**Total: ~38 syscalls**

## True Microkernel: 2-4 Syscalls

### Comparison with seL4 (World's Most Minimal Kernel)

seL4 has **7 kernel syscalls**:
1. `seL4_Send` - Send message, don't wait
2. `seL4_Recv` - Wait for message
3. `seL4_Call` - Send + wait for reply (RPC)
4. `seL4_Reply` - Reply to caller
5. `seL4_Yield` - Give up CPU
6. `seL4_NBSend` - Non-blocking send
7. `seL4_Wait` - Wait on notification

**Everything else** (files, network, drivers, processes) is handled by **userspace servers**.

### For MCU: We Can Do Better Than seL4

**Absolute Minimum: 2 syscalls**

```c
// kernel/syscall.h - THE ENTIRE KERNEL INTERFACE

// 1. Call: Synchronous RPC (send message, wait for reply)
int sys_call(uint32_t dest_pid, Msg *req, Msg *reply);

// 2. Serve: Wait for message, send reply (for servers only)
int sys_serve(Msg *req, Msg *reply);
```

**That's it. Everything else is messages.**

## How It Works

### Syscall 1: `sys_call()` - Client RPC

```c
// Userspace client wants to open a file
Msg req, reply;
req.type = Topen;
strcpy(myexchange, "/env/PATH");
req.io.buf_ofs = 0;

// Call filesystem server (PID 2)
sys_call(2, &req, &reply);

// Reply contains file descriptor
int fd = reply.fid;
```

**Kernel behavior:**
1. Copy `req` from userspace
2. Find dest process (PID 2)
3. Queue message to dest
4. Block current process (state = Waiting)
5. Schedule dest process
6. Dest process calls `sys_serve()`
7. Kernel delivers message
8. Dest process sends reply
9. Kernel unblocks caller, copies reply
10. Return to caller

### Syscall 2: `sys_serve()` - Server Loop

```c
// userspace/envd/main.c - Environment server
void main(void) {
    Msg req, reply;

    while (1) {
        // Wait for request, send reply
        sys_serve(&req, &reply);

        switch (req.type) {
        case Topen:
            // Handle open
            reply.fid = allocate_fid();
            break;

        case Tread:
            // Copy value to exchange buffer
            memcpy(myexchange, lookup_var(req.fid), len);
            reply.io.count = len;
            break;
        }
    }
}
```

**Kernel behavior:**
1. If message queue has message:
   - Copy message to `req`
   - Remember caller PID
2. Else:
   - Block server (state = WaitingForRequest)
   - Schedule next process
3. When server fills `reply`:
   - Copy reply to caller's memory
   - Unblock caller
   - Continue server loop

## What About Process Management?

**Answer: It's also a userspace server!**

```c
// userspace/procd/main.c - Process manager server
void main(void) {
    Msg req, reply;

    while (1) {
        sys_serve(&req, &reply);

        switch (req.type) {
        case Tspawn: {
            // Load a.out binary from exchange buffer
            struct Exec *hdr = (struct Exec*)(myexchange + req.proc.buf_ofs);

            // Ask kernel to create new process slot
            Msg kreq, kreply;
            kreq.type = Tnewproc;
            sys_call(KERNEL_PID, &kreq, &kreply);

            uint32_t new_pid = kreply.proc.pid;

            // Load binary into new process
            load_aout(new_pid, hdr);

            reply.proc.pid = new_pid;
            break;
        }

        case Tkill: {
            // Ask kernel to destroy process
            Msg kreq, kreply;
            kreq.type = Tdestroyproc;
            kreq.proc.pid = req.proc.pid;
            sys_call(KERNEL_PID, &kreq, &kreply);
            break;
        }
        }
    }
}
```

**Wait, but how do you call the kernel?**

The kernel has a special PID (0 or 1) that handles:
- `Tnewproc` - Allocate process slot
- `Tdestroyproc` - Deallocate process
- `Tmapregion` - Map memory (MPU configuration)

So we actually need **one kernel server** that handles these low-level operations.

## Revised: 2 Syscalls + Kernel Server

### The 2 Syscalls (User → User communication)

```c
int sys_call(pid_t dest, Msg *req, Msg *reply);   // Client: call server
int sys_serve(Msg *req, Msg *reply);              // Server: serve requests
```

### The Kernel Server (PID 0)

Kernel answers its own messages for:

```c
// Handled by kernel server (PID 0)
Tnewproc      → Allocate process slot
Tdestroyproc  → Free process slot
Tmapregion    → Configure MPU region
Tunmap        → Remove MPU region
Tyield        → Give up CPU
Tirqwait      → Wait for interrupt
```

**Implementation:**

```c
// kernel/kernel_server.c
// This runs as "PID 0" but executes in kernel mode
void kernel_server_loop(void) {
    Msg req, reply;

    while (1) {
        // Check if someone called PID 0
        if (kernel_has_message()) {
            kernel_recv_message(&req);

            switch (req.type) {
            case Tnewproc: {
                // Allocate process slot
                Proc *p = alloc_proc();
                reply.proc.pid = p->pid;
                break;
            }

            case Tdestroyproc: {
                // Free process
                Proc *p = find_proc(req.proc.pid);
                free_proc(p);
                break;
            }

            case Tmapregion: {
                // Configure MPU
                Proc *p = find_proc(req.proc.pid);
                mpu_set_region(p, req.proc.region, req.proc.base,
                               req.proc.size, req.proc.perms);
                break;
            }

            case Tyield: {
                // Just schedule next process
                schedule();
                break;
            }

            case Tirqwait: {
                // Block until interrupt
                Proc *p = curproc();
                p->state = WaitingForIRQ;
                p->irq_mask = req.proc.irq_mask;
                schedule();
                break;
            }
            }

            kernel_send_reply(&reply);
        }
    }
}
```

## Complete System Architecture

```
Userspace Applications
  ↓ sys_call(server_pid, msg, reply)
  ↓
Userspace Servers
  ├─ PID 2: envd (environment variables)
  ├─ PID 3: gpioserv (GPIO pins)
  ├─ PID 4: uartserv (UART)
  ├─ PID 5: initfs (filesystem)
  ├─ PID 6: procd (process manager)
  └─ PID 7+: user apps
     ↓ sys_call(0, msg, reply) for kernel services
     ↓
Kernel Server (PID 0)
  ├─ Tnewproc
  ├─ Tdestroyproc
  ├─ Tmapregion
  └─ Tirqwait
```

## Example: Opening a File

```c
// user application
int fd = open("/env/PATH", O_RDONLY);

// liblux implementation
int open(const char *path, int mode) {
    Msg req, reply;

    // Find server for path
    // "/env/*" → envd (PID 2)
    // "/dev/gpio0" → gpioserv (PID 3)
    // etc.
    pid_t srv = lookup_server(path);

    req.type = Topen;
    strcpy(myexchange, path);
    req.io.mode = mode;

    // Call server
    sys_call(srv, &req, &reply);

    return reply.fid;
}
```

## Example: Spawning a Process

```c
// user application
pid_t pid = spawn("/bin/hello");

// liblux implementation
pid_t spawn(const char *path) {
    Msg req, reply;

    // Load binary from filesystem
    int fd = open(path, O_RDONLY);
    read(fd, myexchange, EXCHANGE_SIZE);
    close(fd);

    // Ask procd to spawn
    req.type = Tspawn;
    req.proc.buf_ofs = 0;

    sys_call(PROCD_PID, &req, &reply);

    return reply.proc.pid;
}

// procd implementation
// (in sys_serve loop)
case Tspawn: {
    // Ask kernel for new process slot
    Msg kreq, kreply;
    kreq.type = Tnewproc;
    sys_call(KERNEL_PID, &kreq, &kreply);

    pid_t new_pid = kreply.proc.pid;

    // Load a.out binary
    struct Exec *hdr = (struct Exec*)(myexchange + req.proc.buf_ofs);

    // Map memory regions
    kreq.type = Tmapregion;
    kreq.proc.pid = new_pid;
    kreq.proc.region = 0;  // Text
    kreq.proc.base = TEXT_BASE;
    kreq.proc.size = hdr->text_size;
    kreq.proc.perms = MPU_RX;
    sys_call(KERNEL_PID, &kreq, &kreply);

    // Copy text section
    memcpy_to_process(new_pid, TEXT_BASE, myexchange + 32, hdr->text_size);

    // Start process
    kreq.type = Tstart;
    kreq.proc.pid = new_pid;
    kreq.proc.entry = TEXT_BASE + hdr->entry;
    sys_call(KERNEL_PID, &kreq, &kreply);

    reply.proc.pid = new_pid;
}
```

## Kernel Size Estimate (With 2-Syscall Interface)

```
Message router:       1.5 KB  (just dispatch sys_call/sys_serve)
Message queues:       1 KB    (ring buffers)
Process table:        0.5 KB  (struct Proc array)
Scheduler:            1 KB    (priority + round-robin)
Context switch:       0.5 KB  (save/restore ARM registers)
MPU driver:           1 KB    (configure protection regions)
Kernel server:        2 KB    (Tnewproc, Tdestroyproc, Tmapregion)
Syscall entry:        0.5 KB  (SVC handler)
Boot/init:            2 KB    (start first process)
────────────────────────────
Total:               ~10 KB flash
```

**RAM:**
```
Process table:    16 × 128 bytes = 2 KB
Message queues:   16 × 8 msgs × 32 bytes = 4 KB
Kernel stack:     2 KB
────────────────────────────
Total:           ~8 KB RAM
```

## Comparison

| Architecture | Kernel Syscalls | File Ops | Process Control |
|--------------|----------------|----------|-----------------|
| **Lux9 Full** | 38+ | Kernel routes | Kernel handles |
| **seL4** | 7 | Userspace | Userspace |
| **MCU Minimal** | **2** | Userspace | Userspace (procd) |

## Why This Is Better

### For Formal Verification
- 2 syscalls vs 38 → 19x less to verify
- No complex routing logic
- Simple state machine: Call → Waiting → Reply

### For Security
- Minimal attack surface
- Process manager runs in userspace (can be updated)
- Filesystem runs in userspace (crashes don't kill kernel)
- GPIO drivers run in userspace (bugs don't corrupt kernel)

### For Real-Time
- Bounded syscall latency (just message copy + context switch)
- Predictable scheduling
- No dynamic allocation

### For Size
- 10KB kernel flash (vs 16KB with complex routing)
- Simpler code → smaller binary

## What Gets Eliminated from Lux9

❌ ~~File operations in kernel (OPEN, CLOSE, READ, WRITE)~~
❌ ~~Process management in kernel (FORK, EXEC, WAIT)~~
❌ ~~Complex exchange pool with pub/sub~~
❌ ~~WASM runtime~~
❌ ~~Pebble memory management~~
❌ ~~Mount/bind syscalls~~
❌ ~~38+ syscall handlers~~

✅ **2 syscalls: call() and serve()**
✅ **Kernel server for process/memory primitives**
✅ **Everything else in userspace**

## Next Steps

This is the TRUE minimal 9P-inspired microkernel. Want me to:
1. **Prototype it?** (kernel/syscall.c, kernel/msg.c, kernel/sched.c)
2. **Write procd?** (userspace process manager)
3. **Compare with seL4?** (paper draft showing we're even simpler)
