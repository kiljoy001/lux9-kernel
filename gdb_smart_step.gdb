# GDB script with smart loop detection
# Usage: gdb lux9.elf -x gdb_smart_step.gdb

target remote :1234

set logging file gdb_smart_trace.log
set logging overwrite on
set logging enabled on

set pagination off
set confirm off

python
import gdb
import hashlib

print("=== Smart Loop Detection GDB Script ===\n")

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

        # Only print when in post-cr3 execution or related functions
        if func and ('post_cr3' in func or 'after_cr3' in func or func == 'main'):
            # Try to print relevant variables
            try:
                base = gdb.parse_and_eval("base")
                print(f"[{func}:{line}] base = {base}")
            except:
                pass

            try:
                size = gdb.parse_and_eval("size")
                print(f"[{func}:{line}] size = {size}")
            except:
                pass

            try:
                cm = gdb.parse_and_eval("cm")
                if cm:
                    try:
                        cm_base = gdb.parse_and_eval("cm->base")
                        cm_npage = gdb.parse_and_eval("cm->npage")
                        print(f"[{func}:{line}] cm->base = {cm_base}, cm->npage = {cm_npage}")
                    except:
                        pass
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

# Set breakpoint at meminit
break *0x200000

# Continue to breakpoint
continue

printf "\n========================================\n"
printf "=== REACHED CONTINUATION AT 0x200000 ===\n"
printf "========================================\n\n"

# Run smart stepping
python
print("\nStarting smart stepping through post-CR3 execution...\n")

try:
    while smart_step():
        # Check if meminit has returned
        try:
            frame = gdb.selected_frame()
            func = frame.name()
            # If we're back in main (caller), stop
            if func and func == 'main':
                print(f"\nReturned to main(), post-CR3 execution complete")
                break
        except:
            pass
except KeyboardInterrupt:
    print("\nInterrupted by user")
except Exception as e:
    print(f"\nException: {e}")

print(f"\nTotal steps: {step_count}")
print("\n=== CHECKING FINAL CONF.MEM STATE ===\n")

# Print final conf.mem array
for i in range(16):
    try:
        base = gdb.parse_and_eval(f"conf.mem[{i}].base")
        npage = gdb.parse_and_eval(f"conf.mem[{i}].npage")
        if int(npage) != 0:
            print(f"conf.mem[{i}]: base={base} npage={npage}")
    except:
        pass

end

set logging enabled off
quit
