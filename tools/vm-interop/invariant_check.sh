#!/bin/bash
# Runtime invariant checking for VM-Interop system
# Verifies that Coq proof properties hold during execution

set -euo pipefail

# File paths
COMMAND_FILE="/home/scott/Repo/VM-Interop/command.txt"
OUTPUT_FILE="/home/scott/Repo/VM-Interop/output.txt"
LOCK_FILE="/home/scott/Repo/VM-Interop/.lock"
INVARIANT_LOG="/home/scott/Repo/VM-Interop/invariant.log"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Log invariant status
log_invariant() {
    echo "[$(date '+%Y-%m-%d %H:%M:%S')] $1" >> "$INVARIANT_LOG"
}

# Check Invariant 1: commands_processed <= commands_sent
check_no_command_loss() {
    local sent=0
    local processed=0
    
    if [ -f "$COMMAND_FILE" ]; then
        sent=$(wc -l < "$COMMAND_FILE")
    fi
    
    if [ -f "$OUTPUT_FILE" ]; then
        processed=$(grep -c "===END===" "$OUTPUT_FILE" 2>/dev/null || echo 0)
    fi
    
    if [ "$processed" -le "$sent" ]; then
        echo -e "${GREEN}✓${NC} no_command_loss: processed($processed) <= sent($sent)"
        log_invariant "PASS: no_command_loss"
        return 0
    else
        echo -e "${RED}✗${NC} VIOLATION: processed($processed) > sent($sent)"
        log_invariant "FAIL: no_command_loss violation"
        return 1
    fi
}

# Check Invariant 2: Output file is append-only
check_append_only() {
    local size_file="/tmp/output_size_cache"
    local current_size=0
    local previous_size=0
    
    if [ -f "$OUTPUT_FILE" ]; then
        current_size=$(stat -c%s "$OUTPUT_FILE")
    fi
    
    if [ -f "$size_file" ]; then
        previous_size=$(cat "$size_file")
    fi
    
    echo "$current_size" > "$size_file"
    
    if [ "$current_size" -ge "$previous_size" ]; then
        echo -e "${GREEN}✓${NC} append_only: size($current_size) >= previous($previous_size)"
        log_invariant "PASS: append_only"
        return 0
    else
        echo -e "${RED}✗${NC} VIOLATION: output shrank from $previous_size to $current_size"
        log_invariant "FAIL: append_only violation"
        return 1
    fi
}

# Check Invariant 3: At most one lock holder
check_single_lock() {
    local lock_count=0
    
    if [ -f "$LOCK_FILE" ]; then
        lock_count=1
        local lock_age=$(($(date +%s) - $(stat -c%Y "$LOCK_FILE")))
        
        if [ "$lock_age" -gt 60 ]; then
            echo -e "${YELLOW}⚠${NC} Lock held for ${lock_age}s (possible deadlock)"
            log_invariant "WARN: lock held for ${lock_age}s"
        else
            echo -e "${GREEN}✓${NC} single_lock: 1 lock holder (age: ${lock_age}s)"
        fi
    else
        echo -e "${GREEN}✓${NC} single_lock: no lock held"
    fi
    
    return 0
}

# Check Invariant 4: Command-output pairing (atomicity)
check_atomic_pairing() {
    if [ ! -f "$OUTPUT_FILE" ]; then
        echo -e "${GREEN}✓${NC} atomic_pairing: no output yet"
        return 0
    fi
    
    local cmd_markers=$(grep -c "===CMD:" "$OUTPUT_FILE" 2>/dev/null || echo 0)
    local end_markers=$(grep -c "===END===" "$OUTPUT_FILE" 2>/dev/null || echo 0)
    
    if [ "$cmd_markers" -eq "$end_markers" ]; then
        echo -e "${GREEN}✓${NC} atomic_pairing: $cmd_markers commands properly paired"
        log_invariant "PASS: atomic_pairing"
        return 0
    else
        echo -e "${RED}✗${NC} VIOLATION: $cmd_markers CMD markers but $end_markers END markers"
        log_invariant "FAIL: atomic_pairing violation"
        return 1
    fi
}

# Check Invariant 5: Progress (liveness)
check_progress() {
    local state_file="/tmp/progress_state"
    local current_processed=0
    local previous_processed=0
    local current_sent=0
    local previous_sent=0
    
    if [ -f "$OUTPUT_FILE" ]; then
        current_processed=$(grep -c "===END===" "$OUTPUT_FILE" 2>/dev/null || echo 0)
    fi
    
    if [ -f "$COMMAND_FILE" ]; then
        current_sent=$(wc -l < "$COMMAND_FILE")
    fi
    
    if [ -f "$state_file" ]; then
        read previous_sent previous_processed < "$state_file" || true
    fi
    
    echo "$current_sent $current_processed" > "$state_file"
    
    if [ "$current_sent" -gt "$previous_sent" ]; then
        echo -e "${GREEN}✓${NC} progress: new commands sent ($previous_sent → $current_sent)"
    elif [ "$current_processed" -gt "$previous_processed" ]; then
        echo -e "${GREEN}✓${NC} progress: commands processed ($previous_processed → $current_processed)"
    elif [ "$current_sent" -eq "$current_processed" ]; then
        echo -e "${GREEN}✓${NC} progress: all commands processed"
    else
        local pending=$((current_sent - current_processed))
        echo -e "${YELLOW}⚠${NC} progress: $pending commands pending"
    fi
    
    return 0
}

# Continuous monitoring mode
monitor_mode() {
    echo "Starting invariant monitoring (Ctrl+C to stop)..."
    log_invariant "Monitoring started"
    
    while true; do
        clear
        echo "=== VM-Interop Invariant Monitor ==="
        echo "Time: $(date '+%Y-%m-%d %H:%M:%S')"
        echo ""
        
        local failures=0
        
        check_no_command_loss || ((failures++))
        check_append_only || ((failures++))
        check_single_lock || ((failures++))
        check_atomic_pairing || ((failures++))
        check_progress || ((failures++))
        
        echo ""
        if [ "$failures" -eq 0 ]; then
            echo -e "${GREEN}All invariants satisfied${NC}"
        else
            echo -e "${RED}$failures invariant violation(s) detected!${NC}"
        fi
        
        sleep 2
    done
}

# Single check mode
single_check() {
    echo "=== VM-Interop Invariant Check ==="
    echo "Time: $(date '+%Y-%m-%d %H:%M:%S')"
    echo ""
    
    local failures=0
    
    check_no_command_loss || ((failures++))
    check_append_only || ((failures++))
    check_single_lock || ((failures++))
    check_atomic_pairing || ((failures++))
    check_progress || ((failures++))
    
    echo ""
    if [ "$failures" -eq 0 ]; then
        echo -e "${GREEN}✓ All invariants satisfied - system is correct${NC}"
        log_invariant "All invariants satisfied"
        exit 0
    else
        echo -e "${RED}✗ $failures invariant violation(s) - system may be inconsistent${NC}"
        log_invariant "$failures invariant violations detected"
        exit 1
    fi
}

# Main
case "${1:-check}" in
    monitor)
        monitor_mode
        ;;
    check)
        single_check
        ;;
    *)
        echo "Usage: $0 [check|monitor]"
        echo "  check   - Single invariant check (default)"
        echo "  monitor - Continuous monitoring"
        exit 1
        ;;
esac