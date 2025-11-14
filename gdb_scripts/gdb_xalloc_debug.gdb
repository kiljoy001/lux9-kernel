# GDB script to debug xallocz hang
# Usage: gdb lux9.elf -x gdb_xalloc_debug.gdb

target remote :1234

set pagination off
set confirm off

# Break at the 2MB allocation
break xallocz
commands
  silent
  # Only stop if size is around 2MB
  if size > 1900000 && size < 2100000
    printf "\n=== CAUGHT 2MB XALLOCZ CALL ===\n"
    printf "size = %lu\n", size
    printf "zero = %d\n", zero
    continue
  else
    continue
  end
end

# Break at the loop entry
break xalloc.c:193
commands
  silent
  printf "\n=== ENTERED HOLE CHECKING LOOP ===\n"
  printf "l = %p\n", l
  printf "*l (first hole) = %p\n", *l
  if *l != 0
    printf "h->addr = 0x%lx\n", (*l)->addr
    printf "h->size = %lu\n", (*l)->size
    printf "h->link = %p\n", (*l)->link
  end
  continue
end

# Break inside the loop
break xalloc.c:211
commands
  silent
  printf "  hole: h=%p addr=0x%lx size=%lu (need %lu)\n", h, h->addr, h->size, size
  if h->size >= size
    printf "    *** FOUND SUITABLE HOLE ***\n"
  end
  continue
end

# Break at loop increment
break xalloc.c:193
condition 2 loop_count > 0
commands
  silent
  if loop_count % 100 == 0
    printf "  loop_count = %d, h = %p\n", loop_count, h
  end
  if loop_count > 10000
    printf "\n=== LOOP COUNT EXCEEDED 10000 ===\n"
    printf "Still looping, h = %p\n", h
    printf "Current hole: addr=0x%lx size=%lu\n", h->addr, h->size
    quit
  end
  continue
end

# Start execution
continue
