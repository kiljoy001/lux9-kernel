# Create detailed debug script to identify post_cr3_memory_system_operational failure
cat > debug_post_cr3.gdb << 'EOF'
# Connect to QEMU
target remote localhost:1234

# Set breakpoint at post_cr3_memory_system_operational
break post_cr3_memory_system_operational
c

printf "\n=== DEBUGGING post_cr3_memory_system_operational() ===\n"

# Step into function
printf "Entering post_cr3_memory_system_operational()\n"
n

# Check each condition step by step
printf "\n--- CHECK 1: borrowpool.owners ---\n"
printf "borrowpool.owners = %p\n", borrowpool.owners
printf "borrowpool.nbuckets = %d\n", borrowpool.nbuckets
printf "borrowpool.owners == nil? %s\n", (borrowpool.owners == nil) ? "YES" : "NO"
printf "borrowpool.nbuckets == 0? %s\n", (borrowpool.nbuckets == 0) ? "YES" : "NO"

if (borrowpool.owners == nil || borrowpool.nbuckets == 0)
    printf "FAILED: borrow pool check\n"
else
    printf "PASSED: borrow pool check\n"
end

printf "\n--- CHECK 2: mem_coord.state ---\n"
printf "mem_coord.state = %d\n", mem_coord.state
printf "mem_coord.state == MEMORY_KERNEL_ACTIVE? %s\n", (mem_coord.state == MEMORY_KERNEL_ACTIVE) ? "YES" : "NO"

if (mem_coord.state != MEMORY_KERNEL_ACTIVE)
    printf "FAILED: memory coordination state\n"
else
    printf "PASSED: memory coordination state\n"
end

printf "\n=== CONTINUING TO SEE RESULT ===\n"
c

quit
EOF

echo "Created debug_post_cr3.gdb script"