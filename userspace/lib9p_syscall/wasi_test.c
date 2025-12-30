/* WASI Shim Test - Userspace Layer 2
 * Tests WASI shim functionality via Tsyscall messages
 */

#define EXCHANGE_PAGE_ADDR 0x7FFFFEEFF000ULL
#define P9_CONTROL_OFFSET 0xF00
#define Tsyscall 130
#define Rsyscall 131
#define SYS_WASM_COMPILE 100
#define SYS_WASM_EXECUTE 101
#define SYS_WASM_DESTROY 102

typedef unsigned int uint;
typedef unsigned char uchar;
typedef unsigned long long uvlong;

/* P9 Control structure */
struct P9Control {
  uint doorbell;
  uint status;
  uint req_head;
  uint req_tail;
  uint rep_head;
  uint rep_tail;
};

/* Embedded hello.wasm - Prints "Hello from Lux9 WASI Shim!" */
static const uchar wasm_module[] = {
    0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00, 0x01, 0x10, 0x03, 0x60,
    0x04, 0x7f, 0x7f, 0x7f, 0x7f, 0x01, 0x7f, 0x60, 0x01, 0x7f, 0x00, 0x60,
    0x00, 0x00, 0x02, 0x46, 0x02, 0x16, 0x77, 0x61, 0x73, 0x69, 0x5f, 0x73,
    0x6e, 0x61, 0x70, 0x73, 0x68, 0x6f, 0x74, 0x5f, 0x70, 0x72, 0x65, 0x76,
    0x69, 0x65, 0x77, 0x31, 0x08, 0x66, 0x64, 0x5f, 0x77, 0x72, 0x69, 0x74,
    0x65, 0x00, 0x00, 0x16, 0x77, 0x61, 0x73, 0x69, 0x5f, 0x73, 0x6e, 0x61,
    0x70, 0x73, 0x68, 0x6f, 0x74, 0x5f, 0x70, 0x72, 0x65, 0x76, 0x69, 0x65,
    0x77, 0x31, 0x09, 0x70, 0x72, 0x6f, 0x63, 0x5f, 0x65, 0x78, 0x69, 0x74,
    0x00, 0x01, 0x03, 0x02, 0x01, 0x02, 0x05, 0x03, 0x01, 0x00, 0x01, 0x07,
    0x13, 0x02, 0x06, 0x6d, 0x65, 0x6d, 0x6f, 0x72, 0x79, 0x02, 0x00, 0x06,
    0x5f, 0x73, 0x74, 0x61, 0x72, 0x74, 0x00, 0x02, 0x0a, 0x23, 0x01, 0x21,
    0x00, 0x41, 0x00, 0x41, 0x80, 0x08, 0x36, 0x02, 0x00, 0x41, 0x04, 0x41,
    0x1b, 0x36, 0x02, 0x00, 0x41, 0x01, 0x41, 0x00, 0x41, 0x01, 0x41, 0xc0,
    0x00, 0x10, 0x00, 0x1a, 0x41, 0x00, 0x10, 0x01, 0x0b, 0x0b, 0x22, 0x01,
    0x00, 0x41, 0x80, 0x08, 0x0b, 0x1b, 0x48, 0x65, 0x6c, 0x6c, 0x6f, 0x20,
    0x66, 0x72, 0x6f, 0x6d, 0x20, 0x4c, 0x75, 0x78, 0x39, 0x20, 0x57, 0x41,
    0x53, 0x49, 0x20, 0x53, 0x68, 0x69, 0x6d, 0x21, 0x0a};
static const uint wasm_module_len = 201;

/* Simple memory operations */
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

static uint strlen(const char *s) {
  uint len = 0;
  while (*s++)
    len++;
  return len;
}

/* Write little-endian integers */
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

/* Read little-endian integers */
static uint get_u32(const uchar *p) {
  return (uint)p[0] | ((uint)p[1] << 8) | ((uint)p[2] << 16) |
         ((uint)p[3] << 24);
}

static unsigned short get_u16(const uchar *p) {
  return (unsigned short)p[0] | ((unsigned short)p[1] << 8);
}

