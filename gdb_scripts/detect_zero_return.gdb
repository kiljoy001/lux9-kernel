# Automated script to detect zero return address at _intrcommon entry
# Breaks only when return pointer is actually corrupted

# Set architecture
set architecture i386:x86-64

# Connect to QEMU
target remote localhost:1234

# Load kernel symbols
symbol-file lux9.elf

# Break at the true _intrcommon entry (before any stack adjustment)
# Use regular breakpoint, not tbreak, so it persists
break *0xffffffff803301b0

# Commands to run when breakpoint hits
commands 1
  silent
  # Check the return address at rsp+8 (before prologue runs)
  set $ret = *(unsigned long long*)($rsp+8)
  set $vec = *(unsigned long long*)($rsp)

  if ($ret == 0)
    printf "\n========================================\n"
    printf "FOUND IT! Zero return address detected!\n"
    printf "========================================\n\n"

    printf "Registers at corruption:\n"
    info registers

    printf "\nStack contents (rsp=%#llx):\n", $rsp
    x/16gx $rsp

    printf "\nBacktrace:\n"
    bt

    printf "\nVector number: %#llx\n", $vec
    printf "Return address: %#llx (CORRUPTED!)\n", $ret

    printf "\nDisassembly of _intrcommon:\n"
    disassemble _intrcommon

    # Stop here so we can investigate
    printf "\nStopping for manual investigation...\n"
  else
    # Return address is valid, continue silently
    printf "IRQ vector=%#llx ret=%#llx (OK)\n", $vec, $ret
    continue
  end
end

# Start execution
c

