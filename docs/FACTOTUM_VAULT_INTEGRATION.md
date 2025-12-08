# Can the Secure Vault be the Grain of Factotum?

## Short Answer: **YES** - It's the Perfect Foundation

The secure vault we implemented can serve as the **secure storage grain** (core) for a lux9 factotum implementation. Here's why and how:

## Factotum Architecture Analysis

### What Factotum Needs

From analyzing `src/cmd/auth/factotum/`:

1. **Key Storage**: Factotum stores keys with private attributes (`k->privattr`)
   ```c
   struct Key {
       Attr *attr;        // Public attributes (proto, user, server, etc.)
       Attr *privattr;    // Private attributes (!password, !hex, etc.)
       Proto *proto;      // Authentication protocol
       void *priv;        // Protocol-specific parsed key
   }
   ```

2. **Secure Memory**: Keys must persist in memory but be protected from:
   - Other processes
   - Swapping to disk
   - Cold boot attacks (ideally)
   - Memory dumps (as much as possible)

3. **Integration with secstore**: Factotum can load keys from secstore (secure remote storage)
   ```c
   // fs.c:174-181
   if(trysecstore){
       execl("/bin/auth/secstore", "secstore", "-G", "factotum", nil);
   }
   ```

4. **File System Interface**: Factotum is a 9P file server with:
   - `/mnt/factotum/ctl` - Control interface
   - `/mnt/factotum/rpc` - RPC for authentication protocols
   - `/mnt/factotum/confirm` - Key confirmation
   - `/mnt/factotum/needkey` - Key requests
   - `/mnt/factotum/log` - Audit log

### What Our Vault Provides

1. **Secure Storage** ✅
   - Pebble Black allocation (non-swappable)
   - XChaCha20 encryption (256-bit)
   - Argon2id password protection
   - 7-pass secure wipe

2. **Memory Protection** ✅
   - Cannot be paged to disk
   - Isolated from other processes
   - Protected by Pebble ownership

3. **Lock/Unlock Interface** ✅
   - Password-based authentication
   - Cryptographic key derivation
   - Transparent encryption/decryption

4. **Control Interface** ✅
   - `/dev/secureram.ctl` - Control operations
   - `/dev/secureram` - Data storage
   - Status queries

## Integration Architecture

### Option 1: Kernel-Level Factotum (Recommended)

Implement factotum as a kernel device like devauth.c in 9front:

```
/dev/factotum/
├── ctl         -> Control interface (add/delete keys)
├── rpc         -> RPC for authentication protocols
├── proto       -> List of supported protocols
├── confirm     -> Key confirmation
├── needkey     -> Key request notifications
└── log         -> Audit trail

Backend Storage:
    └── /dev/secureram (our vault)
```

**Architecture**:
```c
// kernel/9front-port/devfactotum.c

typedef struct FactotumKey {
    Attr *attr;        // Public attrs
    Attr *privattr;    // Private attrs (stored in vault)
    Proto *proto;
    ulong offset;      // Offset in /dev/secureram
    ulong size;        // Size of key data
} FactotumKey;

static FactotumKey *keys[256];
static int nkeys;

// Store key in vault
static int storekey(FactotumKey *k) {
    Chan *c;
    ulong off;

    // Open vault
    c = namec("/dev/secureram", Aopen, ORDWR, 0);

    // Allocate space
    off = factotum_alloc(k->size);

    // Serialize key data
    uchar *buf = serialize_key(k);

    // Write to vault
    devtab[c->type]->write(c, buf, k->size, off);

    k->offset = off;
    cclose(c);
    free(buf);
    return 0;
}

// Load key from vault
static int loadkey(FactotumKey *k) {
    Chan *c;
    uchar *buf;

    c = namec("/dev/secureram", Aopen, OREAD, 0);
    buf = smalloc(k->size);

    // Read from vault
    devtab[c->type]->read(c, buf, k->size, k->offset);

    // Deserialize
    deserialize_key(k, buf);

    cclose(c);
    crypto_wipe(buf, k->size);
    free(buf);
    return 0;
}
```

**Advantages**:
- No userspace roundtrip for auth operations
- Kernel can authenticate itself (for network protocols, etc.)
- Faster (no context switches)
- More secure (keys never leave kernel)

