# Crypto Server Design

## Overview
File-based cryptographic operations using write→read pattern via 9P filesystem interface.

## Architecture

### **File-Based Crypto Interface**
```
/crypto/
  ├── hash/
  │   ├── sha256      (write data, read hash)
  │   └── sha512      (write data, read hash)
  ├── encrypt/
  │   ├── aes256gcm   (write plaintext+key+nonce, read ciphertext)
  │   └── chacha20    (write plaintext+key+nonce, read ciphertext)
  ├── decrypt/
  │   ├── aes256gcm   (write ciphertext+key+nonce, read plaintext)
  │   └── chacha20    (write ciphertext+key+nonce, read plaintext)
  ├── sign/
  │   ├── ed25519     (write data+privatekey, read signature)
  │   └── verify      (write data+signature+publickey, read "ok"/"fail")
  └── random
      └── bytes       (read random bytes)
```

### **Usage Pattern**
```bash
# Hashing
echo "hello world" > /crypto/hash/sha256
cat /crypto/hash/sha256
# Output: b94d27b9934d3e08a52e52d7da7dabfac484efe37a5380ee9088f7ace2efcde9

# Encryption
echo "plaintext:hello|key:0123456789ABCDEF|nonce:NONCE123" > /crypto/encrypt/aes256gcm
cat /crypto/encrypt/aes256gcm
# Output: <base64 encoded ciphertext>

# Random bytes
dd if=/crypto/random/bytes of=random.dat bs=32 count=1
```

## Implementation

### **Phase 1: Core Infrastructure**
1. ✅ Create devcrypto.c - 9P device driver
2. ✅ Implement /crypto directory structure
3. ✅ Basic read/write operations

### **Phase 2: Hash Operations**
1. ✅ Extract SHA256 from Supercop
2. ✅ Implement hash file operations
3. ✅ Support streaming hashing

### **Phase 3: Encryption**
1. Extract AES-256-GCM from Supercop
2. Implement encrypt/decrypt operations
3. Key and nonce management

### **Phase 4: Signing**
1. Extract Ed25519 from Supercop
2. Implement sign/verify operations
3. Key pair management

### **Phase 5: Security Integration**
1. Borrow checker integration
2. Pebble resource accounting
3. Process isolation
4. Audit logging

## Data Structures

### **Crypto Context**
```c
typedef struct CryptoCtx {
    int type;          /* HASH, ENCRYPT, DECRYPT, SIGN, VERIFY */
    int algorithm;     /* SHA256, AES256GCM, ED25519, etc */
    void *state;       /* Algorithm-specific state */
    uchar *buffer;     /* Input buffer */
    ulong buflen;      /* Buffer length */
    ulong bufsize;     /* Buffer capacity */
    Proc *owner;       /* Owning process */
    uvlong pebble_cost; /* Resource cost */
} CryptoCtx;
```

### **Crypto Operations**
```c
typedef struct CryptoOps {
    char *name;
    int (*init)(CryptoCtx*);
    int (*update)(CryptoCtx*, uchar*, ulong);
    int (*final)(CryptoCtx*, uchar*, ulong*);
    void (*cleanup)(CryptoCtx*);
} CryptoOps;
```

## Security Model

### **Access Control**
- Per-process crypto contexts (isolated)
- Borrow checker prevents concurrent access
- Pebble accounting for resource limits

### **Resource Accounting**
```c
Operation       | Pebble Cost
----------------|------------
SHA256 (1KB)    | 100 pebbles
AES-GCM (1KB)   | 500 pebbles
Ed25519 sign    | 1000 pebbles
Random (1KB)    | 50 pebbles
```

### **Audit Trail**
```c
struct CryptoAudit {
    Proc *proc;
    int operation;
    int algorithm;
    uvlong timestamp;
    ulong data_size;
    int success;
};
```

## Files to Create

1. `kernel/9front-port/devcrypto.c` - Main device driver
2. `kernel/include/crypto.h` - Public API
3. `kernel/crypto/sha256.c` - SHA256 implementation
4. `kernel/crypto/aes256gcm.c` - AES-256-GCM implementation
5. `kernel/crypto/ed25519.c` - Ed25519 implementation
6. `kernel/crypto/random.c` - CSPRNG wrapper

## Testing

```c
/* Test hashing */
int fd = open("/crypto/hash/sha256", OWRITE);
write(fd, "hello", 5);
close(fd);
fd = open("/crypto/hash/sha256", OREAD);
char hash[65];
read(fd, hash, 64);
hash[64] = 0;
print("SHA256: %s\n", hash);
```

## Performance Targets

| Operation | Throughput | Latency |
|-----------|-----------|---------|
| SHA256 | > 100 MB/s | < 10ms/KB |
| AES-GCM | > 50 MB/s | < 20ms/KB |
| Ed25519 sign | > 1000 ops/s | < 1ms |
| Random | > 10 MB/s | < 1ms/KB |

## Next Steps

1. Implement devcrypto.c with basic directory structure
2. Add SHA256 hash operation
3. Test with simple hash operations
4. Add encryption/decryption
5. Add signing/verification
6. Integrate security features
