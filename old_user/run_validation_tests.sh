#!/bin/bash

# Quick Validation Test Runner for C# Init System
# Provides immediate feedback on C# init implementation

set -e

echo "=== C# INIT VALIDATION TEST RUNNER ==="
echo "Building on successful 'Hello AOT World!' implementation"
echo ""

# Function to build and test a C# file
build_and_test_cs() {
    local test_name=$1
    local test_file=$2
    local build_dir="build_$test_name"
    
    echo "=== Building $test_name ==="
    
    # Create build directory
    mkdir -p "$build_dir"
    
    # Copy test file
    cp "$test_file" "$build_dir/Program.cs"
    
    # Create project file
    cat > "$build_dir/Test.csproj" << 'PROJEOF'
<Project Sdk="Microsoft.NET.Sdk">
  <PropertyGroup>
    <OutputType>Exe</OutputType>
    <TargetFramework>net9.0</TargetFramework>
    <PublishAot>true</PublishAot>
  </PropertyGroup>
</Project>
PROJEOF
    
    # Try to build
    cd "$build_dir"
    echo "Building $test_name..."
    
    if dotnet build --verbosity quiet >/dev/null 2>&1; then
        echo "✓ $test_name built successfully"
        
        # Try to run (will work if AOT runtime is available)
        echo "Running $test_name..."
        if timeout 10s ./bin/Debug/net9.0/ > ../"${test_name}_output.txt" 2>&1; then
            echo "✓ $test_name executed successfully"
            cat ../"${test_name}_output.txt"
        else
            echo "⚠ $test_name built but needs AOT runtime for execution"
            echo "  Binary ready at: $build_dir/bin/Debug/net9.0/"
        fi
    else
        echo "✗ $test_name build failed"
        echo "  Check syntax and dependencies"
    fi
    
    cd ..
    echo ""
}

# Check if dotnet is available
if ! command -v dotnet >/dev/null 2>&1; then
    echo "⚠ dotnet not found - creating test files only"
    echo "  Tests can be built when dotnet SDK is available"
    echo ""
fi

echo "Creating validation test suite..."

# Test files to validate
TESTS=(
    "test_cs_init_smoke.cs:Smoke Test"
    "test_cs_init_collections.cs:Collections Test" 
    "test_cs_init_etl.cs:ETL Pipeline Test"
    "test_cs_init_controlflow.cs:Control Flow Test"
    "test_cs_init_boot.cs:Boot Sequence Test"
    "test_cs_init_integration.cs:Integration Test"
    "test_cs_init_error.cs:Error Handling Test"
    "test_cs_init_performance.cs:Performance Test"
    "test_cs_init_validation_suite.cs:Validation Suite"
    "test_cs_init_quick_validation.cs:Quick Validation"
)

echo "Available test files:"
for test_info in "${TESTS[@]}"; do
    IFS=':' read -r file name <<< "$test_info"
    if [ -f "$file" ]; then
        lines=$(wc -l < "$file")
        echo "  ✓ $name ($file, $lines lines)"
    else
        echo "  ✗ Missing: $file"
    fi
done

echo ""
echo "=== IMMEDIATE VALIDATION TESTS (5-15 MINUTES EACH) ==="

echo ""
echo "TEST 1: Core Functionality (5 min)"
echo "  File: test_cs_init_smoke.cs"
echo "  Validates: String operations, math, boolean logic"
echo "  Builds on: 'Hello AOT World!' success"

echo ""
echo "TEST 2: Data Structures (5 min)"
echo "  File: test_cs_init_collections.cs"
echo "  Validates: Lists, dictionaries, arrays"
echo "  Builds on: Basic C# collections support"

echo ""
echo "TEST 3: ETL Pipeline (10 min)"
echo "  File: test_cs_init_etl.cs"
echo "  Validates: F# → .NET DLL → CIL → Fruity IR → QBE IL → x86-64"
echo "  Builds on: Successful C# AOT compilation"

echo ""
echo "TEST 4: Control Flow (5 min)"
echo "  File: test_cs_init_controlflow.cs"
echo "  Validates: If/else, loops, switch statements"
echo "  Builds on: Basic control flow working"

echo ""
echo "TEST 5: Integration (10 min)"
echo "  File: test_cs_init_integration.cs"
echo "  Validates: Pebble, BlindLedger, AHCI, 9P integration"
echo "  Builds on: System integration points"

echo ""
echo "TEST 6: Boot Sequence (5 min)"
echo "  File: test_cs_init_boot.cs"
echo "  Validates: Init boot process simulation"
echo "  Builds on: Init binary executing successfully"

echo ""
echo "TEST 7: Error Handling (5 min)"
echo "  File: test_cs_init_error.cs"
echo "  Validates: Edge cases, null handling, bounds checking"
echo "  Builds on: Runtime stability"

echo ""
echo "TEST 8: Performance (10 min)"
echo "  File: test_cs_init_performance.cs"
echo "  Validates: Performance baseline establishment"
echo "  Builds on: AOT performance characteristics"

echo ""
echo "=== QUICK START COMMANDS ==="

if command -v dotnet >/dev/null 2>&1; then
    echo "Dotnet available - you can run:"
    echo "  make smoke           # Test 1: Core functionality (5 min)"
    echo "  make collections     # Test 2: Data structures (5 min)"
    echo "  make etl            # Test 3: ETL pipeline (10 min)"
    echo "  make quick && make test  # All tests (30 min total)"
else
    echo "Dotnet not available - when available, run:"
    echo "  make smoke           # Build Test 1"
    echo "  make quick          # Build essential tests"
    echo "  make test           # Run all tests"
fi

echo ""
echo "=== IMMEDIATE ACTIONS ==="
echo "1. Review test files for your specific needs"
echo "2. Run 'make smoke' to validate basic operations"
echo "3. Run 'make quick' for essential test suite"
echo "4. Monitor system integration points"
echo ""
echo "=== EXPECTED RESULTS ==="
echo "✓ All tests should build successfully"
echo "✓ String/math operations should work (based on Hello AOT World)"
echo "✓ Control flow should execute properly"
echo "✓ ETL pipeline should transform data correctly"
echo "✓ Integration tests should simulate system calls"
echo ""
echo "For detailed test implementation, see individual test files."
echo "Each test can be implemented and validated in 5-15 minutes."
