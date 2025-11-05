# Specific GDB commands to debug the CR3 switch failure
# Based on the trampoline disassembly:
# 0: 48 89 f0     mov    %rsi,%rax      (save continuation to %rax)
# 3: fa           cli                   (disable interrupts)
# 4: 0f 22 df     mov    %rdi,%cr3      (switch CR3)
# 7: eb 00        jmp    9              (jump to next instruction)
# 9: ff e0        jmp    *%rax          (jump to continuation)

echo === CR3 Switch Debug Commands ===\n

# Break at setuppagetables entry
break setuppagetables

# Break when about to call trampoline
break mmu.c:525
commands
  echo Breaking at trampoline call\n
  print pml4_phys
  print continuation
  x/20i cr3_switch_trampoline
  continue
end

# Set a breakpoint at the trampoline itself
break *cr3_switch_trampoline
commands
  echo === ENTERING CR3 SWITCH TRAMPOLINE ===\n
  echo Parameters:\n
  echo RDI (new CR3): 
  print/x $rdi
  echo RSI (continuation): 
  print/x $rsi
  echo RAX (will be set to continuation): 
  print/x $rsi
  echo \n
  echo Current RIP: 
  print/x $rip
  echo Current CR3: 
  print/x $cr3
  echo \n
  echo About to step through trampoline...\n
end

# Break at the CLI instruction (after saving continuation)
break *cr3_switch_trampoline+3
commands
  echo === ABOUT TO DISABLE INTERRUPTS ===\n
  print/x $rsi  # continuation address saved in RAX
  echo \n
end

# Break at the CR3 switch instruction
break *cr3_switch_trampoline+4
commands
  echo === ABOUT TO SWITCH CR3 ===\n
  echo Old CR3: 
  print/x $cr3
  echo New CR3 (RDI): 
  print/x $rdi
  echo \n
  echo STEP OVER THIS INSTRUCTION!\n
end

# Break right after CR3 switch
break *cr3_switch_trampoline+7
commands
  echo === CR3 SWITCHED! ===\n
  echo New CR3 value: 
  print/x $cr3
  echo Current RIP: 
  print/x $rip
  echo \n
  echo Check if we can access memory...\n
end

# Break at continuation point
break *after_cr3_switch
commands
  echo === RESUMED AFTER CR3 SWITCH ===\n
  echo Current RIP: 
  print/x $rip
  echo Current CR3: 
  print/x $cr3
  echo RSP: 
  print/x $rsp
  echo \n
  echo Stack trace:\n
  backtrace
  echo \n
  echo Check HHDM access:\n
end

# Set display for important registers
display/x $cr3
display/x $rip
display/x $rsp
display/x $rdi
display/x $rsi

echo Ready to debug! Use 'continue' to start.\n
echo Watch for the CR3 switch sequence...\n