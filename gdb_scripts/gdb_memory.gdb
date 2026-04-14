# GDB script for debugging preallocpages memory issue
# Usage: gdb lux9.elf -x gdb_memory.gdb

# Connect to QEMU
target remote :1234

# Enable logging
set logging file gdb_memory_trace.log
set logging overwrite on
set logging on

# Set breakpoint
break preallocpages

# Continue to breakpoint
continue

# Print entry state
printf "\n========================================\n"
printf "=== PREALLOCPAGES ENTRY ===\n"
printf "========================================\n\n"

# Print conf.mem array
printf "conf.mem array (all regions):\n"
printf "%-4s %-18s %-12s %-18s\n", "idx", "base", "npage", "klimit"
printf "%-4s %-18s %-12s %-18s\n", "---", "----", "-----", "------"
set $i = 0
while ($i < 16)
  if (conf.mem[$i].npage != 0)
    printf "[%-2d] 0x%-16lx %-12lu 0x%-16lx\n", $i, conf.mem[$i].base, conf.mem[$i].npage, conf.mem[$i].klimit
  end
  set $i = $i + 1
end

# Step through np calculation
printf "\nCalculating total pages (np)...\n"
next
next
next
set $orig_np_line = $pc
while ($pc == $orig_np_line || $linum < 1247)
  next
end

# Print np after loop
printf "np (total user pages) = %lu\n", np

# Continue to psize calculation
next
next
printf "psize (bytes for Page array) = %ld (0x%lx)\n", psize, psize
printf "psize rounded to 2MB = %ld (0x%lx)\n", psize, psize

# Print constants
printf "\nKey constants:\n"
printf "  VMAPSIZE = 0x%lx (%lu GB)\n", VMAPSIZE, VMAPSIZE/(1024*1024*1024)
printf "  BY2PG = 0x%lx (%lu bytes)\n", BY2PG, BY2PG
printf "  sizeof(Page) = %lu bytes\n", sizeof(Page)
printf "  PGLSZ(1) = 0x%lx (2MB)\n", PGLSZ(1)

# Now step through the search loop
printf "\n========================================\n"
printf "=== MEMORY SEARCH LOOP ===\n"
printf "========================================\n"

# Let it run to end or panic
printf "\nContinuing execution...\n"
continue

printf "\n========================================\n"
printf "=== EXECUTION COMPLETED/PANICKED ===\n"
printf "========================================\n"

# Disable logging
set logging off

printf "\nDebug trace saved to: gdb_memory_trace.log\n"
printf "Please send this file for analysis.\n"
