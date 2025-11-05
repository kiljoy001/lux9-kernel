# Pebble-Enhanced 9P Integration Plan

**Vision:** Transform lux's 9P implementation into the world's first capability-safe distributed filesystem by integrating the pebble memory management system, exchange page system, and borrow checker.

**Target:** Production-ready capability-based 9P with zero-copy I/O, resource quotas, and crash safety

**Timeline:** 3-sprint implementation plan (1-2 weeks)

---

## Executive Summary

Lux possesses three revolutionary systems that, when combined, create unprecedented 9P capabilities:

1. **Pebbling System** - Capability-based resource management with Red-Blue copy-on-write
2. **Exchange System** - Zero-copy page transfer between processes
3. **Borrow Checker** - Rust-style memory safety for IPC

This plan integrates these systems with 9P to create:
- **Capability-scoped FIDs** instead of insecure integer handles
- **Resource quotas** to prevent DoS attacks
- **Zero-copy large file transfers** via exchange system
- **Crash-safe operations** via Red-Blue speculative execution
- **Speculative I/O** that prevents partial write corruption

---

## Technical Architecture

### Core Abstractions

```c
// P9Sess - Pebble-aware 9P session
typedef struct P9Sess {
    PebbleState peb;      // Per-connection resource budget
    uint32_t    msize;    // Negotiated message size
    Qid         root;     // Root directory qid
    struct {
        ulong bytes_used;     // Current budget consumption
        ulong fids_active;    // Active capability FIDs
        ulong blue_buffers;   // Live speculative buffers
    } stats;
} P9Sess;

// P9Cap - Pebble-backed capability FID
typedef struct P9Cap {
    PebbleWhite *token;   // Validated capability token
    void        *black;   // Cached black handle (inode, file object)
    uint64_t     offset;  // Current file offset
    uint32_t     perm;    // Cached permissions
    struct {
        PebbleBlue *active_blue;   // Active speculative buffer
        uint64_t dirty_offset;     // Offset of dirty data
        uint32_t dirty_len;        // Length of dirty region
    } io_state;
} P9Cap;
```

### Blue/Red I/O Buffer Helpers

```c
// kernel/9front-port/p9_peb_io.c
void* p9blue_alloc(P9Sess *sess, usize size, usize align) {
    void *buf = pebble_blue_alloc(sess, size);
    if(buf == nil && pebble_budget_exhausted(sess)) {
        // Fallback to traditional allocation
        return malloc(size);
    }
    return buf;
}

void p9blue_discard(P9Cap *cap) {
    if(cap->io_state.active_blue) {
        pebble_blue_discard(cap->io_state.active_blue);
        cap->io_state.active_blue = nil;
    }
}

void* p9red_commit(P9Cap *cap) {
    if(cap->io_state.active_blue) {
        PebbleRed *red = pebble_red_copy(cap->io_state.active_blue);
        cap->io_state.active_blue = nil;
        return red ? red->red_data : nil;
    }
    return cap->io_state.active_blue;  // Fallback buffer
}
```

---

## Implementation Plan

### Sprint A: Kernel Pebble Integration (1-2 days)

**Goal:** Make 9P "pebble-aware" with minimal surgical changes

**Files to modify:**
- `kernel/9front-port/devmnt.c` - Add session management and budget guards
- `kernel/include/p9_peb.h` - New header for P9Sess/P9Cap types
- `kernel/9front-port/p9_peb_io.c` - Blue/Red buffer helpers
- `kernel/9front-port/chan.c` - FID lifecycle hooks if needed

**Key Changes:**

1. **Session Management (devmnt.c)**
```c
// On Tversion: initialize session with budget
static void
mnt_version(Mnt *m, Fcall *f)
{
    P9Sess *sess = malloc(sizeof(P9Sess));
    pebble_reset_state(&sess->peb);
    sess->peb.black_budget = PEBBLE_DEFAULT_BUDGET;
    sess->msize = f->msize;
    m->aux = sess;  // Store in mount structure
}

// Budget guard for all 9P operations
static int
mnt_check_budget(Mnt *m)
{
    P9Sess *sess = m->aux;
    if(pebble_over_budget(&sess->peb)) {
        error("quota exceeded");
        return 0;
    }
    return 1;
}
```

