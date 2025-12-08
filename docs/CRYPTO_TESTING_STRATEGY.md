# Hardware Crypto Testing Strategy for QEMU

## Overview

This document describes the testing strategy for hardware-accelerated cryptography (SHA256, AES-NI, PCLMULQDQ) in the Lux9 kernel using QEMU.

## 1. QEMU CPU Feature Control

QEMU allows enabling/disabling CPU features to test both hardware and software crypto paths.

### Test Configuration 1: No Hardware Acceleration (Software Fallback)
```bash
qemu-system-x86_64 \
    -M q35 \
    -m 2G \
    -kernel lux9.elf \
    -no-reboot \
    -display none \
    -serial stdio \
    -cpu qemu64,-sha-ni,-aes,-pclmulqdq
```

**Expected Behavior:**
- Boot messages show: "Crypto: Using software SHA256"
- Boot messages show: "Crypto: Using software AES"
- `cpuidentify` should NOT detect SHA extensions
- Crypto operations use sphlib software implementation

### Test Configuration 2: Full Hardware Acceleration
```bash
qemu-system-x86_64 \
    -M q35 \
    -m 2G \
    -kernel lux9.elf \
    -no-reboot \
    -display none \
    -serial stdio \
    -cpu max
```

**Expected Behavior:**
- Boot messages show: "Crypto: SHA extensions available (hardware accelerated)"
- Boot messages show: "Crypto: AES-NI available (hardware accelerated)"
- `cpuidentify` detects SHA extensions
- Crypto operations use hardware instructions

### Test Configuration 3: Selective Feature Testing
```bash
# SHA only
qemu-system-x86_64 -cpu qemu64,+sha-ni,-aes,-pclmulqdq -kernel lux9.elf ...

# AES only
qemu-system-x86_64 -cpu qemu64,-sha-ni,+aes,-pclmulqdq -kernel lux9.elf ...

# All features individually
qemu-system-x86_64 -cpu qemu64,+sha-ni,+aes,+pclmulqdq -kernel lux9.elf ...
```

## 2. Boot-Time Testing

### Automated Boot Test Script

```bash
#!/bin/bash
# test_crypto_boot.sh - Automated crypto boot testing

KERNEL="lux9.elf"
TIMEOUT=10

test_config() {
    local name="$1"
    local cpu="$2"
    local expected="$3"

    echo "=== Testing: $name ==="
    timeout ${TIMEOUT}s qemu-system-x86_64 \
        -M q35 -m 2G \
        -kernel $KERNEL \
        -no-reboot \
        -display none \
        -serial stdio \
        -cpu "$cpu" 2>&1 | tee /tmp/crypto_test_${name}.log

    if grep -q "$expected" /tmp/crypto_test_${name}.log; then
        echo "✓ PASS: $name - Found '$expected'"
    else
        echo "✗ FAIL: $name - Expected '$expected' not found"
        return 1
    fi
}

# Test 1: Software fallback
test_config "software" "qemu64,-sha-ni,-aes" "Using software SHA256"

# Test 2: Hardware SHA
test_config "hw_sha" "qemu64,+sha-ni,-aes" "SHA extensions available"

# Test 3: Hardware AES
test_config "hw_aes" "qemu64,-sha-ni,+aes" "AES-NI available"

# Test 4: Full hardware
test_config "full_hw" "max" "SHA extensions available"

echo "=== Test Summary ==="
echo "Check /tmp/crypto_test_*.log for detailed output"
```

## 3. Runtime Functional Testing

### SHA256 Test Vectors

Add to kernel test code (e.g., in `main()` after crypto init):

```c
void crypto_test_sha256(void)
{
    uint8_t out[32];
    const char *test_vectors[][2] = {
        /* Input -> Expected SHA256 */
        {"abc",
         "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad"},
        {"",
         "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855"},
        {"abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq",
         "248d6a61d20638b8e5c026930c3e6039a33ce45964ff2167f6ecedd419db06c1"},
    };

    for (int i = 0; i < 3; i++) {
        crypto_sha256(out, (uint8_t*)test_vectors[i][0],
                     strlen(test_vectors[i][0]));

        /* Convert to hex and compare */
        char hex[65];
        for (int j = 0; j < 32; j++)
            sprint(hex + j*2, "%02x", out[j]);
        hex[64] = '\0';

        if (strcmp(hex, test_vectors[i][1]) == 0) {
            print("SHA256 test %d: PASS\n", i);
        } else {
            print("SHA256 test %d: FAIL\n", i);
            print("  Expected: %s\n", test_vectors[i][1]);
            print("  Got:      %s\n", hex);
        }
    }
}
```

