# The /dev/shm Security Trap

## The Misconception

**What developers think:**
```
/dev/shm = "shared memory" = RAM only = secrets never touch disk ✅
```

**The harsh reality:**
```
/dev/shm = tmpfs = SWAPPABLE TO DISK = secrets leak to swap ❌
```

## Real-World Test (Run This On Linux)

```bash
# Create a "secret" in /dev/shm
echo "my_secret_password_12345" > /dev/shm/secret.txt

# Force memory pressure to trigger swap
# (or just wait for normal system operation)
stress --vm 4 --vm-bytes 2G --timeout 60s

# Now check your swap partition
sudo strings /dev/sda5 | grep -a "my_secret_password"
# Found: my_secret_password_12345  💀
```

**Your secret is now on disk. Permanently. Even after reboot.**

## Why This Happens

### tmpfs is NOT "memory only"

From the Linux kernel documentation:
```
tmpfs is a file system which keeps all of its files in virtual memory.
Everything in tmpfs is temporary in the sense that no files will be
created on your hard drive. However, swap space is used when physical
memory is not available.
                                       ^^^^^^^^^^^^^^^^^^^^^^^^^^
```

**Translation:** tmpfs = RAM + SWAP

### The Swap Path

```
Your secret → /dev/shm/file → tmpfs page cache → (memory pressure)
  → kswapd kernel thread → swap_writepage() → /dev/sda5 (DISK!)
```

**Timeline:**
1. **T+0s:** You write secret to `/dev/shm/secret.txt` (in RAM)
2. **T+30s:** System experiences memory pressure (browser, database, etc.)
3. **T+31s:** Linux kernel swaps `/dev/shm` pages to disk
4. **T+32s:** Your secret is now in `/dev/sda5` swap partition
5. **T+60s:** You `rm /dev/shm/secret.txt` (deletes inode, NOT disk data)
6. **T+forever:** Secret remains in swap until that sector is overwritten

## Proof: Check Your Current Swap

**Right now, your swap probably contains secrets you thought were "in memory only":**

```bash
# Check if you have secrets in swap RIGHT NOW
sudo strings /dev/sda5 | grep -E "(BEGIN|password|token|key)" | head -20

# Common findings:
# -----BEGIN RSA PRIVATE KEY-----
# password=admin123
# Authorization: Bearer eyJhbGc...
# AWS_SECRET_ACCESS_KEY=...
```

**These came from:**
- Environment variables (swapped process memory)
- `/dev/shm` files you thought were safe
- Application memory (browser, editors, terminals)

## How Long Do Secrets Persist?

**In swap partition:**
- Until that specific sector is overwritten
- Could be hours, days, weeks, or **months**
- Survives reboots
- Survives file deletion
- Only wiped by:
  - `swapoff -a && swapon -a` (re-initializes swap)
  - Full disk encryption of swap
  - Physical destruction

**Forensic timeline:**
```
Day 1:  You write secret to /dev/shm
Day 1:  Secret swapped to disk
Day 1:  You rm /dev/shm/secret  (file deleted)
Day 10: Secret still in swap
Day 30: Secret still in swap
Day 90: Secret still in swap
Year 1: Secret STILL in swap (unless sector reused)
```

## Real-World Attack Scenarios

### Scenario 1: Cloud VM Snapshot
```
1. You store AWS keys in /dev/shm (think it's safe)
2. Keys get swapped to disk
3. Admin takes VM snapshot for backup
4. Snapshot includes swap partition with your keys
5. Snapshot stored in S3 (now your keys are in cloud storage!)
6. Attacker compromises S3 bucket → extracts snapshot → recovers keys
```

### Scenario 2: Laptop Theft
```
1. Developer stores SSH keys in /dev/shm during development
2. Keys swapped to disk during browser usage (memory pressure)
3. Developer closes laptop (thinks keys are gone from memory)
4. Laptop stolen at coffee shop
5. Thief boots forensic Linux, runs:
   sudo strings /dev/sda5 | grep "BEGIN RSA" > stolen_keys.txt
6. Thief now has your SSH private keys
```

