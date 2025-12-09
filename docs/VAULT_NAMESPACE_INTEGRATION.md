# Vault-Namespace Integration - Cryptographic Isolation via Plan 9 Namespaces

## The Core Insight

**Current Plan 9 namespaces:**
- Process groups (Pgrp) share a mount namespace
- `rfork(RFNAMEG)` creates new namespace
- Each namespace has independent mount table (mnthash)
- **But:** All processes still see same physical filesystem

**Vault-enhanced namespaces:**
- Each namespace gets its own **encrypted vault**
- Vault serves as the namespace's **root filesystem**
- Processes in namespace can only see vault contents
- **Cryptographic isolation:** Different encryption keys per namespace

## Architecture: Vaults AS Namespaces

### Current Pgrp Structure

```c
// kernel/include/portdat.h:517
struct Pgrp
{
    long ref;
    RWLock ns;              // Namespace lock
    u64int notallowed[4];   // Device access bitmap
    Mhead *mnthash[MNTHASH]; // Mount point hash table
};
```

### Enhanced Pgrp with Vault

```c
struct Pgrp
{
    long ref;
    RWLock ns;
    u64int notallowed[4];
    Mhead *mnthash[MNTHASH];

    // NEW: Vault-backed namespace
    ProcessVault *vault;        // Encrypted vault for this namespace
    UserCapability vault_cap;   // Capability to access vault
    int vault_locked;           // Vault encryption state
    char vault_root[256];       // Path where vault is mounted
};
```

### Vault Creation on Namespace Fork

```c
// kernel/9front-port/proc.c - Enhanced rfork

Proc*
newproc(void)
{
    Proc *p;
    Pgrp *pg;

    p = xalloc(sizeof(Proc));

    // ... existing initialization ...

    // If RFNAMEG (new namespace), create vault
    if(up->rfork_flags & RFNAMEG) {
        pg = newpgrp();

        // Create vault for namespace
        pg->vault = vault_create(DEFAULT_VAULT_SIZE);
        if(pg->vault == nil)
            error("vault creation failed");

        // Generate unique password (or inherit from parent)
        char vault_pass[64];
        if(up->rfork_flags & RFNSINHERIT) {
            // Inherit parent's vault password
            memmove(vault_pass, up->pgrp->vault_password, 64);
        } else {
            // Generate new random password
            genrandom(vault_pass, 64);
        }

        // Initialize vault
        vault_init(pg->vault, vault_pass);

        // Mint Blind Ledger capability
        ledger_mint(&pg->vault_cap,
                    (uintptr)pg->vault->data,
                    pg->vault->size,
                    p,
                    CAP_PERM_READ|CAP_PERM_WRITE|CAP_PERM_EXEC,
                    pg->vault->salt);

        // Mount vault as namespace root
        snprint(pg->vault_root, sizeof(pg->vault_root),
                "/ns/%d", pg->vault->id);

        p->pgrp = pg;
    }

    return p;
}
```

## Use Case 1: Container Isolation via Namespaces

### Traditional Docker-style Container

```bash
# Current approach (Linux)
unshare --mount --pid --net --ipc
chroot /containers/alpine /bin/sh

# Problems:
# - Rootfs is on disk (can be modified)
# - No encryption
# - Shared kernel namespace
```

### Lux9 Vault-Namespace Container

```bash
# Create new namespace with vault
rfork nameg vault

# Namespace gets:
# - Unique encrypted vault
# - Isolated mount table
# - Capability-based access

# Load container rootfs into vault
tar xzf alpine-rootfs.tar.gz -C /ns/vault

# Lock vault (encrypt at rest)
echo 'lock' > /ns/vault.ctl

# Execute in isolated namespace
/ns/vault/bin/sh

# Security properties:
# ✅ Container rootfs encrypted in RAM
# ✅ Other namespaces cannot access (different keys)
# ✅ Disk compromise doesn't affect running container
# ✅ Container exit → vault wiped automatically
```

### Implementation

