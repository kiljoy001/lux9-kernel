# Per-Process Secure Vaults - Dynamic Isolation Architecture

## The Key Insight

**Current thinking:**
- One global `/dev/secureram` for the whole system
- Shared, requires careful access control

**Your insight:**
- **Spawn a vault per process/user/container**
- Each vault is isolated, encrypted separately
- No shared state, no global locks

**This is brilliant because:**
- Process isolation at the hardware/crypto level
- Each process gets its own encrypted workspace
- Compromise of one vault doesn't affect others
- Natural fit for Lux9's capability model

## Architecture: Vault-per-Process

### Dynamic Vault Creation

**API:**
```c
// Syscall: create isolated vault for current process
int vault = sys_create_vault(size, password);
// Returns: /dev/vault.1234 (unique to process)

// Vault is:
// - Encrypted with process-specific key
// - Only accessible by creating process
// - Automatically wiped on process exit
// - Backed by Blind Ledger capability
```

**Lifecycle:**
```
Process spawns → vault created → process runs → process exits → vault wiped
                 ↓                              ↓
            /dev/vault.PID              7-pass DoD wipe
```

### Implementation Strategy

**Kernel data structure:**
```c
// kernel/9front-port/devram.c

typedef struct ProcessVault {
    int pid;                    // Owner process
    uchar *data;                // Vault memory
    ulong size;                 // Vault size
    int locked;                 // Encryption state
    uchar master_key[32];       // Unique key per vault
    uchar salt[16];             // Unique salt
    uchar nonce[24];            // Unique nonce
    UserCapability cap;         // Blind Ledger capability
    struct ProcessVault *next;  // Linked list
} ProcessVault;

static ProcessVault *vault_list = nil;
static Lock vault_lock;
```

**Device naming:**
```
/dev/vault.1234      → vault for PID 1234
/dev/vault.1234.ctl  → control interface
/dev/vault.5678      → vault for PID 5678 (isolated!)
```

**Creation flow:**
```c
// User syscall
fd = open("/dev/vault.new", O_CREAT);
// Kernel creates:
// 1. Allocate vault memory (xalloc)
// 2. Generate unique salt/nonce
// 3. Mint Blind Ledger capability for this vault
// 4. Return /dev/vault.PID to process
```

## Use Cases

### 1. Per-User Password Manager

**Without vaults:**
```
Problem: All users share /dev/secureram
- User A stores passwords
- User B can read User A's vault (if unlocked)
- Requires complex access control
```

**With per-user vaults:**
```bash
# User A spawns vault
vault_a=$(open /dev/vault.new)  # → /dev/vault.1000
echo 'init AlicePassword' > /dev/vault.1000.ctl
echo 'ssh_key=...' > /dev/vault.1000

# User B spawns vault (ISOLATED)
vault_b=$(open /dev/vault.new)  # → /dev/vault.1001
echo 'init BobPassword' > /dev/vault.1001.ctl
echo 'gpg_key=...' > /dev/vault.1001

# Vaults are cryptographically isolated
# User A cannot access vault.1001 (no capability)
# User B cannot access vault.1000 (no capability)
```

### 2. Sandboxed Compilation

**Goal:** Isolate compiler with unique toolchain per build

```bash
# Spawn vault for build
build_vault=$(open /dev/vault.new)
echo 'init BuildKey' > $build_vault.ctl

# Load verified toolchain into vault
tar xzf trusted-gcc.tar.gz -C /tmp/
cat /tmp/gcc > $build_vault
cat /tmp/ld > $build_vault
cat /tmp/as > $build_vault
echo 'lock' > $build_vault.ctl

# Execute build in isolated namespace
chroot $build_vault /gcc -o program source.c

# Build completes, vault auto-wiped
exit  # ProcessVault destroyed, 7-pass wipe
```

**Security:**
- Each build gets fresh toolchain from vault
- Source code can't trojan compiler (vault locked)
- Multiple builds can't interfere
- No persistent state between builds

### 3. Container Isolation (Docker-style)

**Without vaults:**
```
Docker: Containers share host kernel
- Container escape → full host access
- No cryptographic isolation
```

**With per-container vaults:**
```bash
# Container runtime
create_container() {
    vault=$(open /dev/vault.new)
    echo "init ContainerKey_$(uuidgen)" > $vault.ctl

    # Load container rootfs to vault
    tar xzf alpine-rootfs.tar -C $vault
    echo 'lock' > $vault.ctl

    # Execute container with vault as root
    unshare --mount --pid --net \
        chroot $vault /bin/sh
}

# Each container:
# - Runs from encrypted vault
# - Isolated from other containers (different keys)
# - Auto-wiped on container stop
```

