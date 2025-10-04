# Lux9 Initrd Boot Process

## Overview

Lux9 uses an initial ramdisk (initrd) approach similar to Linux, but adapted for the microkernel + 9P architecture.

## Boot Sequence

```
1. Bootloader (Limine) loads:
   - Kernel binary (lux9.elf)
   - Initrd tarball (initrd.tar) into memory

2. Kernel boots:
   - Initialize hardware (MMU, interrupts, APIC, etc.)
   - Setup initial memory allocator
   - Mount initrd as temporary root filesystem
   - Create first process: /bin/init
   - Jump to scheduler

3. Init process runs:
   - Parse kernel command line for root= parameter
   - Start ext4fs server with root device
   - Mount real root via sys_mount("/", ext4fs_pid)
   - Start other essential servers (devfs, procfs)
   - Pivot to real root or exec /sbin/init from real FS
   - Start system services
```

## Initrd Structure

```
initrd/
├── bin/
│   ├── init           # First userspace process
│   ├── ext4fs         # Filesystem server
│   ├── sh             # Minimal shell (for emergency)
│   └── fscheck        # Filesystem checker
├── lib/
│   └── (none - statically linked binaries)
└── dev/
    └── (empty - devfs will populate)
```

## Initrd Format

**TAR format** (simple, no compression needed for now):
- Easy to parse in kernel
- No dependencies
- Directly addressable in memory

## Kernel Initrd Support

### Data Structures

```c
// kernel/9front-pc64/initrd.h

struct tar_header {
    char name[100];
    char mode[8];
    char uid[8];
    char gid[8];
    char size[12];
    char mtime[12];
    char checksum[8];
    char type;
    // ... rest of TAR header
};

struct initrd_file {
    char *name;
    void *data;
    size_t size;
    struct initrd_file *next;
};

extern struct initrd_file *initrd_root;
```

### Kernel Functions

```c
// kernel/9front-pc64/initrd.c

void initrd_init(void *addr, size_t len);
void* initrd_find(char *path);
int initrd_read(char *path, void *buf, size_t offset, size_t len);
```

## Init Process

```c
// userspace/bin/init.c

int main(int argc, char **argv)
{
    // 1. Parse kernel cmdline
    char *rootdev = get_kernel_param("root");
    if(!rootdev)
        rootdev = "hd0:0";  // Default

    // 2. Start filesystem server
    pid_t fs_pid = fork();
    if(fs_pid == 0) {
        execl("/bin/ext4fs", "ext4fs", rootdev, NULL);
        panic("cannot exec ext4fs");
    }

    // Give server time to initialize
    sleep(100);  // 100ms

    // 3. Mount root filesystem
    if(mount("/", fs_pid, "9P2000") < 0)
        panic("cannot mount root");

    // 4. Verify mount worked
    int fd = open("/etc/fstab", O_RDONLY);
    if(fd < 0) {
        // Root FS not ready, drop to emergency shell
        execl("/bin/sh", "sh", NULL);
    }
    close(fd);

    // 5. Start other servers
    start_server("/bin/devfs", NULL);
    start_server("/bin/procfs", NULL);

    // 6. Execute real init from mounted FS
    execl("/sbin/init", "init", NULL);

    // If that failed, start shell
    execl("/bin/sh", "sh", NULL);

    panic("init: no shell");
}
```

## Kernel Syscalls Needed

### sys_mount
```c
// Mount a 9P server at a path
int sys_mount(char *path, pid_t server_pid, char *proto);

// Example:
mount("/", ext4fs_pid, "9P2000");
mount("/dev", devfs_pid, "9P2000");
mount("/proc", procfs_pid, "9P2000");
```

### sys_get_cmdline
```c
// Get kernel command line parameter
int sys_get_cmdline(char *buf, size_t len);

// Kernel stores: "root=hd0:0 verbose"
// Init parses it
```

### Existing syscalls used:
- `fork()` - Create server process
- `exec()` - Load server binary
- `open()` / `read()` / `write()` - File I/O (via 9P)
- `send()` / `recv()` - IPC messages

## Building Initrd

### Makefile target