### HMAC-SHA256 Test Vectors (RFC 4231)

```c
void crypto_test_hmac(void)
{
    uint8_t out[32];

    /* Test Case 1: RFC 4231 */
    uint8_t key1[20];
    memset(key1, 0x0b, 20);
    const char *data1 = "Hi There";
    const char *expect1 =
        "b0344c61d8db38535ca8afceaf0bf12b881dc200c9833da726e9376c2e32cff7";

    crypto_hmac_sha256(out, key1, 20, (uint8_t*)data1, strlen(data1));

    char hex[65];
    for (int i = 0; i < 32; i++)
        sprint(hex + i*2, "%02x", out[i]);
    hex[64] = '\0';

    if (strcmp(hex, expect1) == 0) {
        print("HMAC-SHA256 test 1: PASS\n");
    } else {
        print("HMAC-SHA256 test 1: FAIL\n");
        print("  Expected: %s\n", expect1);
        print("  Got:      %s\n", hex);
    }
}
```

## 4. Performance Testing

### Benchmark Script

```c
void crypto_benchmark(void)
{
    uint8_t data[1024 * 1024];  /* 1 MB test data */
    uint8_t out[32];
    uint64_t start, end;
    int iterations = 100;

    /* Fill test data */
    for (int i = 0; i < sizeof(data); i++)
        data[i] = i & 0xFF;

    /* Benchmark SHA256 */
    start = fastticks(nil);
    for (int i = 0; i < iterations; i++) {
        crypto_sha256(out, data, sizeof(data));
    }
    end = fastticks(nil);

    uint64_t ticks_per_iter = (end - start) / iterations;
    print("SHA256 benchmark: %llu ticks/MB\n", ticks_per_iter);
    print("Hardware accel: %s\n",
          crypto_hw_sha_available() ? "YES" : "NO");
}
```

### QEMU Performance Comparison

```bash
#!/bin/bash
# benchmark_crypto.sh

echo "=== Software Implementation ==="
timeout 30s qemu-system-x86_64 \
    -cpu qemu64,-sha-ni \
    -kernel lux9.elf -m 2G -M q35 \
    -display none -serial stdio 2>&1 | grep "SHA256 benchmark"

echo ""
echo "=== Hardware Implementation ==="
timeout 30s qemu-system-x86_64 \
    -cpu max \
    -kernel lux9.elf -m 2G -M q35 \
    -display none -serial stdio 2>&1 | grep "SHA256 benchmark"
```

**Expected Results:**
- Hardware should be 2-5x faster than software
- Actual speedup depends on QEMU's TCG vs KVM mode

## 5. KVM vs TCG Testing

### TCG (Software Emulation)
```bash
qemu-system-x86_64 -cpu max -accel tcg -kernel lux9.elf ...
```
- Slower overall
- CPU features emulated
- Works on any host architecture

### KVM (Hardware Virtualization)
```bash
qemu-system-x86_64 -cpu host -accel kvm -kernel lux9.elf ...
```
- Much faster
- Uses actual host CPU features
- Requires KVM support on host

**Testing Strategy:**
1. Use TCG for feature detection testing (reproducible)
2. Use KVM for performance testing (realistic)

## 6. Debugging Tests

### GDB with QEMU

```bash
# Terminal 1: Start QEMU with GDB stub
qemu-system-x86_64 \
    -cpu max \
    -kernel lux9.elf \
    -m 2G -M q35 \
    -display none \
    -serial stdio \
    -s -S

# Terminal 2: Connect GDB
gdb lux9.elf
(gdb) target remote :1234
(gdb) break crypto_sha256
(gdb) break sha256_transform_hw
(gdb) continue
```

### Trace CPU Instructions

```bash
qemu-system-x86_64 \
    -cpu max,+sha-ni \
    -kernel lux9.elf \
    -d in_asm,cpu \
    -D /tmp/qemu_trace.log
```

This logs all executed instructions - search for SHA256RNDS2, SHA256MSG1, SHA256MSG2 to verify hardware path is taken.

## 7. TPM Testing

### Virtual TPM with QEMU

```bash
# Start software TPM emulator
swtpm socket --tpmstate dir=/tmp/tpm --ctrl type=unixio,path=/tmp/tpm.sock &

# Run QEMU with TPM
qemu-system-x86_64 \
    -cpu max \
    -kernel lux9.elf \
    -m 2G -M q35 \
    -chardev socket,id=chrtpm,path=/tmp/tpm.sock \
    -tpmdev emulator,id=tpm0,chardev=chrtpm \
    -device tpm-tis,tpmdev=tpm0
```

