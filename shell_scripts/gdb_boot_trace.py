import gdb

def debug_session():
    gdb.execute("target remote :1234")
    gdb.execute("set pagination off")
    
    # Break at kernel main to confirm boot
    gdb.execute("break main")
    
    # Break at touser (transition to userspace)
    gdb.execute("break touser")
    
    # Break at sys_write (syscall handler)
    gdb.execute("break sys_write")
    
    print("Continuing to main...")
    gdb.execute("continue")
    
    print("Hit main? Continuing to touser...")
    gdb.execute("continue")
    
    print("Hit touser? Stepping into userspace...")
    gdb.execute("stepi")
    gdb.execute("info registers")
    
    print("Continuing to possible syscall...")
    gdb.execute("continue")
    gdb.execute("bt")

debug_session()