```c
// New rfork flag
#define RFVAULT (1<<12)  // Create vault-backed namespace

// kernel/9front-port/sysproc.c
int
sys_rfork(int flags)
{
    if(flags & RFVAULT) {
        // Create new process group with vault
        Pgrp *pg = newpgrp();

        // Create vault
        pg->vault = vault_create(64*1024*1024);
        vault_init(pg->vault, generate_password());

        // Mount vault as root
        vault_mount(pg->vault, "/");

        up->pgrp = pg;
    }

    // ... existing rfork logic ...
}
```

## Use Case 2: Per-User Namespace Isolation

### Goal: Each user gets encrypted namespace

```bash
# User alice logs in
login alice

# System creates namespace with vault
# /ns/alice → encrypted vault
# Key derived from alice's password

# Alice's home directory is in vault
cd /ns/alice/home
ls  # Only alice can see this (capability + encryption)

# User bob logs in (different session)
login bob

# Bob gets different namespace + vault
# /ns/bob → different encryption key
# Bob cannot access /ns/alice (no capability)

# Even root cannot access alice's vault without password
```

### Implementation

```c
// In login process
void user_login(const char *username, const char *password)
{
    // Fork new namespace
    int pid = rfork(RFPROC|RFNAMEG|RFVAULT);

    if(pid == 0) {
        // Child: new namespace with vault

        // Derive vault key from user password
        uchar vault_key[32];
        argon2id_derive(password, username, vault_key);

        // Initialize vault with derived key
        vault_init_with_key(up->pgrp->vault, vault_key);

        // Extract user's home directory to vault
        tar_extract_to_vault("/backup/homes/alice.tar",
                            up->pgrp->vault);

        // Lock vault
        vault_lock(up->pgrp->vault);

        // Set up mount namespace
        bind("/ns/vault", "/home", MREPL);

        // Execute user's shell
        execve("/bin/sh", argv, envp);
    }
}
```

**Security properties:**
- Each user's namespace cryptographically isolated
- User password = vault encryption key
- No capability transfer between users
- Vault wiped on logout

## Use Case 3: Build Namespace Isolation

### Goal: Isolated, reproducible builds

```bash
# Create build namespace
build_in_vault() {
    # Fork namespace with vault
    rfork nameg vault

    # Load verified toolchain to vault
    tar xzf trusted-gcc-13.2.tar.gz -C /ns/vault/toolchain

    # Load source code (untrusted)
    cp -r /src/myproject /ns/vault/src

    # Lock toolchain (read-only, encrypted)
    echo 'lock' > /ns/vault/toolchain.ctl

    # Build
    cd /ns/vault/src
    /ns/vault/toolchain/bin/gcc -o program main.c

    # Extract binary
    cp program /output/

    # Exit namespace → vault wiped
    exit
}

# Properties:
# ✅ Toolchain is verified, encrypted, read-only
# ✅ Source cannot trojan compiler (locked)
# ✅ Each build gets fresh namespace
# ✅ No persistent state between builds
```

## Use Case 4: Namespace Hierarchy (Nested Vaults)

### Goal: Parent/child namespace relationships

```
Root Namespace (System)
    vault: /ns/system
    key: System key
    |
    ├── User Namespace (Alice)
    |   vault: /ns/alice
    |   key: Alice's password
    |   |
    |   ├── Container Namespace (Web Server)
    |   |   vault: /ns/alice/container1
    |   |   key: Random (ephemeral)
    |   |
    |   └── Container Namespace (Database)
    |       vault: /ns/alice/container2
    |       key: Random (ephemeral)
    |
    └── User Namespace (Bob)
        vault: /ns/bob
        key: Bob's password
```

**Implementation:**

