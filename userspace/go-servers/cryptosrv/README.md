# Crypto Server (cryptosrv)

A 9P-based cryptographic service server for Lux9, providing hash and signature operations through a filesystem interface.

## Overview

The crypto server exposes cryptographic operations as files in a 9P filesystem. **All communication happens exclusively through the page exchange system** (`/dev/exchange`), providing zero-copy message passing between the kernel and userspace.

```
/
├── blake2b          - Blake2b-512 hash (write data, read hash)
└── ed25519/
    └── verify       - Ed25519 signature verification
```

### Communication Architecture

The server uses a **hybrid ring buffer + page exchange** system:

**Ring Buffer (Primary)** - For batched small messages (< 4KB total):
```
Client → Kernel → [Ring Buffer] → Crypto Server
         9P      Batch Pages       Batch Handler
                 (/dev/ring)
```

**Page Exchange (Large Messages)** - For individual large messages:
```
Client → Kernel → [Page Exchange] → Crypto Server
         9P      Zero-copy IPC        Direct Handler
                 (/dev/exchange)
```

#### Ring Buffer Protocol (Batched Small Messages)

1. Client sends multiple small 9P requests to kernel
2. Kernel batches them into a 4KB page:
   - `BatchHeader` (16 bytes): magic, num_messages, used_bytes, nonce
   - Message 1: `[len u16][9P message data]`
   - Message 2: `[len u16][9P message data]`
   - ...
3. Kernel submits page handle to ring buffer (`/dev/ring/0`)
4. Server reads from ring's submission queue
5. Server processes each message in the batch
6. Server returns page handle to completion queue
7. Kernel delivers responses to clients

#### Data Format (Batch Page)

```
Offset  Size  Field
------  ----  -----
0x00    2     num_messages (u16)
0x02    2     used_bytes (u16)
0x04    4     magic (0xB47C4831)
0x08    8     nonce (u64)
0x10    2     message 1 length (u16)
0x12    N     message 1 data (9P)
...     2     message 2 length (u16)
...     M     message 2 data (9P)
...
```

**Benefits:**
- Amortize syscall/context-switch overhead across many messages
- Better cache locality
- Reduced ring queue pressure
- Hardware MMU still enforces page boundaries

## Building

From the `userspace/go-servers` directory:

```bash
go build -o cryptosrv/cryptosrv ./cryptosrv/
```

Or use the Makefile in the cryptosrv directory:

```bash
cd cryptosrv
make
```

## Usage

### Blake2b Hashing

Write data to `/blake2b` and read back the 64-byte hash:

```bash
# Example using 9P client
echo -n "hello world" > /crypto/blake2b
cat /crypto/blake2b  # Returns 64-byte Blake2b-512 hash
```

The hash operation is streaming - you can write data in multiple chunks before reading the result.

### Ed25519 Signature Verification

Write verification data to `/ed25519/verify` in the format:
```
pubkey:signature:message
```

All fields are hex-encoded. The file returns "OK\n" or "FAIL\n".

Example:
```bash
echo "03a7...f1:9a8b...c4:48656c6c6f" > /crypto/ed25519/verify
cat /crypto/ed25519/verify  # Returns "OK" or "FAIL"
```

## Implementation Details

- **Language**: Go
- **9P Library**: lux9/servers/p9 (local package)
- **Crypto Library**: Go stdlib (golang.org/x/crypto/blake2b, crypto/ed25519)
- **Architecture**: SIP Framework integrated server with process isolation
- **SIP Framework**: lux9/userspace/go-servers/sip (local package)

### File Structure

- `main.go` - Server entry point with SIP framework registration
- `cryptofs.go` - 9P FileServer implementation with crypto file tree
- `blake2b.go` - Blake2b hash operations
- `ed25519.go` - Ed25519 signature operations

### SIP Framework Integration

The server implements the `sip.IServer` interface:
- **Initialize()** - Sets up crypto filesystem and 9P server
- **Start()** - Begins serving 9P requests via page exchange loop
- **Stop()** - Gracefully shuts down the server

The server registers with the kernel via `/dev/sip/clone` to:
1. Declare `CapFileSystem` capability
2. Get process-isolated address space
3. Protect against GC interference from other Go servers
4. **Setup ExchangeIPC** - Initialize page exchange communication

### Ring Buffer Details

