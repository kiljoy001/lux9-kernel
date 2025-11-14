# GDB script to capture the exact exit point
# Run this after starting GDB

# Set breakpoints on potential exit points
break pageowninit
break putstrn0
break putstrn0+50  # Set breakpoint near end of function

# Set watchpoint on program counter to catch last instruction
watch $pc

# Continue execution  
continue

# When it breaks at $pc change, examine:
# 1. What register values we have
info registers

# 2. Where we are in memory
x/10i $pc

# 3. Call stack
backtrace

# 4. Current function info
info frame

# Continue to next break
continue