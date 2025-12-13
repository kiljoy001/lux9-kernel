# Secure Executable Vault - Immutable Runtime Binaries

## Your Original Vision

**The Idea:**
```
Boot Sequence:
1. Load kernel from disk (one-time trust establishment)
2. Copy critical system binaries to /dev/secureram
3. Lock vault (encrypt at rest)
4. Execute all system utilities from encrypted RAM
5. Disk-based binaries NEVER executed after boot

Goal: Protect runtime binaries from disk-based tampering
```

**This is brilliant because:**
- Disk compromise doesn't affect running system
- Binaries can't be trojaned while system is running
- Runtime integrity protected by encryption
- Immutable execution environment

## Current Design Gap

**What /dev/secureram does now:**
- ✅ Encrypted storage
- ✅ Password protection
- ✅ Secure wipe
- ❌ **NOT executable** (no PROT_EXEC)

**What you need:**
- Execute binaries directly from /dev/secureram
- Map pages as executable
- Verify integrity before unlock
- Prevent modification after load

## Two Design Approaches

### Approach 1: Execute-In-Place from /dev/secureram

**Architecture:**
```
Boot:
  disk:/bin/ls → read → /dev/secureram[offset=0, len=64KB]
  disk:/bin/cat → read → /dev/secureram[offset=64KB, len=32KB]
  ...

Runtime:
  execve("/secure/ls") → mmap(/dev/secureram, offset=0, PROT_EXEC)
                      → verify hash
                      → unlock region
                      → execute
```

**Pros:**
- Binaries never in disk cache after boot
- Encrypted at rest in RAM
- Can lock/unlock individual executables

**Cons:**
- Need PROT_EXEC support in devram.c
- Need ELF loader integration
- Complex offset management

### Approach 2: Initramfs-Style Secure Vault

**Architecture:**
```
Boot:
  1. Load initrd.tar from disk
  2. Verify cryptographic signature (TPM-backed)
  3. Extract to /dev/secureram (entire rootfs)
  4. Mount /dev/secureram as root filesystem
  5. Pivot root to secure vault
  6. Lock vault (encrypt everything)
  7. Disk is now untrusted

Runtime:
  execve("/bin/ls") → already in /dev/secureram
                    → page fault → decrypt on demand
                    → execute
```

**Pros:**
- Standard filesystem semantics
- Entire rootfs protected
- Disk becomes read-only reference

**Cons:**
- Need filesystem support in devram.c
- More complex than current implementation

## Threat Model: What This Protects Against

### ✅ Protects Against:

**1. Disk Tampering (Offline Attack)**
```
Attacker scenario:
1. Attacker gains physical access
2. Boots live USB, mounts disk
3. Replaces /bin/su with trojan
4. Reboots system

Without secure vault:
  ❌ User runs /bin/su → executes trojan → owned

With secure vault:
  ✅ System boots, copies clean /bin/su to /dev/secureram
  ✅ Disk /bin/su is trojaned, but never executed
  ✅ All execution from encrypted vault
```

**2. Runtime Tampering (Rootkit Prevention)**
```
Attacker scenario:
1. Attacker exploits web service, gains root
2. Tries to install rootkit: cp rootkit.so /lib/
3. Tries to modify binary: sed -i 's/...' /bin/login

Without secure vault:
  ❌ Rootkit installed, next boot is compromised

With secure vault:
  ✅ /lib/ is in encrypted vault, read-only
  ✅ Attacker can't modify running binaries
  ✅ Reboot restores clean state
```

**3. Supply Chain Attacks (Package Manager)**
```
Attacker scenario:
1. Attacker compromises package repository
2. User runs: apt-get install update
3. Malicious deb replaces /usr/bin/ssh

Without secure vault:
  ❌ SSH binary trojaned
  ❌ Next SSH connection leaks credentials

With secure vault:
  ✅ Package writes to disk, but disk is untrusted
  ✅ Running system uses vault copy
  ✅ Admin must explicitly update vault
```

**4. Persistent Backdoors**
```
Attacker scenario:
1. Attacker modifies /etc/cron.d/backdoor
2. Backdoor persists across reboots

Without secure vault:
  ❌ Backdoor runs every boot

With secure vault:
  ✅ /etc/ is in vault, locked
  ✅ Disk modifications ignored
  ✅ Reboot = clean state
```

