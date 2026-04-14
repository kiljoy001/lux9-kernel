#!/bin/bash
rm -f boot_rump.log
qemu-system-x86_64 -cdrom lux9.iso -m 1G -serial file:boot_rump.log -display none -no-reboot &
PID=$!
sleep 15
kill $PID
echo "QEMU Test Finished"
