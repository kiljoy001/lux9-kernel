# Hurd-9: Plan 9 Reimagined

## Core Philosophy
"Plan 9 with modern performance" - Keep the elegance, fix the pain points.

## Architecture Overview

### What We Keep from Plan 9
- **9P Protocol**: The heart of everything
- **Per-process namespaces**: Each process sees its own file tree
- **Everything is a file**: Networks, graphics, devices - all files
- **Simple design**: No feature creep
- **rfork model**: Fine-grained process creation control

### What We Improve
- **Memory efficiency**: Pebbling for intelligent memory management
- **IPC performance**: Exchange heaps for zero-copy message passing
- **Modern hardware**: Real GPU support, NVMe, 10GbE
- **Multicore**: Proper SMP/NUMA support
- **Speed**: 100x faster through smart algorithms, not complexity

## Core Kernel Structures

### 1. Process Model (Plan 9 Compatible)

```
Process = {
    pid: u32,
    pgrp: u32,                    // Process group
    namespace: Namespace,         // Private namespace
    fds: [Option<Chan>; NFD],    // File descriptors
    memory: ProcessMemory,        // With pebbling optimization
    user: String,                 // User name (not uid!)
    note: Option<String>,         // Pending note (signal)
}

// rfork flags (Plan 9 compatible)
RFPROC   = 0x01  // New process
RFNOWAIT = 0x02  // Don't wait for child
RFNAMEG  = 0x04  // New namespace group
RFCNAMEG = 0x08  // Clean namespace
RFNOMNT  = 0x10  // Disallow mount
RFENVG   = 0x20  // New environment group
RFFDG    = 0x40  // New file descriptor group
RFREND   = 0x80  // Rendezvous group
```

### 2. Namespace Model

```
Namespace = {
    mounts: Vec<Mount>,           // Mount table
    root: Chan,                   // Root directory
    dot: Chan,                    // Current directory
}

Mount = {
    from: Chan,                   // What we're mounting
    to: String,                   // Where we're mounting it
    flags: MountFlags,            // MCREATE, MREPL, MBEFORE, MAFTER
}

// Every process can reshape its view of the file system
bind("/net.alt", "/net", MREPL)  // Replace network stack
bind("#|", "/tmp/pipe", MCREATE) // Create pipe in namespace
```

### 3. Channel (File Descriptor) Model

```
Chan = {
    qid: Qid,                     // Unique file ID
    path: String,                 // Path in namespace
    dev: Arc<Device>,             // Device providing this file
    mode: Mode,                   // Read/write/exec
    offset: u64,                  // Current position
    // Enhancement: exchange heap for zero-copy
    exchange: Option<ExchangeId>, 
}

Qid = {
    path: u64,                    // Unique ID
    vers: u32,                    // Version for cache coherency
    type: QidType,                // QTDIR, QTFILE, etc.
}
```

### 4. Device Model (Plan 9 Drivers)

```
Device = {
    dc: char,                     // Device character (#c for console)
    name: String,                 // Device name
    
    // Device operations (exactly like Plan 9)
    attach: fn(spec: &str) -> Chan,
    walk: fn(c: &Chan, name: &str) -> Chan,
    stat: fn(c: &Chan) -> Dir,
    open: fn(c: &Chan, mode: Mode) -> Chan,
    create: fn(c: &Chan, name: &str, mode: Mode, perm: u32) -> Chan,
    read: fn(c: &Chan, buf: &mut [u8], offset: u64) -> usize,
    write: fn(c: &Chan, buf: &[u8], offset: u64) -> usize,
    close: fn(c: &Chan),
    
    // Enhancement: zero-copy operations
    exchange_read: fn(c: &Chan, exchange: ExchangeId, offset: u64) -> usize,
    exchange_write: fn(c: &Chan, exchange: ExchangeId, offset: u64) -> usize,
}
```

### 5. 9P Server Framework

