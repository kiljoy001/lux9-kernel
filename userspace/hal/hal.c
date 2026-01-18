#include "../lib/liblux/inc/lux.h"
#include "../lib/liblux/inc/server9p.h"

#define DMDIR 0x80000000
#define OREAD 0
#define OWRITE 1
#define ORDWR 2
#define QTFILE 0x00
#define nil ((void *)0)

extern void *memset(void *s, int c, ulong n);
extern void *memmove(void *dest, const void *src, ulong n);
extern long sys_write(int fd, void *buf, long n);
extern int sys_open(char *path, int mode);
extern int sys_close(int fd);
extern long sys_read(int fd, void *buf, long n);
extern uint convD2M(Dir *d, uchar *buf, uint nbuf);
extern uint sizeD2M(Dir *d);

void *malloc(ulong size) {
  void *p = nil;
  if (pebble_alloc(size, &p) < 0)
    return nil;
  return p;
}

void free(void *ptr) { pebble_free(ptr); }

static void hal_log(const char *msg) {
  long len = 0;
  while (msg[len])
    len++;
  sys_write(2, (void *)msg, len);
}

static void hal_log_int(const char *label, int v) {
  char buf[64];
  int n = 0;
  while (label[n]) {
    buf[n] = label[n];
    n++;
  }
  if (v == 0) {
    buf[n++] = '0';
  } else {
    char tmp[16];
    int t = 0;
    int x = v;
    if (x < 0) {
      buf[n++] = '-';
      x = -x;
    }
    while (x > 0 && t < (int)sizeof(tmp)) {
      tmp[t++] = '0' + (x % 10);
      x /= 10;
    }
    while (t > 0)
      buf[n++] = tmp[--t];
  }
  buf[n++] = '\n';
  sys_write(2, buf, n);
}

static void hal_log_ptr(const char *label, void *ptr) {
  char buf[64];
  int n = 0;
  while (label[n]) {
    buf[n] = label[n];
    n++;
  }
  buf[n++] = '0';
  buf[n++] = 'x';
  uintptr v = (uintptr)ptr;
  int started = 0;
  for (int i = (int)(sizeof(uintptr) * 2) - 1; i >= 0; i--) {
    int nibble = (v >> (i * 4)) & 0xF;
    if (nibble || started || i == 0) {
      started = 1;
      buf[n++] = (nibble < 10) ? ('0' + nibble) : ('a' + (nibble - 10));
    }
  }
  buf[n++] = '\n';
  sys_write(2, buf, n);
}

enum {
  Qroot = 1,
};

#define HAL_DISK_SIZE (16 * 1024 * 1024)
static u8int g_disk[HAL_DISK_SIZE];

static void hal_attach(Req *r) {
  r->fid->qid.path = Qroot;
  r->fid->qid.vers = 0;
  r->fid->qid.type = QTFILE;
  r->ofcall.qid = r->fid->qid;
  srv_respond(r, nil);
}

static void hal_walk(Req *r) {
  if (r->ifcall.nwname == 0) {
    r->ofcall.nwqid = 0;
    srv_respond(r, nil);
    return;
  }
  if (r->ifcall.nwname == 1 && r->ifcall.wname[0][0] == '.' &&
      r->ifcall.wname[0][1] == '\0') {
    r->ofcall.nwqid = 1;
    r->ofcall.wqid[0] = r->fid->qid;
    srv_respond(r, nil);
    return;
  }
  srv_respond(r, "file not found");
}

static void hal_open(Req *r) {
  r->ofcall.qid = r->fid->qid;
  srv_respond(r, nil);
}

static void hal_read(Req *r) {
  if (r->fid->qid.path != Qroot) {
    srv_respond(r, "invalid fid");
    return;
  }

  u64int offset = r->ifcall.offset;
  u32int count = r->ifcall.count;
  if (offset >= HAL_DISK_SIZE) {
    r->ofcall.count = 0;
    srv_respond(r, nil);
    return;
  }
  if (offset + count > HAL_DISK_SIZE)
    count = (u32int)(HAL_DISK_SIZE - offset);

  memmove(r->ofcall.data, g_disk + offset, count);
  r->ofcall.count = count;
  srv_respond(r, nil);
}

