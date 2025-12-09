# Lux9 Implementation Roadmap

## Current Status

**Completed ✅:**
- Blind Ledger (capabilities, epoch, rollback, PA index)
- Pebble (token-based allocation)
- Exchange (safe IPC with rollback)
- Borrow Checker (linear ownership)
- Lock DAG (deadlock prevention)
- Secure Ramdisk (Argon2id + XChaCha20)
- CLR foundations (Fruity IR, QBE backend, CBOR)
- Kernel builds successfully (22MB)

**Deferred 🔄:**
- Dynamic per-process vaults
- Vault-namespace integration
- Container runtime
- (Will revisit after userspace works)

## Phase 1: Get Userspace Working (PRIORITY)

**Goal:** Boot kernel → Run userspace init → Get working shell

### 1.1 Fix Current Userspace Init

```bash
# Current status
userspace/go_test/init - Minimal Go init (writes "Hello", loops forever)

# What we need
- Init that doesn't loop forever
- Init that spawns a shell
- Init that can exec other programs
```

**Tasks:**
- [ ] Fix Go init to not infinite loop
- [ ] Add proper init: mount /dev, spawn shell
- [ ] Test: Boot → See shell prompt

### 1.2 Port Essential Userspace Programs

**Priority 1 (Bootable system):**
- [ ] `/bin/init` - Process 1, mounts /dev, spawns shell
- [ ] `/bin/sh` (rc shell) - Command interpreter
- [ ] `/bin/ls` - List files
- [ ] `/bin/cat` - Read files
- [ ] `/bin/echo` - Print to console

**Priority 2 (Usable system):**
- [ ] `/bin/mkdir` - Create directories
- [ ] `/bin/cp` - Copy files
- [ ] `/bin/rm` - Remove files
- [ ] `/bin/mount` - Mount filesystems
- [ ] `/bin/ps` - List processes

**Priority 3 (Development):**
- [ ] `/bin/grep` - Search files
- [ ] `/bin/sed` - Stream editor
- [ ] `/bin/awk` - Text processing
- [ ] `/bin/make` - Build system
- [ ] `/bin/cc` - C compiler (or tcc)

### 1.3 Build Initrd with Userspace

```bash
# Goal: Create initrd.tar with:
initrd/
├── bin/
│   ├── init
│   ├── sh
│   ├── ls
│   ├── cat
│   └── echo
├── dev/
│   ├── cons
│   ├── null
│   └── zero
└── lib/
    └── (if needed)

# Pack into initrd
tar cf initrd.tar initrd/

# Kernel boots with initrd, extracts, runs /bin/init
```

**Tasks:**
- [ ] Create initrd directory structure
- [ ] Build userspace binaries (Go or C)
- [ ] Pack into initrd.tar
- [ ] Update kernel to extract and run init
- [ ] Test: Boot → Runs /bin/init

### 1.4 Test Boot Sequence

**Expected flow:**
```
1. Bootloader loads kernel + initrd
2. Kernel boots, initializes devices
3. Kernel extracts initrd.tar to /
4. Kernel executes /bin/init (PID 1)
5. /bin/init mounts /dev
6. /bin/init spawns /bin/sh
7. User sees shell prompt: #
8. User runs: ls /
9. Output: bin dev lib
```

**Tasks:**
- [ ] Test full boot sequence
- [ ] Debug any panics/errors
- [ ] Verify shell is interactive
- [ ] Verify ls/cat/echo work

## Phase 2: CLR Integration (NEXT)

**Goal:** Run .NET/C# code in kernel and userspace

### 2.1 CLR Runtime Basics

**Current state:**
- ✅ Fruity IR (intermediate representation)
- ✅ QBE backend (code generation)
- ✅ CBOR serialization
- ❌ GC not implemented
- ❌ JIT not connected
- ❌ System.Object not defined

**Tasks:**
- [ ] Define System.Object base class
- [ ] Implement basic GC (mark-sweep)
- [ ] Connect JIT: IL → Fruity IR → QBE → x86-64
- [ ] Test: Compile "Hello World" C# to native

### 2.2 CLR Syscall Interface

**Goal:** C# code can call Lux9 syscalls

```csharp
// Example C# code
using System.Runtime.InteropServices;

class Program {
    [DllImport("kernel")]
    static extern int sys_write(int fd, byte[] buf, int len);

    static void Main() {
        byte[] msg = Encoding.ASCII.GetBytes("Hello from C#\n");
        sys_write(1, msg, msg.Length);
    }
}
```

**Tasks:**
- [ ] Define syscall marshaling layer
- [ ] Implement DllImport for kernel calls
- [ ] Test: C# calls sys_write, prints to console

### 2.3 CLR Memory Integration

**Goal:** C# allocations use Pebble + Blind Ledger

```csharp
// When C# does:
object obj = new MyClass();

// CLR should:
// 1. Call pebble_black_token(size, &cap)
// 2. Store capability in GC metadata
// 3. Return managed reference to C#

// When GC collects:
// 1. Blind Ledger burns capability
// 2. Pebble returns token to bank
```

**Tasks:**
- [ ] Hook CLR allocator to Pebble
- [ ] Store capabilities in GC metadata
- [ ] Implement GC → ledger_burn on collect
- [ ] Test: Allocate 1M objects, GC, verify no leaks

### 2.4 Simple CLR Test Program

**Goal:** Run a real C# program on Lux9

