set architecture i386:x86-64
set pagination off
# Adjust if your GDB auto-load policy blocks .gdbinit; load symbols explicitly.
symbol-file lux9.elf

# Break at cpuidentify once the higher-half mapping is active.
break cpuidentify

# Run until the breakpoint hits.
continue

# Print registers and nearby instructions.
info registers
x/10i $rip
bt

# You can single-step from here if desired.
