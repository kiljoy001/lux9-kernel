#!/bin/bash

# Build script for userspace drivers

echo "Building userspace drivers..."

# Check if Go is available
if ! command -v go &> /dev/null; then
    echo "Warning: Go not found. Skipping driver compilation."
    echo "Please install Go to build userspace drivers."
    exit 0
fi

# Create bin directory if it doesn't exist
mkdir -p bin

# Build AHCI driver
echo "Building AHCI driver..."
go build -o bin/ahci_driver storage/ahci_main.go
if [ $? -eq 0 ]; then
    echo "AHCI driver built successfully"
else
    echo "Failed to build AHCI driver"
    exit 1
fi

# Build IDE driver
echo "Building IDE driver..."
go build -o bin/ide_driver storage/ide_main.go
if [ $? -eq 0 ]; then
    echo "IDE driver built successfully"
else
    echo "Failed to build IDE driver"
    exit 1
fi

echo "Drivers built successfully!"
echo "Binaries located in bin/ directory:"
ls -la bin/

# Install to system (optional)
if [ "$1" = "install" ]; then
    echo "Installing drivers to /usr/local/bin..."
    sudo mkdir -p /usr/local/bin
    sudo cp bin/ahci_driver /usr/local/bin/
    sudo cp bin/ide_driver /usr/local/bin/
    echo "Drivers installed successfully!"
fi
