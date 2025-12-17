# Hurd-9 Kernel Core Structures Analysis

## What We Have So Far ✓
- Memory management (pebbling, volatile, exchange heap)
- IPC (holographic channels) 
- Consensus (GHOSTDAG integration)

## Critical Missing Structures

### 1. **Process/SIP Management**
```rust
// Core SIP (Software Isolated Process) structure
pub struct SIP {
    id: SipId,
    go_runtime: GoRuntimeState,
    memory_space: AddressSpace,
    capabilities: CapabilitySet,
    ipc_channels: Vec<ChannelHandle>,
    state: SipState,
}

pub enum SipState {
    Starting,
    Running,
    Blocked(BlockReason),
    Migrating,
    Terminated,
}
```

### 2. **Scheduler & Threading**
```rust
// Cooperative scheduler for Go SIPs
pub struct Scheduler {
    runnable: VecDeque<SipId>,
    waiting: BTreeMap<SipId, WaitCondition>,
    current: Option<SipId>,
    quantum_ms: u64,
}

// Go goroutine management
pub struct GoRuntimeState {
    goroutines: Vec<Goroutine>,
    scheduler_state: GoSchedulerState,
    gc_state: GCState,
    stack_pool: StackPool,
}
```

### 3. **Capability System**
```rust
// Plan 9 style capabilities
pub struct Capability {
    namespace: String,      // e.g., "/net/tcp"
    permissions: Permissions,
    delegatable: bool,
}

pub struct CapabilitySet {
    caps: BTreeSet<Capability>,
    parent: Option<SipId>,  // Capability inheritance
}
```

### 4. **9P Protocol Handler**
```rust
// Enhanced 9P with async and multiplexing
pub struct Protocol9P {
    version: ProtocolVersion,
    mux_support: bool,
    async_support: bool,
    message_queue: VecDeque<P9Message>,
}

pub enum P9Message {
    Tversion { msize: u32, version: String },
    Rversion { msize: u32, version: String },
    // ... other 9P messages
    // Enhanced messages
    TAsyncRead { tag: u16, fid: u32, offset: u64, count: u32 },
    RAsyncRead { tag: u16, data: Vec<u8> },
}
```

### 5. **Filesystem/Translator Framework**
```rust
// Hurd-style translators
pub trait Translator {
    fn translate(&self, path: &str) -> Result<VirtualFile, Error>;
    fn supports_async(&self) -> bool;
    fn capabilities_required(&self) -> CapabilitySet;
}

pub struct TranslatorRegistry {
    translators: BTreeMap<String, Box<dyn Translator>>,
    mount_points: BTreeMap<String, SipId>,
}
```

### 6. **Device Driver Interface**
```rust
// VirtIO and rump kernel integration
pub struct DeviceManager {
    virtio_devices: Vec<VirtIODevice>,
    rump_kernels: Vec<RumpKernel>,
    driver_sips: BTreeMap<DeviceId, SipId>,
}

pub enum VirtIODevice {
    Network(VirtIONet),
    Block(VirtIOBlock),
    Console(VirtIOConsole),
}
```

### 7. **Interrupt Handling**
```rust
// Interrupt routing to SIPs
pub struct InterruptController {
    handlers: BTreeMap<u8, SipId>,
    pending: VecDeque<Interrupt>,
    mask: u64,
}

pub struct Interrupt {
    vector: u8,
    data: Option<u64>,
    timestamp: u64,
}
```

### 8. **System Call Interface**
```rust
// Minimal syscall interface for Go runtime
pub enum Syscall {
    // Memory
    Mmap { addr: usize, len: usize, prot: u32 },
    Munmap { addr: usize, len: usize },
    
    // IPC
    ChannelCreate { name: String },
    ChannelSend { id: ChannelId, data: Vec<u8> },
    ChannelRecv { id: ChannelId },
    
    // 9P
    P9Open { path: String },
    P9Read { fd: u32, count: usize },
    P9Write { fd: u32, data: Vec<u8> },
    
    // Process
    SipSpawn { binary: Vec<u8>, args: Vec<String> },
    SipWait { id: SipId },
    
    // Time
    Sleep { duration_ms: u64 },
    GetTime,
}
```

### 9. **Boot & Initialization**
```rust
// Limine bootloader integration
pub struct BootInfo {
    memory_map: MemoryMap,
    modules: Vec<Module>,
    framebuffer: Option<Framebuffer>,
    rsdp: Option<u64>,  // ACPI
}

pub struct InitSystem {
    stage: InitStage,
    essential_sips: Vec<SipId>,
    boot_modules: Vec<Module>,
}

pub enum InitStage {
    EarlyBoot,
    MemoryInit,
    DeviceInit,
    SipInit,
    UserSpace,
}
```

### 10. **Error Handling & Panic**
```rust
// Kernel panic and error recovery
pub struct PanicHandler {
    stack_trace: Vec<usize>,
    error_log: Vec<KernelError>,
    recovery_possible: bool,
}

pub enum KernelError {
    OutOfMemory,
    InvalidSip(SipId),
    CapabilityViolation { sip: SipId, attempted: String },
    DeviceError { device: DeviceId, error: String },
    CorruptedState,
}
```

## Key Design Questions

### 1. **Go Runtime Integration**
- How do we bootstrap Go without libc?
- Where does Go's runtime allocator fit with our memory management?
- How do goroutines interact with our scheduler?

### 2. **SIP Isolation**
- How strong is the isolation? (process-level vs VM-level)
- Can SIPs share any memory directly?
- How do we verify SIP binaries?

### 3. **9P Protocol Extensions**
- Async operations: how do we handle partial reads/writes?
- Multiplexing: can one connection handle multiple operations?
- Security: how do we add authentication to 9P?

### 4. **Capability Model**
- How granular should capabilities be?
- Can capabilities be revoked dynamically?
- How do we handle capability delegation chains?

### 5. **Error Recovery**
- What happens when a SIP crashes?
- Can we recover from kernel panics?
- How do we handle hardware failures?

## Critical Paths

### Short Term (needed for basic kernel):
1. SIP process structure
2. Basic scheduler 
3. Syscall interface
4. Memory management integration

### Medium Term (needed for functionality):
1. 9P protocol handler
2. Capability system
3. Device drivers
4. Go runtime integration

### Long Term (optimization and features):
1. Advanced IPC
2. Consensus integration
3. Migration support
4. Performance tuning