#!/bin/bash
set -e

BUILD_DIR="build"
INITRD_DIR="$BUILD_DIR/initrd"
TAR="$BUILD_DIR/initrd.tar"

# Ensure directories exist
mkdir -p $INITRD_DIR/bin $INITRD_DIR/lib $INITRD_DIR/dev $INITRD_DIR/etc $INITRD_DIR/srv

# Copy binaries
echo "Copying Go binaries..."
# Init (Go)
cp $BUILD_DIR/bin/init-go $INITRD_DIR/bin/init
# Shell (Go)
cp go-servers/sh/sh $INITRD_DIR/bin/sh
# Add other Go servers/drivers here if they exist and are pure Go

# 1. Check binaries for correct format (Plan 9, not ELF)
echo "Verifying binary formats..."
found_error=0
for bin in $INITRD_DIR/bin/*; do
    if file "$bin" | grep -q "ELF"; then
        echo "ERROR: $bin is an ELF binary! Expected Plan 9 executable."
        found_error=1
    else
        echo "OK: $bin seems valid."
    fi
done

if [ $found_error -eq 1 ]; then
    echo "Aborting initrd creation due to invalid binary formats."
    exit 1
fi

# 2. Create tar
echo "Creating $TAR..."
# Use distinct tar command to avoid issues
tar -C $INITRD_DIR -cf $TAR .

# 3. Sign initrd
echo "Signing $TAR..."
# Calculate SHA256
sha256sum $TAR > $TAR.sha256
# Mock signature (in real world, use a private key)
echo "MOCK_SIGNATURE_DATA_FOR_LUX9_INITRD" > $TAR.sig

echo "Initrd build and signing complete."