static uvlong get_u64(const uchar *p) {
  return (uvlong)p[0] | ((uvlong)p[1] << 8) | ((uvlong)p[2] << 16) |
         ((uvlong)p[3] << 24) | ((uvlong)p[4] << 32) | ((uvlong)p[5] << 40) |
         ((uvlong)p[6] << 48) | ((uvlong)p[7] << 56);
}

/* Send Tsyscall and wait for Rsyscall reply */
static int send_tsyscall(volatile uchar *exchange,
                         volatile struct P9Control *ctl, uint scallnr,
                         const uchar *sdata, uint sdata_len, uchar *reply_buf,
                         uint *reply_len) {
  /* Build Tsyscall message: [size:4] [type:1] [tag:2] [scallnr:4] [scount:4]
   * [sdata:n] */
  uchar *req = (uchar *)exchange;
  uint size = 4 + 1 + 2 + 4 + 4 + sdata_len;
  uint pos = 0;

  /* Clear request buffer */
  memset(req, 0, 4096);

  /* Write Tsyscall header */
  put_u32(req + pos, size);
  pos += 4;
  req[pos++] = Tsyscall;
  put_u16(req + pos, 1);
  pos += 2;
  put_u32(req + pos, scallnr);
  pos += 4;
  put_u32(req + pos, sdata_len);
  pos += 4;

  /* Write sdata payload */
  if (sdata_len > 0)
    memcpy(req + pos, sdata, sdata_len);

  /* Ring the doorbell */
  ctl->doorbell = 1;

  /* Issue syscall */
  __asm__ volatile("syscall" ::: "rax", "rcx", "r11", "memory");

  /* Read Rsyscall reply: [size:4] [type:1] [tag:2] [retval:8] [scount:4]
   * [sdata:n] */
  pos = 0;
  /* uint reply_size = */ get_u32(req + pos);
  pos += 4;
  uchar reply_type = req[pos++];
  /* uint reply_tag = */ get_u16(req + pos);
  pos += 2;
  uvlong retval = get_u64(req + pos);
  pos += 8;
  uint scount = get_u32(req + pos);
  pos += 4;

  if (reply_type != Rsyscall) {
    return -1; /* Wrong reply type */
  }

  /* Copy reply data if buffer provided */
  if (reply_buf && scount > 0) {
    memcpy(reply_buf, req + pos, scount);
  }
  if (reply_len) {
    *reply_len = scount;
  }

  return (int)retval;
}

void main(void) {
  volatile uchar *exchange = (volatile uchar *)EXCHANGE_PAGE_ADDR;
  volatile struct P9Control *ctl =
      (volatile struct P9Control *)(exchange + P9_CONTROL_OFFSET);
  uchar reply_buf[256];
  uint reply_len;
  int result;

  /* Wait a moment for kernel to stabilize */
  for (volatile int i = 0; i < 100000; i++)
    ;

  /* Compile WASM module */
  /* sdata format: [module_size:4] [module_bytes:n] */
  uchar compile_sdata[4 + 201];
  put_u32(compile_sdata, wasm_module_len);
  memcpy(compile_sdata + 4, wasm_module, wasm_module_len);

  result = send_tsyscall(exchange, ctl, SYS_WASM_COMPILE, compile_sdata,
                         4 + wasm_module_len, reply_buf, &reply_len);

  if (result != 0) {
    /* Compilation failed - hang */
    while (1)
      ;
  }

  uvlong wasm_pid = get_u64(reply_buf);
  (void)wasm_pid;

  /* Execute _start */
  const char *func_name = "_start";
  uint func_name_len = strlen(func_name);

  /* sdata format: [func_name_len:4] [func_name:n] */
  uchar exec_sdata[4 + 32];
  put_u32(exec_sdata, func_name_len);
  memcpy(exec_sdata + 4, func_name, func_name_len);

  result = send_tsyscall(exchange, ctl, SYS_WASM_EXECUTE, exec_sdata,
                         4 + func_name_len, reply_buf, &reply_len);

  if (result != 0) {
    /* Execution failed */
  } else {
    /* Success */
  }

  /* Destroy WASM instance */
  result = send_tsyscall(exchange, ctl, SYS_WASM_DESTROY, 0, 0, 0, 0);
  (void)result;

  /* Done - hang */
  while (1)
    ;
}
