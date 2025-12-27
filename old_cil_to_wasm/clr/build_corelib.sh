#!/bin/bash
set -e

# Build the CoreLib
echo "Building Lux9.CoreLib..."
dotnet build corelib/Lux9.CoreLib.csproj -c Release

# Locate the output file
OUTPUT_DLL="corelib/bin/Release/net9.0/Lux9.CoreLib.dll"

if [ -f "$OUTPUT_DLL" ]; then
    echo "Build successful: $OUTPUT_DLL"
    ls -lh $OUTPUT_DLL
else
    echo "Build failed: output DLL not found"
    exit 1
fi
