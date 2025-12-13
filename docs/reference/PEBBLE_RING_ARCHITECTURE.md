# Pebble-Ring Architecture
## Overview
This document outlines the architectural transition from lock-based resource management to a lock-free, capability-based messaging system ("Pebble-Ring").

**Goal:** Eliminate deadlocks (like the `imagealloc` retry loop) and improve scalability by replacing shared state locks with asynchronous request queues.

## Core Concepts

### 1. The Manager (Server)
Instead of multiple threads acquiring a lock to manipulate a shared structure (e.g., `struct Imagealloc`), a single **Manager Thread** owns the resource exclusively.
*   **State:** Owned solely by the Manager. No mutexes required.
*   **Loop:** The Manager spins (or sleeps) on a `PebbleRing` input queue.

### 2. The Ring (Transport)
A lock-free Single-Producer/Single-Consumer (SPSC) or Multi-Producer/Single-Consumer (MPSC) ring buffer.
*   **Input:** Clients write `PebbleRequest` messages.
*   **Backpressure:** If the ring is full, the Client handles it (waits or fails), eliminating kernel-side retry loops that hold locks.

### 3. Pebble Capabilities (Authorization)
Access to the Ring is controlled by Pebble Tokens.
*   **White Token:** "Promise" or "Budget" to make a request.
*   **Black Handle:** The allocated resource returned by the Manager.

## Architecture Diagram

```
[Client Process A]      [Client Process B]
       |                       |
       | (1) Issue White       |
       v                       v
   [PebbleRing (MPSC)] <---- (Write Request)
           |
           | (2) Consumes Request
           v
    [Manager Thread] ----> [Private Resource State]
           |               (e.g., Image Allocator)
           |
           | (3) Writes Response/Handle
           v
   [Response Channel]
```

## Implementation Strategy: Image Allocator Pilot

We will migrate the `attachimage` subsystem first, as it is the source of known deadlocks.

### Phase 1: The Request Structure
```c
typedef struct ImageReq {
    PebbleWhite *token;   // Authorization
    Chan *c;              // The channel to attach
    ulong pages;          // Size requested
    Rendez *reply;        // Where to wake the client
    Image **result;       // Where to put the pointer
} ImageReq;
```

### Phase 2: The Ring Buffer
Reuse `ringbuffer/src/ringbuf.c` logic but adapted for kernel:
*   **Kernel Ring:** Fixed-size circular buffer in non-paged memory.
*   **Operations:** `ring_write(ring, &req)` and `ring_read(ring, &req)`.

### Phase 3: The Image Manager
A new kernel process (`kproc`) that runs `image_manager_loop()`:
```c
void image_manager_loop(void) {
    ImageReq req;
    while(1) {
        if(ring_read(&image_ring, &req)) {
            // We are now single-threaded owner of imagealloc!
            // No lock(&imagealloc) needed here.
            Image *img = alloc_image_logic(req.c, req.pages);
            
            *req.result = img;
            wakeup(req.reply);
        } else {
            sleep(&ring_nonzero); // Wait for work
        }
    }
}
```

### Phase 4: Client Refactor
The `attachimage` function becomes:
```c
Image* attachimage(Chan *c, ulong pages) {
    // 1. Validate budget/token
    // 2. Enqueue request
    // 3. Sleep on reply (replaces the lock contention)
}
```

## Advantages
1.  **No Deadlocks:** The Client waits on a *response*, not a *lock*. The Manager processes requests serially. If memory is low, the Manager triggers reclamation logic safely without fighting clients for locks.
2.  **Scalability:** Ring buffers significantly reduce cache line bouncing compared to spinlocks.
3.  **Pebble Integration:** Natural fit for capability passing.

## Migration Plan
1.  **Port RingBuffer**: Ensure `ringbuffer/` code compiles in kernel.
2.  **Create `kproc`**: Start the `ImageManager` at boot.
3.  **Switch**: Redirect `attachimage` calls to the ring.