**Security property:**
- Container escape gets you to vault (encrypted)
- Attacker needs password to decrypt
- Other containers cryptographically isolated
- Host filesystem never executed

### 4. SSH Session Isolation

**Goal:** Each SSH connection gets isolated vault for credentials

```bash
# In sshd
on_user_login() {
    vault=$(open /dev/vault.new)
    echo "init SSHSession_$RANDOM" > $vault.ctl

    # Load user's authorized_keys, known_hosts
    cat ~/.ssh/authorized_keys > $vault/authorized_keys
    cat ~/.ssh/known_hosts > $vault/known_hosts
    echo 'lock' > $vault.ctl

    # Set SSH_VAULT env var
    export SSH_VAULT=$vault
}

# ssh-agent reads from vault
ssh-add $SSH_VAULT/id_rsa

# Session ends → vault wiped
# No credentials persist in memory
```

### 5. Web Browser Tab Isolation

**Goal:** Each browser tab gets isolated vault

```
Browser spawns tab:
  → create vault
  → load tab's JavaScript/data to vault
  → execute JavaScript from vault
  → tab closes → vault wiped

Security:
  - Tab A cannot read Tab B's vault (different keys)
  - XSS in one tab can't escape to others
  - Each tab cryptographically isolated
```

### 6. Microservice Isolation

**Goal:** Each microservice instance gets unique vault

```bash
# Service orchestrator (like Kubernetes)
spawn_microservice() {
    vault=$(open /dev/vault.new)
    echo "init Service_$(uuidgen)" > $vault.ctl

    # Load service binary + config to vault
    cat /releases/api-server-v1.2.3 > $vault/binary
    cat /config/production.yaml > $vault/config
    echo 'lock' > $vault.ctl

    # Execute service
    exec $vault/binary --config $vault/config
}

# Multiple instances of same service:
# - Each has unique vault
# - No shared memory
# - Compromise of instance 1 doesn't affect instance 2
```

## Integration with Blind Ledger

**The perfect fit:**

```c
// Each vault gets a Blind Ledger capability
typedef struct ProcessVault {
    UserCapability cap;  // ← Unforgeable capability
    // ...
};

// Vault access requires capability
int vault_open(int vault_id) {
    // Lookup vault by ID
    ProcessVault *vault = find_vault(vault_id);

    // Verify process has capability
    if(!ledger_verify(&vault->cap, up))
        error("permission denied");

    // Grant access
    return vault_fd;
}
```

**Security properties:**
- Capability is cryptographic proof of ownership
- Can't forge capability (SHA256-based)
- Can transfer capability between processes (if needed)
- Capability destroyed when vault wiped

**Example: Vault Transfer**

```bash
# Process A creates vault
vault=$(open /dev/vault.new)
echo 'init SharedKey' > $vault.ctl

# Process A transfers capability to Process B
transfer_capability $vault $PID_B

# Process B can now access vault
# (using Blind Ledger's ledger_transfer())
```

## Implementation Plan

### Phase 1: Dynamic Vault Creation

**Add to devram.c:**

```c
// Global vault registry
static ProcessVault *vault_list = nil;
static Lock vault_lock;
static int next_vault_id = 1;

// Create new vault for process
static Chan*
ramopen(Chan *c, int omode)
{
    if((ulong)c->qid.path == Qvaultnew) {
        // Allocate new vault
        ProcessVault *vault = xalloc(sizeof(ProcessVault));
        vault->pid = up->pid;
        vault->size = 64*1024*1024;  // Default 64MB
        vault->data = xalloc(vault->size);

        // Generate unique crypto material
        genrandom(vault->salt, 16);
        genrandom(vault->nonce, 24);

        // Mint Blind Ledger capability
        ledger_mint(&vault->cap, (uintptr)vault->data,
                    vault->size, up, CAP_PERM_READ|CAP_PERM_WRITE,
                    vault->salt);

        // Add to registry
        lock(&vault_lock);
        vault->next = vault_list;
        vault_list = vault;
        vault->id = next_vault_id++;
        unlock(&vault_lock);

        // Return /dev/vault.ID
        snprint(c->name, sizeof(c->name), "vault.%d", vault->id);
        return c;
    }
    // ... normal path
}
```

**Add to device table:**

```c
static Dirtab ramdir[] = {
    ".",            {Qdir, 0, QTDIR}, 0, DMDIR|0555,
    "ram",          {Qram},           0, 0666,
    "secureram",    {Qsecureram},     0, 0600,
    "secureram.ctl",{Qsecureramctl},  0, 0600,
    "vault.new",    {Qvaultnew},      0, 0600,  // ← Create new vault
};
```

