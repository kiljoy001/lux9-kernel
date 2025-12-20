#!/bin/bash
# debug_lock_issue.sh - Debug the remaining lock issue

echo "=== Lux9 Kernel Lock Issue Debugger ==="
echo "This script helps investigate the lock loop at 0xffff800000125778"
echo

# Check if kernel exists
if [ ! -f "lux9.elf" ]; then
    echo "ERROR: lux9.elf not found. Please build the kernel first."
    echo "Run: make"
    exit 1
fi

echo "=== Creating debug environment ==="
echo "1. Launching QEMU with GDB stub on port 1234..."
echo "2. Kernel will pause at startup"
echo

# Create temporary directory for debug session
DEBUG_DIR="/tmp/lux9_debug_$(date +%s)"
mkdir -p "$DEBUG_DIR"

echo "Debug session directory: $DEBUG_DIR"
echo

# Create QEMU launch script
cat > "$DEBUG_DIR/run_debug_qemu.sh" << 'EOF'
#!/bin/bash
cd /home/scott/Repo/lux9-kernel
echo "=== QEMU GDB Debug Session ==="
echo "Kernel: lux9.elf"
echo "GDB Port: 1234"
echo "Commands:"
echo "  - Press Ctrl+A then X to exit QEMU"
echo "  - Connect GDB with: gdb ./lux9.elf"
echo "  - In GDB: (gdb) target remote localhost:1234"
echo
echo "Starting QEMU..."
qemu-system-x86_64 \
    -kernel lux9.elf \
    -s -S \
    -nographic \
    -serial stdio \
    -no-reboot
EOF

chmod +x "$DEBUG_DIR/run_debug_qemu.sh"

# Create GDB initialization script
cat > "$DEBUG_DIR/gdb_commands.gdb" << 'EOF'
# GDB Commands for Lux9 Kernel Debugging
define setup_debug
    echo Setting up debug environment...\n
    symbol-file /home/scott/Repo/lux9-kernel/lux9.elf
    set confirm off
    echo Debug environment ready.\n
end

define show_lock_info
    echo === Lock Information ===\n
    if $argc == 1
        printf "Lock at %p:\n", $arg0
        printf "  key: 0x%lx\n", ((Lock*)$arg0)->key
        printf "  pc: 0x%lx\n", ((Lock*)$arg0)->pc
    else
        echo Usage: show_lock_info <lock_address>
    end
end

define show_imagealloc_info
    echo === Imagealloc Information ===\n
    printf "imagealloc.nidle: %lu\n", imagealloc.nidle
    printf "imagealloc.pgidle: %lu\n", imagealloc.pgidle
    printf "conf.nimage: %lu\n", conf.nimage
    printf "imagealloc address: %p\n", &imagealloc
end

define show_attachimage_vars
    echo === Attachimage Variables ===\n
    # This would show local variables if we're in attachimage function
    info locals
end

define setup_breakpoints
    echo Setting up breakpoints...\n
    
    # Breakpoint at attachimage function
    break attachimage
    commands
        echo === attachimage called ===\n
        show_attachimage_vars
        continue
    end
    
    # Breakpoint at the specific lock line
    break kernel/9front-port/segment.c:477
    commands
        echo === Locking imagealloc ===\n
        show_imagealloc_info
        continue
    end
    
    # Break on specific lock address
    # Note: This may need adjustment based on the actual symbol
    echo Breakpoints set up.\n
end

define quick_debug
    setup_debug
    setup_breakpoints
    echo Starting execution...\n
    continue
end

# Initialize the debug environment
setup_debug
echo Ready for debugging.\n
echo Common commands:\n
echo "  quick_debug     - Run with pre-configured breakpoints"\n
echo "  show_lock_info <addr> - Show lock information"\n
echo "  show_imagealloc_info  - Show imagealloc status"\n
echo "  continue        - Continue execution"\n
echo "  bt              - Backtrace"\n
echo "  info registers  - Show CPU registers"\n
EOF

echo "=== Debug Files Created ==="
echo "QEMU script: $DEBUG_DIR/run_debug_qemu.sh"
echo "GDB script: $DEBUG_DIR/gdb_commands.gdb"
echo

echo "=== Quick Start Instructions ==="
echo "Terminal 1:"
echo "  $ $DEBUG_DIR/run_debug_qemu.sh"
echo
echo "Terminal 2:"
echo "  $ cd /home/scott/Repo/lux9-kernel"
echo "  $ gdb ./lux9.elf"
echo "  (gdb) source $DEBUG_DIR/gdb_commands.gdb"
echo "  (gdb) target remote localhost:1234"
echo "  (gdb) quick_debug"
echo

echo "=== Key Investigation Points ==="
echo "1. Check if imagealloc.nidle > conf.nimage causing retry loop"
echo "2. Check if newimage() always returns nil"
echo "3. Check if imagereclaim() frees any pages"
echo "4. Watch for same process acquiring lock twice"
echo "5. Check for interrupt context locks"
echo

echo "Debug session ready. Happy hunting!"