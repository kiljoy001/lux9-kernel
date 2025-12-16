// runtime.c - Userspace Runtime for C# Init AOT (Bare Metal)
// No includes to avoid dependency issues but we define types manually
typedef unsigned long size_t;
typedef unsigned long uint64_t;
typedef unsigned int uint32_t;
typedef unsigned short uint16_t;
typedef unsigned char uint8_t;
typedef long intptr_t;
typedef unsigned long uintptr_t;
#define NULL ((void *)0)

// Syscall constants (From kernel/9front-port/sysproc.c)
#define SYS_bind 2
#define SYS_dup 5
#define SYS_exits 8
#define SYS_open 14
#define SYS_write 20

// ---- Object Layouts (Must be before prototypes) ----
typedef struct {
  uintptr_t header; // SyncBlock + TypeHandle
  uint32_t length;
  uint16_t first_char;
} StringObject;

// ---- Allocator (Static Heap) ----
#define HEAP_SIZE (1024 * 1024)
static uint8_t heap[HEAP_SIZE];
static size_t heap_ptr = 0;

// Match syscall_amd64.o symbol
long __syscall(long n, long a1, long a2, long a3, long a4, long a5, long a6);

static inline long syscall3(long n, long a1, long a2, long a3) {
  return __syscall(n, a1, a2, a3, 0, 0, 0);
}

static inline long syscall1(long n, long a1) {
  return __syscall(n, a1, 0, 0, 0, 0, 0);
}

// Matches init_dotnet crt0.o expectation
void exit(int status) {
  syscall1(SYS_exits, 0);
  while (1) {
  }
}

void *memset(void *s, int c, size_t n) {
  unsigned char *p = s;
  while (n--) {
    *p++ = (unsigned char)c;
  }
  return s;
}

void *lux_alloc(uint64_t size, uint32_t type_token) {
  size = (size + 7) & ~7;
  if (heap_ptr + size > HEAP_SIZE)
    return NULL;
  void *ptr = &heap[heap_ptr];
  heap_ptr += size;
  memset(ptr, 0, size);
  return ptr;
}

void lux_token_mint(void *ptr) {}
void lux_token_burn(void *ptr) {}

// ---- CLR Internals ----

// Entry Point Manually Wired
// Token 06000005 = 100663301 (Init.Main) from recent build
void method_100663301(void *arg);

#define O_RDWR 2

#define MREPL 0x0000
#define MBEFORE 0x0001
#define MAFTER 0x0002
#define MCREATE 0x0004

static inline long syscall2(long n, long a1, long a2) {
  return __syscall(n, a1, a2, 0, 0, 0, 0);
}

void init_namespace() {
  // Bind essential kernel devices
  // bind("#c", "/dev", MREPL)
  syscall3(SYS_bind, (long)"#c", (long)"/dev", MREPL);

  // bind("#e", "/env", MREPL|MCREATE)
  syscall3(SYS_bind, (long)"#e", (long)"/env", MREPL | MCREATE);

  // bind("#p", "/proc", MREPL)
  syscall3(SYS_bind, (long)"#p", (long)"/proc", MREPL);
}

// Write directly to console device for early boot debugging
static void early_write(const char *msg) {
  // Open console directly without namespace
  long fd = syscall2(SYS_open, (long)"#c/cons", O_RDWR);
  if (fd >= 0) {
    int len = 0;
    const char *p = msg;
    while (*p++)
      len++;
    syscall3(SYS_write, fd, (long)msg, len);
    // Don't close - leave it open
  }
}

void init_stdio() {
  // FIRST: Write early boot marker directly to console device
  // This proves init binary is actually running
  early_write("INIT: Binary executing!\n");

  init_namespace();

  early_write("INIT: Namespace setup complete\n");

  long fd = syscall2(SYS_open, (long)"/dev/cons", O_RDWR);
  if (fd < 0) {
    // Fallback if bind failed
    fd = syscall2(SYS_open, (long)"#c/cons", O_RDWR);
  }

  if (fd >= 0) {
    if (fd != 0) {
      syscall2(SYS_dup, fd, 0);
    }
    syscall2(SYS_dup, 0, 1);
    syscall2(SYS_dup, 0, 2);
  }

  early_write("INIT: stdio initialized\n");
}

int main() {
  init_stdio();
  early_write("INIT: Calling C# Main\n");
  method_100663301(NULL);
  early_write("INIT: C# Main returned, exiting\n");
  exit(0);
  return 0;
}

// Internal Calls
int clr_string_get_length(StringObject *str) {
  if (!str)
    return 0;
  return str->length;
}

void *clr_thread_get_current() { return NULL; }

void clr_p9_write(int fd, void *buf, int count) {
  syscall3(SYS_write, fd, (long)buf, count);
}
void clr_p9_read() {}
void clr_p9_attach() {}
void clr_p9_clunk() {}
void clr_p9_stat() {}

// Basic Write implementation for Init::Write
void clr_write(StringObject *s) {
  if (!s)
    return;

  // Simplified UTF-16 to ASCII
  char buf[256];
  int len = s->length;
  if (len > 255)
    len = 255;

  for (int i = 0; i < len; i++) {
    buf[i] = (char)((uint16_t *)&s->first_char)[i];
  }

  __syscall(SYS_write, 1, (uint64_t)buf, len, 0, 0, 0);
}

void *clr_newobj(uint32_t token) {
  // Simple alloc
  return lux_alloc(64, token);
}

// String literal helper with mapping
StringObject *clr_string_from_literal(int id) {
  char *msg = "";

  // Mapping from init.ssa
  if (id == 1)
    msg = "Hello AOT World!\n";
  if (id == 37)
    msg = "Kernel AOT Init Shim Loaded.\n";
  if (id == 97)
    msg = "Math Test: 10 + 20...\n";
  if (id == 143)
    msg = "Math Check OK.\n";
  if (id == 175)
    msg = "Math Check FAILED.\n";

  size_t len = 0;
  const char *p = msg;
  while (*p++)
    len++;

  // Allocate
  StringObject *obj =
      (StringObject *)lux_alloc(sizeof(StringObject) + len * 2 + 2, 0);
  if (!obj)
    return NULL;

  obj->length = len;
  uint16_t *chars = &obj->first_char;
  for (size_t i = 0; i < len; i++) {
    chars[i] = msg[i];
  }
  chars[len] = 0;

  return obj;
}