**Test TPM Integration:**
- Verify `tpm_get_random()` returns different values on each boot
- Test key rotation: Check generation counter increments
- Verify key storage: Same key should be returned within a session

## 8. Automated Test Suite

### Complete Test Runner

```bash
#!/bin/bash
# run_all_crypto_tests.sh

set -e

KERNEL="lux9.elf"
RESULTS="/tmp/crypto_test_results.txt"

echo "Lux9 Crypto Test Suite" > $RESULTS
echo "======================" >> $RESULTS
date >> $RESULTS
echo "" >> $RESULTS

test_run() {
    local name="$1"
    local cpu="$2"
    local check="$3"

    echo -n "Testing $name... "

    timeout 15s qemu-system-x86_64 \
        -M q35 -m 2G \
        -kernel $KERNEL \
        -no-reboot \
        -display none \
        -serial stdio \
        -cpu "$cpu" > /tmp/test_${name}.log 2>&1 || true

    if grep -q "$check" /tmp/test_${name}.log; then
        echo "PASS" | tee -a $RESULTS
        return 0
    else
        echo "FAIL" | tee -a $RESULTS
        return 1
    fi
}

# Run tests
test_run "boot_basic" "qemu64" "Lux9"
test_run "crypto_init" "qemu64" "Crypto: TPM key storage initialized"
test_run "sw_sha256" "qemu64,-sha-ni" "Using software SHA256"
test_run "hw_sha256" "max" "SHA extensions available"
test_run "hw_aes" "max" "AES-NI available"

# Summary
echo "" >> $RESULTS
echo "Test logs in /tmp/test_*.log" >> $RESULTS
cat $RESULTS
```

## 9. Integration with Existing Build System

Add to `Makefile`:

```makefile
.PHONY: test-crypto test-crypto-hw test-crypto-sw

test-crypto: $(KERNEL)
	@./test_crypto_boot.sh

test-crypto-hw: $(KERNEL)
	@echo "Testing hardware crypto..."
	@timeout 15s qemu-system-x86_64 -cpu max -kernel $(KERNEL) \
		-M q35 -m 2G -display none -serial stdio | \
		grep -E "(SHA extensions|AES-NI)"

test-crypto-sw: $(KERNEL)
	@echo "Testing software crypto fallback..."
	@timeout 15s qemu-system-x86_64 -cpu qemu64,-sha-ni,-aes \
		-kernel $(KERNEL) -M q35 -m 2G -display none -serial stdio | \
		grep "Using software"
```

## 10. Expected Test Output

### Successful Hardware Boot:
```
cpuidentify: checking crypto acceleration
cpuidentify: AES-NI detected
cpuidentify: SHA extensions detected
Crypto: Initializing TPM-backed key storage...
Crypto: SHA extensions available (hardware accelerated)
Crypto: AES-NI available (hardware accelerated)
Crypto: Generated TPM random key (generation 1)
Crypto: TPM key storage initialized
SHA256 test 0: PASS
SHA256 test 1: PASS
SHA256 test 2: PASS
HMAC-SHA256 test 1: PASS
SHA256 benchmark: 1234567 ticks/MB
Hardware accel: YES
```

### Successful Software Fallback Boot:
```
cpuidentify: checking crypto acceleration
Crypto: Initializing TPM-backed key storage...
Crypto: Using software SHA256
Crypto: Using software AES
Crypto: TPM random not available, using fastticks entropy
Crypto: TPM key storage initialized
SHA256 test 0: PASS
SHA256 test 1: PASS
SHA256 test 2: PASS
HMAC-SHA256 test 1: PASS
SHA256 benchmark: 4567890 ticks/MB
Hardware accel: NO
```

## 11. Continuous Integration

For automated CI/CD:

```yaml
# .github/workflows/crypto-tests.yml
name: Crypto Tests

on: [push, pull_request]

jobs:
  test:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v2
      - name: Install QEMU
        run: sudo apt-get install -y qemu-system-x86
      - name: Build kernel
        run: make
      - name: Test software crypto
        run: make test-crypto-sw
      - name: Test hardware crypto
        run: make test-crypto-hw
```

## Summary

This testing strategy provides:
- ✅ Feature detection validation
- ✅ Functional correctness (test vectors)
- ✅ Performance comparison
- ✅ Hardware/software path verification
- ✅ TPM integration testing
- ✅ Debugging capabilities
- ✅ Automated CI/CD integration

All tests can run in QEMU without requiring actual hardware, making them suitable for continuous integration and development workflows.
