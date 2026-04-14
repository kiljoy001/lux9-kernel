# Load symbols from the kernel
file lux9.elf

# Connect to QEMU
target remote localhost:1234

# Set breakpoints to investigate memory system after CR3 switch
printf "\n=== STARTING MEMORY SYSTEM DEBUG ===\n"

# First, get to the point where trampoline is copied
b mmu.c:495
c

printf "\n=== Trampoline has been copied to 0x1000 ===\n"

# Set hardware breakpoint at 0x1000 where trampoline will execute
hbreak *0x1000
c

printf "\n============================================\n"
printf "=== AT TRAMPOLINE (0x1000) ===\n"
printf "============================================\n"

# Disassemble trampoline
x/10i $rip

# Execute through trampoline until continuation
printf "\n=== EXECUTING TRAMPOLINE ===\n"

# movq %rsi, %r8
printf "\n[STEP 1: movq %%rsi, %%r8]\n"
si

# movq %rdi, %r9  
printf "\n[STEP 2: movq %%rdi, %%r9]\n"
si

# CLI and UART output - let's skip ahead
printf "\n[SKIPPING TO CR3 SWITCH]\n"
# Step through until we hit mov %r9, %cr3
while ($rip < 0x1015)
    si
end

printf "\n[STEP 3: movq %%r9, %%cr3 - CR3 SWITCH!]\n"
si
printf "  New CR3 = 0x%lx\n", $cr3

# Jump to after_cr3 section
printf "\n[JUMP TO after_cr3_switch SECTION]\n"
si

# NOW WE'RE AT THE CONTINUATION - CHECK MEMORY SYSTEM STATE
printf "\n============================================\n"
printf "=== CHECKING MEMORY SYSTEM AFTER CR3 SWITCH ===\n"
printf "============================================\n"

# Check if we're in after_cr3_switch
printf "Current RIP = 0x%lx\n", $rip
x/5i $rip

# Load addresses of memory system variables
printf "\n=== MEMORY SYSTEM VARIABLE ADDRESSES ===\n"
# We need to find where these are defined in memory
# For now, let's try to access them directly

printf "\n=== CHECKING borrowpool STATE ===\n"
# Try to access borrowpool.owners and borrowpool.nbuckets
# These might cause crashes if not initialized

printf "\n=== CHECKING mem_coord STATE ===\n"
# Try to access mem_coord.state

printf "\n=== ATTEMPTING TO CALL post_cr3_memory_system_operational() ===\n"
# This function should fail due to uninitialized variables
call post_cr3_memory_system_operational()

printf "\n=== QEMU MEMORY MAP ===\n"
monitor info mem

printf "\n=== MEMORY SYSTEM DEBUG COMPLETE ===\n"
quit