```makefile
# userspace/Makefile

INITRD_DIR = initrd
INITRD_TAR = initrd.tar

$(INITRD_TAR): $(SERVERS) $(UTILS) bin/init
	mkdir -p $(INITRD_DIR)/bin
	mkdir -p $(INITRD_DIR)/dev
	cp servers/ext4fs/ext4fs $(INITRD_DIR)/bin/
	cp bin/init $(INITRD_DIR)/bin/
	cp bin/fscheck $(INITRD_DIR)/bin/
	tar -cf $@ -C $(INITRD_DIR) .

initrd: $(INITRD_TAR)
```

### GNUmakefile integration

```makefile
# Top-level GNUmakefile

lux9.iso: lux9.elf userspace/initrd.tar
	rm -rf iso_root
	mkdir -p iso_root/boot
	cp lux9.elf iso_root/boot/
	cp userspace/initrd.tar iso_root/boot/
	cp limine.conf iso_root/boot/
	# ... rest of ISO build
```

## Limine Configuration

```
# limine.conf

PROTOCOL=limine
KERNEL_PATH=boot:///boot/lux9.elf
MODULE_PATH=boot:///boot/initrd.tar
KERNEL_CMDLINE=root=hd0:0
```

Limine will pass initrd location to kernel via boot protocol.

## Kernel Boot Code

```c
// kernel/9front-pc64/main.c

void
main(void)
{
    // Parse Limine boot info
    struct limine_module_response *modules = module_request.response;
    void *initrd_addr = modules->modules[0]->address;
    size_t initrd_size = modules->modules[0]->size;

    // Initialize subsystems
    mmuinit();
    trapinit();
    schedulerinit();

    // Mount initrd
    initrd_init(initrd_addr, initrd_size);

    // Load init binary from initrd
    void *init_binary = initrd_find("/bin/init");
    size_t init_size = initrd_filesize("/bin/init");

    // Create init process
    create_init_process(init_binary, init_size);

    // Start scheduler - never returns
    scheduler();
}
```

## Emergency Shell

If init fails, kernel can drop to emergency shell:

```c
if(init_failed) {
    // Load shell from initrd
    void *sh = initrd_find("/bin/sh");
    create_process(sh, shell_size);
}
```

Shell runs entirely from initrd - can manually start servers, mount, etc.

## Debugging

### Kernel command line options:
- `root=hd0:0` - Root device
- `init=/bin/sh` - Run shell instead of init
- `verbose` - Debug output
- `single` - Single user mode

### Serial output:
- Init logs to serial console
- Servers log errors
- Kernel prints mount/process events

## Migration from Initrd to Real Root

Two approaches:

### Option A: Pivot Root (Linux-style)
```c
// Keep initrd mounted at /initrd
// Mount real root at /mnt
mount("/mnt", ext4fs_pid, "9P2000");

// Switch roots
pivot_root("/mnt", "/initrd");

// Continue with /sbin/init from real FS
```

### Option B: Re-exec (simpler)
```c
// Mount real root at /
mount("/", ext4fs_pid, "9P2000");

// Just exec the real init
execl("/sbin/init", "init", NULL);

// Initrd is discarded (memory freed)
```

**Lux9 uses Option B** - simpler, no pivot needed.

## Memory Management

- Initrd stays in physical memory until no longer needed
- After mounting real root and exec'ing /sbin/init:
  - Kernel can free initrd memory
  - Reclaim ~1-2MB of RAM
- Mark initrd pages as free in pmm

## Testing

```bash
# Build initrd
cd userspace
make initrd

# Build ISO with initrd
cd ..
make

# Test in QEMU
qemu-system-x86_64 -cdrom lux9.iso -serial stdio

# Should see:
# [kernel] Initrd loaded at 0x...
# [kernel] Starting init
# [init] Mounting root from hd0:0
# [init] Starting ext4fs server
# [init] Root filesystem ready
# [sh] #
```

## Future Enhancements

1. **Compressed initrd** - gzip/lz4 compression
2. **Multiple initrds** - Modular loading
3. **Initrd scripts** - Shell scripts in initrd for complex boot
4. **Network boot** - Load initrd over PXE
5. **Encrypted root** - Unlock in initrd before mount

## Summary

Initrd approach gives us:
- ✅ Clean separation of kernel and FS code
- ✅ Flexible boot process
- ✅ Emergency recovery shell
- ✅ Standard bootloader integration
- ✅ Easy to update servers without kernel rebuild
- ✅ Works with any bootloader (Limine, GRUB, etc.)
