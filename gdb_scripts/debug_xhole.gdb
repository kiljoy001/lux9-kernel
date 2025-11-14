set auto-load safe-path /home/scott/Repo/lux9-kernel
file lux9.elf
target remote tcp:localhost:1234

# Set breakpoint at xinit to see if xhole_user_init is called
break xinit
commands
  silent
  printf "Breakpoint reached at xinit\n"
  continue
end

# Set breakpoint at xhole_user_init
break xhole_user_init
commands
  silent
  printf "Breakpoint reached at xhole_user_init function\n"
  bt
  info locals
  continue
end

# Set breakpoint right after the function call
break xinit
if *$pc > 0xffffffff80200000 && *$pc < 0xffffffff80400000
  printf "Function call just happened\n"
end

# Set breakpoint in xallocz to see if holes are being found
break xallocz
commands
  silent
  printf "xallocz called, checking xlists.table = %p\n", (void*)xlists.table
  if xlists.table != 0
    printf "xlists.table->size = %lu\n", xlists.table->size
    printf "xlists.table->addr = %p\n", (void*)xlists.table->addr
  end
  continue
end

# Continue execution
printf "Starting debugging session\n"
continue