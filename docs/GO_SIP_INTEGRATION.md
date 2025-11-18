# Go Servers with SIP (Secure Interface Paging)

## Overview

Lux9 userspace servers communicate via **SIP (Secure Interface Paging)** using exchange pages for zero-copy 9P message passing.

**Key Concept**: Instead of using stdin/stdout for 9P messages, Lux9 servers use the `#X/exchange` device.

## Architecture

```
┌─────────────────────────────────────────┐
│ Client Process                           │
│  open("/crypto/hash/sha256", O_RDWR)    │
└────────────────┬────────────────────────┘
                 │ 9P messages
┌────────────────▼────────────────────────┐
│ Kernel (9P Router)                       │
│  Routes to crypto server via SIP        │
└────────────────┬────────────────────────┘
                 │ Exchange pages
┌────────────────▼────────────────────────┐
│ Crypto Server (Go)                       │
│  fd = open("#X/exchange", O_RDWR)       │
│  read(fd) → 9P request                  │
│  write(fd) → 9P response                │
└──────────────────────────────────────────┘
```

## SIP Exchange Device

### Device Path

```
#X/exchange
```

This is the **exchange device** for zero-copy IPC via page exchange.

### Operations

**Open**: Get a file descriptor for 9P communication
```go
fd, err := syscall.Open("#X/exchange", syscall.O_RDWR)
```

**Read**: Receive 9P message from kernel
```go
msg := make([]byte, 8192)
n, err := syscall.Read(fd, msg)
```

**Write**: Send 9P response to kernel
```go
n, err := syscall.Write(fd, response)
```

**Control**: Prepare pages for exchange
```go
cmd := fmt.Sprintf("prepare 0x%x\n", vaddr)
syscall.Write(fd, []byte(cmd))
```

## Go Implementation

### 1. SIP Connection Structure

```go
type SIPConnection struct {
    exchangeFd int    // File descriptor for #X/exchange
    msgBuf     []byte // Buffer for 9P messages (typically 8KB)
}
```

### 2. Opening Exchange Device

```go
func OpenExchangeDevice() (*SIPConnection, error) {
    // Open the exchange device
    fd, err := syscall.Open("#X/exchange", syscall.O_RDWR)
    if err != nil {
        return nil, fmt.Errorf("failed to open exchange device: %v", err)
    }

    return &SIPConnection{
        exchangeFd: fd,
        msgBuf:     make([]byte, 8192),
    }, nil
}
```

### 3. Server Loop

```go
func (c *SIPConnection) Serve9P(handler func([]byte) ([]byte, error)) error {
    for {
        // Receive 9P request from kernel
        reqMsg, err := c.Receive9PMessage()
        if err != nil {
            return err
        }

        // Handle the message
        respMsg, err := handler(reqMsg)
        if err != nil {
            continue
        }

        // Send 9P response back to kernel
        err = c.Send9PMessage(respMsg)
        if err != nil {
            return err
        }
    }
}
```

### 4. Complete Server Example

```go
package main

import (
    "fmt"
    "syscall"
)

func main() {
    fmt.Println("[server] Starting with SIP...")

    // Open SIP exchange device
    sip, err := OpenExchangeDevice()
    if err != nil {
        fmt.Printf("FATAL: Cannot open exchange device: %v\n", err)
        return
    }
    defer sip.Close()

    // Start 9P server loop
    fmt.Println("[server] Listening for 9P messages via SIP...")
    err = sip.Serve9P(handle9PMessage)
    if err != nil {
        fmt.Printf("FATAL: Server error: %v\n", err)
    }
}

func handle9PMessage(req []byte) ([]byte, error) {
    // Parse 9P request
    // Handle based on message type
    // Generate response
    return resp, nil
}
```

## Exchange Page Mechanism

### Concept

**Exchange pages** enable **zero-copy IPC**:
1. Client writes data to a page
2. Page ownership transfers to server
3. Server processes data in-place
4. Page ownership transfers back with result

**Benefits**:
- No memory copying
- Hardware-enforced isolation (MMU)
- Capability-based security

### Usage

```go
// Prepare a page for exchange
vaddr := uintptr(0x10000000)
cmd := fmt.Sprintf("prepare 0x%x\n", vaddr)
syscall.Write(exchangeFd, []byte(cmd))
```

### Status Query

```go
// Read exchange device status
buf := make([]byte, 1024)
n, err := syscall.Read(exchangeFd, buf)
fmt.Printf("Exchange status:\n%s\n", buf[:n])
```

## 9P Message Format

### Message Structure

9P messages consist of:
1. **Size** (4 bytes): Total message size
2. **Type** (1 byte): Message type (Tversion, Tattach, Topen, etc.)
3. **Tag** (2 bytes): Message identifier
4. **Data**: Type-specific data

### Example: Topen Request

```
Size: 13 bytes
Type: 112 (Topen)
Tag: 1
FID: 1
Mode: 0 (OREAD)
```