### Phase 2: Process Exit Cleanup

**Add to proc.c (process exit):**

```c
// kernel/9front-port/proc.c
void
pexit(char *note, int freemem)
{
    // ... existing cleanup ...

    // Wipe all vaults owned by this process
    extern void vault_cleanup_process(int pid);
    vault_cleanup_process(up->pid);

    // ... rest of exit
}
```

**Implement in devram.c:**

```c
void
vault_cleanup_process(int pid)
{
    lock(&vault_lock);

    ProcessVault **pp = &vault_list;
    while(*pp != nil) {
        ProcessVault *vault = *pp;
        if(vault->pid == pid) {
            // Secure wipe
            secure_wipe(vault->data, vault->size);
            crypto_wipe(vault->master_key, sizeof(vault->master_key));

            // Burn Blind Ledger capability
            ledger_burn(&vault->cap, vault->owner);

            // Free memory
            free(vault->data);

            // Remove from list
            *pp = vault->next;
            free(vault);
        } else {
            pp = &vault->next;
        }
    }

    unlock(&vault_lock);
}
```

### Phase 3: Capability-Based Access

**Verify capability on every access:**

```c
static long
ramread(Chan *c, void *va, long n, vlong off)
{
    if(c->qid.path >= Qvaultbase) {
        // Extract vault ID from qid
        int vault_id = c->qid.path - Qvaultbase;

        // Find vault
        ProcessVault *vault = find_vault(vault_id);
        if(vault == nil)
            error("vault not found");

        // Verify capability
        BlindLedgerEntry entry;
        if(ledger_verify(&vault->cap, &entry) != BLIND_LEDGER_OK)
            error("permission denied");

        // Verify ownership
        if(entry.owner != up)
            error("not vault owner");

        // Proceed with read
        // ...
    }
}
```

### Phase 4: Vault Inheritance/Transfer

**Allow parent→child vault transfer:**

```c
// Syscall: share vault with child process
int sys_vault_share(int vault_fd, int child_pid)
{
    // Find vault from fd
    Chan *c = fdtochan(vault_fd, -1, 0, 1);
    ProcessVault *vault = vault_from_chan(c);

    // Find child process
    Proc *child = find_proc(child_pid);
    if(child->parent != up)
        error("not your child");

    // Transfer Blind Ledger capability
    ledger_transfer(&vault->cap, up, child);

    // Child can now access vault
    return 0;
}
```

**Example usage:**

```bash
# Parent creates vault
vault=$(open /dev/vault.new)
echo 'init ParentKey' > $vault.ctl
echo 'shared_secret=abc123' > $vault

# Fork child
child_pid=$(fork)
if [ $child_pid -eq 0 ]; then
    # Child process
    # Vault is inherited (capability transferred)
    cat $vault  # Works! Reads "shared_secret=abc123"
else
    # Parent
    vault_share $vault $child_pid
fi
```

## Security Properties

### 1. Isolation Guarantee

```
Property: Vault V1 owned by Process P1 is inaccessible to Process P2
Proof:
  - V1.cap = SHA256(secret || V1.physical_address)
  - P2 does not have V1.cap (not in its capability table)
  - ledger_verify(V1.cap, P2) → BLIND_LEDGER_EPERM
  - Any access by P2 fails capability check
```

### 2. Confidentiality

```
Property: Vault data is encrypted at rest, unreadable without password
Mechanism:
  - Data encrypted with XChaCha20(master_key, nonce)
  - master_key = Argon2id(password, salt, 4MB, 3 passes)
  - Salt/nonce unique per vault
  - Password required to derive key
```

### 3. Integrity

```
Property: Vault data cannot be modified while locked
Mechanism:
  - locked flag prevents write()
  - Capability required for unlock
  - Write to locked vault → error("vault is locked")
```

### 4. Ephemeral State

```
Property: Vault wiped on process exit
Guarantee:
  - pexit() → vault_cleanup_process(pid)
  - 7-pass DoD wipe of vault data
  - crypto_wipe(master_key)
  - No data persists after process exit
```

## Performance Considerations

### Memory Overhead

**Per-vault cost:**
```
sizeof(ProcessVault) = ~128 bytes (struct)
+ vault->size (64MB default)
+ Blind Ledger entry (~256 bytes)
≈ 64MB per vault
```