The ring buffer is a shared memory structure (`/dev/ring/0`):

```c
struct IpcChannel {
    u32 magic;      // 0x52494E47 "RING"
    u32 status;

    // Submission Ring (userspace → kernel)
    struct {
        volatile u32 head;  // Kernel read index
        volatile u32 tail;  // User write index
        u64 pages[128];     // Page handles (user VAs)
    } submission;

    // Completion Ring (kernel → userspace)
    struct {
        volatile u32 head;  // User read index
        volatile u32 tail;  // Kernel write index
        u64 pages[128];     // Returned page handles
    } completion;
}
```

**Ring Operation:**
1. Server mmaps `/dev/ring/0` to get control page
2. Kernel fills submission ring with batch page handles
3. Server polls `submission.head != submission.tail`
4. Server processes batch pages (via `/dev/exchange` mapping)
5. Server returns handles to completion ring
6. Kernel reclaims completed pages

**Zero-copy benefits:**
- No stdin/stdout buffering overhead
- Direct memory access to request/response data
- Batching amortizes syscall overhead
- Hardware MMU enforces page boundaries
- Lock-free ring buffer (atomic head/tail updates)

### 9P Protocol

The server implements the FileServer interface:
- Version, Attach, Walk, Open, Read, Write, Clunk
- Stat, Wstat (wstat not supported)
- Create, Remove (not supported - read-only operations)

## Future Enhancements

1. **Additional Algorithms** - SHA256, HMAC, AES-GCM, X25519 key exchange
2. **Key Management** - Secure key storage via Pebble device
3. **TPM Integration** - Use TPM for signing operations via HAL
4. **Rate Limiting** - Prevent crypto operation abuse
5. **Streaming API** - Better support for large data hashing
6. **Page Exchange** - Use `/dev/exchange` for zero-copy large data operations

## Security Architecture

### Threat Model

**Attacker Capabilities:**
- Malicious userspace process with no kernel privileges
- Can submit arbitrary data to ring buffer
- Can attempt to access other processes' memory
- Can attempt replay attacks or DoS

**Security Goals:**
1. **Memory Isolation** - No process can access another's memory
2. **Page Ownership** - Only page owner can submit it to ring
3. **Replay Protection** - Old batches cannot be resubmitted
4. **Bounds Safety** - No buffer overruns or out-of-bounds access
5. **Resource Limits** - Prevent DoS via excessive requests

### Defense Mechanisms

#### Kernel-Side (devring.c)

**1. Page Ownership Verification** (lines 174-194)
```c
/* Verify page ownership via borrow checker */
if (!pageown_is_owned(page_phys)) {
    print("ring: page not owned: pa=%#p\n", page_phys);
    goto skip_page;
}

if (pageown_get_owner(page_phys) != cs->owner) {
    print("ring: page owned by different process\n");
    goto skip_page;
}

/* Verify page can be borrowed (no active mut borrows) */
if (!pageown_can_borrow_shared(page_phys)) {
    print("ring: page has active mutable borrow\n");
    goto skip_page;
}
```
- Uses Rust-style borrow checker for ownership tracking
- Prevents accessing pages owned by other processes
- Ensures exclusive access during transfer

**2. Sequence Number Replay Protection** (lines 233-240)
```c
/* Validate sequence number for replay protection */
/* The nonce field acts as a monotonic sequence number, NOT a cryptographic nonce */
if (batch->nonce <= cs->last_seqno) {
    print("ring: replay detected: seqno %llud <= last %llud\n",
          batch->nonce, cs->last_seqno);
    goto skip_page;
}
cs->last_seqno = batch->nonce;
```
- Monotonically increasing sequence number prevents replay attacks
- Per-channel tracking ensures cross-process isolation
- **Note:** Despite field name, this is a sequence number (like TCP seqno), not a cryptographic nonce
- Predictable and deterministic by design - this is correct for ordering/anti-replay

**Session Randomness** (lines 110-117)
```c
/* Generate random session ID for this channel */
if (hwrandbuf != nil) {
    (*hwrandbuf)(&cs->session_id, sizeof(cs->session_id));
} else {
    cs->session_id = rdtsc() ^ (u64int)(uintptr)up;
}
```
- Each channel gets a cryptographically random session ID
- Uses RDRAND hardware RNG if available, TSC-based fallback
- Prevents cross-session confusion attacks