2. **Capability FID Management**
```c
// On Tattach: issue root capability
static void
mnt_attach(Fid *f, Fcall *fc)
{
    P9Sess *sess = f->c->mux->aux;
    void *root_obj = get_filesystem_root();
    
    P9Cap *cap = malloc(sizeof(P9Cap));
    cap->token = pebble_issue_white(&sess->peb, root_obj, sizeof(root_obj));
    cap->black = root_obj;
    cap->offset = 0;
    cap->perm = 0755;
    
    f->aux = cap;
}

// On Twalk: mint new capability for child
static void
mnt_walk1(Fid *fid, char *name, Qid *qid)
{
    P9Cap *parent_cap = fid->aux;
    void *child = lookup_child(parent_cap->black, name, qid);
    if(child == nil) error("enoent");
    
    P9Cap *child_cap = malloc(sizeof(P9Cap));
    child_cap->token = pebble_issue_white(&sess->peb, child, sizeof(child));
    child_cap->black = child;
    child_cap->offset = 0;
    child_cap->perm = get_file_permissions(child);
    
    fid->aux = child_cap;
}

// On Tclunk: cleanup capability
static void
mnt_clunk(Fid *f)
{
    P9Cap *cap = f->aux;
    if(cap) {
        pebble_white_put(cap->token);
        free(cap);
    }
}
```

3. **Budget Guards**
```c
// Add to all 9P operation entry points
if(!mnt_check_budget(m)) return;

// Track resource usage
sess->stats.bytes_used += operation_cost;
sess->stats.fids_active++;
```

### Sprint B: Userspace Capability Integration (2-3 days)

**Goal:** Update ext4fs to use P9Cap instead of integer FIDs

**Files to modify:**
- `userspace/servers/ext4fs/ext4fs.c` - Replace FID aux with P9Cap
- `userspace/lib/lib9p/fcall.c` - Add capability validation
- `userspace/include/p9_cap.h` - Userspace P9Cap definitions

**Key Changes:**

1. **FID Structure Update**
```c
// ext4fs.c - replace current fid->aux usage
struct Ext4Fid {
    P9Cap *cap;           // Capability instead of integer
    struct ext2_inode *inode;  // Cached inode
    uint64_t offset;      // Current file position
    int flags;           // Open flags
};
```

2. **Capability Validation**
```c
// lib9p/fcall.c - validate capabilities
int fcall_validate_cap(P9Cap *cap) {
    if(!pebble_white_verify(cap->token, &cap->black))
        return 0;
    return 1;
}
```

3. **Ext4FS Read with Blue/Red**
```c
// ext4fs.c - speculative file read
static int
ext4fs_read_speculative(Fcall *fcall, Ext4Fid *fid, uint64_t offset, uint32_t count)
{
    P9Cap *cap = fid->cap;
    P9Sess *sess = get_session_from_fid(fid);
    
    // Allocate speculative buffer
    void *blue_buf = p9blue_alloc(sess, count);
    if(blue_buf == nil) {
        // Fallback to traditional read
        return ext4fs_read_traditional(fcall, fid, offset, count);
    }
    
    // Perform read into speculative buffer
    ssize_t nread = ext2fs_read(fid->inode, offset, blue_buf, count);
    if(nread < 0) {
        p9blue_discard(cap);
        return -1;
    }
    
    // Commit to stable buffer
    void *red_buf = p9red_commit(cap);
    fcall->data = red_buf;
    fcall->count = nread;
    
    return 0;
}
```

### Sprint C: Exchange Integration for Zero-Copy (2-4 days)

**Goal:** Add exchange-backed data channels for large transfers

**Strategy:** Keep traditional 9P wire format for control, use exchange handles for payload

**Key Components:**

1. **Exchange-Backed Payload Channel**
```c
// Enhanced 9P read for large files
static int
mnt_read_large(Mnt *m, Fid *fid, uint64_t offset, uint32_t count)
{
    P9Cap *cap = fid->aux;
    
    // For large reads: use exchange system
    if(count >= EXCHANGE_THRESHOLD) {
        return mnt_read_exchange(m, fid, offset, count);
    }
    
    // For small reads: use Blue/Red
    return mnt_read_pebble(m, fid, offset, count);
}

static int
mnt_read_exchange(Mnt *m, Fid *fid, uint64_t offset, uint32_t count)
{
    P9Cap *cap = fid->aux;
    P9Sess *sess = m->aux;
    
    // Prepare pages for exchange
    usize npages = (count + PAGE_SIZE - 1) / PAGE_SIZE;
    ExchangeHandle *handles = malloc(npages * sizeof(ExchangeHandle));
    
    usize prepared = exchange_prepare_range(fid->offset, count, handles);
    if(prepared != npages) {
        // Cleanup partial preparation
        for(usize i = 0; i < prepared; i++)
            exchange_cancel(handles[i]);
        free(handles);
        return -1;
    }
    
    // Send exchange handles in 9P response
    Fcall response = {
        .type = Rread,
        .count = sizeof(ExchangeHandle) * npages,
        .data = (char*)handles
    };
    
    // Note: handles will be transferred via separate mechanism
    return write_response(m, &response);
}
```

