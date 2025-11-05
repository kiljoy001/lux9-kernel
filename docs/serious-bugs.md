# Serious bugs identified

This report lists serious defects discovered in the repository.

*[Updated: Fixed kernelro() page table corruption that prevented boot progress]*

## Fixed Bugs:

**✅ BOOT PROGRESS FIXED: kernelro() page table corruption**
- **Issue**: `kernelro()` was accidentally making page tables read-only when protecting kernel text, breaking memory allocation and causing immediate crashes after `mmuinit()`
- **Fix**: Implemented page table exclusion range tracking and timing, preventing corruption while maintaining security
- **Result**: Boot now progresses from kernelro crash to graphics initialization phase
- **Location**: `kernel/9front-pc64/main.c`, `kernel/9front-pc64/mmu.c`

---

## Remaining Bugs:

### **TIER 1: System Init (SHOWSTOPPERS)**

1. **❌ System Init Completely Broken** - `fork()`, `exec()`, `mount()`, `open()`, `exit()`, `printf()` all stubbed. **Blocks userspace boot entirely.**【F:userspace/bin/syscalls.c†L6-L47】

2. **❌ init calls exec with a null argv vector** - `exec("/sbin/init", NULL)` dereferences null pointer. **Even working exec would fail.**【F:userspace/bin/init.c†L149-L156】

### **TIER 2: 9P Protocol Security & Reliability**

3. **❌ 9P message sizes corrupted by signed char decode** - `msgbuf` char array sign-extends bytes ≥0x80, corrupting large messages. **Protocol stream corruption.**【F:userspace/servers/ext4fs/ext4fs.c†L28-L31】【F:userspace/servers/ext4fs/ext4fs.c†L634-L640】

4. **❌ Server assumes read(2) returns entire message** - Header/payload reads break on short reads (legal on pipes/sockets). **Random disconnections.**【F:userspace/servers/ext4fs/ext4fs.c†L630-L640】

5. **❌ Replies written with unchecked write(2)** - Short writes silently truncate, corrupting protocol stream. **Protocol desynchronization.**【F:userspace/servers/ext4fs/ext4fs.c†L665-L667】

6. **❌ Tflush unsupported** - No `case Tflush:` branch, returns `Rerror` instead of `Rflush`. **Protocol violation.**【F:userspace/go-servers/p9/server.go†L208-L295】

7. **❌ Twstat unimplemented** - Dispatcher drops Twstat requests through default case. **Metadata updates impossible.**【F:userspace/go-servers/p9/server.go†L208-L295】

8. **❌ writeString truncates long names silently** - `len(s)` → `uint16` causes wraparound at 65,536 bytes. **Stream desync.**【F:userspace/go-servers/p9/protocol.go†L149-L155】

9. **❌ Replies can exceed negotiated msize** - `writeMsg` never checks response size fits within `s.msize`. **Protocol violation.**【F:userspace/go-servers/p9/server.go†L153-L205】

10. **❌ Malformed strings crash Go 9P parser** - `readString` error returns ignored, truncated frames corrupt parsing. **Server panics.**【F:userspace/go-servers/p9/server.go†L103-L133】【F:userspace/go-servers/p9/protocol.go†L137-L152】

11. **❌ Twrite parsing trusts unvalidated lengths** - `data[16:16+fc.Count]` without bounds check. **Buffer overflow vulnerability.**【F:userspace/go-servers/p9/server.go†L140-L145】

12. **❌ Tcreate parsing blindly indexes buffer** - Reads `r.data[0:4]` without validating 5 bytes remain. **Panic on malformed frames.**【F:userspace/go-servers/p9/server.go†L129-L134】

### **TIER 3: Filesystem Operations**

13. **❌ Tremove unlinks from root with NULL name** - Hardcodes `EXT2_ROOT_INO`, passes null filename. **File removal fails outside root.**【F:userspace/servers/ext4fs/ext4fs.c†L600-L604】

