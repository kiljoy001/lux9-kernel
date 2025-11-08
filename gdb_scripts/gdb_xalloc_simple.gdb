# Simple GDB script to catch 2MB xallocz call
# Usage: gdb lux9.elf -x gdb_xalloc_simple.gdb

target remote :1234

set logging file gdb_xalloc_simple.log
set logging overwrite on
set logging enabled on

set pagination off

# Break at xallocz
break xallocz

# Use commands to print and decide
commands
  silent
  printf "xallocz: size=%lu zero=%d\n", size, zero
  # If this is the 2MB allocation, step through it
  if size > 1900000 && size < 2100000
    printf "\n=== FOUND 2MB ALLOCATION ===\n"
    printf "Stepping through...\n"
    # Step to the loop
    next
    next
    next
    next
    next
    next
    # Print loop state
    printf "After initial setup:\n"
    printf "  l = %p\n", l
    printf "  *l = %p\n", *l
    if *l != 0
      printf "  first hole: addr=0x%lx size=%lu\n", (*l)->addr, (*l)->size
    end
    # Step into loop
    next
    printf "Entered loop, h = %p\n", h
    if h != 0
      printf "  h->addr = 0x%lx\n", h->addr
      printf "  h->size = %lu\n", h->size
      printf "  h->link = %p\n", h->link
    end
    # Let it run a bit
    next
    next
    next
    printf "After few iterations:\n"
    printf "  loop_count = %d\n", loop_count
    printf "  h = %p\n", h
    # Stop here for manual inspection
    set logging enabled off
    return
  else
    continue
  end
end

continue
