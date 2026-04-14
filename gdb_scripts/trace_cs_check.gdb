set architecture i386:x86-64
target remote localhost:1234
symbol-file lux9.elf

# Break at the CS comparison instruction
break *0xffffffff80331b54

commands 1
  silent
  set $cs_on_stack = *(unsigned short*)($rsp+0x30)
  set $vec_on_stack = *(unsigned long long*)($rsp+0x18)

  printf "\n========== CS CHECK ==========\n"
  printf "At cmpw $0x8,0x30(%%rsp) instruction\n"
  printf "  RSP = %#llx\n", $rsp
  printf "  CS at rsp+0x30 = %#x\n", $cs_on_stack
  printf "  Vector at rsp+0x18 = %#llx\n", $vec_on_stack
  printf "  Expected CS = 0x8\n"

  if ($cs_on_stack == 0x8)
    printf "  SHOULD JUMP to _intrnested\n"
  else
    printf "  ERROR: CS mismatch! Will NOT jump\n"
  end

  printf "Stack dump from rsp:\n"
  x/16gx $rsp
  printf "==============================\n\n"

  continue
end

c
