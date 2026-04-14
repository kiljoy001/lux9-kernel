# Enhanced GDB script to investigate boot hang after main_after_cr3

target remote :1234

set pagination off
set confirm off

# Set logging for detailed analysis  
set logging file gdb_boot_hang_analysis.log
set logging overwrite on
set logging enabled on

# Enhanced loop detection 
set $last_pc = 0x0
set $repeat_count = 0
set $max_repeats = 50

# Critical breakpoints for boot hang investigation
break main_after_cr3
commands 1
printf "\n=== ENTERED main_after_cr3 ===\n"
continue
end

break xinit
commands 2
printf "\n=== ENTERED xinit (Memory Allocator Init) ===\n"  
continue
end

break pageowninit
commands 3
printf "\n=== ENTERED pageowninit (Page Ownership Init) ===\n"
continue
end

break exchangeinit
commands 4  
printf "\n=== ENTERED exchangeinit ===\n"
continue
end

break trapinit
commands 5
printf "\n=== ENTERED trapinit ===\n"
continue
end

break panic
commands 6
printf "\n=== PANIC DETECTED ===\n"
printf "Getting backtrace...\n"
backtrace 15
printf "\nRegister state:\n"
info registers
printf "\n=== INVESTIGATION COMPLETE ===\n"
quit
end

# Enhanced loop detection with debugging
define check_loop
    set $current_pc = $pc
    if $current_pc == $last_pc
        set $repeat_count = $repeat_count + 1
        if $repeat_count >= $max_repeats
            printf "\n=== CRITICAL: INFINITE LOOP DETECTED! ===\n"
            printf "Location: PC 0x%lx (repeated %d times)\n", $current_pc, $repeat_count
            printf "Getting analysis...\n"
            backtrace 10
            printf "\n=== INSTRUCTION ANALYSIS ===\n"
            disassemble /m $pc-5,$pc+5
            printf "\n=== REGISTER ANALYSIS ===\n"  
            info registers rax rbx rcx rdx rsi rdi rsp rbp
            printf "\n=== STACK ANALYSIS ===\n"
            x/16x $rsp
            printf "\n====== INVESTIGATION COMPLETE ======\n"
            quit
        end
    else
        set $last_pc = $current_pc
        set $repeat_count = 0
    end
end

# Start execution
printf "\n=== BOOT HANG INVESTIGATION STARTING ===\n"
printf "Targeting hang after main_after_cr3: calling pageowninit\n\n"

# Auto-continue to see where it hangs
continue

printf "\n=== INVESTIGATION COMPLETE ===\n"
set logging enabled off
quit