2. **Payload Transfer Protocol**
```c
// Ancillary data for exchange handles
struct P9ExchangePayload {
    uint32_t nhandles;           // Number of exchange handles
    ExchangeHandle handles[];    // Array of handles
};

// Enhanced 9P message with ancillary data
struct P9Message {
    struct Fcall fc;             // Main 9P message
    struct P9ExchangePayload *payload;  // Exchange handles
};
```

---

## Testing Strategy

### Existing Tests (Must Continue Passing)
- `userspace/test_9p_comprehensive.sh` - Protocol validation
- `userspace/test_ext4fs_simple.sh` - Basic file operations
- `shell_scripts/test_9p_protocol_validation.sh` - Message format validation

### New Tests for Pebble Integration

1. **Budget Exhaustion Test**
```bash
#!/bin/sh
# test_budget_exhaustion.sh
# Verify that clients hitting budget limits get clean errors

echo "Testing budget exhaustion behavior..."
for i in $(seq 1 100); do
    client_test --quota-test &
done
wait

# Verify all clients hit "quota exceeded" cleanly
# No crashes, no corruption, proper error codes
```

2. **Speculative Write Safety Test**
```c
// test_speculative_write.c
// Simulate failures during write operations

void test_partial_write_failure() {
    // Setup large file write
    P9Cap *file_cap = open_large_file();
    void *blue_buf = p9blue_alloc(sess, 1*GB);
    
    // Simulate failure midway through write
    simulate_disk_failure();
    
    // Verify: blue buffer discarded, no partial data written
    assert(file_has_no_corruption());
    assert(blue_buffer_was_discarded());
}
```

3. **Capability Security Test**
```c
// test_capability_security.c
// Verify capability-based access control

void test_capability_hijacking() {
    // Try to access file without valid capability
    P9Cap fake_cap = { .token = invalid_token };
    int result = p9_read_operation(&fake_cap);
    
    // Should fail with permission denied
    assert(result == P9_ERROR_PERMISSION);
}

void test_capability_forgery() {
    // Try to forge capability token
    P9Cap forged_cap = { .token = fake_but_valid_format_token };
    int result = p9_write_operation(&forged_cap);
    
    // Should fail - token doesn't map to real resource
    assert(result == P9_ERROR_INVALID_CAPABILITY);
}
```

---

## Performance Benefits

### Expected Improvements

1. **Large File Transfers**
   - **Before:** Copy data through kernel buffer → userspace buffer → network
   - **After:** Exchange handles enable zero-copy page mapping
   - **Impact:** 10x speedup for multi-GB transfers

2. **DoS Protection**
   - **Before:** Single malicious client can exhaust all memory
   - **After:** Each client limited to pebble budget (default 256MB)
   - **Impact:** System remains responsive under attack

3. **Crash Safety**
   - **Before:** Partial writes can corrupt filesystems
   - **After:** Red-Blue ensures atomicity or clean rollback
   - **Impact:** Zero data corruption from client crashes

### Performance Monitoring

```c
// Add to P9Sess stats
struct {
    ulong bytes_read_zero_copy;   // Bytes transferred via exchange
    ulong bytes_read_copied;      // Bytes copied traditionally  
    ulong speculative_operations; // Blue/Red operations performed
    ulong capability_validations; // Token validation operations
    ulong budget_exceeded_events; // Times quota was hit
} perf_stats;

// Export via /proc or 9P stat interface
```

---

## Security Enhancements

### Threat Model Coverage

1. **Resource Exhaustion**
   - **Threat:** Client opens unlimited FIDs, allocates unbounded memory
   - **Defense:** Pebble budgets cap per-client resource usage
   - **Detection:** Log budget exceeded events

2. **Capability Forgery**
   - **Threat:** Client creates fake FID tokens to access unauthorized resources
   - **Defense:** White tokens validate against actual kernel objects
   - **Detection:** Invalid token attempts logged

3. **Use-After-Free**
   - **Threat:** Client accesses FID after server cleanup
   - **Defense:** Borrow checker tracks ownership, prevents stale access
   - **Detection:** Borrow checker violations logged

