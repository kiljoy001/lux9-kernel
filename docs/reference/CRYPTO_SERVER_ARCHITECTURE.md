# Crypto Server Architecture (Corrected for Lux9 Microkernel)

## Overview
The crypto server is a **userspace 9P server** that provides cryptographic operations via a filesystem interface. This follows Lux9's microkernel architecture where all services run in userspace.

## Architecture

### **Lux9 Microkernel Design**
- **Kernel**: Minimal - IPC (via 9P), memory management, scheduling
- **Services**: Userspace processes that serve filesystems via 9P
- **Communication**: All IPC happens through 9P protocol messages

### **Crypto Server Components**

```
userspace/servers/crypto/
├── sha256.c          # Supercop-based SHA256 implementation
├── random.c          # CSPRNG implementation
├── crypto_lib.h      # Crypto primitives library
├── cryptosrv.c       # Main 9P server
└── Makefile          # Build configuration
```

## 9P Filesystem Interface

```
/crypto/
  ├── hash/
  │   └── sha256      (write data, read hash)
  └── random/
      └── bytes       (read random bytes)
```

### **Usage Pattern**
```bash
# Mount crypto server
mount /srv/cryptosrv /crypto

# Hash data
echo "hello world" > /crypto/hash/sha256
cat /crypto/hash/sha256
# Output: b94d27b9934d3e08a52e52d7da7dabfac484efe37a5380ee9088f7ace2efcde9

# Get random bytes
dd if=/crypto/random/bytes of=random.dat bs=32 count=1
```

## Implementation Plan

### **Phase 1: Core Infrastructure** ✅
- [x] Design architecture
- [x] Remove incorrect kernel device driver
- [x] Create userspace server directory structure

### **Phase 2: Crypto Library** (Next)
- [ ] Port Supercop SHA256 to userspace
- [ ] Implement CSPRNG for random bytes
- [ ] Create crypto library interface

### **Phase 3: 9P Server** (Next)
- [ ] Implement 9P protocol handlers
- [ ] Implement file operations (walk, open, read, write, clunk)
- [ ] Integrate crypto library

### **Phase 4: Advanced Features** (Future)
- [ ] Add AES-256-GCM encryption
- [ ] Add Ed25519 signing
- [ ] Add ChaCha20-Poly1305
- [ ] Resource accounting integration

## 9P Protocol Implementation

### **Message Handlers Required**
- `Tversion/Rversion` - Protocol negotiation
- `Tattach/Rattach` - Attach to filesystem root
- `Twalk/Rwalk` - Navigate directory tree
- `Topen/Ropen` - Open files
- `Tread/Rread` - Read file contents (hash output, random bytes)
- `Twrite/Rwrite` - Write data (hash input)
- `Tclunk/Rclunk` - Close files
- `Tstat/Rstat` - Get file metadata

### **File Operations**

#### **SHA256 Hash**
1. **Open** `/crypto/hash/sha256` - Initialize SHA256 context
2. **Write** - Stream data to hash
3. **Read** - Finalize and return hex-encoded hash
4. **Clunk** - Clean up context

#### **Random Bytes**
1. **Open** `/crypto/random/bytes` (read-only)
2. **Read** - Return random bytes
3. **Clunk** - Clean up

## Security Model

### **Process Isolation**
- Each client connection has isolated crypto contexts
- No shared state between processes
- Memory is process-local

### **Resource Accounting** (Future)
- Track compute time per operation
- Integrate with pebble system for resource limits
- Prevent DoS via excessive crypto operations

## Communication Flow

```
Client Process          Kernel (9P IPC)         Crypto Server
     |                         |                      |
     |--[write /crypto/hash/sha256]------------------>|
     |                         |                      |
     |                         |    [Twrite msg]      |
     |                         |--------------------->|
     |                         |                      | (Update SHA256)
     |                         |    [Rwrite ack]      |
     |                         |<---------------------|
     |<-[write returns]--------|                      |
     |                         |                      |
     |--[read /crypto/hash/sha256]------------------->|
     |                         |                      |
     |                         |    [Tread msg]       |
     |                         |--------------------->|
     |                         |                      | (Finalize hash)
     |                         |    [Rread data]      |
     |                         |<---------------------|
     |<-[hash data]------------|                      |
```

## Differences from Original Kernel Approach

### **What Was Wrong**
❌ `kernel/9front-port/devcrypto.c` - Kernel device driver
❌ `kernel/crypto/` - Crypto code in kernel
❌ Direct kernel implementation violates microkernel design

### **What Is Correct**
✅ `userspace/servers/crypto/` - Userspace server
✅ 9P protocol for all communication
✅ Crypto runs in isolated userspace process
✅ Follows Plan 9 philosophy: "everything is a filesystem"

## References
- **9P Protocol**: userspace/include/9p.h
- **Example Server**: userspace/servers/ext4fs/
- **Supercop Crypto**: crypto-standards/supercop/
- **Original Design**: docs/CRYPTO_SERVER_DESIGN.md

## Next Steps

1. Implement SHA256 library in userspace
2. Create 9P server skeleton
3. Implement hash file operations
4. Add random bytes support
5. Test with client applications
6. Add encryption and signing

**Status**: Infrastructure setup complete, ready for implementation.
