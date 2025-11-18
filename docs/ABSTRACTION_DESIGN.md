# Crypto Server Abstraction Layer

## Design Goals

1. **Easy algorithm selection** - Use any of 1,443 algorithms via simple file paths
2. **Unified API** - Same interface regardless of algorithm type
3. **Zero configuration** - Auto-discover and register all Supercop algorithms
4. **Performance** - Direct dispatch without overhead
5. **Extensibility** - Add new algorithms by just dropping in files

## Architecture

```
┌─────────────────────────────────────────────────┐
│ User Application                                 │
│   open("/crypto/aead/aes256gcm", O_RDWR)        │
│   write(fd, "key:...|nonce:...|data:...")       │
│   read(fd, ciphertext, size)                     │
└────────────────┬────────────────────────────────┘
                 │ 9P messages
┌────────────────▼────────────────────────────────┐
│ Crypto Server (9P handler)                       │
│  - Parses path: category="aead", algo="aes256gcm"│
│  - Looks up in registry                          │
└────────────────┬────────────────────────────────┘
                 │
┌────────────────▼────────────────────────────────┐
│ Algorithm Registry                               │
│  registry["aead"]["aes256gcm"] → Algorithm       │
│    - Function pointers to implementation         │
│    - Metadata (key size, nonce size, etc.)       │
└────────────────┬────────────────────────────────┘
                 │
┌────────────────▼────────────────────────────────┐
│ Supercop Implementation                          │
│  supercop/crypto_aead/aes256gcmv1/encrypt.c      │
│    - Actual crypto code                          │
└──────────────────────────────────────────────────┘
```

## File-Based Interface

### **Virtual Filesystem Layout**
```
/crypto/
├── hash/
│   ├── sha256          # Write data, read hash
│   ├── sha512
│   ├── sha3-256
│   ├── blake2b
│   └── ... (174 total)
│
├── aead/               # Authenticated Encryption
│   ├── aes128gcm       # AES-128-GCM
│   ├── aes256gcm       # AES-256-GCM (for secure disk!)
│   ├── chacha20poly1305
│   └── ... (504 total)
│
├── stream/             # Stream Ciphers
│   ├── chacha20
│   ├── aes256ctr
│   └── ... (43 total)
│
├── sign/               # Digital Signatures
│   ├── ed25519
│   ├── dilithium2      # Post-quantum
│   └── ... (205 total)
│
├── kem/                # Key Encapsulation (post-quantum)
│   ├── kyber512
│   └── ... (214 total)
│
└── rng/                # Random Number Generation
    └── bytes           # Read random bytes
```

## Usage Examples

### **1. Hashing (Simple)**
```c
// Hash some data with SHA256
int fd = open("/crypto/hash/sha256", O_RDWR);
write(fd, "hello world", 11);
char hash[64];
read(fd, hash, 64);  // Returns hex-encoded hash
close(fd);

// Switch to BLAKE2b - just change the path!
fd = open("/crypto/hash/blake2b", O_RDWR);
write(fd, "hello world", 11);
read(fd, hash, 64);
close(fd);
```

### **2. Secure Disk Encryption (AES-256-GCM)**
```c
// Encrypt disk sector with AES-256-GCM
int fd = open("/crypto/aead/aes256gcm", O_RDWR);

// Write: key|nonce|plaintext
char input[4096 + 32 + 12];  // data + key + nonce
memcpy(input, key, 32);
memcpy(input + 32, nonce, 12);
memcpy(input + 44, sector_data, 4096);

write(fd, input, 4096 + 44);

// Read: ciphertext + auth tag
char output[4096 + 16];
read(fd, output, 4096 + 16);

close(fd);
```

### **3. Easy Algorithm Switching**
```c
// Original: AES-256-GCM
int fd = open("/crypto/aead/aes256gcm", O_RDWR);

// Switch to ChaCha20-Poly1305 (just change path!)
int fd = open("/crypto/aead/chacha20poly1305", O_RDWR);

// Same API, different algorithm!
```

### **4. Digital Signatures**
```c
// Sign with Ed25519
int fd = open("/crypto/sign/ed25519", O_RDWR);

// Write: secretkey|message
write(fd, key_and_message, 32 + msglen);

// Read: signature
char signature[64];
read(fd, signature, 64);

close(fd);
```

## Registry Auto-Discovery

### **Startup Sequence**
```c
int main() {
    // 1. Initialize registry
    crypto_registry_init();

    // 2. Scan Supercop directories
    scan_and_register("supercop/crypto_hash", CRYPTO_CAT_HASH);
    scan_and_register("supercop/crypto_aead", CRYPTO_CAT_AEAD);
    scan_and_register("supercop/crypto_sign", CRYPTO_CAT_SIGN);
    // ... etc for all categories

    // 3. Now registry contains all 1,443 algorithms!
    printf("Registered %d algorithms\n", crypto_registry.count);

    // 4. Start 9P server
    serve_9p();
}
```

