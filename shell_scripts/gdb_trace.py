import gdb
import time

def debug_session():
    gdb.execute("target remote :1234")
    gdb.execute("set pagination off")
    
    print("Initial state:")
    gdb.execute("info registers")
    gdb.execute("x/10i $pc")

    # Set breakpoint at kernel main
    gdb.execute("break main")
    gdb.execute("break userinit")
    gdb.execute("break init0")
    
    print("Continuing execution (will timeout in 5s if not hit)...")
    
    # We can't easily timeout a 'continue' in via python API synchronously without blocking
    # but we can try just 'c' and if it returns (breakpoint hit) great.
    # If it hangs, we rely on the outer timeout command calling this script? 
    # No, that kills GDB.
    
    # For now, let's just inspect entry state and maybe step a bit.
    # Or assuming the user manually started QEMU without -S, we just attach and inspect.
    pass

debug_session()
