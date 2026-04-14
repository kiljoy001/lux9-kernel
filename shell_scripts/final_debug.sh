#!/bin/bash
# final_debug.sh - Targeted debugging for PTE issues

killall qemu-system-x86_64 2>/dev/null
qemu-system-x86_64 -M q35 -m 2G -cdrom lux9.iso -boot d -s -S -serial stdio & 
sleep 1

gdb -batch \
    -ex "target remote :1234" \
    -ex "break proc0" \
    -ex "continue" \
    -ex "printf \"==== DEBUGGING PTE SETUP ====\\n\"" \
    -ex "# Watch for key MMU functions" \
    -ex "break mmuwalk" \
    -ex "commands" \
    -ex "  printf \"mmuwalk(va=0x%016lx, level=%d, create=%d)\\n\", \$rsi, \$rdx, \$rcx" \
    -ex "  continue" \
    -ex "end" \
    -ex "break mmucreate" \
    -ex "commands" \
    -ex "  printf \"mmucreate(level=%d, index=%d)\\n\", \$rdx, \$rcx" \
    -ex "  continue" \
    -ex "end" \
    -ex "break dbg_getpte" \
    -ex "commands" \
    -ex "  printf \"dbg_getpte(va=0x%016lx)\\n\", \$rdi" \
    -ex "  continue" \
    -ex "end" \
    -ex "continue" \
    lux9.elf