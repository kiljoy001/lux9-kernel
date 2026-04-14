# Set remote connection timeout to 10 seconds
set remotetimeout 10

# Connect to QEMU GDB server
target remote :1234
# Continue immediately after connection to unfreeze QEMU
continue

# Set architecture
set architecture i386:x86-64

# Load kernel symbols
add-symbol-file lux9.elf 0xffffffff80100000

# Set a breakpoint at the entry point of qbe_compile_page
b qbe_compile_page
# When hit, print registers and disassembly, then continue
commands
  echo 
Breakpoint at qbe_compile_page
  info registers
  x/10i $pc
  continue
end

# Set a breakpoint at the entry point of exchange_fmemopen_handle
b exchange_fmemopen_handle
commands
  echo 
 Breakpoint at exchange_fmemopen_handle
  info registers
  x/10i $pc
  continue
end

# Set a breakpoint at the entry point of parse (QBE's parser)
b parse
commands
  echo 
 Breakpoint at parse (QBE parser)
  info registers
  x/10i $pc
  # Print arguments of parse: (FILE *in_fp, char *name, void (*emit_data)(Dat *), void (*emit_func)(Fn *))
  # Assuming standard x86-64 System V ABI: rdi, rsi, rdx, rcx, r8, r9
  # parse(in_fp (rdi), "<exchange-page>" (rsi), emit_data (rdx), emit_func (rcx))
  echo in_fp: 
  p $rdi
  echo name: 
  p $rsi
  echo emit_data: 
  p $rdx
  echo emit_func: 
  p $rcx
  continue
end

# Set a breakpoint at the entry point of die_ (QBE's error handler)
b die_
commands
  echo 
 Breakpoint at die_ (QBE error handler)
  info registers
  x/10i $pc
  # Print arguments of die_: (char *file, char *s, ...)
  # Assuming standard x86-64 System V ABI: rdi, rsi
  echo file: 
  p $rdi
  echo error_message_format: 
  p $rsi
  continue
end

# Set a breakpoint at the kernel's panic function
b panic
commands
  echo 
 Breakpoint at kernel panic!
  info registers
  x/10i $pc
  # Print argument of panic: (const char *fmt, ...)
  echo panic_message_format: 
  p $rdi
  bt
  quit
end

# Continue execution - this one will hit after all breakpoints are set
continue