```csharp
// test.cs
using System;

class Test {
    static void Main() {
        Console.WriteLine("Lux9 CLR Test");

        // Test allocation
        for(int i = 0; i < 1000; i++) {
            string s = "Test " + i;
        }

        Console.WriteLine("1000 allocations OK");

        // Test GC
        GC.Collect();
        Console.WriteLine("GC completed");

        // Test syscall
        byte[] data = new byte[100];
        int n = Syscall.Read(0, data);
        Console.WriteLine($"Read {n} bytes");
    }
}
```

**Tasks:**
- [ ] Compile test.cs to Fruity IR
- [ ] JIT to native code
- [ ] Execute in kernel
- [ ] Verify output: "Lux9 CLR Test", "1000 allocations OK", etc.

## Phase 3: Userspace Stability (AFTER CLR)

**Goal:** Rock-solid userspace with all essential tools

### 3.1 Core Utilities (Plan 9 Port)

**Option A: Port from 9front**
- Take existing Plan 9 userspace
- Adapt syscalls to Lux9
- Build with gcc/clang

**Option B: Rewrite in Go**
- Simple, fast compilation
- Easy syscalls
- Good for rapid development

**Option C: Rewrite in C#**
- Test CLR integration
- Managed memory safety
- Cool factor

**Decision needed:** Which approach?

### 3.2 Device Drivers (Userspace)

**Goal:** Move drivers to userspace where possible

```
Current (kernel drivers):
- AHCI (disk)
- IDE (disk)
- UART (serial)
- PCI enumeration

Future (userspace drivers):
- Network card (via /dev/pci passthrough)
- Graphics (via /dev/pci passthrough)
- USB (via /dev/pci passthrough)
```

**Tasks:**
- [ ] Design /dev/pci interface for userspace
- [ ] Implement safe MMIO via capabilities
- [ ] Port example driver (e1000 NIC?)
- [ ] Test: Userspace driver sends network packet

### 3.3 Filesystem

**Current:** No persistent filesystem, only initrd

**Options:**
- Plan 9 fossil (native Plan 9 fs)
- 9P filesystem (network-based)
- Simple tar-based fs
- ext2 (for compatibility)

**Tasks:**
- [ ] Choose filesystem
- [ ] Implement basic read/write
- [ ] Test: Write file, reboot, read file

## Phase 4: Return to Vaults (FUTURE)

**Only after userspace is stable!**

### 4.1 Tier 2: Per-Process Vaults (RFSECURE)

- [ ] Add RFSECURE flag to rfork
- [ ] Implement ProcessVault structure
- [ ] Test: ssh-agent with vault

### 4.2 Tier 3: Namespace Vaults (RFFORTRESS)

- [ ] Add RFFORTRESS flag
- [ ] Integrate vault with Pgrp
- [ ] Test: Container with encrypted rootfs

### 4.3 Container Runtime

- [ ] Implement container CLI
- [ ] Image format (tar + manifest)
- [ ] Network namespaces
- [ ] Test: Run Alpine container

## Milestones

### Milestone 1: Bootable System ⏳
**Criteria:**
- [x] Kernel compiles
- [ ] Kernel boots
- [ ] Init runs
- [ ] Shell prompt appears
- [ ] `ls /` works

**Target:** 1-2 weeks

### Milestone 2: Working Shell 📋
**Criteria:**
- [ ] All Milestone 1 criteria
- [ ] Can run programs: ls, cat, echo, grep
- [ ] Can navigate directories
- [ ] Can read/write files (if fs implemented)

**Target:** 2-3 weeks

### Milestone 3: CLR Hello World 📋
**Criteria:**
- [ ] C# compiles to Fruity IR
- [ ] QBE generates x86-64
- [ ] JIT executes in kernel
- [ ] "Hello World" prints

**Target:** 3-4 weeks

### Milestone 4: CLR Integration 📋
**Criteria:**
- [ ] GC works (allocate + collect)
- [ ] Pebble integration complete
- [ ] Blind Ledger capabilities stored
- [ ] No memory leaks in stress test

**Target:** 4-6 weeks

### Milestone 5: Stable Userspace 📋
**Criteria:**
- [ ] 20+ essential utilities
- [ ] Filesystem works
- [ ] Can build programs
- [ ] System feels "usable"

**Target:** 6-8 weeks

### Milestone 6: Vaults & Containers 📋
**Criteria:**
- [ ] Per-process vaults work
- [ ] Namespace vaults work
- [ ] Container runtime works
- [ ] Can run multi-container app

**Target:** 8-12 weeks

## Immediate Next Steps (This Week)

**Priority 1:**
1. Fix Go init (remove infinite loop)
2. Add basic init: mount /dev, spawn shell
3. Build minimal initrd with Go programs
4. Test boot sequence

**Priority 2:**
5. Port rc shell (or simple sh in Go)
6. Implement ls, cat, echo in Go
7. Test interactive shell

**Priority 3:**
8. Document current CLR state
9. Identify CLR blockers
10. Create CLR test plan

## Decision Points

**Right now, we need to decide:**

1. **Userspace language?**
   - Option A: Port Plan 9 C code (mature, stable)
   - Option B: Write in Go (fast dev, good syscalls)
   - Option C: Write in C# (test CLR, cool factor)

2. **Init system?**
   - Simple init (just spawn shell)
   - Full init (service management)

3. **Filesystem?**
   - Defer (use initrd only)
   - Implement now (which one?)

**My recommendation:**
- Userspace: **Go** (fastest path to working system)
- Init: **Simple** (spawn shell, that's it)
- Filesystem: **Defer** (initrd is enough for now)

**This gets us to a working shell fastest, then we tackle CLR.**

---

**Focus for next session:**
1. Fix userspace/go_test/init
2. Build working initrd
3. Boot to shell prompt

Everything else waits until we can type commands interactively.
