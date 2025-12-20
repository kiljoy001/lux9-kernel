# MOST IMPORTANT: Quick diagnosis for CR3 switch hang

# First, verify the current state when it hangs
echo === CRITICAL CHECK 1: Can we see anything? ===\n

# Check if we're still in QEMU or hung
echo Try Ctrl+C to break into GDB if hung\n

# Step 1: Check where we actually are
echo Current RIP: 
print/x $rip

echo Current CR3: 
print/x $cr3

# Step 2: Check if we're in the trampoline
echo Are we in trampoline? (0x1000-0x100b)\n
print ($rip >= 0x1000 && $rip <= 0x100b)

# Step 3: Most likely issue - continuation address
echo === CRITICAL CHECK 2: Continuation Address ===\n
echo If this is wrong, CR3 switch will hang!\n
print continuation

# Step 4: Check if continuation is in KZERO range
echo Is continuation in KZERO range? (should be true)\n
print (continuation >= KZERO)

# Step 5: Check new page table validity
echo === CRITICAL CHECK 3: New Page Table ===\n
echo New CR3 value: 
print/x $rdi
echo First page table entry (should be valid):\n
print *(u64int*)$rdi

# Step 6: Check if HHDM access works
echo === CRITICAL CHECK 4: HHDM Access ===\n
echo Testing access to HHDM base...\n
print hhdm_base
echo Try to read from HHDM...\n
print *(volatile u64int*)hhdm_base

echo === QUICK FIXES TO TRY ===\n
echo If continuation address is wrong, kernel boot order is still broken\n
echo If page table entries are zero, page table setup failed\n
echo If HHDM access fails, memory mapping is broken\n