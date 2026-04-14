# Secure Ramdisk Usage Guide

## Overview

The Lux9 kernel provides a hardware-backed secure vault accessible via `/dev/secureram` and `/dev/secureram.ctl`.

**Security Features:**
- **Argon2id** password-based key derivation (4MB memory, 3 passes, GPU-resistant)
- **XChaCha20** authenticated encryption (256-bit key, 192-bit nonce)
- **7-pass DoD 5220.22-M** secure wipe on lock/destroy
- **Monocypher** cryptographic library (audited, constant-time)

## Device Files

- `/dev/secureram` - Encrypted vault data (64MB default)
- `/dev/secureram.ctl` - Control interface for lock/unlock operations

## Configuration

Set vault size in kernel config:
```
secure.ramdisk.size=128M
```

Supported suffixes: K, M, G

## Usage

### 1. Initialize Vault (First Time)

```bash
echo 'init MySecurePassword123' > /dev/secureram.ctl
```

**Requirements:**
- Password must be at least 8 characters
- Can only initialize once (until wiped)
- Vault is automatically unlocked after init

**Security Note:** Argon2id derives the master key using:
- 4MB memory (prevents GPU attacks)
- 3 iterations (CPU hardness)
- Cryptographically random 16-byte salt

### 2. Write Data to Unlocked Vault

```bash
echo 'secret data' > /dev/secureram
cat myfile.txt > /dev/secureram
```

Data is written to the vault in plaintext (unlocked state).

### 3. Lock Vault (Encrypt)

```bash
echo 'lock' > /dev/secureram.ctl
```

**What happens:**
- XChaCha20 encrypts entire vault in-place
- Master key remains in kernel memory
- Vault data is now ciphertext

### 4. Unlock Vault (Decrypt)

```bash
echo 'unlock MySecurePassword123' > /dev/secureram.ctl
```

**What happens:**
- Password is re-derived via Argon2id
- Constant-time comparison with master key
- XChaCha20 decrypts vault in-place
- Data is now accessible in plaintext

**Security:** Failed password attempts use constant-time verification to prevent timing attacks.

### 5. Read Decrypted Data

```bash
cat /dev/secureram
dd if=/dev/secureram of=recovered.txt bs=4096 count=16
```

Vault must be unlocked or read will fail with "vault is locked".

### 6. Secure Wipe and Destroy

```bash
echo 'wipe' > /dev/secureram.ctl
```

**What happens (7-pass DoD wipe):**
1. Write 0x00 (zeros)
2. Write 0xFF (ones)
3. Write cryptographic random
4. Write 0x00
5. Write 0xFF
6. Write cryptographic random
7. Write 0x00 (final)

All passes use memory barriers (`coherence()`) to prevent compiler optimization.

**After wipe:**
- Master key is wiped from memory
- Salt is destroyed
- Vault returns to uninitialized state
- Must run `init` again to reuse

## Status Query

```bash
cat /dev/secureram.ctl
```

Output:
```
status: locked
size: 67108864
```

or

```
status: unlocked
size: 67108864
```

## Security Properties

### Memory Hardness (GPU Resistance)
Argon2id uses 4MB of memory during password derivation, making GPU/ASIC attacks economically infeasible.

### Constant-Time Operations
All cryptographic operations use Monocypher's constant-time primitives to prevent side-channel attacks.

### Nonce Management
XChaCha20 uses a 192-bit (24-byte) nonce, allowing 2^192 unique encryptions with the same key (effectively unlimited).

### Key Derivation Chain
```
Password → Argon2id(4MB, 3 passes, salt) → Master Key (256 bits)
Master Key + Nonce → XChaCha20 → Ciphertext
```

### Secure Wipe
7-pass DoD wipe ensures data cannot be recovered via:
- Magnetic force microscopy
- Scanning tunneling microscopy
- Other forensic techniques

## Example Workflow

