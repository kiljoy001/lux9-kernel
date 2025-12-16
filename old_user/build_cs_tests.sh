#!/bin/bash

# Build script for C# Init Tests
# This script compiles C# tests and creates AOT binaries

set -e

echo "=== C# INIT TESTS BUILD SCRIPT ==="

# Configuration
DOTNET_VERSION="net9.0"
BUILD_DIR="test_builds"
mkdir -p $BUILD_DIR

# Test files to build
TESTS=(
    "test_cs_init_smoke.cs"
    "test_cs_init_collections.cs" 
    "test_cs_init_etl.cs"
    "test_cs_init_controlflow.cs"
    "test_cs_init_boot.cs"
    "test_cs_init_integration.cs"
    "test_cs_init_error.cs"
    "test_cs_init_performance.cs"
    "test_cs_init_runner.cs"
)

echo "Building individual test binaries..."

# Build each test
for test in "${TESTS[@]}"; do
    echo "Building $test..."
    
    # Create temporary project
    test_name=$(basename "$test" .cs)
    temp_dir="$BUILD_DIR/$test_name"
    mkdir -p "$temp_dir"
    
    # Create project file
    cat > "$temp_dir/Test.csproj" << 'PROJECT_EOF'
<Project Sdk="Microsoft.NET.Sdk">
  <PropertyGroup>
    <OutputType>Exe</OutputType>
    <TargetFramework>net9.0</TargetFramework>
    <PublishAot>true</PublishAot>
    <OptimizationLevel>Speed</OptimizationLevel>
  </PropertyGroup>
</Project>
PROJECT_EOF
    
    # Copy source
    cp "$test" "$temp_dir/Program.cs"
    
    # Build
    cd "$temp_dir"
    dotnet publish -c Release -r linux-x64 --self-contained false || echo "Build failed for $test_name"
    cd - > /dev/null
    
    # Copy output
    if [ -f "$temp_dir/bin/Release/net9.0/linux-x64/publish/Test" ]; then
        cp "$temp_dir/bin/Release/net9.0/linux-x64/publish/Test" "$BUILD_DIR/${test_name}_aot"
        echo "✓ Built $test_name AOT binary"
    else
        echo "✗ Failed to build $test_name"
    fi
done

echo "Build completed. Check $BUILD_DIR for results."