### Go 9P Libraries

**Option 1**: Use existing library
```go
import "github.com/lionkov/go9p/p"

// Parse message
var fcall p.Fcall
err := fcall.UnmarshalBinary(msg)

// Generate response
resp, err := fcall.MarshalBinary()
```

**Option 2**: Implement minimal 9P parser
```go
// Parse message manually
size := binary.LittleEndian.Uint32(msg[0:4])
msgType := msg[4]
tag := binary.LittleEndian.Uint16(msg[5:7])
data := msg[7:]
```

## Crypto Server with SIP

### File Structure

```
userspace/servers/crypto-go/
├── main.go      # Main server + crypto functions
├── sip.go       # SIP exchange device handling
└── 9p.go        # 9P message parsing (TODO)
```

### main.go

```go
func main() {
    // Test crypto
    testCrypto()

    // Open SIP exchange device
    sip, err := OpenExchangeDevice()
    if err != nil {
        log.Fatal(err)
    }
    defer sip.Close()

    // Start 9P server via SIP
    err = sip.Serve9P(handle9PMessage)
    if err != nil {
        log.Fatal(err)
    }
}
```

### sip.go (Provided)

Handles:
- Opening `#X/exchange`
- Reading/writing 9P messages
- Exchange page preparation
- Status queries

### 9p.go (TODO)

Will handle:
- 9P message parsing
- Fcall structure definitions
- Message type dispatch
- Virtual filesystem structure

## Virtual Filesystem

### Crypto Server Layout

```
/crypto/
├── hash/
│   ├── sha256      # Write data, read hash
│   ├── sha512
│   └── blake2b
├── aead/
│   ├── aes256gcm   # Authenticated encryption
│   └── chacha20poly1305
├── sign/
│   └── ed25519     # Digital signatures
└── rng/
    └── bytes       # Random number generator
```

### File Operations

**Topen**: Open a crypto file
```go
case p.Topen:
    // Return Qid for file (sha256, aes256gcm, etc.)
    // Initialize crypto context
```

**Twrite**: Write data to process
```go
case p.Twrite:
    // Buffer input data
    // For hash: accumulate data
    // For AEAD: parse key|nonce|plaintext
```

**Tread**: Read result
```go
case p.Tread:
    // Execute crypto operation
    // For hash: return SHA-256(data)
    // For AEAD: return ciphertext+tag
```

**Tclunk**: Close file
```go
case p.Tclunk:
    // Clean up crypto context
    // Free buffers
```

## Testing

### Test Exchange Device

```bash
# In Lux9 (when running)
$ cat #X/exchange
<shows exchange status>

$ echo "prepare 0x10000000" > #X/exchange
# Prepares page for exchange
```

### Test Crypto Server

```bash
# Start crypto server
$ /bin/crypto &

# Test hash
$ echo "hello" > /crypto/hash/sha256
$ cat /crypto/hash/sha256
2cf24dba5fb0a30e26e83b2ac5b9e29e1b161e5c1fa7425e73043362938b9824

# Test AES-GCM
$ cat > /crypto/aead/aes256gcm
<key><nonce><plaintext>
^D
$ cat /crypto/aead/aes256gcm
<ciphertext+tag>
```

## Comparison to Traditional 9P

### Traditional 9P (stdin/stdout)

```
Server listens on stdin/stdout
↓
Kernel routes 9P to server via pipe
↓
Messages copied: kernel → userspace → kernel
```

**Problem**: Multiple copies, no zero-copy

### Lux9 SIP (Exchange Pages)

```
Server listens on #X/exchange
↓
Kernel maps exchange page to server
↓
Messages transferred via page ownership
```

**Benefit**: Zero-copy, hardware isolation

## Benefits Recap

### For Server Developers

✅ **Simple API**: Open `#X/exchange`, read/write 9P messages
✅ **Zero-copy**: Exchange pages eliminate copying
✅ **Type-safe**: Go provides safety guarantees
✅ **Hardware isolation**: MMU enforces page ownership

### For System Performance

✅ **Fast IPC**: No memory copying
✅ **Scalable**: Each server has own exchange pages
✅ **Secure**: Capability-based page transfer
✅ **Verifiable**: Software can verify before accepting page

## Next Steps

1. ✅ Implement SIP connection handling (sip.go)
2. ⏳ Implement 9P message parsing (9p.go)
3. ⏳ Define crypto filesystem structure
4. ⏳ Connect crypto functions to 9P handlers
5. ⏳ Test on Lux9 kernel

## Summary

**Lux9 Go servers use SIP exchange pages for 9P communication:**

- Open `#X/exchange` instead of using stdin/stdout
- Read/write 9P messages via exchange device
- Benefit from zero-copy page transfer
- Leverage Go's safety and built-in crypto

**This is the Lux9 way of doing microkernel IPC!**