```c
struct Pgrp
{
    // ... existing fields ...

    ProcessVault *vault;
    Pgrp *parent_ns;        // Parent namespace
    Pgrp *child_ns[16];     // Child namespaces
    int num_children;
};

// Create child namespace
Pgrp* namespace_fork_child(Pgrp *parent)
{
    Pgrp *child = newpgrp();

    // Create vault for child
    child->vault = vault_create(DEFAULT_SIZE);

    // Generate unique key (NOT inherited)
    char child_key[32];
    genrandom(child_key, 32);
    vault_init(child->vault, child_key);

    // Link to parent
    child->parent_ns = parent;
    parent->child_ns[parent->num_children++] = child;

    // Mint capability (child can access its vault)
    ledger_mint(&child->vault_cap, ...);

    return child;
}
```

**Isolation property:**
- Parent cannot access child's vault (no capability)
- Child cannot access parent's vault (no capability)
- Sibling namespaces isolated (different keys)

## Integration with Mount System

### Current Mount Mechanism

```c
// kernel/9front-port/chan.c
Chan* domount(Chan *c, Mhead **mp, Mnt **mm)
{
    // Mount server at mount point
    // Updates Pgrp->mnthash
}
```

### Vault-Aware Mount

```c
// Enhanced mount: mount vault as filesystem
Chan* vault_mount(ProcessVault *vault, const char *mountpoint)
{
    Chan *c;
    Mhead *mh;

    // Create channel to vault device
    c = devattach('V', vault->id);  // 'V' = vault device

    // Verify capability
    BlindLedgerEntry entry;
    if(ledger_verify(&vault->cap, &entry) != BLIND_LEDGER_OK)
        error("invalid vault capability");

    // Add to namespace mount table
    mh = newmount(c, mountpoint);

    lock(&up->pgrp->ns);
    // Insert into mnthash
    u32int hash = hashpath(mountpoint);
    mh->next = up->pgrp->mnthash[hash % MNTHASH];
    up->pgrp->mnthash[hash % MNTHASH] = mh;
    unlock(&up->pgrp->ns);

    return c;
}
```

### Example: Multi-Vault Namespace

```bash
# Process creates namespace
rfork nameg

# Mount multiple vaults at different points
mount /dev/vault.1 /bin       # System binaries
mount /dev/vault.2 /home      # User data
mount /dev/vault.3 /tmp       # Ephemeral
mount /disk /archive          # Disk (read-only)

# Each vault has different:
# - Encryption key
# - Access policy
# - Lifecycle
```

## Capability-Based Namespace Access

### Problem: Traditional namespace sharing

```c
// Process A creates namespace
rfork(RFNAMEG)

// Process B wants to access A's namespace
// How does B prove it has permission?
```

### Solution: Capability transfer

```c
// Process A creates vault-namespace
Pgrp *pg = newpgrp();
pg->vault = vault_create(SIZE);

// A mints capability for the vault
UserCapability ns_cap;
ledger_mint(&ns_cap, (uintptr)pg->vault->data, ...);

// A transfers capability to B
ledger_transfer(&ns_cap, procA, procB);

// B can now access the vault
BlindLedgerEntry entry;
if(ledger_verify(&ns_cap, &entry) == BLIND_LEDGER_OK) {
    // B accesses via capability
    vault_read(pg->vault, ...);
}
```

**Example: Shared namespace between processes**

```bash
# Process A creates shared namespace
vault_ns=$(rfork nameg vault shared)

# A gets capability
cap=$(vault_capability $vault_ns)

# A transfers to Process B
capability_send $PID_B $cap

# B attaches to shared namespace
vault_attach $cap
cd /ns/shared
ls  # Sees same files as A
```

## Device Driver: devvault.c

### New Device: Vault Filesystem

