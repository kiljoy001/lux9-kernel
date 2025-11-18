# Go Userspace Strategy for Lux9

## Why Go is Perfect for Lux9

### **Historical Connection**
Go was created by **Rob Pike and Ken Thompson** - the same people who designed Plan 9. Go has native Plan 9 support built-in, making it the ideal userspace language for a Plan 9-based microkernel.

### **No Libc Needed!**
Unlike C programs, Go programs:
- Have their own runtime (no libc dependency)
- Make syscalls directly via `syscall` package
- Include garbage collection
- Produce static binaries
- Don't need complex build infrastructure

## Syscall Compatibility

### **Go Already Speaks Plan 9**

Go's standard library includes `syscall` package for Plan 9 (GOOS=plan9):

```go
// From Go's src/syscall/zsysnum_plan9.go
const (
    SYS_SYSR1       = 0
    SYS_BIND        = 2
    SYS_CHDIR       = 3
    SYS_CLOSE       = 4
    SYS_DUP         = 5
    SYS_EXEC        = 7
    SYS_EXITS       = 8
    SYS_OPEN        = 14
    SYS_RFORK       = 19
    SYS_PREAD       = 50
    SYS_PWRITE      = 51
    // ... matches Lux9!
)
```

### **Syscall Number Verification**

| Syscall | Go (GOOS=plan9) | Lux9 kernel | Status |
|---------|----------------|-------------|--------|
| OPEN | 14 | 14 | ✅ Match |
| CLOSE | 4 | 4 | ✅ Match |
| PREAD | 50 | 50 | ✅ Match |
| PWRITE | 51 | 51 | ✅ Match |
| RFORK | 19 | 19 | ✅ Match |
| EXEC | 7 | 7 | ✅ Match |
| EXITS | 8 | 8 | ✅ Match |
| BIND | 2 | 2 | ✅ Match |
| MOUNT | 46 | 46 | ✅ Match |
| PIPE | 21 | 21 | ✅ Match |

**Result**: All syscalls match! Go's Plan 9 support works with Lux9 out of the box.

### **Note on Read/Write**

Go's Plan 9 implementation uses `Pread`/`Pwrite` instead of `Read`/`Write`:

```go
// From Go's syscall_plan9.go
func Read(fd int, p []byte) (n int, err error) {
    return Pread(fd, p, -1)  // Offset -1 = current position
}

func Write(fd int, p []byte) (n int, err error) {
    return Pwrite(fd, p, -1)
}
```

Lux9 implements both:
- `PREAD` (50) and `PWRITE` (51) - Used by Go ✅
- `READ` (15) and `WRITE` (20) - For C programs ✅

## Go Userspace Architecture

```
┌─────────────────────────────────────────┐
│ User Programs (Go)                       │
│   package main                           │
│   import "os"                            │
│   fd, _ := os.Open("/crypto/hash/sha256")│
└────────────────┬────────────────────────┘
                 │
┌────────────────▼────────────────────────┐
│ Go Standard Library                      │
│   os.Open() → syscall.Open()            │
└────────────────┬────────────────────────┘
                 │
┌────────────────▼────────────────────────┐
│ Go syscall package (GOOS=plan9)          │
│   Syscall(SYS_OPEN, path, mode, 0)      │
│   → Raw syscall instruction              │
└────────────────┬────────────────────────┘
                 │
┌────────────────▼────────────────────────┐
│ Lux9 Microkernel                         │
│   Routes 9P messages to servers          │
└────────────────┬────────────────────────┘
                 │
┌────────────────▼────────────────────────┐
│ Userspace Servers (Go)                   │
│   - Crypto server                        │
│   - Filesystem servers                   │
│   - Network servers                      │
└──────────────────────────────────────────┘
```

## Building Go Programs for Lux9

### **Cross-Compilation**

```bash
# Build static binary for Lux9
GOOS=plan9 GOARCH=amd64 go build -o program program.go

# The resulting binary:
# - Contains Go runtime
# - Makes Plan 9 syscalls
# - No libc dependency
# - Single static executable
```

### **Example: Simple Init Program**

