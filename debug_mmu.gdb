# GDB script for debugging mmuswitch
set confirm off
set pagination off
set print pretty on

# Set breakpoint in mmuswitch
break mmuswitch
break mmuwalk
break putmmu

# Continue execution
continue