#!/bin/bash
# Seal the signing key to the TPM
# Usage: ./tools/tpm_seal_key.sh <hex_key_string> [output_file]

set -e

KEY_HEX="$1"
OUTPUT="${2:-root.priv.tpm}"

if [ -z "$KEY_HEX" ]; then
    echo "Usage: $0 <hex_key_string> [output_file]"
    echo "Example: $0 b6bff2..."
    exit 1
fi

# Convert hex key to binary
echo -n "$KEY_HEX" | xxd -r -p > key.bin

echo "Creating primary key (SRK)..."
tpm2_createprimary -C o -c primary.ctx

echo "Sealing key to TPM..."
# We seal it to the OWNER hierarchy (standard for user data)
# You can add -L sha256:0,1,2,3 to bind it to PCR state (Secure Boot)
tpm2_create -C primary.ctx -i key.bin -u key.pub -r key.priv
tpm2_load -C primary.ctx -u key.pub -r key.priv -c key.ctx
tpm2_evictcontrol -C o -c key.ctx 0x81010001 >/dev/null 2>&1 || true # Clear old handle if exists
tpm2_evictcontrol -C o -c key.ctx 0x81010001

echo "Key sealed and persisted at handle 0x81010001."
echo "To unseal during build, sign_manager.py will use this handle."

# Cleanup
rm -f key.bin key.pub key.priv primary.ctx key.ctx
echo "Done."
