# Context Switch Optimization Techniques for Lux9 Microkernel

## Overview

Microkernel architecture requires frequent context switches for IPC between applications and filesystem servers. This document outlines techniques to minimize context switch overhead while maintaining the clean separation of the microkernel design.

## Baseline Architecture

```
Applications
     ↓
Userspace 9P client library (protocol handling)
     ↓
Kernel IPC (message passing between PIDs)
     ↓
Servers (ext4fs, devfs, procfs, etc.)
```

**Problem:** Each filesystem operation = 2 context switches (app→server, server→app)

**Goal:** Reduce switches from 2N to much smaller constant, regardless of N operations

---

## Optimization Techniques

### 1. Shared Memory Rings (Zero-Copy IPC)

**Concept:** Map same physical memory into both app and server address spaces. Only use syscalls for notifications, not data transfer.

```c
// Kernel manages ring buffer metadata
struct shared_ring {
    volatile uint32_t read_pos;
    volatile uint32_t write_pos;
    uint8_t data[RING_SIZE];  // Actual 9P messages here
};

// Setup (once per connection)
map_shared_ring(app_pid, server_pid, &ring);

// Application writes 9P message directly
memcpy(&ring.data[ring.write_pos], &fcall, msg_size);
ring.write_pos += msg_size;

// Tiny syscall - just wake server
sys_notify(server_pid);  // No data copy!

// Server reads directly from shared memory
while(ring.read_pos != ring.write_pos) {
    Fcall *msg = &ring.data[ring.read_pos];
    process_9p_message(msg);
    ring.read_pos += msg->size;
}
```

**Benefits:**
- Context switch still happens, but no memory copy
- Especially important for large reads/writes (multi-MB files)
- Ring buffer allows queuing multiple requests

**Implementation:**
- Kernel syscall: `sys_create_shared_ring(pid_t peer) → ring_id`
- Kernel tracks ring ownership, unmaps on process exit
- Use proper memory barriers for correctness

---

### 2. Batched Requests (Amortize Switches)

**Concept:** Send multiple 9P operations in one IPC message.

```c
// lib9pclient batching API
struct batch {
    int count;
    Fcall msgs[MAX_BATCH];  // e.g., 16 or 64
};

// Application queues operations
batch_t *b = batch_create();
batch_add(b, Topen, "/etc/passwd", O_RDONLY);
batch_add(b, Tstat, "/etc/group");
batch_add(b, Tread, fid, 0, 1024);
// ... add more

// Send all at once
batch_send(server_pid, b);

// Server processes entire batch
for(int i = 0; i < batch->count; i++) {
    responses[i] = handle_request(&batch->msgs[i]);
}

// Send all responses back
batch_reply(app_pid, responses, batch->count);
```

**Benefits:**
- N operations = 2 context switches (instead of 2N)
- Works with existing 9P2000 protocol
- Easy to implement

**Trade-offs:**
- Increased latency (wait to fill batch)
- More memory for batching

**Heuristics:**
- Flush batch after timeout (e.g., 1ms)
- Flush when batch full
- Flush on sync operations

---

### 3. Async 9P with Multiplexing (9P2024)

**Concept:** Send multiple requests without waiting for responses. Use tags to match replies.

```c
// Send 16 reads asynchronously
for(int i = 0; i < 16; i++) {
    9p_read_async(fds[i], offset, len, callbacks[i]);
}

// lib9pclient sends all 16 via shared ring
// ONE context switch to server

// Server processes all 16 (potentially in parallel)
// Server sends all 16 responses
// ONE context switch back to app

// lib9pclient dispatches responses to callbacks by tag
```

**Benefits:**
- 2 context switches for arbitrary N operations
- Pipelining hides latency
- Natural fit for GhostDAG ordering in 9P2024

**Implementation:**
- Requires async server implementation
- Tag management in lib9pclient
- Callback/promise based API

---

### 4. Polling Mode for Hot Paths

**Concept:** High-priority servers spin instead of sleeping.

```c
// devfs console server runs in polling mode
void console_server_main(void) {
    while(1) {
        if(ring_has_data(&console_ring)) {
            process_console_request();
            write_response();
        } else {
            // Yield CPU but don't sleep
            __builtin_ia32_pause();  // x86 PAUSE instruction
        }
    }
}
```

**Benefits:**
- Zero context switch latency for console I/O
- Sub-microsecond response time

**Trade-offs:**
- Wastes CPU cycles
- Only use for critical servers (console, timer)

**Hybrid approach:**
- Poll for N iterations
- Then sleep if no work

---

### 5. User-Level Scheduling (Scheduler Activations)

**Concept:** Kernel upcalls into userspace scheduler instead of immediately switching.

```c
// Kernel calls this instead of forcing context switch
void scheduler_upcall(int event, pid_t target) {
    switch(event) {
    case MSG_ARRIVED:
        // Application decides whether to switch now
        if(in_critical_section || holding_locks) {
            // Defer switch
            queue_pending(target);
        } else {
            // Switch immediately
            yield_to(target);
        }
        break;

    case QUANTUM_EXPIRED:
        // Pick next thread to run
        schedule_next();
        break;
    }
}
```