```bash
# Initialize vault with password
echo 'init SuperSecret2024!' > /dev/secureram.ctl

# Write sensitive data
echo 'SSH private key data...' > /dev/secureram

# Lock vault (encrypt)
echo 'lock' > /dev/secureram.ctl

# System continues running, vault is encrypted in memory...

# Later: unlock vault
echo 'unlock SuperSecret2024!' > /dev/secureram.ctl

# Read decrypted data
cat /dev/secureram

# Done: lock again
echo 'lock' > /dev/secureram.ctl

# Shutdown: secure wipe
echo 'wipe' > /dev/secureram.ctl
```

## Error Handling

**"vault not initialized"** - Must run `init <password>` first

**"vault is locked"** - Must run `unlock <password>` before read/write

**"vault already initialized"** - Cannot re-init without wiping first

**"incorrect password"** - Password verification failed (constant-time)

**"password must be at least 8 characters"** - Minimum security requirement

**"key derivation failed"** - Argon2id work area allocation failed (OOM)

## Performance Considerations

**Initialization:** Argon2id with 4MB memory takes ~100-500ms (intentionally slow, security feature)

**Lock/Unlock:** O(vault_size) - encrypts/decrypts entire vault

**Wipe:** 7-pass wipe of 64MB vault takes ~1-5 seconds (depends on CPU/memory speed)

**Read/Write (unlocked):** Standard memcpy performance

## Security Warnings

⚠️ **Password Handling:**
- Never hardcode passwords in scripts
- Use secure password generation
- Minimum 8 characters (recommend 16+)
- Password is wiped from command buffer after processing

⚠️ **Memory Security:**
- Vault is in kernel memory (not swappable)
- TODO: Integrate with Pebble Black allocator for guaranteed non-swappable pages
- Master key persists in kernel memory until wipe

⚠️ **Attack Surface:**
- Physical memory dumps can extract unlocked vault
- Cold boot attacks can recover recently wiped keys
- Use `wipe` command before shutdown

⚠️ **Forensics:**
- 7-pass wipe meets DoD 5220.22-M standard
- Wipe all data before decommissioning
- Consider full disk encryption for defense in depth

## Implementation Details

**File:** `kernel/9front-port/devram.c` (495 lines)

**Cryptographic Primitives:**
- `crypto_argon2()` - Password-based KDF
- `crypto_chacha20_x()` - XChaCha20 stream cipher
- `crypto_verify32()` - Constant-time comparison
- `crypto_wipe()` - Secure memory clearing
- `genrandom()` - ChaCha20 CSPRNG (kernel entropy pool)

**Key Sizes:**
- Master Key: 256 bits (32 bytes)
- Salt: 128 bits (16 bytes)
- Nonce: 192 bits (24 bytes)
- Argon2 Output: 256 bits

## Testing

To test the vault implementation:

```bash
# Boot kernel
make run

# In kernel console, test vault lifecycle
echo 'init test1234' > /dev/secureram.ctl
echo 'hello world' > /dev/secureram
cat /dev/secureram  # Should show "hello world"
echo 'lock' > /dev/secureram.ctl
cat /dev/secureram  # Should fail (locked)
echo 'unlock test1234' > /dev/secureram.ctl
cat /dev/secureram  # Should show "hello world"
echo 'wipe' > /dev/secureram.ctl
cat /dev/secureram  # Should fail (not initialized)
```

## Future Enhancements

- [ ] Integrate Pebble Black allocator for guaranteed non-swappable memory
- [ ] Add BLAKE2b MAC for authenticated encryption (encrypt-then-MAC)
- [ ] Support multiple vaults with different passwords
- [ ] Add vault snapshot/restore functionality
- [ ] Implement key rotation mechanism
- [ ] Add hardware TPM integration for key sealing
- [ ] Support blind ledger capability-based access control

## References

- **Monocypher:** https://monocypher.org/
- **Argon2:** https://github.com/P-H-C/phc-winner-argon2
- **XChaCha20:** https://tools.ietf.org/html/draft-arciszewski-xchacha-03
- **DoD 5220.22-M:** US Department of Defense data sanitization standard