### ❌ Does NOT Protect Against:

**1. Memory Tampering (Runtime Code Injection)**
- Attacker with kernel exploits can still modify vault memory
- Would need hardware support (SGX/TrustZone)

**2. Boot-Time Attacks**
- If bootloader/kernel compromised, vault can be trojaned during load
- Would need Secure Boot + TPM attestation

**3. Side-Channel Attacks**
- Spectre/Meltdown can leak vault contents
- Would need hardware mitigations

## Implementation Plan

### Phase 1: Add Execute Permission (Minimal)

**Goal:** Allow mmap(PROT_EXEC) on /dev/secureram

**Changes to devram.c:**

```c
// Add executable region tracking
typedef struct SecureExecRegion {
    ulong offset;
    ulong size;
    uchar hash[32];  // SHA256 of binary
    int locked;
} SecureExecRegion;

static SecureExecRegion exec_regions[64];

// New control command: load <offset> <size> <hash>
// Example: echo 'load 0 65536 sha256:abcd...' > /dev/secureram.ctl

static long
ramwrite(Chan *c, void *va, long n, vlong off)
{
    // ...
    case Qsecureramctl:
        if(strcmp(argv[0], "load") == 0) {
            ulong offset = strtoul(argv[1], 0, 0);
            ulong size = strtoul(argv[2], 0, 0);
            // Compute hash of loaded binary
            // Store in exec_regions[]
        }
}

// Allow mmap with PROT_EXEC
// (integration with segment.c needed)
```

**Usage:**
```bash
# Boot sequence
echo 'init RootPassword' > /dev/secureram.ctl

# Load system binaries
cat /bin/ls > /dev/secureram  # offset 0
echo 'load 0 65536 sha256:abc...' > /dev/secureram.ctl

cat /bin/cat >> /dev/secureram  # offset 65536
echo 'load 65536 32768 sha256:def...' > /dev/secureram.ctl

# Lock vault
echo 'lock' > /dev/secureram.ctl

# Execute (needs kernel support)
/secure/ls  # maps /dev/secureram[0:65536] as PROT_EXEC
```

### Phase 2: Filesystem Support (Better UX)

**Goal:** Mount /dev/secureram as a filesystem

**Option A: tar-based (like initramfs)**

```c
// devram.c: add tar extraction
static void
extract_tar_to_vault(uchar *tar_data, ulong tar_size)
{
    // Parse tar archive
    // Extract files to vault
    // Build directory structure in memory
}

// Control command: loadtar
ramwrite() {
    if(strcmp(argv[0], "loadtar") == 0) {
        // Read tar from stdin
        // Extract to vault
        // Lock when done
    }
}
```

**Usage:**
```bash
# Create secure rootfs
tar czf secure-rootfs.tar /bin /sbin /lib /etc

# Boot: load into vault
cat secure-rootfs.tar > /dev/secureram
echo 'loadtar' > /dev/secureram.ctl
echo 'lock' > /dev/secureram.ctl

# Mount as root
mount -t secureram /dev/secureram /secure
chroot /secure /bin/sh
```

**Option B: Custom filesystem (ramfs-like)**

```c
// New file: kernel/9front-port/devsecurefs.c
// Implement Plan 9 filesystem protocol
// Store files in encrypted vault
// Support: create, read, write, remove, walk

Dev securefs = {
    .attach = securefsattach,
    .walk = securefswalk,
    .read = securefsread,
    .write = securefswrite,
    // ... full filesystem implementation
};
```

**Usage:**
```bash
# Boot: mount secure filesystem
mount -t securefs /dev/secureram /secure

# Copy system files
cp -r /bin /sbin /lib /etc /secure/

# Lock
echo 'lock' > /dev/secureram.ctl

# Use as root
chroot /secure /bin/sh
```

### Phase 3: Integrity Verification

**Goal:** Cryptographically verify binaries before execution

**Components:**

1. **Boot-time manifest:**
```bash
# Create signed manifest
sha256sum /bin/* /sbin/* > manifest.txt
gpg --sign manifest.txt

# Store in vault metadata
echo 'setmanifest' > /dev/secureram.ctl < manifest.txt.sig
```