static void hal_write(Req *r) {
  if (r->fid->qid.path != Qroot) {
    srv_respond(r, "invalid fid");
    return;
  }

  u64int offset = r->ifcall.offset;
  u32int count = r->ifcall.count;
  if (offset >= HAL_DISK_SIZE) {
    r->ofcall.count = 0;
    srv_respond(r, nil);
    return;
  }
  if (offset + count > HAL_DISK_SIZE)
    count = (u32int)(HAL_DISK_SIZE - offset);

  memmove(g_disk + offset, r->ifcall.data, count);
  r->ofcall.count = count;
  srv_respond(r, nil);
}

static void hal_stat(Req *r) {
  Dir d;
  memset(&d, 0, sizeof(d));
  d.name = "ahci0";
  d.mode = 0666;
  d.length = HAL_DISK_SIZE;
  d.qid.path = Qroot;
  d.qid.vers = 0;
  d.qid.type = QTFILE;

  int sz = sizeD2M(&d);
  r->ofcall.stat = malloc(sz);
  if (!r->ofcall.stat) {
    srv_respond(r, "out of memory");
    return;
  }
  r->ofcall.nstat = convD2M(&d, (uchar *)r->ofcall.stat, sz);
  srv_respond(r, nil);
}

static void hal_probe_pci(void) {
  char buf[512];
  int fd = sys_open("#F/PCI/bus", OREAD);
  if (fd < 0)
    fd = sys_open("/dev/family/PCI/bus", OREAD);
  if (fd < 0)
    return;
  long n = sys_read(fd, buf, sizeof(buf) - 1);
  if (n > 0) {
    buf[n] = '\0';
    hal_log("HAL: PCI bus listing:\n");
    sys_write(2, buf, n);
  }
  sys_close(fd);
}

int main(int argc, char **argv) {
  hal_log("HAL: starting\n");
  hal_log_int("HAL: argc=", argc);
  int argc_scan = 0;
  if (argv) {
    while (argv[argc_scan] && argc_scan < 64)
      argc_scan++;
  }
  if (argc_scan > 0)
    argc = argc_scan;
  if (argc > 0)
    hal_log_ptr("HAL: argv0 ptr=", argv[0]);
  if (argc > 1)
    hal_log_ptr("HAL: argv1 ptr=", argv[1]);
  if (argv)
    hal_log_ptr("HAL: argv2 ptr=", argv[2]);
  if (argc > 1 && argv[1]) {
    hal_log("HAL: argv1=");
    long len = 0;
    while (argv[1][len])
      len++;
    sys_write(2, argv[1], len);
    sys_write(2, "\n", 1);
    hal_log_int("HAL: argv1 byte=", (int)(unsigned char)argv[1][0]);
  }
  if (argc > 0 && argv[0]) {
    hal_log("HAL: argv0=");
    long len = 0;
    while (argv[0][len])
      len++;
    sys_write(2, argv[0], len);
    sys_write(2, "\n", 1);
    hal_log_int("HAL: argv0 byte=", (int)(unsigned char)argv[0][0]);
  }
  memset(g_disk, 0, sizeof(g_disk));
  hal_probe_pci();

  int pipe_fd = -1;
  if (argc > 1) {
    int v = 0;
    char *p = argv[1];
    while (*p >= '0' && *p <= '9') {
      v = v * 10 + (*p - '0');
      p++;
    }
    pipe_fd = v;
  }

  Srv s = {
      .attach = hal_attach,
      .walk = hal_walk,
      .open = hal_open,
      .read = hal_read,
      .write = hal_write,
      .stat = hal_stat,
  };

  srv_init(&s);
  if (pipe_fd >= 0) {
    srv_loop(&s, pipe_fd, pipe_fd);
  } else {
    srv_loop(&s, 0, 1);
  }
  return 0;
}
