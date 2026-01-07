# VM Interoperability Directory

This directory serves as a shared filesystem between different VMs and operating systems using the 9P protocol.

## Access Methods

### From 9front/Plan 9 (using rc shell)
```
srv tcp!10.0.2.2!9564 interop
mount /srv/interop /n/interop
```

Or use the connect script:
```
rc /n/interop/connect.rc
```

### From Linux
```bash
sudo mount -t 9p -o trans=tcp,port=9564 10.0.2.2 /mnt/interop
```

### From macOS (with plan9port)
```bash
9pfuse 'tcp!localhost!9564' /mnt/interop
```

## Server Details
- Protocol: 9P2000 (via u9fs)
- Port: 9564
- Root: /home/scott/Repo/VM-Interop
- Authentication: none (local network only)

## Files can be shared between:
- 9front VMs
- Linux VMs
- BSD systems
- Any OS with 9P client support