**Challenges**:
- More complex (implement all protocols in kernel)
- Larger kernel image
- Protocol updates require kernel rebuild

### Option 2: Userspace Factotum with Vault Backend

Keep factotum in userspace but use vault for key storage:

```
Userspace Factotum (9P server)
    ↓
/dev/secureram (kernel vault)
```

**Architecture**:
```c
// userspace factotum with vault backend

static int savekeyring(void) {
    int fd;
    uchar *buf;
    int n;

    // Serialize keyring
    buf = serialize_keyring(ring, &n);

    // Write to vault
    fd = open("/dev/secureram", OWRITE);
    write(fd, buf, n);
    close(fd);

    crypto_wipe(buf, n);
    free(buf);
    return 0;
}

static int loadkeyring(void) {
    int fd, n;
    uchar buf[1024*1024];  // 1MB max keyring

    // Read from vault
    fd = open("/dev/secureram", OREAD);
    n = read(fd, buf, sizeof(buf));
    close(fd);

    // Deserialize
    deserialize_keyring(&ring, buf, n);

    crypto_wipe(buf, n);
    return 0;
}
```

**Advantages**:
- Simpler (existing factotum code works)
- Protocol updates don't require kernel changes
- Smaller kernel
- Easier debugging

**Challenges**:
- Keys briefly in userspace memory
- Context switches for auth operations
- Userspace can be killed/restarted

### Option 3: Hybrid Approach (Best of Both Worlds)

Kernel provides crypto primitives and secure storage, userspace provides protocol logic:

```
Userspace Factotum (protocol logic)
    ↓
/dev/factotum/proto (kernel crypto ops)
    ↓
/dev/secureram (kernel vault)
```

**Key Operations in Kernel**:
- PBKDF2/Argon2 (key derivation)
- HMAC-SHA256 (challenge-response)
- RSA/ECDSA signing
- AES/ChaCha20 encryption

**Protocol Logic in Userspace**:
- p9sk1, p9any, chap, mschap, etc.
- Negotiation
- State machine

## Practical Implementation Plan

### Phase 1: Vault-Backed Keyring (Quick Win)

Modify existing factotum to use vault:

1. **Change key storage** from memory-only to vault-backed:
   ```c
   // factotum/rpc.c - replace key storage

   // OLD: Keys in malloc'd memory
   ring->key[i] = emalloc(sizeof(Key));

   // NEW: Keys in vault
   ring->key[i] = vault_alloc_key();
   ```

2. **Persistence**: Keys survive factotum restart
   ```c
   // On startup
   loadkeyring_from_vault();

   // On key add/delete
   savekeyring_to_vault();
   ```

3. **Protection**: Keys are encrypted at rest
   ```bash
   # Initialize vault
   echo "init MyFactotumPassword" > /dev/secureram.ctl

   # Start factotum (loads from vault)
   factotum -m /mnt/factotum
   ```

**Files to Modify**:
- `src/cmd/auth/factotum/rpc.c` - Key add/delete
- `src/cmd/auth/factotum/fs.c` - Startup/shutdown
- `src/cmd/auth/factotum/util.c` - Vault I/O functions

### Phase 2: Kernel Crypto Accelerator

Add `/dev/factotum/crypto` for crypto operations:

```c
// kernel/9front-port/devfactotum.c

enum {
    Qdir,
    Qcrypto,    // Crypto operations
};

// Write: operation request
// Read: operation result

// Example operations:
// - pbkdf2 <password> <salt> <iterations>
// - hmac-sha256 <key> <data>
// - rsa-sign <key_offset> <hash>
// - chacha20 <key> <nonce> <data>
```

**Usage from userspace factotum**:
```c
// Instead of libsec's pbkdf2
int fd = open("/dev/factotum/crypto", ORDWR);
fprint(fd, "pbkdf2 %s %s %d", password, salt, iterations);
read(fd, derived_key, 32);
close(fd);
```

### Phase 3: Full Kernel Factotum

Implement complete factotum in kernel (long-term):

**New Kernel Device**: `kernel/9front-port/devfactotum.c`

**Files**:
- `devfactotum.c` - 9P file server
- `factotum_proto.c` - Protocol implementations
- `factotum_key.c` - Key management
- `factotum_rpc.c` - RPC handling

