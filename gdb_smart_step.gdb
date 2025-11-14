# Simple GDB script to track boot progress and skip loops
# Usage: gdb lux9.elf -x gdb_smart_step.gdb

target remote :1234

set pagination off
set confirm off

# Set logging
set logging file gdb_simple_trace.log
set logging overwrite on
set logging enabled on

# Simple loop detection
set $last_pc = 0x0
set $repeat_count = 0
set $max_repeats = 100

# Breakpoints for key phases
break main_after_cr3
commands 1
printf "\n=== ENTERED main_after_cr3 ===\n"
continue
end

break mmuinit
commands 2  
printf "\n=== ENTERED mmuinit ===\n"
printf "About to test memory protection setup...\n"
continue
end

break panic
commands 3
printf "\n=== PANIC DETECTED ===\n"
printf "Getting backtrace...\n"
backtrace
continue
end

# Define simple loop detection command
define check_loop
    set $current_pc = $pc
    if $current_pc == $last_pc
        set $repeat_count = $repeat_count + 1
        if $repeat_count >= $max_repeats
            printf "\n=== LOOP DETECTED! Breaking at PC 0x%lx (repeated %d times) ===\n", $current_pc, $repeat_count
            backtrace
            quit
        end
    else
        set $last_pc = $current_pc
        set $repeat_count = 0
    end
end

# Define step with loop checking
define smart_step
    printf "[Step] "
    stepi
    check_loop
    printf "PC: 0x%lx\n", $pc
end

# Start execution
printf "\n=== Starting Simple Boot Tracker ===\n"
printf "Will track execution and detect loops\n\n"

continue

printf "\n=== Tracker Complete ===\n"
set logging enabled off
quit
