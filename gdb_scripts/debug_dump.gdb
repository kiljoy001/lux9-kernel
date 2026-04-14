target remote :1234
set pagination off
set confirm off

# Break at _iretkernel
break _iretkernel
commands
  silent
  printf "\n=== Reached _iretkernel ===\n"
  printf "RSP = %p\n", $rsp
  
  printf "Disassembly:\n"
  x/5i $pc
  
  printf "Stack before addq (top 8 words):\n"
  x/8gx $rsp
  
  printf "Calculated IRETQ frame location (RSP+40):\n"
  x/5gx $rsp + 40
  
  printf "Registers:\n"
  info registers
  
  printf "GDT (m->gdt):\n"
  x/8gx &m->gdt[0]
  
  continue
end

# Also break at panic to catch the end state
break panic
commands
  silent
  printf "\n=== PANIC ===\n"
  bt
  info registers
  x/10gx $rsp
  quit
end

continue