### **Algorithm Registration**
```c
// For each algorithm, create descriptor
CryptoAlgorithm algo_aes256gcm = {
    .name = "aes256gcm",
    .category = CRYPTO_CAT_AEAD,
    .impl = "ref",  // or "aesni" for optimized

    .key_bytes = 32,
    .nonce_bytes = 12,
    .output_bytes = 0,  // Variable
    .auth_bytes = 16,

    .aead_encrypt = aes256gcm_encrypt,
    .aead_decrypt = aes256gcm_decrypt,
};

crypto_registry_add(&algo_aes256gcm);
```

## Abstraction Benefits

### **For Users**
✅ **Simple** - Just open a file path
✅ **Discoverable** - `ls /crypto/aead/` shows all options
✅ **No configuration** - Works out of the box
✅ **Easy switching** - Change one string to use different algorithm

### **For Secure Disk**
✅ **AES-256-GCM** - Strong authenticated encryption
✅ **Easy fallback** - Can switch to ChaCha20-Poly1305
✅ **Per-sector encryption** - Stream API for large data
✅ **Authentication** - Built-in integrity checking

### **For Developers**
✅ **Extensible** - Add new algorithms by dropping in wrappers
✅ **Testable** - Each algorithm is independent
✅ **Maintainable** - Clean separation of concerns
✅ **Flexible** - Can use multiple implementations of same algorithm

## Secure Disk Integration

### **Disk Encryption Flow**
```
Secure Disk Driver
    ↓
    │ For each sector write:
    │
    ├─> open("/crypto/aead/aes256gcm", O_RDWR)
    ├─> write(fd, key|nonce|sector_data)
    ├─> read(fd, encrypted_sector)
    └─> close(fd)

    │ For each sector read:
    │
    ├─> open("/crypto/aead/aes256gcm", O_RDWR)
    ├─> write(fd, key|nonce|encrypted_sector|tag)
    ├─> read(fd, plaintext_sector)
    └─> close(fd)
```

### **Configuration Options**
```bash
# Config file for secure disk
encryption_algorithm=/crypto/aead/aes256gcm
key_size=32
nonce_size=12

# Can easily switch to:
# encryption_algorithm=/crypto/aead/chacha20poly1305
# encryption_algorithm=/crypto/aead/aes128gcm
```

## Implementation Strategy

### **Phase 1: Core Algorithms** (Week 1)
Build wrappers for most commonly needed:
- SHA256, SHA512 (hashing)
- AES-256-GCM (secure disk!)
- ChaCha20-Poly1305 (alternative AEAD)
- Ed25519 (signing)

### **Phase 2: Extended Suite** (Week 2)
Add more standard algorithms:
- BLAKE2b, SHA-3
- AES-128-GCM, AES-192-GCM
- X25519 (key exchange)

### **Phase 3: Full Registry** (Week 3)
Auto-discover and register all 1,443 algorithms:
- Parse Supercop directory structure
- Read API headers for sizes
- Generate wrappers automatically

### **Phase 4: Optimization** (Ongoing)
Replace reference implementations with optimized versions:
- AES-NI for AES
- AVX2 for ChaCha20
- Architecture-specific optimizations

## API Flexibility

### **One-Shot API** (Simple)
```c
crypto_hash_data("sha256", data, len, hash, &hashlen);
crypto_aead_encrypt_data("aes256gcm", plaintext, plen,
                         key, keylen, nonce, noncelen,
                         ad, adlen, ciphertext, &clen, tag, &taglen);
```

### **Streaming API** (Large Data)
```c
CryptoContext *ctx = crypto_stream_start("hash", "sha256");
crypto_stream_update(ctx, chunk1, len1);
crypto_stream_update(ctx, chunk2, len2);
crypto_stream_final(ctx, hash, &hashlen);
```

### **File-Based API** (9P Users)
```c
int fd = open("/crypto/hash/sha256", O_RDWR);
write(fd, data, len);
read(fd, hash, 64);
close(fd);
```

All three APIs use the same underlying abstraction!

## Summary

The abstraction layer provides:

✅ **Unified interface** for 1,443 algorithms
✅ **Easy selection** via file paths
✅ **Zero overhead** - direct function pointers
✅ **Secure disk ready** - AES-256-GCM built-in
✅ **Extensible** - add algorithms by convention
✅ **Future-proof** - includes post-quantum algorithms

**Next**: Implement the registry and build wrappers for core algorithms (starting with AES-256-GCM for secure disk).