**Benefits:**
- Application controls scheduling policy
- Can defer switches during critical sections
- Enables gang scheduling (run related threads together)

**Complexity:**
- Requires kernel trust in userspace
- More complex than traditional scheduling

---

### 6. RPC Fastpath for Small Messages

**Concept:** Kernel optimizes tiny messages (version, stat, etc.)

```c
int sys_send(pid_t dest, void *msg, size_t len) {
    if(len <= 64 && dest_is_waiting(dest)) {
        // FASTPATH: minimal context save
        // Copy message into dest's stack
        // Jump directly to dest's receive handler
        fastpath_switch(dest, msg);  // ~100 cycles
    } else {
        // Full context switch
        full_context_switch(dest);  // ~1000+ cycles
    }
}
```

**Benefits:**
- 10x faster for small messages
- Tversion, Tstat, Tclunk use fast path
- No API changes

**Implementation:**
- Requires careful register saving
- Security: validate dest actually waiting
- Architecture-specific

---

### 7. Futex-Style Wait (Avoid Unnecessary Switches)

**Concept:** Check if response ready before switching.

```c
// Response flag in shared memory
volatile int response_ready = 0;

// Client sends request
send_request(server_pid, &msg);

// Check if response already ready (server was fast!)
if(atomic_load(&response_ready) == 1) {
    // Hot path - no context switch needed
    return process_response();
}

// Only switch if actually need to wait
sys_futex_wait(&response_ready, 0);
```

**Benefits:**
- Eliminates switches when server very fast
- Good for cached reads, stat on pseudo-filesystems

---

### 8. Multiplexed Servers (Reduce Server Count)

**Concept:** One server handles multiple related filesystems.

Instead of:
```
devfs (PID 10)  ← 2 switches
procfs (PID 11) ← 2 switches
sysfs (PID 12)  ← 2 switches
```

Use:
```
sysserver (PID 10) handles /dev, /proc, /sys ← 2 switches total
```

**Benefits:**
- Batch operations across filesystems
- Reduced total switch count

**Trade-offs:**
- Less isolation
- Larger server process

---

### 9. Kernel Threads for Servers (Controversial)

**Concept:** Servers run in kernel mode but in separate address spaces.

```c
// Server registered as kernel thread
int sys_read(int fd) {
    server_t *srv = fd_to_server(fd);

    // Switch address space but stay in kernel mode
    switch_address_space(srv->mm);

    // Direct function call - no context switch!
    result = srv->ops->read(fd, buf, len);

    // Switch back
    switch_address_space(current->mm);
    return result;
}
```

**Benefits:**
- Zero context switches
- L4-style "user servers in kernel mode"

**Trade-offs:**
- More complex fault isolation
- Server bugs can crash kernel
- Loses some microkernel purity

---

### 10. Hardware Assist (x86 VMFUNC)

**Concept:** Use CPU features to switch address spaces without kernel.

```c
// Requires Intel VT-x VMFUNC
// Client switches to server's EPT (address space)
asm volatile("vmfunc" : : "a"(server_eptp), "c"(0));

// Now in server's address space, no kernel involved
result = server_handle_9p(&msg);

// VMFUNC back to client
asm volatile("vmfunc" : : "a"(client_eptp), "c"(0));
```

**Benefits:**
- Address space switch without syscall
- ~100 cycles vs ~1000 for context switch

**Requirements:**
- Modern Intel CPU (Haswell+)
- Complex setup
- Security model carefully designed

---

## Recommended Combination

For Lux9 with 9P2024, use:

1. **Shared memory rings** (zero-copy)
2. **Async 9P with batching** (multiplexing)
3. **Futex-style waits** (skip unnecessary switches)
4. **Fastpath for small messages** (optimize common case)

### Performance Estimate

**Naive implementation:**
- 1000 file reads = 2000 context switches
- ~1000 cycles/switch = 2,000,000 cycles
- @3GHz = ~0.67ms overhead

**With optimizations:**
- 1000 file reads batched (64 at a time) = 32 switches
- Shared memory (zero copy)
- Fastpath for metadata
- Total: ~16,000 cycles = ~0.005ms overhead

**Result: ~130x reduction in context switch overhead**

---

## Implementation Priority

### Phase 1 (MVP):
- [ ] Shared memory rings
- [ ] Basic batching in lib9pclient
- [ ] Futex-style waits

### Phase 2 (Performance):
- [ ] Async 9P2024 support
- [ ] RPC fastpath
- [ ] Polling mode for console

### Phase 3 (Advanced):
- [ ] User-level scheduling
- [ ] Hardware-assisted switching (if available)

---

## References

- L4 microkernel IPC optimizations
- Plan 9 9P protocol
- seL4 fastpath implementation
- Barrelfish OS message passing
- Linux futex implementation

---

## Metrics to Track

```c
struct ipc_stats {
    uint64_t total_switches;
    uint64_t fastpath_hits;
    uint64_t zero_copy_transfers;
    uint64_t batched_operations;
    uint64_t avg_batch_size;
    uint64_t cycles_per_switch;
};
```

Measure before and after each optimization to quantify improvement.
