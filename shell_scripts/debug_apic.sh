#!/bin/bash
killall qemu-system-x86_64 2>/dev/null
# Using -cpu host as requested to test APIC behavior with host features
qemu-system-x86_64 -M q35 -m 2G -cdrom lux9.iso -boot d -s -S -serial stdio &
sleep 1
gdb -batch -ex "target remote :1234" \
    -ex "break i8253init" \
    -ex "c" \
    -ex "n" \
    -ex "n" \
    -ex "n" \
    -ex "printf \"After i8253init. m->cpuhz = %lld (addr: %p)\\n\", m->cpuhz, &m->cpuhz" \
    -ex "break todsetfreq" \
    -ex "c" \
    -ex "printf \"Entered todsetfreq. freq = %lld, m->cpuhz = %lld (addr: %p)\\n\", freq, m->cpuhz, &m->cpuhz" \
    -ex "q" \
    lux9.elf
