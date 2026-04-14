# Simplified GDB script for memory debugging
# Usage: gdb lux9.elf -x gdb_memory_simple.gdb

# Connect to QEMU
target remote :1234

# Enable logging
set logging file gdb_memory_trace.log
set logging overwrite on
set logging enabled on

# Set breakpoint at preallocpages entry
break preallocpages

# Continue to breakpoint
continue

printf "\n========================================\n"
printf "=== PREALLOCPAGES ENTRY ===\n"
printf "========================================\n\n"

# Print conf.mem array
printf "conf.mem array (all regions):\n"
printf "%-4s %-18s %-12s %-18s %-18s\n", "idx", "base", "npage", "kbase", "klimit"
printf "%-4s %-18s %-12s %-18s %-18s\n", "---", "----", "-----", "-----", "------"
set $i = 0
while ($i < 16)
  if (conf.mem[$i].npage != 0)
    printf "[%-2d] 0x%-16lx %-12lu 0x%-16lx 0x%-16lx\n", $i, conf.mem[$i].base, conf.mem[$i].npage, conf.mem[$i].kbase, conf.mem[$i].klimit
  end
  set $i = $i + 1
end

# Print key constants
printf "\nKey constants:\n"
printf "  VMAPSIZE = 0x%lx (%lu GB)\n", 0x80000000, 0x80000000/(1024*1024*1024)
printf "  sizeof(Page) = %lu bytes\n", sizeof(Page)
printf "  PGLSZ(1) = 0x%lx (2MB)\n", 0x200000

# Check conf.npage
printf "\nTotal system pages:\n"
printf "  conf.npage = %lu\n", conf.npage

# Let execution continue to panic
printf "\nContinuing execution to see panic...\n"
continue

printf "\n========================================\n"
printf "=== EXECUTION COMPLETED/PANICKED ===\n"
printf "========================================\n"

# Disable logging
set logging enabled off

printf "\nDebug trace saved to: gdb_memory_trace.log\n"
