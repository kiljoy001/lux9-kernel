#!/bin/bash
echo "Running xalloc test..."
echo "test" > /tmp/qemu_input
cd /home/scott/Repo/lux9-kernel
timeout 30s make run < /tmp/qemu_input
