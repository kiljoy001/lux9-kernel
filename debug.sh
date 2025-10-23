#!/bin/bash
killall qemu-system-x86_64 2>/dev/null
qemu-system-x86_64 -M q35 -m 2G -cdrom lux9.iso -boot d -s -S -serial stdio & 
sleep 1
gdb -batch -ex "target remote :1234" -ex "break proc0" -ex "continue" -ex "while 1" -ex "stepi" -ex "if \$rip < 0xffff000000000000" -ex "printf \"SUSPICIOUS: RIP=0x%016lx RSP=0x%016lx\n\", \$rip, \$rsp" -ex "end" -ex "end" lux9.elf
