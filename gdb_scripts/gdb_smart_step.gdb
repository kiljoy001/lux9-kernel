# GDB script with smart loop detection for configuration debugging
# Usage: gdb lux9.elf -x gdb_smart_step.gdb

target remote | /home/scott/Repo/lux9-kernel/shell_scripts/qemu_gdb_stdio.sh

set logging file gdb_smart_trace.log
set logging overwrite on
set logging enabled on

set pagination off
set confirm off

python
import gdb
import hashlib

print("=== Smart Loop Detection for Config Debug ===\n")

# Circular buffers for loop detection
small_buffer = []
large_buffer = []
small_size = 5
large_size = 20

# Track seen signatures
seen_small = set()
seen_large = set()

# Tracking variables
step_count = 0
max_steps = 10000

def hash_buffer(buf):
    """Hash the sequence of line numbers"""
    return hashlib.md5(str(buf).encode()).hexdigest()

def add_to_buffer(buf, item, max_size):
    """Add to circular buffer, returns True when full"""
    buf.append(item)
    if len(buf) > max_size:
        buf.pop(0)
    return len(buf) == max_size

def is_looping():
    """Check if we're in a loop based on line sequence signatures"""
    try:
        line = int(gdb.parse_and_eval("$linum"))
    except:
        return False

    # Add to both buffers
    small_full = add_to_buffer(small_buffer, line, small_size)
    large_full = add_to_buffer(large_buffer, line, large_size)

    # Check small buffer first (tight loops)
    if small_full:
        sig = hash_buffer(small_buffer)
        if sig in seen_small:
            return True
        seen_small.add(sig)

    # Check large buffer
    if large_full:
        sig = hash_buffer(large_buffer)
        if sig in seen_large:
            return True
        seen_large.add(sig)

    return False

def get_current_function():
    """Get current function name"""
    try:
        frame = gdb.selected_frame()
        return frame.name()
    except:
        return "unknown"

def print_status():
    """Print current location and relevant variables"""
    try:
        line = int(gdb.parse_and_eval("$linum"))
        func = get_current_function()

        # Focus on main_after_cr3 and config-related functions
        if func and ('main_after_cr3' in func or 'waserror' in func or 'setconfenv' in func or 'ksetenv' in func or func == 'main'):
            # Try to print relevant variables
            try:
                arch_val = gdb.parse_and_eval("arch")
                print(f"[{func}:{line}] arch = {arch_val}")
            except:
                pass

            try:
                conffile_val = gdb.parse_and_eval("conffile")
                print(f"[{func}:{line}] conffile = {conffile_val}")
            except:
                pass
                
            try:
                buf_val = gdb.parse_and_eval("buf")
                print(f"[{func}:{line}] buf = {buf_val}")
            except:
                pass
                
            try:
                cpuserver_val = gdb.parse_and_eval("cpuserver")
                print(f"[{func}:{line}] cpuserver = {cpuserver_val}")
            except:
                pass
    except:
        pass

def smart_step():
    """Step or next based on loop detection - always use next to avoid stepping into calls"""
    global step_count
    step_count += 1

    if step_count > max_steps:
        print(f"\nReached max steps ({max_steps}), stopping")
        return False

    # Always use 'next' to stay in the current function
    # Loop detection just helps us know we're iterating
    gdb.execute("next", to_string=True)

    print_status()
    return True

end

# Set breakpoints for configuration sequence
break main_after_cr3
commands 1
printf "\n=== ENTERED main_after_cr3 ===\n"
printf "Starting smart stepping sequence...\n"
end

break printinit
commands 2
printf "\n--- printinit reached ---\n"
printf "pc: %#lx\n", $pc
backtrace 3
continue
end

break pageowninit
commands 3
printf "\n--- pageowninit reached ---\n"
printf "pc: %#lx\n", $pc
backtrace 3
continue
end

break xinit
commands 4
printf "\n--- xinit reached ---\n"
printf "pc: %#lx\n", $pc
backtrace 3
continue
end

# Continue to first breakpoint
printf "Starting execution to main_after_cr3...\n"
continue

printf "\n========================================\n"
printf "=== REACHED main_after_cr3 ===\n"
printf "========================================\n\n"

# Run smart stepping through main_after_cr3
python
print("\nStarting smart stepping through main_after_cr3...\n")

try:
    # Set a breakpoint specifically at the configuration setup line
    config_line = None
    try:
        # Try to find the exact line with configuration setup
        lines = gdb.execute("list main_after_cr3", to_string=True).split('\n')
        for i, line in enumerate(lines):
            if "BOOT: setting up configuration environment" in line:
                # This is approximate, we'll step through anyway
                print(f"Found config setup line: {line}")
                break
    except:
        pass
    
    # Step through until we hit config setup
    target_reached = False
    while smart_step():
        # Check if we've reached the configuration setup
        try:
            frame = gdb.selected_frame()
            func = frame.name()
            if func and func == 'main_after_cr3':
                # Try to see if we're past the config setup
                try:
                    pc = int(gdb.parse_and_eval("$pc"))
                    # This is where the configuration setup should be
                    if pc > 0xffffffff80317500 and pc < 0xffffffff80317600:
                        print(f"[CONFIG REGION] PC: 0x{pc:x} - stepping through config setup")
                        # More detailed stepping in config region
                        for i in range(20):  # Step 20 more times through config
                            if not smart_step():
                                break
                        target_reached = True
                        break
                except:
                    pass
                    
            # If we're back in main (caller), stop
            elif func and func == 'main':
                print(f"\nReturned to main(), main_after_cr3 complete")
                break
        except:
            pass
            
        # Special detection for configuration setup failure
        try:
            # Check if we've stuck in a loop in config area
            if is_looping():
                print("\n=== DETECTED CONFIG SETUP LOOP ===")
                print("Configuration environment setup is stuck in a loop")
                break
        except:
            pass
            
except KeyboardInterrupt:
    print("\nInterrupted by user")
except Exception as e:
    print(f"\nException: {e}")

print(f"\nTotal steps: {step_count}")
print("\n=== CHECKING FINAL CONFIG STATE ===\n")

# Print final configuration variables
try:
    arch_val = gdb.parse_and_eval("arch")
    print(f"arch: {arch_val}")
    
    conffile_val = gdb.parse_and_eval("conffile")
    print(f"conffile: {conffile_val}")
    
    cpuserver_val = gdb.parse_and_eval("cpuserver")
    print(f"cpuserver: {cpuserver_val}")
    
    conf_monitor_val = gdb.parse_and_eval("conf.monitor")
    print(f"conf.monitor: {conf_monitor_val}")
    
except Exception as e:
    print(f"Could not read config variables: {e}")

end

set logging enabled off
quit
