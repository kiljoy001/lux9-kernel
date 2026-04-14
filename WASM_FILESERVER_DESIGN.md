# Simplified WASM Fileserver Architecture
## Design Based on Resurrection Server Monitoring

### Executive Summary

This document presents a simplified WASM fileserver architecture that leverages the existing **resurrection server infrastructure** for process monitoring and lifecycle management. Instead of complex IPC mechanisms, we use **standard 9P protocol** throughout, with the resurrection server providing automatic restart, health monitoring, and reliability.

**Key Insight**: The resurrection server can monitor a simple 9P WASM fileserver process, providing all the reliability and lifecycle management we need without complex coordination mechanisms.

---

## Architecture Overview

```
┌─────────────────────────────────────────────────────────────────┐
│                        Client Processes                         │
│                     (WASM Applications)                         │
└────────────────────────┬────────────────────────────────────────┘
                         │ Standard 9P Protocol
                         │ /wasm namespace operations
                         ▼
┌─────────────────────────────────────────────────────────────────┐
│              Simple 9P WASM Fileserver Process                   │
│  ┌─────────────────────────────────────────────────────────┐   │
│  │ • Standard 9P protocol implementation                   │   │
│  │ • WASM module compilation (.wasm → compiled cache)      │   │
│  │ • File operations (read, write, stat, walk)            │   │
│  │ • WASM execution through file operations                │   │
│  │ • NO custom IPC - only 9P messages                     │   │
│  └─────────────────────────────────────────────────────────┘   │
└────────────────────────┬────────────────────────────────────────┘
                         │ Monitored by
                         ▼
┌─────────────────────────────────────────────────────────────────┐
│                   Resurrection Server                           │
│  ┌─────────────────────────────────────────────────────────┐   │
│  │ • Process spawn and lifecycle management               │   │
│  │ • Health monitoring and automatic restart              │   │
│  │ • Resource allocation and limits                       │   │
│  │ • Binary hash verification                             │   │
│  │ • Rate limiting and restart policies                  │   │
│  └─────────────────────────────────────────────────────────┘   │
└────────────────────────┬────────────────────────────────────────┘
                         │ 9P /srv interface
                         ▼
┌─────────────────────────────────────────────────────────────────┐
│                       Kernel 9P Stack                           │
│  • Standard Plan 9 9P2000 protocol                            │
│  • File system mounting and operations                         │
│  • Process management and IPC                                  │
└─────────────────────────────────────────────────────────────────┘
```

---

## Design Principles

### 1. **Simplicity First**
- **Standard 9P Protocol**: No custom protocol extensions or complex IPC
- **Existing Infrastructure**: Leverage resurrection server for monitoring
- **Clean Separation**: 9P file server handles file ops, WASM runtime handles compilation/execution

### 2. **Resurrection Server Integration**
- **Automatic Restart**: Monitored process with crash recovery
- **Health Monitoring**: Process state tracking and restart policies
- **Binary Verification**: SHA256 hash verification of WASM modules
- **Resource Management**: Process limits and capability isolation

### 3. **Clean Boundaries**
- **9P Server**: Handles file operations, path resolution, WASM module loading
- **WASM Runtime**: Compiles .wasm files, manages execution contexts
- **No Cross-Component Complexity**: Communication only through standard file operations

### 4. **Reliability**
- **Automatic Recovery**: Resurrection server restarts crashed fileservers
- **State Recovery**: Restart from known-good WASM module state
- **Health Monitoring**: Continuous process health checks

---

## Core Components

### 1. Simple 9P WASM Fileserver Process

**Responsibilities:**
- Implement standard 9P protocol for `/wasm` namespace
- Handle file operations: read, write, stat, walk, open, create
- Manage WASM module compilation and caching
- Execute WASM modules through file operations
- NO custom IPC or complex coordination

**File Types Exposed:**
```
/wasm/                          # Root directory
/wasm/modules/                  # Available WASM modules
/wasm/modules/hello.wasm        # Raw WASM bytecode
/wasm/modules/hello.compiled    # Pre-compiled/cache files
/wasm/exec/                     # Execution namespace
/wasm/exec/hello                # Execute module (read returns result)
/wasm/compile/                  # Compilation namespace  
/wasm/compile/request            # Write .wasm data to compile
/wasm/compile/status            # Read compilation status
/wasm/cache/                    # Compilation cache
/wasm/cache/<module_hash>       # Cached compiled modules
```

**Key Operations:**
- **Tread**: Read WASM module content, execution results, compilation status
- **Twrite**: Write WASM bytecode for compilation, configuration
- **Tcreate**: Create new WASM modules or execution instances
- **Tstat**: Get module metadata, compilation status, execution stats
- **Twalk**: Navigate WASM namespace hierarchy

### 2. Resurrection Server Integration

**Service Registration:**
```c
// Register WASM fileserver with resurrection server
register_service("wasm-fileserver", 
                "/boot/wasm_fileserver",
                wasm_module_hash,  // SHA256 of known-good binary
                1);               // Critical service
```

