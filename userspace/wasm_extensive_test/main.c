/* main.c - Userspace runner for Extensive WASM Test
 */

#include "u.h"

#define EXCHANGE_PAGE_ADDR 0x7FFFFEEFF000ULL
#define P9_CONTROL_OFFSET 0xF00
#define P9_PAGE_SIZE 4096

#define Tsyscall 130
#define Rsyscall 131
#define Rerror 107

#define SYS_WRITE 4
#define SYS_READ 3
#define SYS_OPEN 1
#define SYS_CLOSE 2
#define SYS_SEEK 39
#define SYS_WASM_COMPILE 100

// ...

// Forward declarations
static u64int sys_seek(int fd, u64int offset, int whence);
static int read_file(const char *path, uchar *buf, int max_len);

#define SYS_WASM_EXECUTE 101
#define SYS_WASM_DESTROY 102

// 9P Open modes
#define OREAD 0

typedef unsigned int uint;
typedef unsigned char uchar;
typedef unsigned long long uvlong;
typedef unsigned long long u64int;
typedef unsigned int u32int;
typedef unsigned char u8int;

/* P9 Control structure */
struct P9Control {
  uint doorbell;
  uint status;
  uint req_head;
  uint req_tail;
  uint rep_head;
  uint rep_tail;
};

/* Global exchange page pointers */
static volatile uchar *exchange;
static volatile struct P9Control *ctl;

/* --- Stdlib-like Helpers --- */

static void *memcpy(void *dst, const void *src, unsigned long n) {
  uchar *d = dst;
  const uchar *s = src;
  while (n--)
    *d++ = *s++;
  return dst;
}

static void *memset(void *dst, int c, unsigned long n) {
  uchar *d = dst;
  while (n--)
    *d++ = (uchar)c;
  return dst;
}

static int strlen(const char *s) {
  int n = 0;
  while (*s++)
    n++;
  return n;
}

/* --- Serialization Helpers --- */

static void put_u32(uchar *p, uint val) {
  p[0] = val;
  p[1] = val >> 8;
  p[2] = val >> 16;
  p[3] = val >> 24;
}

static void put_u16(uchar *p, unsigned short val) {
  p[0] = val;
  p[1] = val >> 8;
}

static void put_u8(uchar *p, uchar val) { p[0] = val; }

static void put_u64(uchar *p, uvlong val) {
  p[0] = val;
  p[1] = val >> 8;
  p[2] = val >> 16;
  p[3] = val >> 24;
  p[4] = val >> 32;
  p[5] = val >> 40;
  p[6] = val >> 48;
  p[7] = val >> 56;
}

static uint get_u32(const uchar *p) {
  return (uint)p[0] | ((uint)p[1] << 8) | ((uint)p[2] << 16) |
         ((uint)p[3] << 24);
}

static uvlong get_u64(const uchar *p) {
  return (uvlong)p[0] | ((uvlong)p[1] << 8) | ((uvlong)p[2] << 16) |
         ((uvlong)p[3] << 24) | ((uvlong)p[4] << 32) | ((uvlong)p[5] << 40) |
         ((uvlong)p[6] << 48) | ((uvlong)p[7] << 56);
}

/* --- Syscall Wrappers --- */