```c
// kernel/9front-port/devvault.c

#include "u.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "error.h"

enum {
    Qdir,
    Qvaultnew,      // Create new vault
    Qvaultctl,      // Control interface
    Qvaultdata,     // Vault data
};

// Device table
Dev vaultdevtab = {
    'V',  // Device character
    "vault",

    vaultreset,
    vaultinit,
    devshutdown,
    vaultattach,
    vaultwalk,
    vaultstat,
    vaultopen,
    devcreate,
    vaultclose,
    vaultread,
    devbread,
    vaultwrite,
    devbwrite,
    devremove,
    devwstat,
};

// Attach to vault device
static Chan*
vaultattach(char *spec)
{
    int vault_id;
    ProcessVault *vault;

    // Parse spec: "vault:123" → vault ID 123
    if(spec != nil && strncmp(spec, "vault:", 6) == 0) {
        vault_id = atoi(spec + 6);
        vault = find_vault(vault_id);

        if(vault == nil)
            error("vault not found");

        // Verify capability
        BlindLedgerEntry entry;
        if(ledger_verify(&vault->cap, &entry) != BLIND_LEDGER_OK)
            error("permission denied");

        if(entry.owner != up)
            error("not vault owner");
    }

    return devattach('V', spec);
}

// Walk vault filesystem
static Walkqid*
vaultwalk(Chan *c, Chan *nc, char **name, int nname)
{
    // Walk vault as filesystem
    // Each vault has internal directory structure
    ProcessVault *vault = vault_from_chan(c);

    // Unlock vault if needed
    if(vault->locked)
        error("vault is locked");

    // Walk through vault's directory tree
    // (stored in vault->fs_root)
    return vault_fs_walk(vault, name, nname);
}

// Read from vault file
static long
vaultread(Chan *c, void *va, long n, vlong off)
{
    ProcessVault *vault = vault_from_chan(c);

    // Verify capability
    if(ledger_verify(&vault->cap, NULL) != BLIND_LEDGER_OK)
        error("permission denied");

    // Check vault state
    if(vault->locked)
        error("vault is locked");

    // Read from vault filesystem
    return vault_fs_read(vault, c->qid.path, va, n, off);
}

// Write to vault file
static long
vaultwrite(Chan *c, void *va, long n, vlong off)
{
    ProcessVault *vault = vault_from_chan(c);

    // Verify capability
    if(ledger_verify(&vault->cap, NULL) != BLIND_LEDGER_OK)
        error("permission denied");

    // Check vault state
    if(vault->locked)
        error("vault is locked");

    // Write to vault filesystem
    return vault_fs_write(vault, c->qid.path, va, n, off);
}
```

## Namespace Operations

### 1. Create Vault Namespace

```c
// Syscall: create namespace with vault
int sys_vault_namespace(ulong size, const char *password)
{
    Pgrp *pg;

    // Create new process group
    pg = newpgrp();

    // Create vault
    pg->vault = vault_create(size);
    vault_init(pg->vault, password);

    // Mint capability
    ledger_mint(&pg->vault_cap, (uintptr)pg->vault->data,
                pg->vault->size, up, CAP_PERM_ALL,
                pg->vault->salt);

    // Mount as root
    vault_mount(pg->vault, "/");

    // Switch to new namespace
    up->pgrp = pg;

    return pg->vault->id;
}
```

### 2. Share Namespace

```c
// Syscall: share vault namespace with another process
int sys_namespace_share(int vault_id, int target_pid)
{
    ProcessVault *vault = find_vault(vault_id);
    Proc *target = find_proc(target_pid);

    // Verify caller owns vault
    if(ledger_verify(&vault->cap, NULL) != BLIND_LEDGER_OK)
        return -1;

    // Transfer capability to target
    UserCapability new_cap;
    if(ledger_transfer(&vault->cap, up, target) != BLIND_LEDGER_OK)
        return -1;

    // Target can now access vault
    return 0;
}
```

### 3. Lock Namespace

```c
// Syscall: lock (encrypt) entire namespace
int sys_namespace_lock(void)
{
    Pgrp *pg = up->pgrp;

    if(pg->vault == nil)
        return -1;

    // Verify ownership
    if(ledger_verify(&pg->vault_cap, NULL) != BLIND_LEDGER_OK)
        return -1;

    // Lock vault (encrypt)
    vault_lock(pg->vault);
    pg->vault_locked = 1;

    return 0;
}
```