2. **Execution-time verification:**
```c
// In exec() syscall (kernel/9front-port/sysproc.c)
int
sysexec(char *file, char **argv)
{
    // Check if file is in /secure/
    if(strncmp(file, "/secure/", 8) == 0) {
        // Verify hash against manifest
        if(verify_secure_binary(file) < 0)
            error("binary hash mismatch");
    }
    // ... normal exec
}
```

3. **TPM attestation (future):**
```c
// Extend TPM PCR with binary hashes
tpm_pcr_extend(PCR_SECUREVAULT, binary_hash, 32);

// Remote attestation can verify:
// - Boot sequence was clean
// - Vault manifest matches expected
// - No tampering detected
```

### Phase 4: Copy-on-Write Protection

**Goal:** Prevent modification of vault binaries

**Implementation:**

```c
// Mark vault pages as COW after load
typedef struct VaultPage {
    uchar *data;
    int refcount;
    int cow;  // Copy-on-write flag
} VaultPage;

// On write attempt to locked vault page:
if(page->cow) {
    // Allocate new page
    uchar *new_page = xalloc(PAGESIZE);
    memmove(new_page, page->data, PAGESIZE);
    // Update mapping to new page
    // Original stays immutable
}
```

**Effect:**
- Processes can't modify vault binaries
- Each process gets private copy on write
- Original remains encrypted and immutable

## Complete Boot Sequence Example

### Traditional Linux (Vulnerable)
```
1. BIOS/UEFI loads bootloader from disk
2. Bootloader loads kernel from /boot/
3. Kernel mounts root filesystem from /dev/sda1
4. init executes /sbin/init from disk
5. System services execute from /bin/, /sbin/
6. User logs in, executes /bin/bash from disk

Vulnerability: Attacker can modify any step 2-6 on disk
```

### Lux9 with Secure Vault (Hardened)
```
1. BIOS/UEFI loads bootloader from disk (Secure Boot)
2. Bootloader loads kernel + initrd (verified signature)
3. Kernel boots, mounts /dev/sda1 (disk is now UNTRUSTED)
4. initrd script:
   a. Extract clean binaries to /dev/secureram
   b. Verify cryptographic manifest
   c. Lock vault with password/TPM
   d. Pivot root to /secure/ (vault-based)
5. init executes /secure/bin/init (from vault)
6. System services execute from /secure/ (vault)
7. User logs in, executes /secure/bin/bash (vault)

Protection: Disk can be modified, but NEVER executed
           All execution from encrypted, verified vault
```

### Boot Script Example

```bash
#!/bin/rc
# /boot/secure-init.rc

# Initialize secure vault
echo 'init BootTimePassword' > /dev/secureram.ctl

# Extract trusted binaries (from signed initrd)
tar xzf /initrd/trusted-rootfs.tar -C /tmp/
cd /tmp/

# Copy to vault with verification
for file in bin/* sbin/* lib/*.so* etc/*
do
    hash=$(sha256sum $file | cut -d' ' -f1)
    cat $file > /dev/secureram
    echo "load $offset $(stat -c%s $file) $hash" > /dev/secureram.ctl
    offset=$(($offset + $(stat -c%s $file)))
done

# Lock vault
echo 'lock' > /dev/secureram.ctl

# Mount disk as read-only (untrusted)
mount -o ro /dev/sda1 /disk

# Pivot to secure vault
mount -t securefs /dev/secureram /secure
chroot /secure /bin/init

# Disk is now only used for:
# - Logs (write-only, untrusted)
# - User data (untrusted)
# - Package cache (untrusted)
# NO execution from disk!
```

## Security Properties

### Immutability Guarantee
```
Invariant: After vault lock, no binary in vault can be modified
Proof:
  - Vault pages marked read-only
  - Write triggers COW (private copy)
  - Original encrypted with XChaCha20
  - Key protected by Argon2id password
  - Modification requires password unlock
```

### Integrity Verification
```
Property: Only verified binaries execute
Mechanism:
  1. Boot-time manifest (SHA256 hashes)
  2. Signature verification (GPG/TPM)
  3. Exec-time hash check
  4. Mismatch → execution denied
```

