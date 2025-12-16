#!/bin/bash

# Immediate Validation Script for C# Init System
# Provides quick feedback on system health in 5-15 minutes

set -e

echo "========================================"
echo "C# INIT SYSTEM IMMEDIATE VALIDATION"
echo "========================================"
echo "Timestamp: $(date)"
echo ""

# Function to test compilation
test_compilation() {
    local test_name=$1
    local source_file=$2
    
    echo "=== Testing $test_name ==="
    
    if [ ! -f "$source_file" ]; then
        echo "❌ FAIL: Source file $source_file not found"
        return 1
    fi
    
    # Quick syntax check with C# compiler
    if command -v csc >/dev/null 2>&1; then
        csc /t:library "$source_file" 2>/dev/null && echo "✅ PASS: C# compilation successful"
    elif command -v mcs >/dev/null 2>&1; then
        mcs -target:library "$source_file" 2>/dev/null && echo "✅ PASS: C# compilation successful"
    else
        echo "⚠️  WARN: C# compiler not available, syntax check skipped"
    fi
    
    # Check file size (should be reasonable)
    local size=$(wc -c < "$source_file")
    if [ $size -gt 50 ] && [ $size -lt 10000 ]; then
        echo "✅ PASS: File size reasonable ($size bytes)"
    else
        echo "❌ FAIL: File size suspicious ($size bytes)"
    fi
    
    # Basic syntax check with grep
    if grep -q "class.*Test" "$source_file" && grep -q "static void Main" "$source_file"; then
        echo "✅ PASS: Basic structure valid"
    else
        echo "❌ FAIL: Basic structure invalid"
    fi
    
    echo ""
}

# Function to validate ETL pipeline
test_etl_pipeline() {
    echo "=== ETL Pipeline Validation ==="
    
    # Check for key ETL components
    local etl_file="test_cs_init_etl_validation.cs"
    if [ -f "$etl_file" ]; then
        echo "✅ PASS: ETL validation test exists"
        
        # Check for pipeline stages
        if grep -q "F#.*→.*NET.*DLL.*→.*CIL.*→.*Fruity.*IR.*→.*QBE.*IL.*→.*x86-64" "$etl_file"; then
            echo "✅ PASS: Pipeline stages documented"
        else
            echo "❌ FAIL: Pipeline stages not documented"
        fi
        
        # Check for data transformation logic
        if grep -q "ProcessWithDotNetDLL\|TransformToCIL\|GenerateFruityIR\|CompileToQBE" "$etl_file"; then
            echo "✅ PASS: Pipeline transformation logic present"
        else
            echo "❌ FAIL: Pipeline transformation logic missing"
        fi
    else
        echo "❌ FAIL: ETL validation test not found"
    fi
    
    echo ""
}

# Function to test integration points
test_integration() {
    echo "=== Integration Points Validation ==="
    
    # Check for Lux9 system integration
    local integration_file="test_cs_init_integration.cs"
    if [ -f "$integration_file" ]; then
        echo "✅ PASS: Integration test exists"
        
        # Check for 9P filesystem integration
        if grep -q "P9FileSystem\|9P" "$integration_file"; then
            echo "✅ PASS: 9P filesystem integration present"
        else
            echo "❌ FAIL: 9P filesystem integration missing"
        fi
        
        # Check for AHCI storage integration
        if grep -q "AHCIStorage\|storage" "$integration_file"; then
            echo "✅ PASS: AHCI storage integration present"
        else
            echo "❌ FAIL: AHCI storage integration missing"
        fi
        
        # Check for crypto service integration
        if grep -q "CryptoService\|crypto" "$integration_file"; then
            echo "✅ PASS: Crypto service integration present"
        else
            echo "❌ FAIL: Crypto service integration missing"
        fi
    else
        echo "❌ FAIL: Integration test not found"
    fi
    
    echo ""
}

# Function to validate test coverage
test_coverage() {
    echo "=== Test Coverage Validation ==="
    
    local test_files=(
        "test_cs_init_smoke.cs"
        "test_cs_init_collections.cs"
        "test_cs_init_etl.cs"
        "test_cs_init_etl_validation.cs"
        "test_cs_init_controlflow.cs"
        "test_cs_init_boot.cs"
        "test_cs_init_integration.cs"
        "test_cs_init_error.cs"
        "test_cs_init_performance.cs"
        "test_cs_init_runner.cs"
    )
    
    local found=0
    local missing=0
    
    for file in "${test_files[@]}"; do
        if [ -f "$file" ]; then
            echo "✅ PASS: $file exists"
            ((found++))
        else
            echo "❌ FAIL: $file missing"
            ((missing++))
        fi
    done
    
    echo ""
    echo "Coverage: $found/${#test_files[@]} tests present"
    
    if [ $missing -eq 0 ]; then
        echo "✅ PASS: Complete test coverage"
    else
        echo "❌ FAIL: Missing $missing test files"
    fi
    
    echo ""
}

# Function to validate build system
test_build_system() {
    echo "=== Build System Validation ==="
    
    local makefile="Makefile.cs_tests"
    if [ -f "$makefile" ]; then
        echo "✅ PASS: Makefile exists"
        
        # Check for key targets
        if grep -q "smoke:\|etl:\|integration:\|performance:\|runner:" "$makefile"; then
            echo "✅ PASS: Key build targets present"
        else
            echo "❌ FAIL: Key build targets missing"
        fi
        
        # Check for quick build option
        if grep -q "quick:" "$makefile"; then
            echo "✅ PASS: Quick build option available"
        else
            echo "❌ FAIL: Quick build option missing"
        fi
        
        # Check for clean target
        if grep -q "clean:" "$makefile"; then
            echo "✅ PASS: Clean target available"
        else
            echo "❌ FAIL: Clean target missing"
        fi
    else
        echo "❌ FAIL: Makefile not found"
    fi
    
    echo ""
}

# Main validation flow
main() {
    echo "Starting immediate validation (should complete in ~5 minutes)..."
    echo ""
    
    # Test individual compilation
    test_compilation "Smoke Test" "test_cs_init_smoke.cs"
    test_compilation "Collections Test" "test_cs_init_collections.cs"
    test_compilation "ETL Test" "test_cs_init_etl.cs"
    test_compilation "Control Flow Test" "test_cs_init_controlflow.cs"
    test_compilation "Boot Test" "test_cs_init_boot.cs"
    test_compilation "Integration Test" "test_cs_init_integration.cs"
    test_compilation "Error Test" "test_cs_init_error.cs"
    test_compilation "Performance Test" "test_cs_init_performance.cs"
    test_compilation "ETL Validation" "test_cs_init_etl_validation.cs"
    test_compilation "Test Runner" "test_cs_init_runner.cs"
    
    # Test pipeline functionality
    test_etl_pipeline
    
    # Test integration points
    test_integration
    
    # Test coverage
    test_coverage
    
    # Test build system
    test_build_system
    
    echo "========================================"
    echo "VALIDATION COMPLETE"
    echo "========================================"
    echo ""
    echo "Next steps:"
    echo "1. Run 'make quick' to build essential tests"
    echo "2. Run 'make smoke' to test basic functionality"
    echo "3. Run 'make etl' to validate ETL pipeline"
    echo "4. Run 'make all && make test' for comprehensive testing"
    echo ""
    echo "For detailed analysis, see C_INIT_TESTS_README.md"
}

# Run validation
main "$@"