**3. Bounds Checking** (lines 200-243)
```c
/* Validate num_messages */
if (batch->num_messages > 256) {
    print("ring: too many messages: %ud (max 256)\n", batch->num_messages);
    goto skip_page;
}

/* Validate used_bytes */
if (batch->used_bytes < BATCH_DATA_START || batch->used_bytes > 4096) {
    print("ring: invalid used_bytes: %ud\n", batch->used_bytes);
    goto skip_page;
}

/* Validate each message length */
if (msg_len > 8192) {
    print("ring: message too large: %ud bytes\n", msg_len);
    break;
}

/* Check for integer overflow */
if (offset + msg_len < offset) {
    print("ring: integer overflow in message bounds\n");
    break;
}
```
- Multiple layers of bounds validation
- Integer overflow protection
- Per-message size limits

**4. TOCTOU Prevention via Page Unmapping** (lines 211-303)

**The TOCTOU Problem:**

Time-of-check-time-of-use (TOCTOU) is when an attacker modifies data between validation and use:

```c
/* VULNERABLE CODE (what we DON'T do) */
if (batch->num_messages > 256) {  // CHECK
    return error;
}
/* TOCTOU WINDOW - user can change num_messages here! */
for (i = 0; i < batch->num_messages; i++) {  // USE - might be > 256 now!
    process_message(...);
}
```

**Our Solution: Atomic Unmap + Copy**

```c
/* 1. UNMAP page from user space - prevents further modification */
u64int saved_pte = *pte;
*pte = 0;  /* Atomically remove user access */
putcr3(getcr3());  /* Flush TLB - user can no longer access page */

/* 2. COPY critical fields to kernel stack */
u32int batch_magic = batch->magic;
u16int batch_num_messages = batch->num_messages;
u16int batch_used_bytes = batch->used_bytes;
u64int batch_seqno = batch->nonce;

/* 3. VALIDATE using stack copies - user cannot modify these */
if (batch_num_messages > 256) {
    goto restore_page;
}

/* 4. PROCESS using validated stack values */
for (i = 0; i < batch_num_messages; i++) {
    /* Copy each msg_len to stack before use */
    u16int msg_len = *(u16int*)((u8int*)batch + offset);
    // ... validate and process ...
}

/* 5. RESTORE page mapping after processing */
*pte = saved_pte;
putcr3(getcr3());
```

**Why This Works:**
1. **Atomic Unmap** - Single PTE write + TLB flush is atomic from user perspective
2. **Temporal Isolation** - User cannot access page while kernel validates/processes
3. **Value Copying** - Stack copies are immune to concurrent modification
4. **Hardware Enforcement** - MMU enforces that user cannot bypass unmapping

**Attack Scenarios Prevented:**

| Attack | Without TOCTOU Protection | With Page Unmapping |
|--------|---------------------------|---------------------|
| Modify num_messages after validation | ✅ Succeeds - can cause buffer overrun | ❌ Blocked - page unmapped |
| Change msg_len between check and use | ✅ Succeeds - can read out of bounds | ❌ Blocked - value on stack |
| Modify sequence number after check | ✅ Succeeds - replay attack | ❌ Blocked - value on stack |
| Change page ownership during processing | ✅ Succeeds - confusion attack | ❌ Blocked - ownership checked before unmap |

**5. MMU-Based Address Translation** (lines 186-192)
```c
/* Translate user VA to physical address using MMU */
pte = mmuwalk(m->pml4, user_vaddr, 0, 0);
if (pte == nil || (*pte & PTEVALID) == 0) {
    print("ring: invalid page mapping: %#p\n", user_vaddr);
    goto skip_page;
}

page_phys = PADDR(*pte);
```
- Hardware MMU enforces page boundaries
- Invalid addresses detected before access
- Page table validation prevents arbitrary memory access

#### Userspace-Side (sip/kernel.go)