**Monitoring Capabilities:**
- **Process Health**: Monitor PID, check if running
- **Automatic Restart**: Restart on crash with rate limiting
- **Binary Verification**: Ensure only verified WASM modules are served
- **Resource Limits**: Memory, CPU, execution time limits

**Service Configuration:**
```c
// Configuration via 9P write to /srv/wasm-fileserver
write(fd, "exec=/boot/wasm_fileserver", 27);
write(fd, "max_memory=64M", 15);
write(fd, "timeout=30s", 11);
```

### 3. Communication Protocol

**Standard 9P Messages:**
- **Tattach**: Mount `/wasm` namespace with capability
- **Twalk**: Navigate WASM directory structure
- **Topen/Tcreate**: Open WASM modules or execution handles
- **Tread/Twrite**: Read module content, write compilation requests
- **Tclunk**: Release file handles

**No Custom Extensions:**
- All operations use standard 9P message types
- WASM-specific semantics encoded in file paths and data
- Capability-based security through standard 9P attach

### 4. WASM Integration Strategy

**Module Compilation:**
```
Client writes .wasm data → /wasm/compile/request
Fileserver compiles → cache in /wasm/cache/<hash>
Client reads status → /wasm/compile/status
```

**Module Execution:**
```
Client opens → /wasm/exec/hello
Fileserver creates execution context
Client reads → gets execution result
Fileserver cleans up context
```

**Compilation Cache:**
- Hash-based caching of compiled WASM modules
- Invalidated on module modification
- Shared across execution instances

---

## Implementation Details

### Process Lifecycle

```
1. Init starts resurrection server
   ↓
2. Resurrection server starts WASM fileserver
   ↓
3. WASM fileserver mounts /wasm via 9P
   ↓
4. Clients mount /wasm namespace
   ↓
5. Resurrection monitors fileserver health
   ↓
6. On crash → automatic restart from known-good state
```

### WASM File Types and Operations

#### 1. Module Files (`/wasm/modules/*.wasm`)
```
Tread: Return raw WASM bytecode
Twrite: Update module content (requires write capability)
Tstat: Module metadata (size, hash, compilation status)
```

#### 2. Compilation Cache (`/wasm/cache/<hash>`)
```
Tread: Return compiled module data
Tcreate: Pre-compile and cache module
Tstat: Cache status (valid, expired, size)
```

#### 3. Execution Handles (`/wasm/exec/<name>`)
```
Tcreate: Create execution context
Tread: Read execution result/output
Twrite: Write input data for execution
Tclunk: Clean up execution context
```

#### 4. Compilation Interface (`/wasm/compile/*`)
```
Twrite: Submit .wasm data for compilation
Tread: Check compilation status/result
Tstat: Compilation queue/status
```

### Security Model

**Capability-Based Access:**
- Clients must have capability to access `/wasm` namespace
- Different capability levels for read vs write vs execute
- Resurrection server enforces capability isolation

**Process Isolation:**
- WASM fileserver runs in isolated process
- Cannot affect resurrection server or other services
- Resource limits enforced by kernel

**Binary Verification:**
- WASM fileserver binary hash verified on restart
- Only known-good versions can be started
- Prevents compromised restart attacks

### Error Handling and Recovery

**Fileserver Crash:**
1. Resurrection server detects crash via `sys_wait()`
2. Marks service as `SRV_CRASHED`
3. Restarts from verified binary if `auto_restart` enabled
4. Rate limiting prevents restart loops

**WASM Execution Failure:**
1. Fileserver catches WASM runtime errors
2. Returns error through 9P response
3. Cleans up execution context
4. Client can retry with different input

**Network/Client Disconnect:**
1. Standard 9P fid cleanup
2. Fileserver handles `Tclunk` messages
3. Resources cleaned up properly

---

## Key Design Questions Answered

### 1. How does the resurrection server monitor the WASM fileserver?

**Answer:** Using the existing resurrection server infrastructure:
- **Process Monitoring**: Tracks fileserver PID, monitors via `sys_wait()`
- **Health Checks**: Periodic status verification through 9P interface
- **Automatic Restart**: Restart on crash with rate limiting
- **Binary Verification**: SHA256 hash verification on every start

### 2. What WASM-specific file types should the 9P server expose?

**Answer:** Simple file hierarchy:
- `/wasm/modules/` - Raw WASM bytecode files
- `/wasm/cache/` - Compiled module cache
- `/wasm/exec/` - Execution handles and results
- `/wasm/compile/` - Compilation interface

### 3. How are WASM compilation and execution mapped to 9P operations?

**Answer:** Direct mapping:
- **Compilation**: `Twrite` → `/wasm/compile/request` → compilation → `Tread` → `/wasm/compile/status`
- **Execution**: `Tcreate` → `/wasm/exec/name` → execution context → `Tread` → results → `Tclunk` → cleanup
- **Module Access**: `Tread` → `/wasm/modules/module.wasm` → raw bytes

### 4. What happens if the WASM fileserver crashes?