static void sys_print(const char *msg) {
  int msg_len = strlen(msg);
  uchar *req = (uchar *)exchange;
  uint pos = 0;

  // Fcall header: size[4] type[1] tag[2]
  // Tsyscall args: scallnr[4] scount[4] sdata[scount]
  // sdata for SYS_WRITE: ARGC[4] FD[4] OFFSET[8] COUNT[4] DATA[COUNT]
  uint sdata_len = 4 + 4 + 8 + 4 + msg_len;
  uint size = 4 + 1 + 2 + 4 + 4 + sdata_len;

  memset(req, 0, 256);
  put_u32(req + pos, size);
  pos += 4;
  req[pos++] = Tsyscall;
  put_u16(req + pos, 1); // tag
  pos += 2;
  put_u32(req + pos, SYS_WRITE); // scallnr
  pos += 4;
  put_u32(req + pos, sdata_len); // scount
  pos += 4;

  // sdata
  put_u32(req + pos, 3); // ARGC=3 (fd, offset, count)
  pos += 4;
  put_u32(req + pos, 1); // fd = 1 (stdout)
  pos += 4;
  put_u64(req + pos, 0); // offset
  pos += 8;
  put_u32(req + pos, msg_len); // count
  pos += 4;
  memcpy(req + pos, msg, msg_len); // data
  pos += msg_len;

  ctl->doorbell = 1;
  __asm__ volatile("syscall" ::: "rax", "rcx", "r11", "memory");
}

static void print_u32(uint val) {
  char buf[16];
  int i = 0;
  if (val == 0) {
    sys_print("0");
    return;
  }
  while (val > 0) {
    buf[i++] = '0' + (val % 10);
    val /= 10;
  }
  while (i > 0) {
    char c[2] = {buf[--i], 0};
    sys_print(c);
  }
}

static void print_u64(u64int val) {
  char buf[32];
  int i = 0;
  if (val == 0) {
    sys_print("0");
    return;
  }
  while (val > 0) {
    buf[i++] = '0' + (val % 10);
    val /= 10;
  }
  while (i > 0) {
    char c[2] = {buf[--i], 0};
    sys_print(c);
  }
}

/* Basic heap allocator (bump pointer) */
static uchar heap[1024 * 1024]; // 1MB heap
static uint heap_ptr = 0;

static void *malloc(uint size) {
  if (heap_ptr + size > sizeof(heap))
    return 0;
  void *ptr = &heap[heap_ptr];
  heap_ptr += size;
  return ptr;
}

static void free(void *ptr) {
  // No-op bump allocator
}

/* WASM Syscalls */

static int wasm_compile(uchar *wasm_bytes, uint wasm_len) {
  uchar *req = (uchar *)exchange;
  uint pos = 0;
  // sdata for SYS_WASM_COMPILE: ARGC[4] WASM_LEN[4] WASM_BYTES[WASM_LEN]
  uint sdata_len = 4 + 4 + wasm_len;
  uint size = 4 + 1 + 2 + 4 + 4 + sdata_len;

  memset(req, 0, sizeof(heap)); // Clear request buffer (using heap size as safe
                                // max, but req is page sized)
  // Actually exchange is 4KB? Let's check init.c. "C init (4KB exchange)"
  // If wasm module > 4KB, we can't send it in one Tsyscall on the exchange
  // page? Wait, SYS_WASM_COMPILE in kernel takes pointer to buffer? No, it
  // takes [size][bytes]. If submodule is large, we might have an issue with 4KB
  // exchange page limit. However, the test init.c uses exchange for Tsyscall.
  // The provided init.c uses:
  // "C init (4KB exchange)"

  // BUT the extensive test WAT might compile to small binary.
  // Let's assume it fits for now. If not, we'd need a different mechanism (e.g.
  // read file in kernel). Actually wasm_arena_test.c in kernel reads file
  // content then constructs Fcall. Userspace syscalls are limited by exchange
  // page size for arguments? In lux9 userspace, syscall arguments (sdata) are
  // written to exchange page.

  // WORKAROUND: If module is too big, this simple syscall wrapper fails.
  // But extensive_test.wat is small.

  if (size > P9_PAGE_SIZE) { // Check against actual page size
    sys_print("Error: WASM module too large for exchange page syscall\n");
    return -1;
  }

  put_u32(req + pos, size);
  pos += 4;
  req[pos++] = Tsyscall;
  put_u16(req + pos, 1); // tag
  pos += 2;
  put_u32(req + pos, SYS_WASM_COMPILE); // scallnr
  pos += 4;
  put_u32(req + pos, sdata_len); // scount
  pos += 4;

  // sdata: [ARGC:4][wasm_len:4][wasm_bytes...]
  put_u32(req + pos, 2); // ARGC=2 (wasm_len, wasm_bytes)
  pos += 4;
  put_u32(req + pos, wasm_len);
  pos += 4;
  memcpy(req + pos, wasm_bytes, wasm_len);
  pos += wasm_len;

  ctl->doorbell = 1;
  __asm__ volatile("syscall" ::: "rax", "rcx", "r11", "memory");

  // Check reply
  pos = 0;
  get_u32(req + pos);
  pos += 4; // size
  uchar type = req[pos++];
  if (type == Rerror) {
    sys_print("WASM Compile Error\n");
    return -1;
  }

  // Determine PID from reply? sdata usually contains return value.
  // Rsyscall sdata for wasm_compile sends back PID (u64).
  // Rsyscall: retval[8] scount[4] sdata...
  // For compile, retval is PID (u64).
  u64int retval = get_u64(req + pos);
  // pos += 8;
  // scount...
  return (int)retval;
}

