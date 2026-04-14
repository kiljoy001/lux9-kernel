# GDB script to investigate boot hang efficiently
# Usage: gdb lux9.elf -x gdb_focused_hang.gdb

target remote :1234
set pagination off
set confirm off

# Set logging
set logging file gdb_focused_investigation.log
set logging overwrite on
set logging enabled on

# Break at key boot phases to identify hang location
break main_after_cr3
commands 1
printf "\n=== ENTERED main_after_cr3 ===\n"
continue
end

break pageowninit
commands 2  
printf "\n=== ENTERED pageowninit ===\n"
continue
end

break exchangeinit
commands 3
printf "\n=== ENTERED exchangeinit ===\n" 
printf "This is where the hang likely occurs\n"
continue
end

break trapinit
commands 4
printf "\n=== ENTERED trapinit ===\n"
continue
end

break main
commands 5
printf "\n=== ENTERED main ===\n"
continue
end

break panic
commands 6
printf "\n=== PANIC DETECTED ===\n"
backtrace 15
printf "\n=== PANIC ANALYSIS COMPLETE ===\n"
continue
end

# Simple loop detection
define check_loop
    set $current_pc = $pc
    if $current_pc == $last_pc
        set $repeat_count = $repeat_count + 1
        if $repeat_count >= 20
            printf "\n=== LOOP DETECTED at PC 0x%lx ===\n", $current_pc
            backtrace 10
            printf "\n=== INVESTIGATION COMPLETE ===\n"
            quit
        end
    else
        set $last_pc = $current_pc
        set $repeat_count = 0
    end
end

# Auto-continue to find hang point
printf "\n=== BOOT HANG INVESTIGATION STARTING ===\n"
printf "Will break at main_after_cr3 → pageowninit → exchangeinit → trapinit → main\n"
printf "The hang should occur at one of these breakpoints\n\n"

continue

printf "\n=== INVESTIGATION COMPLETE ===\n"
printf "Check gdb_focused_investigation.log for detailed trace\n"
set logging enabled off
quit