# GDB script to debug meminit() and capture loop iterations
# Usage: gdb lux9.elf -x gdb_meminit.gdb

target remote :1234

set logging file gdb_meminit_trace.log
set logging overwrite on
set logging enabled on

# Break at meminit entry
break meminit
commands
  silent
  printf "\n========================================\n"
  printf "=== MEMINIT ENTRY ===\n"
  printf "========================================\n\n"
  continue
end

# Break inside the loop at line 678 (after "found MemRAM")
break memory_9front.c:678
commands
  silent
  printf "\n--- Loop iteration ---\n"
  printf "base = 0x%lx\n", base
  continue
end

# Break at line 680 (after size calculation)
break memory_9front.c:680
commands
  silent
  printf "size = 0x%llx (%llu bytes, %llu pages)\n", (unsigned long long)size, (unsigned long long)size, (unsigned long long)(size/4096)
  continue
end

# Break at line 690 (after memmapalloc)
break memory_9front.c:690
commands
  silent
  printf "memmapalloc returned: 0x%lx\n", cm->base
  continue
end

# Break at line 697 (after setting npage)
break memory_9front.c:697
commands
  silent
  printf "conf.mem[%d]: base=0x%lx npage=%lu\n", (int)(cm - &conf.mem[0]), cm->base, cm->npage
  continue
end

# Break at line 702 (after loop exits)
break memory_9front.c:702
commands
  silent
  printf "\n========================================\n"
  printf "=== LOOP COMPLETED ===\n"
  printf "========================================\n\n"
  printf "Final conf.mem array:\n"
  printf "idx  base               npage\n"
  printf "---  ----               -----\n"
  set $i = 0
  while ($i < 16)
    if (conf.mem[$i].npage != 0)
      printf "[%-2d] 0x%-16lx %lu\n", $i, conf.mem[$i].base, conf.mem[$i].npage
    end
    set $i = $i + 1
  end
  printf "\n"
  continue
end

# Start execution
continue

# Wait for completion
printf "\n========================================\n"
printf "=== GDB SESSION COMPLETE ===\n"
printf "========================================\n"

set logging enabled off
quit