```
// Any process can serve 9P
FileServer = {
    root: FileTree,               // Files being served
    connections: Vec<Connection>, // Active 9P connections
    
    // Enhancement: async 9P operations
    async_handler: AsyncHandler,
}

// Easy to write file servers
impl FileServer {
    fn serve_net(&mut self, addr: &str) { ... }
    fn serve_pipe(&mut self, pipe: Chan) { ... }
}
```

### 6. Memory Management (Enhanced)

```
ProcessMemory = {
    segments: Vec<Segment>,       // Text, data, bss, stack
    
    // Plan 9 style
    seg_brk: usize,              // Break (heap end)
    seg_stack: usize,            // Stack base
    
    // Enhancements
    pebbling: PebblingState,     // For memory pressure
    exchanges: Vec<Exchange>,    // Zero-copy IPC regions
}

// Smart memory under pressure using pebbling
PebblingState = {
    dependency_tree: Tree<Page>, // Which pages depend on which
    pebbles: Set<Page>,          // Minimal set to keep in RAM
}
```

### 7. Authentication (Factotum Compatible)

```
// Plan 9's factotum - authentication agent
Factotum = {
    keys: Vec<Key>,              // Stored keys
    protocols: Vec<Protocol>,    // p9sk1, dp9ik, etc.
}

// Each process has an auth file descriptor
Process.auth_fd -> /mnt/factotum/rpc

// Authentication is just file I/O
auth_fd.write("proto=p9sk1 dom=9grid user=glenda")
auth_fd.read() -> "ok"
```

## Key Improvements Over Original Plan 9

### 1. Performance
- **Exchange heaps**: Zero-copy between processes
- **Pebbling**: 10x memory efficiency under pressure
- **Parallel 9P**: Multiple outstanding requests
- **GPU acceleration**: Modern graphics, not just CPU drawing

### 2. Scalability
- **Real SMP**: Not just "run one process per CPU"
- **NUMA aware**: Memory locality optimization
- **Lock-free structures**: Where possible

### 3. Modern Hardware
- **NVMe**: Direct queue support
- **10GbE/40GbE**: Zero-copy networking
- **USB 3.0**: Not just "here's a file"
- **GPU compute**: OpenCL/Vulkan through 9P

### 4. Compatibility
- **Run Plan 9 binaries**: Directly, no changes
- **Source compatible**: Compile Plan 9 C code
- **Protocol compatible**: Talk to Plan 9 systems
- **Namespace compatible**: Same bind/mount semantics

## Implementation Priority

### Phase 1: Core (What we need for Coq proofs)
1. Process model with rfork
2. Namespace operations (bind, mount)
3. Basic 9P protocol
4. Memory management with pebbling
5. Exchange heaps for IPC

### Phase 2: Compatibility
1. Plan 9 binary loader
2. System call compatibility layer
3. Factotum authentication
4. /proc filesystem
5. rc shell

### Phase 3: Enhancements
1. GPU device (#G)
2. Network acceleration
3. Async 9P operations
4. Advanced pebbling strategies

## Why Plan 9 Users Will Love This

1. **It's still Plan 9**: Same concepts, same elegance
2. **But fast**: 100x performance improvements
3. **Modern hardware**: Use your GPU, NVMe, etc.
4. **Real applications**: Can run modern software through compatibility
5. **Active development**: Not abandoned like Plan 9

## Example: A File Server in Hurd-9

```rust
// Just like Plan 9, but in Rust
struct MyFS;

impl Device for MyFS {
    fn read(&self, _c: &Chan, buf: &mut [u8], offset: u64) -> usize {
        let msg = b"Hello from Hurd-9!\n";
        buf[..msg.len()].copy_from_slice(msg);
        msg.len()
    }
}

fn main() {
    let fs = MyFS;
    fs.serve_net("tcp!*!564");  // Serve on port 564
}
```

## The Pitch

"Imagine Plan 9 that can run on your modern laptop, uses all your cores efficiently, manages memory intelligently, and runs 100x faster while keeping all the elegance. That's Hurd-9."