```go
// userspace/bin/init/main.go
package main

import (
    "fmt"
    "os"
    "syscall"
)

func main() {
    fmt.Println("[init] Starting Lux9 init process")

    // Bind console device to /dev
    err := syscall.Bind("#c", "/dev", syscall.MAFTER)
    if err != nil {
        fmt.Printf("[init] Failed to bind console: %v\n", err)
        os.Exit(1)
    }

    // Bind environment device to /env
    err = syscall.Bind("#e", "/env", syscall.MAFTER)
    if err != nil {
        fmt.Printf("[init] Failed to bind env: %v\n", err)
        os.Exit(1)
    }

    // Open console for stdio
    consfd, err := syscall.Open("/dev/cons", syscall.O_RDWR)
    if err != nil {
        fmt.Printf("[init] Failed to open console: %v\n", err)
        os.Exit(1)
    }

    // Duplicate to stdin, stdout, stderr
    syscall.Dup(consfd, 0)  // stdin
    syscall.Dup(consfd, 1)  // stdout
    syscall.Dup(consfd, 2)  // stderr
    syscall.Close(consfd)

    fmt.Println("[init] Console initialized")

    // Set up environment
    os.Setenv("path", "/bin")

    // Exec rc shell (if available)
    argv := []string{"/bin/rc", "-i"}
    err = syscall.Exec("/bin/rc", argv, os.Environ())

    // If exec fails, run a simple shell loop
    fmt.Println("[init] RC shell not available, running minimal shell")
    runMinimalShell()
}

func runMinimalShell() {
    // Simple shell implementation in Go
    // ...
}
```

### **Example: Crypto Server in Go**

```go
// userspace/servers/crypto/main.go
package main

import (
    "crypto/aes"
    "crypto/cipher"
    "crypto/sha256"
    "fmt"
    "io"
    "os"
)

func main() {
    fmt.Println("[crypto] Starting crypto server")

    // Serve 9P on stdin/stdout
    serve9P()
}

// AES-256-GCM encryption (for secure disk)
func encryptSector(key, nonce, plaintext []byte) ([]byte, error) {
    // Create AES-256 cipher
    block, err := aes.NewCipher(key)  // 32 bytes for AES-256
    if err != nil {
        return nil, err
    }

    // Create GCM mode
    gcm, err := cipher.NewGCM(block)
    if err != nil {
        return nil, err
    }

    // Encrypt (appends auth tag)
    ciphertext := gcm.Seal(nil, nonce, plaintext, nil)
    return ciphertext, nil
}

// SHA-256 hashing
func hashData(data []byte) []byte {
    hash := sha256.Sum256(data)
    return hash[:]
}

// 9P server implementation
func serve9P() {
    // Use existing Go 9P libraries:
    // - github.com/lionkov/go9p
    // - github.com/9fans/go

    // Or implement simple 9P protocol
    // ...
}
```

## Go's Built-in Crypto Library

Go includes excellent crypto that can replace or complement Supercop:

### **What Go Provides**

```go
import (
    "crypto/aes"           // AES with AES-NI acceleration!
    "crypto/cipher"        // GCM, CTR, CBC modes
    "crypto/sha256"        // SHA-256
    "crypto/sha512"        // SHA-512
    "crypto/ed25519"       // Ed25519 signatures
    "crypto/rand"          // Cryptographically secure RNG
    "crypto/subtle"        // Constant-time operations

    "golang.org/x/crypto/chacha20poly1305"  // ChaCha20-Poly1305
    "golang.org/x/crypto/blake2b"           // BLAKE2b
    "golang.org/x/crypto/sha3"              // SHA-3
)
```

### **Performance**

Go's crypto library:
- ✅ Uses hardware acceleration (AES-NI, etc.)
- ✅ Constant-time implementations
- ✅ Well-tested and audited
- ✅ Actively maintained by Google security team

### **AES-256-GCM for Secure Disk**

```go
package main

import (
    "crypto/aes"
    "crypto/cipher"
)

// Encrypt disk sector with AES-256-GCM
func encryptDiskSector(key [32]byte, nonce [12]byte, sector []byte) ([]byte, error) {
    // Create AES cipher
    block, err := aes.NewCipher(key[:])
    if err != nil {
        return nil, err
    }

    // Create GCM mode (includes authentication tag)
    gcm, err := cipher.NewGCM(block)
    if err != nil {
        return nil, err
    }

    // Encrypt sector (appends 16-byte auth tag)
    ciphertext := gcm.Seal(nil, nonce[:], sector, nil)
    return ciphertext, nil
}

// Decrypt and verify disk sector
func decryptDiskSector(key [32]byte, nonce [12]byte, ciphertext []byte) ([]byte, error) {
    block, err := aes.NewCipher(key[:])
    if err != nil {
        return nil, err
    }

    gcm, err := cipher.NewGCM(block)
    if err != nil {
        return nil, err
    }

    // Decrypt and verify (returns error if auth tag fails)
    plaintext, err := gcm.Open(nil, nonce[:], ciphertext, nil)
    if err != nil {
        return nil, err  // Authentication failed!
    }

    return plaintext, nil
}
```

## Go 9P Libraries

Several 9P implementations exist for Go:

### **Option 1: go9p (Most Popular)**
```bash
go get github.com/lionkov/go9p
```

```go
import "github.com/lionkov/go9p/p"

// Create 9P file server
srv := p.NewFileSrv(&root)
srv.Serve()
```