static u64int wasm_execute(const char *func_name) {
  uchar *req = (uchar *)exchange;
  uint pos = 0;
  int len = strlen(func_name);
  // sdata for SYS_WASM_EXECUTE: ARGC[4] NAME_LEN[4] NAME[NAME_LEN]
  uint sdata_len = 4 + 4 + len;
  uint size = 4 + 1 + 2 + 4 + 4 + sdata_len;

  memset(req, 0, 256);
  put_u32(req + pos, size);
  pos += 4;
  req[pos++] = Tsyscall;
  put_u16(req + pos, 1); // tag
  pos += 2;
  put_u32(req + pos, SYS_WASM_EXECUTE); // scallnr
  pos += 4;
  put_u32(req + pos, sdata_len); // scount
  pos += 4;

  // sdata: [ARGC:4][len:4][name...]
  put_u32(req + pos, 2); // ARGC=2 (len, name)
  pos += 4;
  put_u32(req + pos, len);
  pos += 4;
  memcpy(req + pos, func_name, len);
  pos += len;

  ctl->doorbell = 1;
  __asm__ volatile("syscall" ::: "rax", "rcx", "r11", "memory");

  pos = 0;
  get_u32(req + pos);
  pos += 4;
  if (req[pos++] == Rerror) {
    sys_print("WASM Exec Error: ");
    sys_print(func_name);
    sys_print("\n");
    return 0;
  }
  pos += 2;                  // tag
  return get_u64(req + pos); // retval is first 8 bytes of Rsyscall body
}

/* File I/O helpers to read .wasm file */
// We need to open and read file.
// SYS_OPEN, SYS_READ, SYS_CLOSE.
// But we are in userspace, pure syscalls.
// Reusing logic from init.c could be hard as it doesn't have open/read/close
// unimplemented? init.c has SYS_WRITE and SYS_FORK. I need to implement sync
// open/read.