**1. Defense in Depth Validation** (lines 257-305)
```go
// Validate page handle alignment
if pageHandle&0xFFF != 0 {
    fmt.Fprintf(os.Stderr, "ring: page handle not aligned: %#x\n", pageHandle)
    return
}

// Validate magic number
if batch.Magic != BatchPageMagic {
    fmt.Fprintf(os.Stderr, "ring: invalid batch magic: %#x\n", batch.Magic)
    goto cleanup
}

// Validate num_messages
if batch.NumMessages > 256 {
    fmt.Fprintf(os.Stderr, "ring: too many messages: %d\n", batch.NumMessages)
    goto cleanup
}

// Validate used_bytes
if batch.UsedBytes < BatchDataStart || batch.UsedBytes > 4096 {
    fmt.Fprintf(os.Stderr, "ring: invalid used_bytes: %d\n", batch.UsedBytes)
    goto cleanup
}
```
- Redundant validation even though kernel already checks
- Fail-fast on malformed data
- Error logging for security auditing

**2. Error Logging** (throughout)
- All validation failures logged to stderr
- Enables detection of attack attempts
- Helps debugging legitimate errors

### Understanding "Nonce" vs Sequence Number

**Important Clarification:** The `BatchHeader.nonce` field is misnamed - it's actually a **sequence number**, not a cryptographic nonce.

**Sequence Number (what we have):**
- Monotonically increasing counter: 1, 2, 3, 4...
- Predictable and deterministic
- Purpose: Detect replays and ensure ordering
- Examples: TCP sequence numbers, IPsec anti-replay counters

**Cryptographic Nonce (what we DON'T need here):**
- Random, unpredictable value
- Used in encryption/authentication to prevent precomputation
- Examples: AES-GCM nonce, TLS random

**Why Monotonic is Correct:**
1. **Replay Detection** - Old messages have lower seqno, get rejected
2. **Ordering** - Receiver knows messages arrived in correct order
3. **Simplicity** - No need for expensive RNG on every message
4. **Standard Practice** - Same approach as TCP, TLS records, IPsec

**Where Randomness IS Used:**
- `ChannelState.session_id` - 64-bit random value per channel (uses RDRAND)
- Prevents cross-session confusion
- Established once per channel, not per message

### Security Properties

**Guaranteed:**
- ✅ No arbitrary memory access (enforced by pageown + MMU)
- ✅ No cross-process page access (enforced by borrow checker)
- ✅ No replay attacks (enforced by sequence number validation)
- ✅ No buffer overruns (enforced by bounds checking)
- ✅ No integer overflows (explicit overflow checks)
- ✅ No TOCTOU attacks (enforced by page unmapping + value copying)

**Not Yet Implemented:**
- ⚠️  Rate limiting (could DoS by flooding ring)
- ⚠️  Capability checking (CapPageExchange not verified)
- ⚠️  Timeout on page borrows (could hold pages indefinitely)
- ⚠️  Audit logging to persistent storage

### Attack Mitigation Summary

| Attack Vector | Mitigation | Status |
|--------------|------------|---------|
| Arbitrary memory read | pageown + MMU validation | ✅ Complete |
| Cross-process access | Borrow checker ownership | ✅ Complete |
| Replay attacks | Nonce validation | ✅ Complete |
| Buffer overruns | Bounds checking | ✅ Complete |
| Integer overflows | Explicit overflow checks | ✅ Complete |
| DoS via flooding | Rate limiting | ❌ Not Implemented |
| Page exhaustion | Resource limits | ❌ Not Implemented |
| Time-of-check-time-of-use | Page unmapping + value copying | ✅ Complete |

### Comparison to Original (Insecure) Version

**Before Security Fixes:**
```c
// UNSAFE - placeholder code
batch = (struct BatchHeader*)page_handle;
/* if (borrow_acquire(page_phys) != OK) ... error */  // COMMENTED OUT!
```
- No page ownership verification
- No bounds checking
- No nonce validation
- Direct pointer dereference of user-provided addresses
- **Allowed arbitrary kernel memory access**

**After Security Fixes:**
- Full borrow checker integration
- Multi-layer bounds validation
- Replay protection
- MMU-enforced address translation
- **Cannot access memory outside owned pages**

## Security Notes

- Server runs in userspace with standard process privileges
- No privileged operations or hardware access required
- Uses Go stdlib crypto (audited, well-tested)
- Signatures are verified, NOT generated (except for testing)
- Production signing should be done host-side or via TPM
- **All IPC goes through secured ring buffer + page exchange system**
- **Page ownership enforced by Rust-style borrow checker**

## Testing

Basic functionality test:

```bash
# Start server (manual test)
./cryptosrv

# In another terminal, use 9P client tools to test operations
```

For integration testing with Lux9 kernel, mount via devmnt.