**Scalability:**
```
1000 processes × 64MB = 64GB RAM
- Too much for single-vault-per-process on constrained systems
- Solution: smaller default (1MB? 16MB?)
- Or: lazy allocation (on-demand)
```

### CPU Overhead

**Vault operations:**
```
Create vault:  ~100-500ms (Argon2id key derivation)
Lock/unlock:   ~50-200ms (XChaCha20 encrypt/decrypt)
Wipe on exit:  ~1-5s (7-pass DoD wipe)
```

**Optimization: Lazy key derivation**
```c
// Don't derive key until first lock/unlock
vault->initialized = 0;  // Key not derived yet

// On first lock:
if(!vault->initialized) {
    derive_key_from_password(password, vault->salt, vault->master_key);
    vault->initialized = 1;
}
```

## Advanced Use Cases

### 7. Vault-per-File Encryption

**Idea:** Each sensitive file gets its own vault

```bash
# Encrypt file to vault
encrypt_file() {
    local file=$1
    vault=$(open /dev/vault.new)
    echo "init $(pwgen 32)" > $vault.ctl
    cat $file > $vault
    echo 'lock' > $vault.ctl

    # Store vault ID + metadata
    echo "vault_id=$(basename $vault)" > $file.meta
}

# Decrypt file from vault
decrypt_file() {
    local file=$1
    vault_id=$(cat $file.meta | grep vault_id | cut -d= -f2)
    cat /dev/vault.$vault_id  # Requires capability!
}
```

### 8. Distributed Vault Synchronization

**Idea:** Sync vault state between machines

```bash
# Export vault (encrypted)
vault_export() {
    vault=$1
    # Vault is already encrypted (locked)
    dd if=$vault bs=1M | ssh remote "cat > /tmp/vault.bin"
}

# Import vault on remote
vault_import() {
    vault=$(open /dev/vault.new)
    cat /tmp/vault.bin > $vault
    # Vault is imported in locked state
    # User must provide password to unlock
}
```

### 9. Vault Snapshot/Rollback

**Idea:** Checkpoint vault state, rollback on error

```bash
# Take snapshot
vault_snapshot() {
    vault=$1
    echo 'lock' > $vault.ctl  # Encrypt
    cp $vault /tmp/vault.snapshot  # Copy encrypted
}

# Rollback
vault_rollback() {
    vault=$1
    cat /tmp/vault.snapshot > $vault  # Restore encrypted state
    echo "unlock $PASSWORD" > $vault.ctl  # Decrypt
}
```

### 10. Ephemeral Credentials Service

**Idea:** Short-lived vaults for temporary credentials

```c
// Service that creates time-limited vaults
void credential_service() {
    while(1) {
        // Client requests credentials
        Client *client = accept_client();

        // Create vault
        int vault = open("/dev/vault.new", O_RDWR);
        write_to(vault, ".ctl", "init TempKey");

        // Generate credentials
        char *creds = generate_oauth_token();
        write(vault, creds, strlen(creds));

        // Lock vault
        write_to(vault, ".ctl", "lock");

        // Send vault capability to client
        send_capability(client, vault);

        // Schedule wipe after 1 hour
        timer_add(3600, vault_wipe, vault);
    }
}
```

## Comparison to Other Systems

| System | Isolation Unit | Encryption | Auto-Wipe | Capability-Based |
|--------|----------------|------------|-----------|------------------|
| **Lux9 Vaults** | Process | XChaCha20 | Yes (7-pass) | Yes (Blind Ledger) |
| Docker | Container | No | No | No (namespaces) |
| FreeBSD Jail | Jail | No | No | No (chroot) |
| SELinux | Label | No | No | No (MAC labels) |
| Capsicum | Process | No | No | Yes (caps) |
| Qubes OS | VM | Yes (dm-crypt) | Manual | No (Xen) |

**Lux9's unique combination:**
- Process-level isolation (like Capsicum)
- Cryptographic enforcement (like Qubes)
- Automatic cleanup (unique)
- Capability-based (like Capsicum, but crypto)

## Next Steps

**To implement per-process vaults:**

1. **Extend devram.c** with ProcessVault registry
2. **Add /dev/vault.new** device file
3. **Integrate with process exit** (vault cleanup)
4. **Integrate with Blind Ledger** (capability verification)
5. **Test**: spawn 100 processes, each with vault, verify isolation

**Want me to implement this?** The foundation is already there:
- ✅ Encryption (XChaCha20)
- ✅ Secure wipe (7-pass)
- ✅ Blind Ledger (capabilities)
- ✅ Password derivation (Argon2id)

We just need to make it **dynamic** and **per-process**.