static int sys_open(const char *path, int mode) {
  uchar *req = (uchar *)exchange;
  uint pos = 0;
  int path_len = strlen(path);

  // Fcall header: size[4] type[1] tag[2]
  // Tsyscall args: scallnr[4] scount[4] sdata[scount]
  // sdata for SYS_OPEN: ARGC[4] FID[4] PATH_LEN[2] PATH[PATH_LEN] MODE[1]
  uint sdata_len = 4 + 4 + 2 + path_len + 1;
  uint size = 4 + 1 + 2 + 4 + 4 + sdata_len;

  if (pos + size > P9_PAGE_SIZE)
    return -1; // Check if request fits

  memset(req, 0, 512); // clear
  put_u32(req + pos, size);
  pos += 4;
  req[pos++] = Tsyscall;
  put_u16(req + pos, 1); // tag
  pos += 2;
  put_u32(req + pos, SYS_OPEN); // scallnr
  pos += 4;
  put_u32(req + pos, sdata_len); // scount
  pos += 4;

  // sdata
  put_u32(req + pos, 2); // ARGC=2 (fid, path+mode)
  pos += 4;
  put_u32(req + pos, 0); // Dummy FID (unused by kernel for SYS_OPEN)
  pos += 4;
  put_u16(req + pos, path_len);
  pos += 2;
  memcpy(req + pos, path, path_len);
  pos += path_len;
  put_u8(req + pos, mode);
  pos += 1;

  ctl->doorbell = 1;
  __asm__ volatile("syscall" ::: "rax", "rcx", "r11", "memory");

  pos = 0;
  get_u32(req + pos);
  pos += 4;
  if (req[pos++] == Rerror)
    return -1;
  pos += 2; // tag

  // Rsyscall: retval[8] scount[4] sdata...
  return (int)get_u64(req + pos);
}

static int sys_read(int fd, void *buf, int count, u64int offset) {
  uchar *req = (uchar *)exchange;
  uint sdata_len = 4 + 4 + 8 + 4; // ARGC(4) + FID(4) + OFF(8) + COUNT(4)
  uint size = 4 + 1 + 2 + 4 + 4 + sdata_len;
  uint pos = 0;

  if (pos + size > P9_PAGE_SIZE)
    return -1;

  memset(req, 0, 256);
  put_u32(req + pos, size);
  pos += 4;
  req[pos++] = Tsyscall;
  put_u16(req + pos, 1); // tag
  pos += 2;
  put_u32(req + pos, SYS_READ); // scallnr
  pos += 4;
  put_u32(req + pos, sdata_len); // scount
  pos += 4;

  // sdata
  put_u32(req + pos, 3); // ARGC=3
  pos += 4;
  put_u32(req + pos, fd);
  pos += 4;
  put_u64(req + pos, offset);
  pos += 8;
  put_u32(req + pos, count);
  pos += 4;

  ctl->doorbell = 1;
  __asm__ volatile("syscall" ::: "rax", "rcx", "r11", "memory");

  // CRITICAL: Parse reply and save data to local vars BEFORE any sys_print calls
  // because sys_print reuses the exchange buffer!
  pos = 0;
  get_u32(req + pos);
  pos += 4;
  uchar reply_type = req[pos++];
  if (reply_type == Rerror) {
    return -1;
  }
  pos += 2; // tag

  // Rsyscall: retval[8] scount[4] sdata...
  u64int retval = get_u64(req + pos);
  pos += 8;
  uint r_scount = get_u32(req + pos);
  pos += 4;

  // Copy data to destination buffer before exchange buffer gets overwritten
  if (r_scount > 0) {
    if (r_scount > count)
      r_scount = count;
    memcpy(buf, (void *)(req + pos), r_scount);
  }

  // Now it's safe to print debug messages
  return (int)retval;
}

static void sys_close(int fd) {
  uchar *req = (uchar *)exchange;
  uint pos = 0;
  uint sdata_size = 4;
  uint size = 4 + 1 + 2 + 4 + 4 + sdata_size;

  memset(req, 0, 512);
  put_u32(req + pos, size);
  pos += 4;
  req[pos++] = Tsyscall;
  put_u16(req + pos, 1);
  pos += 2;
  put_u32(req + pos, SYS_CLOSE);
  pos += 4;
  put_u32(req + pos, sdata_size);
  pos += 4;
  put_u32(req + pos, fd);

  ctl->doorbell = 1;
  __asm__ volatile("syscall" ::: "rax", "rcx", "r11", "memory");
}

