#!/bin/bash
# Test TPM2 SAPI seal/unseal with swtpm

set -e

echo "=== TPM2 Seal/Unseal Test ==="
echo

# Clean up any existing swtpm
pkill -9 swtpm 2>/dev/null || true
rm -rf /tmp/tpm-test-$$

# Start swtpm
echo "[1] Starting swtpm TPM emulator..."
mkdir -p /tmp/tpm-test-$$
swtpm socket \
    --tpm2 \
    --tpmstate dir=/tmp/tpm-test-$$ \
    --ctrl type=unixio,path=/tmp/tpm-test-$$/swtpm-sock \
    --log file=/tmp/tpm-test-$$/swtpm.log,level=20 \
    --daemon

sleep 1

# Check if swtpm is running
if ! pgrep -f "swtpm socket" > /dev/null; then
    echo "ERROR: swtpm failed to start"
    exit 1
fi

echo "   ✓ swtpm running (PID=$(pgrep -f 'swtpm socket'))"
echo

# Build test if needed
if [ ! -f test/test_tpm_sapi ]; then
    echo "[2] Building test_tpm_sapi..."
    cd test && make test_tpm_sapi && cd ..
fi

echo "[3] Running TPM seal/unseal test..."
echo

# Run the test with swtpm
./test/test_tpm_sapi

RESULT=$?

# Cleanup
echo
echo "[4] Cleaning up..."
pkill -9 swtpm 2>/dev/null || true
rm -rf /tmp/tpm-test-$$

if [ $RESULT -eq 0 ]; then
    echo
    echo "=== TEST PASSED ✓ ==="
else
    echo
    echo "=== TEST FAILED ✗ ==="
fi

exit $RESULT
