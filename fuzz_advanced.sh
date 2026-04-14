#!/bin/bash
# Advanced fuzzing for resurrection crash
# Targets the specific cclose/poolfreel race condition

echo "=== ADVANCED RESURRECTION FUZZING ==="
echo "Target: cclose/poolfreel race in namec/sysexec path"
echo "Method: QEMU randomization + filesystem timing variations"
echo

# Initialize results tracking
RESULTS_FILE="kernel/fuzz_advanced_results.log"
echo "=== Advanced Fuzzing Session - $(date) ===" > $RESULTS_FILE

# Set up randomization
export QEMU_A9_RANDOM_SEED=$RANDOM
export QEMU_RANDOM_DUMP="on"

run_advanced_fuzz() {
    local iteration=$1
    local variation=$2
    
    LOG_FILE="kernel/fuzz_adv_${variation}_${iteration}.log"
    VAR_ARGS=""
    
    # Variation 1: Memory pressure + timing
    if [ $variation -eq 1 ]; then
        VAR_ARGS="-m 384M -rtc-td-hack -global PIIX4_PM.disable_s3=1 -global PIIX4_PM.disable_s4=1"
    
    # Variation 2: CPU stress + KVM
    elif [ $variation -eq 2 ]; then
        VAR_ARGS="-cpu max,+sse4.1,+sse4.2,+avx -accel kvm -enable-kvm"
    
    # Variation 3: Disk I/O timing
    elif [ $variation -eq 3 ]; then
        VAR_ARGS="-drive if=ide,index=0,media=cdrom,format=raw,readonly=on -device qxl-vga"
    
    # Variation 4: Network/PCI randomization
    elif [ $variation -eq 4 ]; then
        VAR_ARGS="-device e1000 -global i440FX-pciaddr=0x1e -no-hpet -no-vmmouse"
    fi
    
    # Execute with timeout and capture results
    timeout 8s qemu-system-x86_64 \
        -cdrom kernel/lux9.iso \
        -boot d \
        -M q35 \
        $VAR_ARGS \
        -display none \
        -serial file:$LOG_FILE \
        -no-reboot -no-shutdown \
        -no-shutdown \
        2>/dev/null
    
    # Immediate analysis for crash patterns
    if [ -f "$LOG_FILE" ]; then
        local log_size=$(stat -c%s "$LOG_FILE" 2>/dev/null || stat -f%z "$LOG_FILE" 2>/dev/null || echo "0")
        
        # Check for the specific crash sequence
        if strings "$LOG_FILE" 2>/dev/null | grep -q "poolfreel.*low v=10" && \
           strings "$LOG_FILE" 2>/dev/null | grep -q "D2B.*FATAL"; then
            echo "*** CRASH REPRODUCED! Iteration $iteration, variation $variation ***" | tee -a $RESULTS_FILE
            echo "Log: $LOG_FILE ($log_size bytes)" | tee -a $RESULTS_FILE
            strings "$LOG_FILE" | grep -A 5 -B 5 "poolfreel.*low v=10" >> $RESULTS_FILE
            return 1
        fi
        
        # Check progress
        if [ $((iteration % 25)) -eq 0 ]; then
            echo "Progress: $iteration iterations, variation $variation, log: $log_size bytes"
            if [ $log_size -gt 70000 ]; then
                strings "$LOG_FILE" | grep -q "SYSCALL.*exec successful" && echo "  -> Exec successful" || echo "  -> Incomplete"
            fi
        fi
    fi
    
    return 0
}

# Run focused fuzzing campaigns
echo "Starting focused fuzzing campaign..."
echo

for variation in 1 2 3 4; do
    echo "=== Variation $variation ==="
    for i in {1..50}; do
        if ! run_advanced_fuzz $i $variation; then
            echo "CRASH REPRODUCED! Stopping campaign."
            exit 0
        fi
    done
    echo "Variation $variation completed (50 iterations)"
done

echo
echo "=== INTENSIVE MODE ==="
echo "Running intensive cycles with maximum randomization..."

# Intensive mode: rapid cycles with maximum randomization
for i in {1..100}; do
    # Random variation selection
    variation=$((RANDOM % 4 + 1))
    
    # Add extra randomization
    export QEMU_A9_RANDOM_SEED=$((RANDOM + i))
    
    if ! run_advanced_fuzz $i $((variation + 10)); then
        echo "CRASH REPRODUCED in intensive mode!"
        break
    fi
    
    # Progress reporting
    if [ $((i % 20)) -eq 0 ]; then
        echo "Intensive mode: $i/100 iterations completed"
    fi
done

echo
echo "=== FUZZING SESSION COMPLETE ==="
echo "Results saved to: $RESULTS_FILE"

# Summary
echo
echo "SUMMARY:"
echo "  Total iterations: 300"
echo "  Variations tested: 4 memory/CPU/timing patterns"
echo "  Results: Check $RESULTS_FILE for details"
echo "  Logs: kernel/fuzz_adv_*.log"

# Final crash check
echo
echo "Checking all logs for crash patterns..."
for log in kernel/fuzz_adv_*.log; do
    if [ -f "$log" ]; then
        if strings "$log" 2>/dev/null | grep -q "poolfreel.*low v=10"; then
            echo "FOUND CRASH in $log"
            strings "$log" | grep -A 3 -B 3 "poolfreel.*low v=10"
        fi
    fi
done