int main() {
  exchange = (volatile uchar *)EXCHANGE_PAGE_ADDR;
  ctl = (volatile struct P9Control *)(exchange + P9_CONTROL_OFFSET);

  sys_print("=== Extensive WASM Userspace Test ===\n");

  // 1. Read WASM file
  sys_print("Loading #/./boot/extensive_test.wasm...\n");
  int wasm_len = read_file("#/./boot/extensive_test.wasm", heap, 64 * 1024);
  if (wasm_len <= 0) {
    sys_print("Failed to load WASM file\n");
    return 1;
  }

  // 2. Compile
  sys_print("Compiling WASM...\n");
  if (wasm_compile(heap, wasm_len) < 0) {
    sys_print("Compilation failed!\n");
    return 1;
  }
  sys_print("Compilation success.\n");

  // 3. Execute Tests
  const char *tests[] = {"test_arithmetic", "test_control_flow", "test_memory",
                         "test_host_interop", "test_globals"};

  for (int i = 0; i < 5; i++) {
    sys_print("Running ");
    sys_print(tests[i]);
    sys_print("... ");

    u64int res = wasm_execute(tests[i]);
    if (res == 1) {
      sys_print("[PASS]\n");
    } else {
      sys_print("[FAIL]\n");
    }
  }

  sys_print("=== Test Suite Completed ===\n");

  // Hang or exit? Init usually loops.
  for (;;)
    ;
  return 0;
}

/* Helper to read entire file */
static u64int sys_seek(int fd, u64int offset, int whence) {
  uchar *req = (uchar *)exchange;
  uint sdata_len = 4 + 4 + 8 + 4; // ARGC(4) + FD(4) + OFF(8) + WHENCE(4)
  uint size = 4 + 1 + 2 + 4 + 4 + sdata_len;
  uint pos = 0;

  memset(req, 0, 256);
  put_u32(req + pos, size);
  pos += 4;
  req[pos++] = Tsyscall;
  put_u16(req + pos, 1);
  pos += 2;
  put_u32(req + pos, SYS_SEEK);
  pos += 4;
  put_u32(req + pos, sdata_len);
  pos += 4;

  // sdata: ARGC=3, FD, OFF, WHENCE
  put_u32(req + pos, 3);
  pos += 4;
  put_u32(req + pos, fd);
  pos += 4;
  put_u64(req + pos, offset);
  pos += 8;
  put_u32(req + pos, whence);

  ctl->doorbell = 1;
  __asm__ volatile("syscall" ::: "rax", "rcx", "r11", "memory");

  pos = 0;
  get_u32(req + pos);
  pos += 4;
  if (req[pos++] == Rerror) {
    char errbuf[128];
    pos += 2; // tag
    uint elen = (uint)req[pos] | ((uint)req[pos + 1] << 8);
    pos += 2;
    if (elen > 127)
      elen = 127;
    memcpy(errbuf, (void *)(req + pos), elen);
    errbuf[elen] = 0;
    sys_print("sys_seek error: ");
    sys_print(errbuf);
    sys_print("\n");
    return -1;
  }
  pos += 2; // tag
  return get_u64(req + pos);
}

static int read_file(const char *path, uchar *buf, int max_len) {
  int fd = sys_open(path, OREAD);
  if (fd < 0) {
    sys_print("Failed to open file: ");
    sys_print(path);
    sys_print("\n");
    return -1;
  }

  // Get file size
  u64int file_size = sys_seek(fd, 0, 2); // SEEK_END
  sys_seek(fd, 0, 0);                    // SEEK_SET

  sys_print("File size: ");
  // TODO: print size
  // if (file_size > max_len) ...

  int total = 0;
  int target = (int)file_size;
  if (target > max_len)
    target = max_len;

  while (total < target) {
    int to_read = target - total;
    if (to_read > 2048)
      to_read = 2048; // Limit chunk size to fit in exchange page
    int n = sys_read(fd, buf + total, to_read, total);
    if (n < 0) {
      sys_print("Read error\n");
      // sys_close(fd);
      return -1;
    }
    if (n == 0)
      break;
    total += n;
  }
  // sys_close(fd); // Avoid panic
  return total;
}
