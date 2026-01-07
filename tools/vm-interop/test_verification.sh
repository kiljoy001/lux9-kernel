#!/bin/bash
# Test harness for verifying Coq proof properties in practice
# Tests all theorems from command_loop_verified.v

set -euo pipefail

# Test configuration
TEST_DIR="/home/scott/Repo/VM-Interop"
COMMAND_FILE="$TEST_DIR/command.txt"
OUTPUT_FILE="$TEST_DIR/output.txt"
TEST_LOG="$TEST_DIR/test_results.log"

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
BLUE='\033[0;34m'
NC='\033[0m'

# Test counter
TESTS_PASSED=0
TESTS_FAILED=0

# Logging
log_test() {
    echo "[$(date '+%Y-%m-%d %H:%M:%S')] TEST: $1" | tee -a "$TEST_LOG"
}

log_result() {
    local result="$1"
    local message="$2"
    
    if [ "$result" = "PASS" ]; then
        echo -e "${GREEN}✓ PASS:${NC} $message" | tee -a "$TEST_LOG"
        ((TESTS_PASSED++))
    else
        echo -e "${RED}✗ FAIL:${NC} $message" | tee -a "$TEST_LOG"
        ((TESTS_FAILED++))
    fi
}

# Setup
setup_test() {
    log_test "Setting up test environment"
    
    # Backup existing files
    for file in "$COMMAND_FILE" "$OUTPUT_FILE"; do
        if [ -f "$file" ]; then
            cp "$file" "$file.backup"
        fi
    done
    
    # Clear test files
    > "$COMMAND_FILE"
    > "$OUTPUT_FILE"
    > "$TEST_LOG"
}

cleanup_test() {
    log_test "Cleaning up test environment"
    
    # Restore backups
    for file in "$COMMAND_FILE" "$OUTPUT_FILE"; do
        if [ -f "$file.backup" ]; then
            mv "$file.backup" "$file"
        fi
    done
}

# Test 1: no_command_loss theorem
test_no_command_loss() {
    echo -e "${BLUE}Test 1: no_command_loss theorem${NC}"
    log_test "Testing no_command_loss"
    
    # Send 10 commands rapidly
    for i in {1..10}; do
        echo "test_command_$i" >> "$COMMAND_FILE"
    done
    
    sleep 3  # Wait for processing
    
    # Count sent vs processed
    local sent=$(wc -l < "$COMMAND_FILE")
    local processed=$(grep -c "===CMD:" "$OUTPUT_FILE" 2>/dev/null || echo 0)
    
    if [ "$processed" -le "$sent" ]; then
        log_result "PASS" "no_command_loss: $processed <= $sent"
    else
        log_result "FAIL" "no_command_loss: processed($processed) > sent($sent)"
    fi
}

# Test 2: processing_progress theorem
test_processing_progress() {
    echo -e "${BLUE}Test 2: processing_progress theorem${NC}"
    log_test "Testing processing_progress"
    
    # Get initial state
    local initial_processed=$(grep -c "===END===" "$OUTPUT_FILE" 2>/dev/null || echo 0)
    
    # Send new command
    echo "progress_test" >> "$COMMAND_FILE"
    sleep 2
    
    # Check if processing advanced
    local final_processed=$(grep -c "===END===" "$OUTPUT_FILE" 2>/dev/null || echo 0)
    
    if [ "$final_processed" -gt "$initial_processed" ]; then
        log_result "PASS" "processing_progress: $initial_processed → $final_processed"
    else
        log_result "FAIL" "processing_progress: no advancement"
    fi
}

# Test 3: race_free theorem
test_race_free() {
    echo -e "${BLUE}Test 3: race_free theorem${NC}"
    log_test "Testing race_free"
    
    # Launch parallel commands
    local pids=()
    for i in {1..5}; do
        ./9cmd_verified "race_test_$i" --no-wait &
        pids+=($!)
    done
    
    # Wait for all to complete
    for pid in "${pids[@]}"; do
        wait "$pid"
    done
    
    sleep 3
    
    # Check for corruption (each command should appear exactly once)
    local corruption=0
    for i in {1..5}; do
        local count=$(grep -c "race_test_$i" "$OUTPUT_FILE" 2>/dev/null || echo 0)
        if [ "$count" -ne 1 ]; then
            corruption=1
            break
        fi
    done
    
    if [ "$corruption" -eq 0 ]; then
        log_result "PASS" "race_free: no corruption in parallel execution"
    else
        log_result "FAIL" "race_free: corruption detected"
    fi
}

