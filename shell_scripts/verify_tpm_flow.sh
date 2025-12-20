#!/bin/bash
set -e

# Setup a local SWTPM for testing
TPM_DIR="/tmp/debug-tpm"
rm -rf "$TPM_DIR/*"
mkdir -p "$TPM_DIR/"

# Explicitly define data and control sockets
DATA_SOCK="$TPM_DIR/tpm.sock"
CTRL_SOCK="$TPM_DIR/tpm.sock.ctrl"

echo "Starting SWTPM for verification..."
swtpm socket --tpmstate dir="$TPM_DIR" \
             --tpm2 \
             --server type=unixio,path="$DATA_SOCK" \
             --ctrl type=unixio,path="$CTRL_SOCK" \
             --flags not-need-init,startup-clear \
             --log level=20 &
SWTPM_PID=$!
sleep 2

# Configure tpm2-tools to use swtpm TCTI
# The TCTI likely infers the control socket or needs explicit config.
# If using "swtpm", usually "path" points to the data socket.
export TPM2TOOLS_TCTI="swtpm:path=$DATA_SOCK"

echo "1. Startup"
tpm2_startup -c

echo "2. CreatePrimary (ECC NIST P256, AES-128-CFB)"
tpm2_createprimary -C o -g sha256 -G ecc -c primary.ctx > primary.out
cat primary.out

echo "3. Inspect Primary Attributes"
tpm2_readpublic -c primary.ctx

echo "4. Create (Seal Data)"
echo "12345678901234567890123456789012" > secret.data
tpm2_create -C primary.ctx -i secret.data -u key.pub -r key.priv

echo "5. Load"
tpm2_load -C primary.ctx -u key.pub -r key.priv -c key.ctx

echo "6. Unseal"
tpm2_unseal -c key.ctx > unsealed.data

diff secret.data unsealed.data && echo "SUCCESS: Data verified"

# Cleanup
rm -f primary.ctx key.pub key.priv key.ctx secret.data unsealed.data primary.out
kill $SWTPM_PID
rm -rf "$TPM_DIR"
