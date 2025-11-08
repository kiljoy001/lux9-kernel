# Smart GDB script to find exact hang location after pageowninit()
# Properly continues investigation through boot phases

target remote :1234
set pagination off
set confirm off

# Set logging for detailed analysis
set logging file gdb_investigation.log
set logging overwrite on
set logging enabled on

# Critical breakpoints for hang investigation
break exchangeinit
commands 1
printf "\n=== ENTERED exchangeinit ===\n"
printf "This is likely where the hang occurs\n"
printf "PC: 0x%lx, Function: ", $pc
info symbol $pc
printf "\n=== Getting backtrace ===\n"
backtrace 8
printf "\n=== Disassembling current area ===\n"
disassemble /m $pc-10,$pc+10
printf "\n=== Continuing to next phase ===\n"
continue
end

break trapinit
commands 2
printf "\n=== ENTERED trapinit ===\n"
printf "Trap/interrupt handling starting\n"
printf "PC: 0x%lx, Function: ", $pc
info symbol $pc
continue
end

break main
commands 3
printf "\n=== ENTERED main ===\n"
printf "Main kernel loop reached!\n"
printf "This means the hang was resolved!\n"
printf "PC: 0x%lx, Function: ", $pc
info symbol $pc
continue
end

break panic
commands 4
printf "\n=== PANIC DETECTED ===\n"
printf "Getting detailed panic analysis...\n"
printf "PC: 0x%lx, Function: ", $pc
info symbol $pc
printf "\n=== Backtrace ===\n"
backtrace 20
printf "\n=== Disassembly around panic ===\n"
disassemble /m $pc-20,$pc+20
printf "\n=== Memory around PC ===\n"
x/20x $pc
printf "\n=== Register state ===\n"
info registers
printf "\n=== PANIC ANALYSIS COMPLETE ===\n"
continue
end

# PC tracking for loop detection
define track_pc
    printf "PC: 0x%016lx ", $pc
    info symbol $pc
    stepi
end

# Start investigation
printf "\n=== BOOT HANG INVESTIGATION ===\n"
printf "Focusing on hang after pageowninit()\n"
printf "Breaking at: exchangeinit → trapinit → main → panic\n"
printf "Use 'track_pc' for manual stepping\n"
printf "Automatic investigation will continue to next breakpoint\n\n"

continue

printf "\n=== INVESTIGATION COMPLETE ===\n"
printf "Check gdb_investigation.log for full details\n"
set logging enabled off
quit