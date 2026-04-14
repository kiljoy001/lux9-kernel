#!/bin/bash
killall qemu-system-x86_64 2>/dev/null
qemu-system-x86_64 -M q35 -m 2G -cdrom lux9.iso -boot d -s -S -serial stdio & 
sleep 1
gdb -batch -ex "target remote :1234" -ex "break kernel/9front-pc64/devarch.c:757" -ex "c" -ex "si" -ex "info registers" -ex "x/i \$rip" -ex "q" lux9.elf
