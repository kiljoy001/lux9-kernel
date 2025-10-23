#!/bin/bash
# mmu_debug.sh - Focused MMU debugging

killall qemu-system-x86_64 2>/dev/null
qemu-system-x86_64 -M q35 -m 2G -cdrom lux9.iso -boot d -s -S -serial stdio & 
sleep 1

gdb -batch \
    -ex "target remote :1234" \
    -ex "break proc0" \
    -ex "continue" \
    -ex "printf \"==== ENTERING PROC0 DEBUG ====\\n\"" \
    -ex "break segpage" \
    -ex "continue" \
    -ex "printf \"==== HIT SEGPGAE ====\\n\"" \
    -ex "info args" \
    -ex "while 1" \
    -ex "  stepi" \
    -ex "  # Check for stack corruption every few steps" \
    -ex "  if (\$rip & 0x1f) == 0" \
    -ex "    if \$rsp < 0xffff800000000000" \
    -ex "      printf \"WARNING: RSP CORRUPTION: 0x%016lx\\n\", \$rsp" \
    -ex "    end" \
    -ex "    printf \"RIP: 0x%016lx RSP: 0x%016lx\\n\", \$rip, \$rsp" \
    -ex "  end" \
    -ex "end" \
    lux9.elf