# GDB script to trace xallocz with loop detection
# Usage: gdb lux9.elf -x gdb_xalloc_trace.gdb

target remote :1234

set logging file gdb_xalloc_trace.log
set logging overwrite on
set logging enabled on

set pagination off
set confirm off

python
import gdb
import hashlib

print("=== Smart xallocz Tracer ===\n")

# Loop detection
small_buffer = []
large_buffer = []
small_size = 5
large_size = 20
seen_small = set()
seen_large = set()
step_count = 0
max_steps = 1000  # Limit to 1000 steps

def hash_buffer(buf):
    return hashlib.md5(str(buf).encode()).hexdigest()

def add_to_buffer(buf, item, max_size):
    buf.append(item)
    if len(buf) > max_size:
        buf.pop(0)
    return len(buf) == max_size

def is_looping():
    try:
        line = int(gdb.parse_and_eval("$linum"))
    except:
        return False

    small_full = add_to_buffer(small_buffer, line, small_size)
    large_full = add_to_buffer(large_buffer, line, large_size)

    if small_full:
        sig = hash_buffer(small_buffer)
        if sig in seen_small:
            return True
        seen_small.add(sig)

    if large_full:
        sig = hash_buffer(large_buffer)
        if sig in seen_large:
            return True
        seen_large.add(sig)

    return False

def print_xalloc_vars():
    """Print xallocz relevant variables"""
    try:
        func = gdb.selected_frame().name()
        line = int(gdb.parse_and_eval("$linum"))

        if func and 'xalloc' in func:
            try:
                size = gdb.parse_and_eval("size")
                print(f"[{func}:{line}] size = {size}")
            except:
                pass

            try:
                h = gdb.parse_and_eval("h")
                if h and int(h) != 0:
                    h_addr = gdb.parse_and_eval("h->addr")
                    h_size = gdb.parse_and_eval("h->size")
                    print(f"[{func}:{line}] h = {h}, h->addr = {h_addr}, h->size = {h_size}")
            except:
                pass

            try:
                loop_count = gdb.parse_and_eval("loop_count")
                if int(loop_count) % 50 == 0:
                    print(f"[{func}:{line}] loop_count = {loop_count}")
            except:
                pass
    except:
        pass

def smart_step():
    global step_count
    step_count += 1

    if step_count > max_steps:
        print(f"\nReached max steps ({max_steps}), stopping")
        return False

    gdb.execute("next", to_string=True)
    print_xalloc_vars()
    return True

end

# Set conditional breakpoint to catch 2MB allocation
python
class XallocBreakpoint(gdb.Breakpoint):
    def __init__(self):
        super(XallocBreakpoint, self).__init__("xallocz")

    def stop(self):
        try:
            size = int(gdb.parse_and_eval("size"))
            zero = int(gdb.parse_and_eval("zero"))
            # Only stop for 2MB allocation
            if size > 1900000 and size < 2100000:
                print(f"\n=== CAUGHT 2MB XALLOCZ ===")
                print(f"size = {size}, zero = {zero}")
                return True
        except:
            pass
        return False

bp = XallocBreakpoint()
end

# Continue to breakpoint
continue

printf "\n========================================\n"
printf "=== REACHED 2MB XALLOCZ CALL ===\n"
printf "========================================\n\n"

# Run smart stepping
python
print("\nStarting smart stepping through xallocz()...\n")

try:
    while smart_step():
        # Check if we returned from xallocz
        try:
            frame = gdb.selected_frame()
            func = frame.name()
            if func and func != 'xallocz':
                print(f"\nReturned from xallocz to {func}")
                break
        except:
            pass
except KeyboardInterrupt:
    print("\nInterrupted by user")
except Exception as e:
    print(f"\nException: {e}")

print(f"\nTotal steps: {step_count}")

end

set logging enabled off
quit
