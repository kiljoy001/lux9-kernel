# Crypto Testing - Quick Reference

## Build Commands
```bash
# Clean build
make clean && make

# Build with ISO (for full testing)
make iso
```

## Quick Test Commands

### Automated Test Suite
```bash
./test_crypto.sh
```

### Manual Testing

**Software Crypto (No Acceleration):**
```bash
qemu-system-x86_64 \
  -cpu qemu64,-sha-ni,-aes,-pclmulqdq \
  -cdrom lux9.iso -boot d \
  -M q35 -m 2G \
  -display none -serial stdio
```

**Hardware Crypto (Full Acceleration):**
```bash
qemu-system-x86_64 \
  -cpu max \
  -cdrom lux9.iso -boot d \
  -M q35 -m 2G \
  -display none -serial stdio
```

**With KVM (Best Performance):**
```bash
qemu-system-x86_64 \
  -cpu host -accel kvm \
  -cdrom lux9.iso -boot d \
  -M q35 -m 2G \
  -display none -serial stdio
```

**SHA Only (Test Selective Features):**
```bash
qemu-system-x86_64 \
  -cpu qemu64,+sha-ni,-aes \
  -cdrom lux9.iso -boot d \
  -M q35 -m 2G \
  -display none -serial stdio
```

## Expected Output

### Boot Messages to Look For:

**Hardware Detected:**
```
cpuidentify: SHA extensions detected
cpuidentify: AES-NI detected
Crypto: SHA extensions available (hardware accelerated)
Crypto: AES-NI available (hardware accelerated)
```

**Software Fallback:**
```
Crypto: Using software SHA256
Crypto: Using software AES
```

**TPM Integration:**
```
Crypto: Initializing TPM-backed key storage...
Crypto: Generated TPM random key (generation 1)
Crypto: TPM key storage initialized
```

## Debugging

**Check Boot Log:**
```bash
timeout 10s qemu-system-x86_64 -cpu max -cdrom lux9.iso -boot d \
  -M q35 -m 2G -serial stdio 2>&1 | grep -E "(Crypto:|cpuidentify)"
```

**GDB Debugging:**
```bash
# Terminal 1:
qemu-system-x86_64 -cpu max -cdrom lux9.iso -boot d -M q35 -m 2G -s -S

# Terminal 2:
gdb lux9.elf
(gdb) target remote :1234
(gdb) break crypto_sha256
(gdb) break sha256_transform_hw
(gdb) continue
```

**Check Assembly Path:**
```bash
# Verify SHA256 hardware instructions are used
qemu-system-x86_64 -cpu max -cdrom lux9.iso -boot d \
  -d in_asm,cpu -D /tmp/qemu_trace.log

# Search for SHA256RNDS2 in trace:
grep SHA256RNDS2 /tmp/qemu_trace.log
```

## Test Logs

All test logs saved to:
- `/tmp/crypto_test_sw_fallback.log`
- `/tmp/crypto_test_hw_full.log`
- `/tmp/crypto_test_hw_sha_only.log`
- `/tmp/crypto_test_hw_aes_only.log`

View with:
```bash
cat /tmp/crypto_test_*.log | grep -E "(Crypto:|SHA|AES)"
```

## Common Issues

**Issue: "Error loading uncompressed kernel"**
- Solution: Use `-cdrom lux9.iso -boot d` instead of `-kernel lux9.elf`

**Issue: No crypto messages in output**
- Solution: Increase timeout, kernel may be slow to boot
- Check: `timeout 30s qemu-system-x86_64 ...`

**Issue: Hardware not detected in QEMU**
- Check CPU model: Use `-cpu max` or `-cpu host` with KVM
- Verify: `qemu-system-x86_64 -cpu max -cpu help | grep sha`

## File Locations

- **Source:** `kernel/crypto/crypto.c`, `kernel/crypto/hwcrypto.S`
- **Header:** `kernel/include/crypto.h`
- **Tests:** `test_crypto.sh`, `CRYPTO_TESTING_STRATEGY.md`
- **Docs:** `CRYPTO_IMPLEMENTATION_COMPLETE.md`

## API Usage Example

```c
#include "crypto.h"

void example_usage(void) {
    uint8_t hash[32];
    uint8_t hmac[32];
    const uint8_t *data = "Hello, World!";
    size_t len = 13;

    // SHA256 hash (auto HW/SW dispatch)
    crypto_sha256(hash, data, len);

    // TPM-backed HMAC (uses rotated key)
    crypto_tpm_hmac_sha256(hmac, data, len);

    // Check hardware availability
    if (crypto_hw_sha_available()) {
        print("Using hardware SHA256\n");
    }
}
```

## Performance Testing

```bash
# Compare software vs hardware performance
echo "=== Software ==="
timeout 20s qemu-system-x86_64 -cpu qemu64,-sha-ni \
  -cdrom lux9.iso -boot d -M q35 -m 2G -serial stdio 2>&1 | grep benchmark

echo "=== Hardware ==="
timeout 20s qemu-system-x86_64 -cpu max \
  -cdrom lux9.iso -boot d -M q35 -m 2G -serial stdio 2>&1 | grep benchmark
```

## SHA256 Test Vectors (for validation)

```
Input:  "abc"
SHA256: ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad

Input:  ""
SHA256: e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855
```

## Quick Health Check

```bash
# One-liner to verify crypto is working
timeout 15s qemu-system-x86_64 -cpu max -cdrom lux9.iso -boot d \
  -M q35 -m 2G -serial stdio 2>&1 | \
  grep -q "Crypto: SHA extensions available" && \
  echo "✅ Hardware crypto is working!" || \
  echo "❌ Hardware crypto not detected"
```
