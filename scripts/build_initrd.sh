#!/bin/bash
# Canonical initrd build script
# Uses init manifest database to ensure only verified binaries are used

set -e

REPO_ROOT="/home/scott/Repo/lux9-kernel"
INIT_DB="$REPO_ROOT/scripts/init_db.py"
INITRD_SOURCE="$REPO_ROOT/initrd"
BUILD_DIR="$REPO_ROOT/build"
INITRD_OUTPUT="$BUILD_DIR/initrd.tar"

# Initialize database if needed
if [ ! -f "$BUILD_DIR/init_manifest.sqlite" ]; then
    echo "Initializing init manifest database..."
    "$INIT_DB" init
fi

# Get latest verified init from database
echo "Fetching latest init from manifest database..."
INIT_BINARY=$("$INIT_DB" latest)

if [ -z "$INIT_BINARY" ]; then
    echo "ERROR: No init binary registered in database" >&2
    echo "Run: scripts/init_db.py register <init_binary>" >&2
    exit 1
fi

echo "Using init binary: $INIT_BINARY"
"$INIT_DB" verify "$INIT_BINARY"

# Copy init to initrd source
mkdir -p "$INITRD_SOURCE/boot"
cp "$INIT_BINARY" "$INITRD_SOURCE/boot/init"
chmod 755 "$INITRD_SOURCE/boot/init"

# Build initrd.tar (UNCOMPRESSED!)
echo "Building initrd.tar..."
mkdir -p "$BUILD_DIR"
tar -cf "$INITRD_OUTPUT" -C "$INITRD_SOURCE" boot

# Verify it's a valid tar
if ! tar -tf "$INITRD_OUTPUT" > /dev/null 2>&1; then
    echo "ERROR: Generated initrd.tar is invalid!" >&2
    exit 1
fi

echo "✅ Built initrd.tar: $INITRD_OUTPUT"
tar -tf "$INITRD_OUTPUT"
ls -lh "$INITRD_OUTPUT"

# For Makefile compatibility, also create initrd.tar.gz (but uncompressed!)
# The .gz extension is misleading but kept for backward compatibility
cp "$INITRD_OUTPUT" "$REPO_ROOT/initrd.tar.gz"
echo "✅ Created initrd.tar.gz (uncompressed, for Makefile compatibility)"