## Security Properties

### 1. Namespace Isolation

```
Property: Namespace N1 cannot access Namespace N2
Proof:
  - N1 has vault V1 with capability C1
  - N2 has vault V2 with capability C2
  - C1 ≠ C2 (different SHA256 hashes)
  - N1 does not have C2 (not in its capability table)
  - ledger_verify(C2, N1) → BLIND_LEDGER_EPERM
  - Therefore, N1 cannot access V2
```

### 2. Cryptographic Isolation

```
Property: Namespace data is encrypted at rest
Mechanism:
  - Vault locked → XChaCha20(data, key, nonce)
  - Key derived from password via Argon2id
  - Memory dump reveals ciphertext only
```

### 3. Lifecycle Guarantee

```
Property: Namespace wiped on last process exit
Implementation:
  - Pgrp refcount tracks processes in namespace
  - When refcount → 0, trigger cleanup:
    - 7-pass DoD wipe of vault
    - Burn Blind Ledger capability
    - Free vault memory
```

## Performance Considerations

### Overhead per Namespace

```
Cost of namespace creation:
  - Pgrp allocation: ~100 bytes
  - Vault allocation: ~64MB (configurable)
  - Argon2id key derivation: ~100-500ms (one-time)
  - Capability minting: ~10μs

Total: ~100ms + 64MB per namespace
```

### Optimization: Lazy Vault Allocation

```c
struct Pgrp
{
    // ... existing ...

    ProcessVault *vault;
    int vault_allocated;  // Lazy allocation flag
};

// Allocate vault on first use
void pgrp_ensure_vault(Pgrp *pg)
{
    if(!pg->vault_allocated) {
        pg->vault = vault_create(DEFAULT_SIZE);
        vault_init(pg->vault, generate_password());
        pg->vault_allocated = 1;
    }
}
```

## Complete Example: Container System

```bash
#!/bin/rc
# Lux9 container runtime

container_create() {
    name=$1
    image=$2

    # Create vault namespace
    vault_ns=$(sys_vault_namespace 128M "container_$name")

    # Extract image to vault
    tar xzf /images/$image.tar.gz -C /vault/$vault_ns/

    # Lock vault (encrypt rootfs)
    echo 'lock' > /vault/$vault_ns.ctl

    # Fork into namespace
    pid=$(rfork nameg vault attach:$vault_ns)

    if [ $pid -eq 0 ]; then
        # Inside container namespace

        # Mount vault as root
        mount /vault/$vault_ns /

        # Setup minimal /dev
        bind /dev/cons /dev/cons
        bind /dev/null /dev/null

        # Execute container init
        exec /bin/init
    fi

    # Parent: return container ID
    echo $vault_ns
}

container_stop() {
    container_id=$1

    # Kill all processes in namespace
    namespace_kill $container_id

    # Vault auto-wiped when last process exits
}

# Example usage
container_create web nginx
container_create db postgres
container_create app myapp
```

## Next Steps

**To implement vault-namespace integration:**

1. **Phase 1: Basic Integration**
   - [ ] Add `ProcessVault *vault` to `Pgrp`
   - [ ] Create vault on `rfork(RFNAMEG|RFVAULT)`
   - [ ] Mount vault in new namespace
   - [ ] Test: create namespace, access vault

2. **Phase 2: devvault.c**
   - [ ] Implement vault filesystem device
   - [ ] Support walk/read/write on vault files
   - [ ] Integrate with mount system
   - [ ] Test: `bind /vault/123 /tmp`

3. **Phase 3: Namespace Lifecycle**
   - [ ] Wipe vault when `Pgrp->ref` hits 0
   - [ ] Capability cleanup on namespace destroy
   - [ ] Test: create/destroy 1000 namespaces

4. **Phase 4: Container Runtime**
   - [ ] Build container management tools
   - [ ] Image format (tar + manifest)
   - [ ] Network namespace integration
   - [ ] Test: Run isolated web server

**Want me to start implementing Phase 1?**
