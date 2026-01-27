#!/bin/bash
# Send commands to 9front and get output

run_in_9front() {
    echo "$1" > /home/scott/Repo/VM-Interop/command.txt
    echo "Sent command: $1"
    sleep 2  # Wait for execution
    if [ -f /home/scott/Repo/VM-Interop/output.txt ]; then
        cat /home/scott/Repo/VM-Interop/output.txt
        rm /home/scott/Repo/VM-Interop/output.txt
    fi
}

case "$1" in
    build-vmx)
        run_in_9front "cp /n/interop/vmx/* /sys/src/cmd/vmx/ && cd /sys/src/cmd/vmx && mk clean && mk install && echo VMX BUILT"
        ;;
    test-alpine)
        run_in_9front "dd -if /dev/zero -of /tmp/alpine.img -bs 1048576 -count 1024 && vmx -M 512 -I /n/interop/alpine-virt-3.19.0-x86_64.iso -d /tmp/alpine.img kernel"
        ;;
    *)
        # Pass through any command
        run_in_9front "$*"
        ;;
esac