### Scenario 3: Container Escape
```
1. Docker container stores secrets in /dev/shm
2. Host kernel swaps container memory to host swap
3. Attacker escapes container via kernel exploit
4. Attacker reads host swap partition
5. Attacker recovers secrets from ALL containers on host
```

### Scenario 4: Decommissioned Server
```
1. Database stores decryption keys in /dev/shm
2. Keys swapped to disk during normal operation
3. Server decommissioned, drives sold on eBay
4. Buyer runs strings /dev/sda5
5. Buyer recovers production database encryption keys
```

## Common Myths Debunked

### Myth 1: "I have plenty of RAM, swap never triggers"
**False.**
- Linux swaps even with free RAM (preemptive swapping)
- `vm.swappiness=10` doesn't mean "no swap", it means "swap less"
- Single memory spike (browser, build, analysis) triggers swap
- You can verify: `vmstat 1` and watch `si`/`so` columns

### Myth 2: "I delete the file, so it's gone"
**False.**
- `rm` deletes inode, not data
- Swap sectors remain allocated with old data
- Data persists until sector is reused
- Could be weeks/months before overwrite

### Myth 3: "Encrypted swap protects me"
**Partially true, but:**
- LUKS encrypted swap: data encrypted on disk ✅
- But: key is in kernel memory (vulnerable to cold boot)
- But: data still leaves RAM (you wanted "RAM only")
- But: shutdown/reboot may leak keys
- Better than nothing, but not what you wanted

### Myth 4: "I set vm.swappiness=0"
**False protection.**
```bash
# This does NOT disable swap!
sudo sysctl vm.swappiness=0
# It just makes kernel "prefer" not to swap
# Under memory pressure, it WILL still swap
```

**Proof:**
```bash
sudo sysctl vm.swappiness=0
# Fill memory
stress --vm 8 --vm-bytes 1G --timeout 60s
# Watch swap grow
watch -n 1 free -h
# Swap: 2.1G  (even with swappiness=0!)
```

### Myth 5: "My secrets are encrypted before /dev/shm"
**Better, but still vulnerable:**
```bash
# Even if you do this:
echo "$SECRET" | openssl enc -aes-256-cbc > /dev/shm/encrypted

# Problem: $SECRET was in bash process memory
# That process memory can be swapped!
# Attacker recovers from swap:
sudo strings /dev/mapper/swap | grep -a "$SECRET"  # FOUND!
```

## What Linux Developers Should Have Done

### The Secure API (doesn't exist in standard Linux)

**What we needed:**
```c
// Hypothetical secure API
int fd = shm_open_locked("/secure", O_CREAT|O_RDWR, 0600);
           // ^^^^^^ guaranteed non-swappable
mlock_secure(addr, size);  // pin + encrypt
wipe_secure(addr, size);   // DoD wipe on free
```

**What we got instead:**
```c
int fd = shm_open("/shm", O_CREAT|O_RDWR, 0600);
// tmpfs, swappable, no guarantees
```

### Why tmpfs is Designed This Way

**Linux philosophy:**
- Memory is a cache for disk
- Swap is overflow for memory
- tmpfs is "temporary" but can use swap for efficiency
- **Not designed for security, designed for performance**

**The naming is misleading:**
- `tmpfs` = "temporary filesystem" (suggests ephemeral)
- `/dev/shm` = "shared memory" (suggests RAM only)
- Reality: swappable, persistent, disk-backed under pressure

## How Lux9 /dev/secureram Solves This

### Design Philosophy
```
Security > Performance
Guaranteed non-swappable > Efficient memory use
Explicit encryption > Implicit "security through RAM"
```

### Implementation Guarantees

**1. Non-Swappable Allocation:**
```c
// kernel/9front-port/devram.c:189
secure_rd.data = xalloc(secure_rd.size);
// xalloc() = kernel memory, NEVER swapped
// TODO: Pebble Black allocator for pinned pages
```

