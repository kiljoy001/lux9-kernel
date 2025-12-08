target remote :1234
set pagination off
set confirm off
set logging file gdb_panic.log
set logging overwrite on
set logging enabled on

# Break at panic
break panic
commands
  silent
  printf "\n=== PANIC CAUGHT ===\n"
  
  # Print backtrace
  printf "\n--- Backtrace ---\n"
  bt
  
  # Inspect registers
  printf "\n--- Registers ---\n"
  info registers
  
  # Panic is usually called from trap().
  # trap(Ureg *ureg)
  # ureg is passed in RDI (System V ABI).
  # If panic is called, we are in panic.
  # Caller frame (trap) should have ureg.
  # Let's look up the stack.
  
  frame 1
  printf "\n--- Frame 1 (trap?) ---\n"
  info args
  info locals
  
  # If we can find ureg pointer
  if $rdi != 0
     printf "\n--- Ureg at RDI (if valid) ---\n"
     # Cast to Ureg* and dereference
     # We need to load symbol table for Ureg struct
     print *(struct Ureg *)$rdi
  end
  
  # Quit GDB (which will kill QEMU connection)
  quit
end

# Also break at _iretkernel just in case
break _iretkernel
commands
  silent
  printf "\n=== Reached _iretkernel (Unexpected?) ===\n"
  bt
  continue
end

# Continue execution
continue