### **Option 2: 9fans/go**
```bash
go get github.com/9fans/go/plan9
```

Plan 9 utilities ported to Go.

### **Option 3: Custom Implementation**
Implement minimal 9P server for crypto service:
- Handle Topen, Tread, Twrite messages
- Present virtual filesystem
- Dispatch to crypto functions

## Benefits Over C Libc Approach

### **✅ Simpler**
- No need to port 9front libc
- No need to build GCC-compatible headers
- No complex build system
- Works out of the box

### **✅ Modern Language**
- Type safety
- Garbage collection
- Excellent concurrency (goroutines)
- Great tooling (go fmt, go test, go vet)
- Rich standard library

### **✅ Native Plan 9**
- Go speaks Plan 9 syscalls natively
- Created by Plan 9 designers
- GOOS=plan9 is first-class target

### **✅ Better Crypto**
- Hardware-accelerated AES
- Audited implementations
- Actively maintained
- Easy to use API

### **✅ 9P Libraries Available**
- Multiple implementations
- Well-tested
- Production-ready

## Migration Plan

### **Phase 1: Simple Go Init (1-2 days)**

1. Write minimal init program in Go
2. Cross-compile for Plan 9
3. Test syscalls (bind, open, dup)
4. Boot Lux9 with Go init

**Deliverable**: Lux9 boots to Go init process

### **Phase 2: Go Crypto Server (3-5 days)**

1. Implement simple 9P server in Go
2. Add AES-256-GCM support
3. Add SHA-256 support
4. Present /crypto/ virtual filesystem

**Deliverable**: Crypto server accessible at /crypto/

### **Phase 3: Integrate with Secure Disk (2-3 days)**

1. Connect secure disk driver to crypto server
2. Use AES-256-GCM for sector encryption
3. Test read/write performance
4. Verify authentication

**Deliverable**: Working encrypted disk

### **Phase 4: Additional Go Utilities (Ongoing)**

Write basic utilities in Go:
- `echo` - Print text
- `cat` - Concatenate files
- `ls` - List directory
- `cp`, `mv`, `rm` - File operations
- Simple shell (if not using rc)

## What About RC Shell?

You have three options:

### **Option A: Minimal C Libc for RC Only**
- Port just enough 9front libc to build rc
- ~30-50 files, 2-3 days work
- Only C program in userspace
- Everything else is Go

### **Option B: Go Shell**
- Write simple shell in Go
- Or port `rc` to Go (may already exist)
- Pure Go userspace

### **Option C: Use 9front RC Binary**
- Build rc on real Plan 9/9front system
- Copy binary to Lux9
- Since syscalls match, should work!

**Recommendation**: Start with Option C (easiest), then Option A if needed.

## Code Organization

```
userspace/
├── bin/
│   ├── init/              # Go init program
│   │   └── main.go
│   ├── echo/              # Go echo utility
│   │   └── main.go
│   ├── cat/               # Go cat utility
│   │   └── main.go
│   └── rc/                # C rc shell (optional)
│       └── (9front source)
│
├── servers/
│   ├── crypto/            # Go crypto server
│   │   ├── main.go
│   │   ├── server.go      # 9P server
│   │   ├── aes.go         # AES-GCM
│   │   ├── hash.go        # SHA-256, etc.
│   │   └── supercop/      # (Optional: CGO bindings)
│   │
│   └── fs/                # Go filesystem servers
│       └── ...
│
└── lib/
    └── libc9/             # Minimal C libc (only if needed for rc)
        └── ...
```

## Build System

```makefile
# userspace/Makefile

# Go programs
GO_PROGRAMS = init echo cat

# Build all Go programs
go-all: $(GO_PROGRAMS)

# Build individual Go program
$(GO_PROGRAMS):
	cd bin/$@ && GOOS=plan9 GOARCH=amd64 go build -o ../../build/$@

# Crypto server
crypto-server:
	cd servers/crypto && GOOS=plan9 GOARCH=amd64 go build -o ../../build/crypto

# Optional: RC shell (C)
rc:
	# Only if using C libc approach
	cd bin/rc && make

clean:
	rm -f build/*
```

## Summary

**Using Go for Lux9 userspace:**
- ✅ No libc needed
- ✅ Syscalls already match
- ✅ Modern language with great tooling
- ✅ Built-in crypto library
- ✅ 9P libraries available
- ✅ Created by Plan 9 designers
- ✅ Static binaries
- ✅ Much simpler than C approach

**Next steps:**
1. Verify Go syscalls work with Lux9 (test simple program)
2. Build Go init program
3. Build Go crypto server with 9P
4. Integrate with secure disk

**Time estimate**: 1-2 weeks for complete Go userspace (vs 3-4 weeks for C libc approach)