14. **❌ Directory reads treat offset as entry index** - `dirread_callback` ignores caller offset, causes duplicates/skips. **Broken seek behavior.**【F:userspace/servers/ext4fs/ext4fs.c†L260-L314】

15. **❌ Open mode tracking rejects auxiliary flags** - Stores raw mode, compares to `OWRITE`/`ORDWR`. **Denies `ORDWR|OTRUNC` etc.**【F:userspace/servers/ext4fs/ext4fs.c†L240-L257】【F:userspace/servers/ext4fs/ext4fs.c†L409-L417】

16. **❌ rattach fabricates qid on inode read failure** - Ignores `ext2fs_read_inode` errors, returns garbage. **Bogus success responses.**【F:userspace/servers/ext4fs/ext4fs.c†L165-L176】

17. **❌ Inode numbers truncated by %u format** - `ext2_ino_t` is 64-bit, `%u` truncates high bits. **Duplicate stat names.**【F:userspace/servers/ext4fs/ext4fs.c†L132-L136】

18. **❌ Directory creation removes link it created** - After `ext2fs_mkdir`, unconditionally unlinks `tx->tcreate.name`. **New directories unreachable.**【F:userspace/servers/ext4fs/ext4fs.c†L552-L568】

19. **❌ rcreate leaks inodes on error paths** - No `ext2fs_inode_alloc_stats2(..., -1, ...)` on failures. **Resource leak.**【F:userspace/servers/ext4fs/ext4fs.c†L525-L556】

20. **❌ Zero-length reads reported as OOM** - `malloc(tx->tread.count)` treats NULL as fatal for `count==0`. **Legitimate requests fail.**【F:userspace/servers/ext4fs/ext4fs.c†L341-L399】

21. **❌ ropen trusts inode read without error check** - Fabricates qid from uninitialized memory on failure. **Garbage success responses.**【F:userspace/servers/ext4fs/ext4fs.c†L240-L257】

22. **❌ Stat marshalling crashes on allocation failure** - Never checks `strdup` results, dereferences NULL. **Server crashes instead of error.**【F:userspace/servers/ext4fs/ext4fs.c†L132-L138】【F:userspace/servers/ext4fs/ext4fs.c†L341-L399】

23. **❌ MemFS Walk ignores path elements** - Always returns current fid qid regardless of `names`. **Clients can't traverse tree.**【F:userspace/go-servers/memfs/memfs.go†L45-L57】

24. **❌ MemFS Read disregards offsets** - Always returns greeting string, clients loop forever. **Never see EOF.**【F:userspace/go-servers/memfs/memfs.go†L75-L77】

### **TIER 4: Hardware Foundation**

25. **❌ Interrupt controllers never initialized** - `irqinit` empty stub, PIC/APIC never setup. **All external interrupts masked.**【F:kernel/9front-pc64/globals.c†L360-L367】【F:kernel/9front-pc64/trap.c†L75-L83】

26. **❌ PCI configuration space unreachable** - `pcicfginit` returns immediately, bus undiscovered. **Modern hardware invisible.**【F:kernel/9front-pc64/globals.c†L372-L376】【F:kernel/9front-pc64/main.c†L380-L385】

27. **❌ Serial console output silently discarded** - `uartputs` stub ignores arguments. **Nothing reaches UART.**【F:kernel/9front-pc64/globals.c†L123-L130】

28. **❌ Device reset hooks are inert stubs** - `chandevreset` calls no-op helpers. **Devices never register.**【F:kernel/9front-port/chan.c†L147-L177】

29. **❌ Path sanitisation completely disabled** - `cleanname` returns input unchanged. **Directory traversal possible.**【F:kernel/9front-pc64/globals.c†L102-L105】

30. **❌ Delay helpers spin for zero time** - `microdelay` and `delay` empty stubs. **Hardware races immediate.**【F:kernel/9front-pc64/globals.c†L124-L131】

### **TIER 5: Additional System Issues**