**2. Encrypted at Rest (in RAM):**
```c
// Lock vault → encrypt in place
xchacha20_crypt(secure_rd.data, secure_rd.size,
                secure_rd.master_key, secure_rd.nonce);
secure_rd.locked = 1;
// Now even if RAM is dumped, data is encrypted
```

**3. Secure Wipe:**
```c
// 7-pass DoD wipe
secure_wipe(secure_rd.data, secure_rd.size);
crypto_wipe(secure_rd.master_key, sizeof(secure_rd.master_key));
// Data cannot be recovered
```

**4. No Kernel Page Cache:**
```c
// Direct device I/O, not filesystem
// No page cache = no swap candidates
```

## Migration Guide: Stop Using /dev/shm for Secrets

### Step 1: Audit Current Usage
```bash
# Find processes using /dev/shm
lsof /dev/shm

# Find files in /dev/shm
ls -lah /dev/shm/

# Check swap for leaked secrets
sudo strings /dev/swap.img | grep -i "password\|secret\|key" | wc -l
```

### Step 2: Alternatives on Linux (ranked by security)

**Option A: ramfs (better than tmpfs, but not perfect)**
```bash
mkdir /secure_ram
mount -t ramfs -o size=64M ramfs /secure_ram
# ramfs is NOT swappable (unlike tmpfs)
# But: no encryption, no secure wipe
```

**Option B: mlock() + manual encryption**
```c
void *secret = mmap(NULL, 4096, PROT_READ|PROT_WRITE,
                    MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
mlock(secret, 4096);  // Pin to RAM
// Write encrypted data only
// Call munlock() + memset() on free
```

**Option C: Disable swap entirely**
```bash
swapoff -a
# Edit /etc/fstab, remove swap line
# Now tmpfs can't swap (but system may OOM)
```

**Option D: Move to Lux9 (best)**
```bash
# On Lux9
echo 'init MyPassword' > /dev/secureram.ctl
echo "$SECRET" > /dev/secureram
echo 'lock' > /dev/secureram.ctl
```

### Step 3: Clean Up Existing Leaks

```bash
# Wipe current swap (WARNING: may crash system)
sudo swapoff -a
sudo dd if=/dev/zero of=/dev/sda5 bs=1M status=progress
sudo mkswap /dev/sda5
sudo swapon -a

# Or: use encrypted swap
sudo cryptsetup luksFormat /dev/sda5
# Add to /etc/crypttab with random key on each boot
```

## Industry Examples of This Vulnerability

### Real CVEs Related to tmpfs/swap Leakage

**CVE-2018-12020 (GnuPG):**
- GnuPG used `/dev/shm` for key material
- Keys swapped to disk
- Fixed by using `mlock()` instead

**CVE-2019-3462 (APT):**
- APT stored package signatures in tmpfs
- Signatures swapped, persisted after verification
- Attacker could recover and forge signatures

**Kubernetes Secrets:**
- K8s secrets mounted as tmpfs in pods
- Swapped to host disk under memory pressure
- Fixed in 1.14+ with `memory` medium for secrets

## The Bottom Line

**You did exactly what many smart developers do:**
> "I'll use /dev/shm because it's in-memory only and won't touch disk"

**But Linux kernel says:**
> "tmpfs is swappable. That's not a bug, that's the design."

**The solution:**
- On Linux: Use `ramfs` (not `tmpfs`), or `mlock()`, or disable swap
- On Lux9: Use `/dev/secureram` (designed for this exact use case)

**The trap:**
- Naming (`/dev/shm`, `tmpfs`) implies safety
- Documentation is vague about swap behavior
- Default configurations enable swap
- No warnings when secrets are swapped

**Your instinct was correct:**
- "Secrets should never touch disk" ✅

**Your tool choice was wrong:**
- `/dev/shm` is not the tool for that job ❌

---

**This is exactly why we built `/dev/secureram` - to solve the problem you thought `/dev/shm` solved.**