**Integration with Vault**:
```c
// All keys stored in vault
static uchar *factotum_keyring_base;  // Base address in vault
static ulong factotum_keyring_size;   // Size of keyring

static void factotum_init(void) {
    Chan *c;

    // Open vault
    c = namec("/dev/secureram", Aopen, ORDWR, 0);

    // Reserve space for factotum keyring
    factotum_keyring_base = 0;  // Start of vault
    factotum_keyring_size = 1024*1024;  // 1MB

    // Load existing keys
    load_keyring_from_vault(c);

    cclose(c);
}
```

## Security Comparison

| Feature | Userspace Factotum | Kernel Factotum w/ Vault |
|---------|-------------------|--------------------------|
| **Key Storage** | malloc (swappable) | Vault (non-swappable) |
| **Encryption** | None (plaintext) | XChaCha20 (encrypted) |
| **Password Protection** | None | Argon2id |
| **Memory Protection** | Process isolation | Pebble + Encryption |
| **Persistence** | secstore (network) | Vault (local) |
| **Cold Boot Protection** | No | Limited (encrypted) |
| **Secure Wipe** | free() | 7-pass DoD wipe |

## Vault as Factotum Grain: Design Principles

### 1. **Grain = Core Storage Layer**

The vault is the **grain** (seed/core) because it provides:
- Secure storage substrate
- Cryptographic protection
- Memory isolation
- Password authentication

Factotum is the **shell** (interface) that grows from the grain:
- Protocol implementations
- 9P file system
- RPC handling
- Key management logic

### 2. **Separation of Concerns**

```
┌─────────────────────────────────────┐
│   Factotum (Protocol Logic)         │
│   - p9sk1, chap, rsa, etc.          │
│   - Authentication negotiation      │
│   - 9P file server                  │
└──────────────┬──────────────────────┘
               │
               ↓
┌─────────────────────────────────────┐
│   Vault (Secure Storage Grain)      │
│   - XChaCha20 encryption            │
│   - Argon2id password KDF           │
│   - Pebble Black allocation         │
│   - Secure wipe                     │
└─────────────────────────────────────┘
```

### 3. **Trust Boundary**

**Vault (Grain)**: Trusted Computing Base (TCB)
- Must be correct
- Must be secure
- Minimal attack surface

**Factotum (Shell)**: Can be more complex
- Protocol bugs don't compromise key storage
- Can be restarted without losing keys
- Easier to update/patch

## Next Steps

### Immediate (Week 1):
1. ✅ Vault implemented (DONE)
2. Port factotum to lux9 userspace
3. Modify factotum to use `/dev/secureram` for key storage

### Short-term (Month 1):
4. Add vault persistence across reboots
5. Implement vault backup/restore
6. Add audit logging to vault

### Long-term (Quarter 1):
7. Implement kernel crypto accelerator (`/dev/factotum/crypto`)
8. Port key management to kernel
9. Implement full kernel factotum

## Code Roadmap

### Files to Create:
1. `kernel/9front-port/devfactotum.c` - Kernel factotum device
2. `kernel/factotum_proto.c` - Protocol implementations
3. `userspace/cmd/auth/factotum/vault.c` - Vault I/O for userspace factotum

### Files to Modify:
1. `kernel/9front-port/devram.c` - Add factotum-specific APIs
2. `src/cmd/auth/factotum/rpc.c` - Use vault for key storage
3. `src/cmd/auth/factotum/fs.c` - Load/save keyring from vault

## Conclusion

**The secure vault is an excellent grain for factotum** because it provides:

1. **Secure Storage**: XChaCha20 + Argon2id
2. **Memory Protection**: Pebble Black (non-swappable)
3. **Password Protection**: User authentication required
4. **Secure Wipe**: DoD 7-pass on cleanup
5. **Simple Interface**: `/dev/secureram` + `/dev/secureram.ctl`

The vault solves factotum's fundamental security problem: **how to safely store authentication keys in memory**. Current Plan 9 factotum stores keys in swappable userspace memory, which can be:
- Paged to disk (exposing keys)
- Read by root
- Recovered from memory dumps

Our vault eliminates these vulnerabilities while providing a clean interface for factotum to build upon.

**The grain metaphor is perfect**: The vault is the small, secure kernel from which the entire factotum authentication system can grow.