**Answer:** Automatic recovery via resurrection server:
- **Detection**: `sys_wait()` detects child termination
- **Restart**: Automatic restart from verified binary
- **Rate Limiting**: Prevent restart loops (max 5 restarts per minute)
- **State Recovery**: Restart to clean, known-good state

### 5. How are capabilities and permissions managed?

**Answer:** Standard 9P + capability system:
- **Attach**: Clients present capability UUID during `Tattach`
- **Validation**: Kernel validates capabilities before routing to fileserver
- **Operations**: Different capability levels for read/write/execute
- **Isolation**: Fileserver process cannot access resurrection server capabilities

---

## Advantages of This Approach

### 1. **Simplicity**
- **No Complex IPC**: Standard 9P protocol throughout
- **Existing Infrastructure**: Leverage resurrection server capabilities
- **Clean Code**: Simple 9P server implementation
- **Standard Patterns**: Follows Plan 9 file server conventions

### 2. **Reliability**
- **Automatic Recovery**: Resurrection server handles crash recovery
- **Health Monitoring**: Continuous process health tracking
- **Binary Verification**: Prevent compromised restarts
- **Rate Limiting**: Avoid restart loops

### 3. **Maintainability**
- **Standard Tools**: Use existing 9P debugging/monitoring tools
- **Familiar Patterns**: Plan 9 developers understand this model
- **Isolated Components**: Clear boundaries between components
- **Simple Testing**: Test each component independently

### 4. **Security**
- **Capability Isolation**: Standard Plan 9 capability system
- **Process Isolation**: Fileserver runs in separate process
- **Binary Verification**: Prevent compromised binaries
- **Resource Limits**: Kernel-enforced resource constraints

### 5. **Performance**
- **Efficient IPC**: 9P is optimized for file operations
- **Minimal Overhead**: No complex coordination protocols
- **Direct Mapping**: WASM operations map naturally to file operations
- **Caching**: Compilation cache for performance

---

## Implementation Strategy

### Phase 1: Basic 9P Fileserver
1. Implement simple 9P server for `/wasm` namespace
2. Handle basic file operations (read, write, stat, walk)
3. Mount as standard 9P file system
4. Test with resurrection server integration

### Phase 2: WASM Integration
1. Add WASM runtime integration (wasm3 or similar)
2. Implement module compilation
3. Add execution interface
4. Test compilation and execution flows

### Phase 3: Advanced Features
1. Compilation caching
2. Performance optimization
3. Monitoring and metrics
4. Security hardening

### Phase 4: Production Ready
1. Comprehensive testing
2. Performance tuning
3. Documentation and examples
4. Production deployment

---

## Example Usage

### Client Application
```c
// Mount WASM namespace
int fd = sys_open("/wasm", OREAD);

// Read available modules
char modules[1024];
sys_read(fd, modules, sizeof(modules));
// Returns: "modules/\nexec/\ncompile/\n"

// Compile new module
int compile_fd = sys_open("/wasm/compile/request", OWRITE);
sys_write(compile_fd, wasm_bytes, wasm_size);

// Check compilation status
sys_seek(compile_fd, 0, 0);
char status[256];
sys_read(compile_fd, status, sizeof(status));

// Execute module
int exec_fd = sys_open("/wasm/exec/hello", OREAD);
char result[1024];
sys_read(exec_fd, result, sizeof(result));

sys_close(compile_fd);
sys_close(exec_fd);
sys_close(fd);
```

### Resurrection Server Configuration
```c
// Register WASM fileserver service
register_service("wasm-fileserver",
                "/boot/wasm_fileserver", 
                wasm_binary_hash,  // SHA256
                1);               // Critical service

// Service auto-starts and monitored
// Crash → automatic restart from verified binary
// Health monitoring → continuous supervision
```

---

## Comparison with Complex Approaches

| Aspect | Simplified Approach | Complex IPC Approach |
|--------|-------------------|---------------------|
| **Complexity** | Low - Standard 9P | High - Custom protocols |
| **Code Size** | Small - ~1000 lines | Large - ~5000+ lines |
| **Reliability** | High - Resurrection server | Medium - Complex coordination |
| **Debugging** | Easy - Standard tools | Hard - Custom mechanisms |
| **Maintenance** | Simple - Plan 9 patterns | Difficult - Complex interactions |
| **Performance** | Good - Direct file ops | Variable - IPC overhead |

---

## Conclusion

This simplified WASM fileserver architecture demonstrates that **complexity is often unnecessary**. By leveraging existing Plan 9 infrastructure (9P protocol + resurrection server), we achieve a robust, maintainable, and performant solution without custom IPC mechanisms or complex coordination protocols.

The key insight is that **file operations are sufficient** for WASM module management and execution. By treating WASM compilation and execution as file operations, we get:

- **Reliability**: Automatic restart and health monitoring
- **Simplicity**: Standard 9P protocol throughout  
- **Maintainability**: Clean separation of concerns
- **Security**: Capability-based access control
- **Performance**: Efficient file-based interface

This approach exemplifies the Plan 9 philosophy: **simplicity and clarity over complexity and features**.