4. **Partial Write Corruption**
   - **Threat:** Client crash during write leaves filesystem inconsistent
   - **Defense:** Red-Blue ensures atomic operations
   - **Detection:** Blue discard events indicate safe rollbacks

### Security Monitoring

```c
// Security event logging
void p9_security_event(uint32_t event_type, uint32_t client_id, const char *details) {
    if(pebble_debug) {
        print("P9SEC: type=%u client=%u %s\n", event_type, client_id, details);
    }
    // Also could send to security logging system
}

// Event types
#define P9SEC_BUDGET_EXCEEDED    1
#define P9SEC_INVALID_CAPABILITY 2  
#define P9SEC_STALE_FID_ACCESS   3
#define P9SEC_PARTIAL_WRITE_ROLLBACK 4
```

---

## Integration Strategy

### Backward Compatibility

1. **Graceful Degradation**
   - If pebble system disabled: use traditional 9P
   - If blue allocation fails: fall back to kalloc
   - If exchange fails: fall back to copy-based transfer

2. **Configuration Options**
   ```
   bootargs:
     pebble=1              # Enable pebble integration
     pebble.debug=1        # Enable debug logging  
     pebble.quota=512M     # Set default budget
     9p.exchange=1         # Enable zero-copy transfers
     9p.threshold=64K      # Size threshold for exchange
   ```

### Deployment Phases

1. **Phase 1: Foundation (Sprint A)**
   - Deploy budget guards only
   - Monitor performance impact
   - Verify backward compatibility

2. **Phase 2: Capabilities (Sprint B)**
   - Deploy capability FIDs
   - Monitor security benefits
   - Update user documentation

3. **Phase 3: Zero-Copy (Sprint C)**
   - Deploy exchange integration
   - Benchmark performance improvements
   - Celebrate revolutionary achievement!

---

## Risk Assessment

### Low Risk
- **Budget guards:** Simple guard checks, minimal impact
- **Capability FIDs:** Replace integer with structure, same lifetime management

### Medium Risk  
- **Blue/Red buffers:** New allocation path, must handle fallbacks
- **Exchange integration:** Complex page mapping, must handle failures gracefully

### Mitigation Strategies
1. **Incremental deployment:** Each sprint can be rolled back independently
2. **Fallback paths:** All new features have traditional equivalents
3. **Comprehensive testing:** Extensive test coverage before deployment
4. **Performance monitoring:** Real-time metrics to detect regressions

---

## Success Metrics

### Functional Success
- [ ] All existing 9P tests pass without modification
- [ ] New capability-based security prevents unauthorized access
- [ ] Budget exhaustion handled cleanly without crashes
- [ ] Partial write failures result in clean rollbacks

### Performance Success  
- [ ] Large file transfers show measurable speedup
- [ ] System remains responsive under resource exhaustion attacks
- [ ] No regression in small file operation latency
- [ ] Memory usage per client stays within budget limits

### Security Success
- [ ] Capability tokens cannot be forged or hijacked
- [ ] Resource budgets prevent DoS attacks
- [ ] Borrow checker prevents use-after-free vulnerabilities
- [ ] Security events are properly logged and monitored

---

## Future Extensions

### Advanced Features (Post-Initial Deployment)

1. **Multi-Process 9P Servers**
   - Use exchange system to share server state across processes
   - Capability delegation between server components

2. **Cross-Machine 9P**
   - Extend exchange protocol over network
   - Capability-based authentication for remote access

3. **Persistent Capabilities**
   - Store capability tokens in filesystems
   - Capability inheritance and delegation models

4. **Performance Optimizations**
   - Predictive pre-allocation of blue buffers
   - Exchange handle caching
   - Advanced page sharing strategies

---

## Conclusion

This integration plan transforms lux's 9P implementation into the world's first production-ready capability-safe distributed filesystem. By combining Plan 9's proven IPC model with modern capability security and zero-copy performance, we create a system that is:

- **More Secure:** Capability-based access control with resource quotas
- **More Reliable:** Crash-safe operations with atomic rollbacks  
- **More Performant:** Zero-copy I/O for large transfers
- **More Elegant:** Unified security and resource management model

The incremental deployment strategy ensures minimal risk while maximizing revolutionary benefits. This positions lux as the gold standard for secure distributed filesystems in the modern era.

**Next Steps:** Complete current bug fixes, then begin Sprint A implementation with confidence that every change brings us closer to this revolutionary vision.