# GDB Script for MMU Debugging
# Save as debug_mmu.gdb and run with: gdb -x debug_mmu.gdb lux9.elf

# Connect to QEMU
target remote :1234

# Set up logging
set logging on mmu_debug.log
set logging redirect on

# Set breakpoints at critical functions
break proc0
break segpage
break mmuwalk
break mmucreate
break mmuswitch
break dbg_getpte

# Continue to proc0
continue

# When we hit proc0, set up detailed monitoring
commands
    printf "=== HIT PROC0 ===\n"
    info registers
    x/4gx m->pml4
    continue
end

# Set up continuous stepping with monitoring
define mmu_step_debug
    while 1
        stepi
        # Log every 1000 instructions to avoid huge logs
        if ($rip & 0x3ff) == 0
            printf "RIP: 0x%016lx RSP: 0x%016lx\n", $rip, $rsp
            # Check critical page table entries
            if m->pml4 != 0
                x/2gx m->pml4+255
            end
            # Check stack validity
            if $rsp < 0xffff800000000000
                printf "WARNING: Stack pointer invalid: 0x%016lx\n", $rsp
            end
        end
        # Break on suspicious conditions
        if $rip == 0
            printf "ERROR: RIP is zero!\n"
            info registers
            bt
            return
        end
        # Break on page fault handler (common in triple faults)
        if $rip >= 0xffffffff80200000 && $rip <= 0xffffffff80201000
            printf "PAGE FAULT HANDLER INVOKED\n"
            info registers
        end
    end
end

# Run the debugging
mmu_step_debug