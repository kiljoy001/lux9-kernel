# GDB script to investigate device initialization crash after mmuinit()
# Usage: gdb lux9.elf -x gdb_device_debug.gdb

target remote :1234

set logging file gdb_device_debug.log
set logging overwrite on
set logging enabled on

set pagination off
set confirm off

# Set breakpoint right after mmuinit returns
break mmuinit
commands 1
printf "\n=== HIT mmuinit breakpoint ===\n"
printf "Setting breakpoint right after mmuinit returns to main_after_cr3...\n"
# Set breakpoint at the next statement in main_after_cr3 after mmuinit() call
break *0xffffffff8031753a  # Approximate - will adjust based on actual address
printf "Continuing to step through post-mmuinit execution...\n"
continue
end

# Alternative: set breakpoint on main_after_cr3 after mmuinit call
break main_after_cr3
commands 2
printf "\n=== ENTERED main_after_cr3 ===\n"
printf "Stepping through to find crash point after mmuinit...\n"
# Continue normally but we're looking for the crash
continue
end

# If we hit a crash, try to get backtrace
break panic
commands 3
printf "\n=== PANIC DETECTED ===\n"
print "Getting backtrace from panic..."
backtrace 10
backtrace -10
printf "\nRegisters at panic:\n"
info registers
continue
end

printf "\n=== Starting device initialization debug ===\n"
printf "This will help us find what crashes after mmuinit completes successfully\n\n"

# Start execution
continue

printf "\n=== Device debug complete ===\n"

set logging enabled off
quit