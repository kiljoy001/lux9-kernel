# GDB commands to debug CR3 switch failure in setuppagetables()
# Load symbols for the kernel
symbol-file lux9.elf

# Set breakpoints to trace the issue
break setuppagetables
break cr3_switch_trampoline
break *0x1000  # Break at trampoline address in low memory

# Commands to run before starting the kernel
echo Setting up CR3 switch debugging...\n

# Set display of CR3 register
display/x $cr3
display/x $rip
display/x $rsp

# Set pagination off for easier debugging
set pagination off
set confirm off

echo Ready to debug. Use 'continue' to start kernel and trace CR3 switch.\n
echo Key breakpoints:\n
echo - setuppagetables: Entry to setuppagetables()\n
echo - cr3_switch_trampoline: The trampoline function\n
echo - *0x1000: Low memory trampoline execution\n
echo \n
echo When you hit the break at cr3_switch_trampoline, use:\n
echo 'stepi' to step through the trampoline\n
echo 'info registers' to check CR3 before switch\n
echo 'nexti' to step to MOV CR3 instruction\n
echo 'info registers' to check CR3 after switch\n
echo \n
echo If it hangs at CR3 switch, check:\n
echo 'backtrace' to see call stack\n
echo 'info registers' for CPU state\n
echo 'x/20i $rip' to see current instruction\n