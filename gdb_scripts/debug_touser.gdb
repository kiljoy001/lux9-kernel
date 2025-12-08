target remote :1234

# Break at touser to see what it pushes
break touser
commands
  silent
  printf "\n=== Reached touser ===\n"
  # Disassemble to see pushq immediates (UDSEL, UESEL)
  disassemble touser
  
  # Step until IRETQ frame is built (just before iretq)
  # The iretq is at the end of touser.
  # We can search for it or just step.
  # Let's step a few times.
  stepi 10
  printf "\n=== Stack at touser IRETQ ===\n"
  x/5gx $rsp
  
  # Also dump GDT to confirm segments
  printf "\n=== GDT Entries ===\n"
  # GDT is at m->gdt. We need to find m.
  # m is in %gs:0 or global 'm' variable.
  printf "m = %p\n", m
  if m != 0
    printf "m->gdt = %p\n", m->gdt
    # Dump entries 4, 5, 6 (UD, UD64, UE64)
    # Each entry is 8 bytes? No, Segdesc is 8 bytes (2 ints).
    # Wait, Segdesc structure: d0, d1 (u32int).
    x/4gx &m->gdt[4]
  end
  
  continue
end

# Break at _iretkernel to see the crash state
break _iretkernel
commands
  silent
  printf "\n=== Reached _iretkernel ===\n"
  printf "RSP = %p\n", $rsp
  printf "Stack (top 8 words):\n"
  x/8gx $rsp
  
  # Calculate what addq $40 will do
  printf "After addq $40, RSP will be %p\n", $rsp + 40
  printf "IRETQ will pop from %p:\n", $rsp + 40
  x/5gx $rsp + 40
  
  continue
end

# Continue execution
continue