### Trusted Computing Base (TCB)
```
What you must trust:
  ✅ Bootloader (Secure Boot)
  ✅ Kernel (signed)
  ✅ Initrd (signed)
  ✅ CPU (hardware)

What you DON'T need to trust:
  ❌ Disk (can be compromised)
  ❌ Package manager (can be trojaned)
  ❌ Running processes (can't modify vault)
```

## Use Cases

### 1. Hardened Server
```
Goal: Web server that can't be persistently compromised

Architecture:
- /secure/bin/nginx → vault (immutable)
- /secure/lib/*.so → vault (immutable)
- /var/www/ → disk (mutable, untrusted)
- /var/log/ → disk (write-only)

Threat model:
- Attacker exploits nginx, gains root
- Attacker tries: cp backdoor.so /lib/
  → Fails (vault locked, read-only)
- Attacker tries: sed -i /secure/bin/nginx
  → Fails (COW protection)
- Admin reboots server
  → Clean state restored from vault
  → Attacker's changes lost
```

### 2. Kiosk System
```
Goal: Public terminal that resets to clean state

Architecture:
- /secure/ → entire OS (vault)
- /home/ → tmpfs (wiped on reboot)

Operation:
- Boot: load clean OS to vault, lock
- User session: all changes in tmpfs
- Reboot: tmpfs cleared, vault restored
- Result: guaranteed clean state every boot
```

### 3. Incident Response Workstation
```
Goal: Forensic analysis without contamination

Architecture:
- /secure/tools/ → forensic tools (vault)
- /evidence/ → mounted read-only
- /analysis/ → disk (logs, reports)

Security:
- Tools can't be trojaned by malware
- Evidence can't modify tooling
- Clean tool chain every boot
```

### 4. Air-Gapped Build Server
```
Goal: Reproducible builds with verified toolchain

Architecture:
- /secure/toolchain/ → gcc, make, etc. (vault)
- /src/ → source code (disk, untrusted)
- /build/ → tmpfs (ephemeral)

Guarantee:
- Toolchain hash verified at boot
- Source can't trojan compiler
- Build output is reproducible
- Toolchain immutable across builds
```

## Comparison to Existing Technologies

| Technology | Lux9 Secure Vault | Intel SGX | ARM TrustZone | dm-verity | IMA/EVM |
|------------|-------------------|-----------|---------------|-----------|---------|
| **Protection** | Runtime integrity | Enclave isolation | Secure world | Read-only root | File signatures |
| **Hardware Required** | None | SGX CPU | ARM CPU | None | None |
| **Encrypted at Rest** | Yes (XChaCha20) | Yes | Yes | No | No |
| **Executable** | Yes (planned) | Yes | Yes | Yes | Yes |
| **Modify Protection** | Yes (lock) | Yes (SGX) | Yes (TZ) | Yes (hash) | Yes (sig) |
| **Performance** | Native | Slow (context switch) | Moderate | Native | Moderate |
| **Complexity** | Low | High | High | Low | Moderate |

**Lux9's advantage:** Software-only, no special hardware, simpler than SGX/TrustZone

## Next Steps

**To implement your vision, we need:**

1. **Phase 1 (Minimal Viable):**
   - [ ] Add PROT_EXEC support to devram.c
   - [ ] Implement region tracking (offset, size, hash)
   - [ ] Add `load` control command
   - [ ] Test: copy /bin/ls, execute from vault

2. **Phase 2 (Usable):**
   - [ ] Implement tar extraction in devram.c
   - [ ] Create boot script to populate vault
   - [ ] Add COW protection for vault pages
   - [ ] Test: boot with /secure/ as root

3. **Phase 3 (Secure):**
   - [ ] Add cryptographic manifest support
   - [ ] Implement exec-time hash verification
   - [ ] Add TPM sealing/attestation
   - [ ] Security audit and penetration testing

**Want me to start implementing Phase 1?**

The current `/dev/secureram` already has:
- ✅ Encryption (XChaCha20)
- ✅ Locking mechanism
- ✅ Password protection
- ✅ Secure wipe

We just need to add:
- [ ] Executable page mappings
- [ ] Region tracking
- [ ] Hash verification