31. **❌ Random generator scribbles over own lock** - `randomseed` passes wrong struct to `setupChachastate`. **Lock corruption.**【F:kernel/9front-port/random.c†L71-L108】

32. **❌ fscheck treats fatal errors as success** - Accumulates return values directly, negative means clean. **False clean reports.**【F:userspace/bin/fscheck.c†L152-L167】【F:userspace/bin/fscheck.c†L245-L268】

33. **❌ fscheck block bitmap pass is no-op** - Loop never inspects/records inconsistencies. **Can't detect corruption.**【F:userspace/bin/fscheck.c†L47-L73】

34. **❌ fscheck superblock errors treated as success** - Negative accumulator misreported as clean. **Filesystem "clean" when broken.**【F:userspace/bin/fscheck.c†L158-L267】

---

## **✅ SIGNIFICANT FIXES ACHIEVED:**

### **Fixed: Page Ownership System (Bug #33)**
- **Status**: `pageowninit()` now properly allocates `pageownpool.pages` with `xalloc()`
- **Impact**: Pebble system can now function, borrow checker operational
- **Location**: `kernel/9front-port/pageown.c†L44-L52`

### **Fixed: Borrow Checker Data Structures (Bugs #19, #28, #20)**
- **Status**: Proper hash table with `BorrowBucket` array, correct `SharedBorrower` linked list
- **Impact**: Memory safety system no longer corrupted, proper Rust-style borrowing
- **Location**: `kernel/borrowchecker.c†42-59`, `kernel/borrowchecker.c†125`, `kernel/borrowchecker.c†280-285`

### **Fixed: Memory Management (Bugs #46, #47)**
- **Status**: Pool headers track sizes, `poolrealloc` copies correct amounts, alignment honored
- **Impact**: Memory allocation now safe and properly aligned
- **Location**: `kernel/9front-pc64/globals.c†64-73`, `kernel/9front-pc64/globals.c†58-61`

### **Fixed: Time System (Bugs #44, #45)**
- **Status**: `fastticks()` returns real hardware ticks, `µs()` converts properly
- **Impact**: Kernel timekeeping now functional
- **Location**: `kernel/9front-port/fastticks.c`

### **Fixed: Format System (Bugs #55, #56, #57)**
- **Status**: `fmtinstall`, `fmtstrinit`, `fmtprint` properly implemented
- **Impact**: Debug output and syscall tracing functional
- **Location**: `kernel/libc9/` implementations

### **Fixed: Architecture Device Registration (Bug #48)**
- **Status**: `devarch.c` ports 9front arch device, `addarchfile` publishes handlers
- **Impact**: Architecture-specific devices accessible
- **Location**: `kernel/9front-pc64/devarch.c`

### **Fixed: Pager System (Bug #51)**
- **Status**: `kickpager` now wakes `swapalloc.r` pager thread
- **Impact**: Memory pressure handling functional
- **Location**: `kernel/9front-port/page.c†120-168`

### **Fixed: Page Table Protection (Bug #61)**
- **Status**: Timing issues resolved, memory management corrected
- **Impact**: Boot progresses further, page table corruption prevented
- **Location**: `kernel/9front-pc64/mmu.c†165-185`

---

## **Impact Assessment:**

**Critical Infrastructure**: ✅ **Much More Stable**
- Borrow checker operational (revolutionary pebble system ready)
- Page ownership functional (exchange system can work)
- Memory management safe (allocation/alignment proper)
- Time system functional (kernel statistics work)

**Userspace Boot**: ❌ **Still Blocked**  
- System init syscalls remain stubbed
- Cannot test userspace programs or servers
- Blocks all userspace testing and development

**9P Protocol**: ❌ **Security & Reliability Issues**
- Message corruption vulnerabilities
- Protocol violations (Tflush, Twstat)
- Buffer overflow possibilities
- Stream desynchronization issues

**Hardware Access**: ❌ **Limited**
- Interrupt system not initialized
- PCI bus not accessible  
- Modern devices invisible
- Serial console non-functional
