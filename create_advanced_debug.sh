# Advanced GDB script to trace post_cr3_memory_system_operational() step by step
cat > debug_cr3_step.gdb << 'EOF'
# Connect to QEMU
target remote localhost:1234

# Set breakpoint at post_cr3_memory_system_operational
break post_cr3_memory_system_operational
c

printf "\n===========================================\n"
printf "=== STEPPING THROUGH post_cr3_memory_system_operational() ===\n"
printf "===========================================\n"

# Step into the function
printf "\n--- STEP 1: Entering function ---\n"
step
printf "Current line: %s\n", (char*)$pc

printf "\n--- STEP 2: Check borrow pool owners ---\n"
printf "Variable borrowpool.owners = %p\n", borrowpool.owners
printf "Variable borrowpool.nbuckets = %d\n", borrowpool.nbuckets
next
printf "Current line after next: %s\n", (char*)$pc

printf "\n--- STEP 3: Check condition ---\n"
printf "borrowpool.owners == nil? %s\n", (borrowpool.owners == nil) ? "YES" : "NO"
printf "borrowpool.nbuckets == 0? %s\n", (borrowpool.nbuckets == 0) ? "YES" : "NO"
printf "Full condition (borrowpool.owners == nil || borrowpool.nbuckets == 0)? %s\n", ((borrowpool.owners == nil || borrowpool.nbuckets == 0) ? "TRUE" : "FALSE")

if (borrowpool.owners == nil || borrowpool.nbuckets == 0)
    printf "CONDITION FAILED - borrow pool check\n"
else
    printf "CONDITION PASSED - borrow pool check\n"
end

printf "\n--- STEP 4: Continue stepping ---\n"
next
printf "Current line after next: %s\n", (char*)$pc

printf "\n--- STEP 5: Check memory coordination state ---\n"
printf "Variable mem_coord.state = %d\n", mem_coord.state
printf "MEMORY_KERNEL_ACTIVE = %d\n", MEMORY_KERNEL_ACTIVE
printf "mem_coord.state == MEMORY_KERNEL_ACTIVE? %s\n", (mem_coord.state == MEMORY_KERNEL_ACTIVE) ? "YES" : "NO"
next
printf "Current line after next: %s\n", (char*)$pc

printf "\n--- STEP 6: Final condition check ---\n"
if (mem_coord.state != MEMORY_KERNEL_ACTIVE)
    printf "FINAL CONDITION FAILED - memory coordination state\n"
    printf "mem_coord.state = %d, expected %d\n", mem_coord.state, MEMORY_KERNEL_ACTIVE
else
    printf "FINAL CONDITION PASSED - memory coordination state\n"
end

printf "\n--- STEP 7: Return value ---\n"
printf "About to return from function\n"
continue

printf "\n===========================================\n"
printf "=== FUNCTION COMPLETED ===\n"
printf "===========================================\n"

quit
EOF

echo "Created advanced debug_cr3_step.gdb script"