#!/bin/bash
cd /home/scott/Repo/lux9-kernel || exit

# Log file path
LOGFILE="/home/scott/Repo/lux9-kernel/qemu.log"

# Run QEMU headless with serial output redirected to logfile
qemu-system-x86_64 \
  -M q35 \
  -m 2G \
  -cdrom lux9.iso \
  -boot d \
  -nographic \
  -serial file:"$LOGFILE" \
  -no-reboot \
  -s -S &

# Wait two seconds for VM launch:
sleep 2

# Start GDB and connect automatically
gdb -ex "target remote tcp:localhost:1234" lux9.elf