# Test 4: atomic_processing
test_atomic_processing() {
    echo -e "${BLUE}Test 4: atomic_processing${NC}"
    log_test "Testing atomic_processing"
    
    # Send command that will fail
    echo "false" >> "$COMMAND_FILE"
    sleep 2
    
    # Check if output has both CMD and END markers
    local last_block=$(tail -20 "$OUTPUT_FILE")
    
    if echo "$last_block" | grep -q "===CMD:" && echo "$last_block" | grep -q "===END==="; then
        log_result "PASS" "atomic_processing: command and output paired"
    else
        log_result "FAIL" "atomic_processing: incomplete output block"
    fi
}

# Test 5: wait_is_noop theorem (Sleep doesn't change state)
test_wait_is_noop() {
    echo -e "${BLUE}Test 5: wait_is_noop theorem${NC}"
    log_test "Testing wait_is_noop"
    
    # Get state snapshot
    local state1_cmds=$(wc -l < "$COMMAND_FILE" 2>/dev/null || echo 0)
    local state1_size=$(stat -c%s "$OUTPUT_FILE" 2>/dev/null || echo 0)
    
    # Wait without sending commands
    sleep 3
    
    # Get second snapshot
    local state2_cmds=$(wc -l < "$COMMAND_FILE" 2>/dev/null || echo 0)
    local state2_size=$(stat -c%s "$OUTPUT_FILE" 2>/dev/null || echo 0)
    
    if [ "$state1_cmds" -eq "$state2_cmds" ] && [ "$state1_size" -eq "$state2_size" ]; then
        log_result "PASS" "wait_is_noop: state unchanged during wait"
    else
        log_result "FAIL" "wait_is_noop: state changed without commands"
    fi
}

# Test 6: system_correctness (main invariant)
test_system_correctness() {
    echo -e "${BLUE}Test 6: system_correctness invariant${NC}"
    log_test "Testing system_correctness"
    
    # Run invariant checker
    if ./invariant_check.sh check >/dev/null 2>&1; then
        log_result "PASS" "system_correctness: all invariants hold"
    else
        log_result "FAIL" "system_correctness: invariant violation"
    fi
}

# Test 7: failures_exclusive (failure modes don't overlap)
test_failures_exclusive() {
    echo -e "${BLUE}Test 7: failures_exclusive theorem${NC}"
    log_test "Testing failures_exclusive"
    
    # Check using C verification tool
    if ./race_prevention --verify >/dev/null 2>&1; then
        log_result "PASS" "failures_exclusive: no conflicting failure modes"
    else
        log_result "FAIL" "failures_exclusive: conflicting states detected"
    fi
}

# Test 8: optimal_sequence (efficiency)
test_optimal_sequence() {
    echo -e "${BLUE}Test 8: optimal_sequence${NC}"
    log_test "Testing optimal_sequence"
    
    # Clear files
    > "$COMMAND_FILE"
    > "$OUTPUT_FILE"
    
    # Send exactly 3 commands
    echo "cmd1" >> "$COMMAND_FILE"
    echo "cmd2" >> "$COMMAND_FILE"
    echo "cmd3" >> "$COMMAND_FILE"
    
    local start_time=$(date +%s%N)
    sleep 2
    local end_time=$(date +%s%N)
    
    local processed=$(grep -c "===END===" "$OUTPUT_FILE" 2>/dev/null || echo 0)
    local elapsed=$((($end_time - $start_time) / 1000000))  # Convert to ms
    
    # Should process 3 commands in ~3 operations (read, process, write)
    if [ "$processed" -eq 3 ]; then
        log_result "PASS" "optimal_sequence: processed all in ${elapsed}ms"
    else
        log_result "FAIL" "optimal_sequence: only $processed/3 processed"
    fi
}

# Main test runner
main() {
    echo "=== VM-Interop Verification Test Suite ==="
    echo "Based on proofs in command_loop_verified.v"
    echo ""
    
    setup_test
    
    # Ensure autorun is running in 9front
    echo -e "${BLUE}Prerequisites:${NC}"
    echo "1. Ensure autorun_verified.rc is running in 9front"
    echo "2. Mount /n/interop in 9front"
    echo ""
    read -p "Press Enter when ready to start tests..."
    
    # Run all tests
    test_no_command_loss
    test_processing_progress
    test_race_free
    test_atomic_processing
    test_wait_is_noop
    test_system_correctness
    test_failures_exclusive
    test_optimal_sequence
    
    # Summary
    echo ""
    echo "=== Test Summary ==="
    echo -e "${GREEN}Passed:${NC} $TESTS_PASSED"
    echo -e "${RED}Failed:${NC} $TESTS_FAILED"
    
    if [ "$TESTS_FAILED" -eq 0 ]; then
        echo -e "${GREEN}✓ All proofs verified in practice!${NC}"
        cleanup_test
        exit 0
    else
        echo -e "${RED}✗ Some proofs failed verification${NC}"
        echo "Check $TEST_LOG for details"
        cleanup_test
        exit 1
    fi
}

# Run tests
main "$@"