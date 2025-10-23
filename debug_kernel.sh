#!/bin/bash
# debug_kernel.sh - Complete debug script that runs QEMU and GDB together

echo "=== Lux9 Kernel MMU Debugging ==="
echo "This will run QEMU and GDB automatically"
echo ""

# Clean up previous logs
rm -f mmu_debug.log qemu_debug.log

# Function to clean up on exit
cleanup() {
    echo ""
    echo "Cleaning up..."
    kill $QEMU_PID 2>/dev/null
    rm -f /tmp/gdb_commands.gdb
    exit 0
}

# Set up cleanup trap
trap cleanup EXIT INT TERM

# Create temporary GDB script
cat > /tmp/gdb_commands.gdb << 'EOF'
# Connect to QEMU
target remote :1234

# Set up logging
set logging on mmu_debug.log
set logging redirect on

# Disable pagination for continuous output
set pagination off

# Set breakpoints at critical functions
break proc0
break segpage
break mmuwalk
break mmucreate
break mmuswitch

# Continue to proc0
continue

# When we hit proc0, show state and start detailed stepping
commands
    printf "=== HIT PROC0 - Starting detailed analysis ===\n"
    info registers
    printf "PML4 base: 0x%016lx\n", m->pml4
    printf "MMU head: 0x%016lx\n", up->mmuhead
    printf "Starting detailed stepping...\n"
    
    # Step through with monitoring
    while 1
        stepi
        # Check every 256 instructions
        if ($rip & 0xff) == 0
            # Monitor critical registers
            if $rsp < 0xffff800000000000 && $rsp != 0
                printf "WARNING: Suspicious RSP: 0x%016lx\n", $rsp
            end
            # Check for null dereferences
            if ($rip >= 0 && $rip < 0x1000) || ($rip >= 0x8000000000000000 && $rip < 0x8000000000001000)
                printf "ERROR: Suspicious RIP: 0x%016lx\n", $rip
                info registers
                bt
                return
            end
        end
        # Break on specific fault conditions
        if $rip == 0xffffffff80200000  # Adjust this to your actual page fault handler
            printf "PAGE FAULT DETECTED\n"
            info registers
        end
    end
end

# Handle segpage breakpoints
break segpage
commands
    printf "=== segpage called ===\n"
    info args
    continue
end

# Handle mmuwalk breakpoints  
break mmuwalk
commands
    printf "=== mmuwalk called (va=0x%016lx, level=%d) ===\n", $rsi, $rdx
    continue
end

# Handle mmucreate breakpoints
break mmucreate
commands
    printf "=== mmucreate called (level=%d, index=%d) ===\n", $rdx, $rcx
    continue
end

# Handle mmuswitch breakpoints
break mmuswitch
commands
    printf "=== mmuswitch called ===\n"
    continue
end
EOF

echo "Starting QEMU..."
qemu-system-x86_64 \
    -M q35 \
    -m 2G \
    -cdrom lux9.iso \
    -boot d \
    -s -S \
    -d int,mmu \
    -D qemu_debug.log \
    -serial stdio \
    -no-shutdown \
    -no-reboot &

QEMU_PID=$!
echo "QEMU started with PID $QEMU_PID"

# Wait a moment for QEMU to start
sleep 2

echo "Starting GDB..."
echo "Debug output will be logged to mmu_debug.log"
echo "QEMU debug output in qemu_debug.log"
echo ""
echo "Press Ctrl+C to stop debugging"

# Run GDB with our script
gdb -batch -x /tmp/gdb_commands.gdb lux9.elf

echo ""
echo "=== DEBUGGING COMPLETE ==="
echo "GDB log: mmu_debug.log"
echo "QEMU log: qemu_debug.log"