/*
 * PCI Family Implementation - HAL Userspace
 */

#include "family_pci.h"

struct FamilyExchangePage *global_pci_family = nil;
struct PCIFamilyContext *global_pci_ctx = nil;

extern int sys_open(char *path, int mode);
extern int sys_close(int fd);
extern long sys_read(int fd, void *buf, long n);
extern long sys_seek(int fd, long offset, int whence);

static char *pci_find_devices_field(char *buf) {
  static char needle[] = "devices:";
  int i;

  if (buf == nil)
    return nil;
  for (; *buf != 0; buf++) {
    for (i = 0; needle[i] != 0; i++) {
      if (buf[i] != needle[i])
        break;
    }
    if (needle[i] == 0)
      return buf;
  }
  return nil;
}

static int pci_read_count_from_ctl(void) {
  char buf[64];
  char *p;
  int fd;
  long n;

  fd = sys_open("#J/ctl", OREAD);
  if (fd < 0)
    return -1;
  n = sys_read(fd, buf, sizeof(buf) - 1);
  sys_close(fd);
  if (n <= 0)
    return -1;
  buf[n] = 0;

  p = pci_find_devices_field(buf);
  if (p == nil)
    return -1;
  p += 8;
  while (*p == ' ' || *p == '\t')
    p++;
  return atoi(p);
}

static int pci_open_config(u8int bus, u8int dev, u8int func) {
  char path[64];

  if (snprint(path, sizeof(path), "%s/%04x:%02x:%02x.%d/config",
              PCI_SHARP_ROOT, 0, bus, dev, func) >= (int)sizeof(path))
    return -1;
  return sys_open(path, OREAD);
}

static int pci_read_config_bytes(u8int bus, u8int dev, u8int func, u8int offset,
                                 void *data, int n) {
  int fd;
  long rv;

  if (data == nil || n <= 0 || offset + n > PCI_CONFIG_SPACE_SIZE)
    return -1;

  fd = pci_open_config(bus, dev, func);
  if (fd < 0)
    return -1;
  if (offset != 0 && sys_seek(fd, offset, 0) < 0) {
    sys_close(fd);
    return -1;
  }
  rv = sys_read(fd, data, n);
  sys_close(fd);
  return rv == n ? 0 : -1;
}

/*
 * Configuration Space Access Stubs
 * HAL currently talks to the kernel PCI sharp device directly.
 */
int pci_config_read32(u8int bus, u8int dev, u8int func, u8int offset,
                      u32int *data) {
  u8int raw[4];

  if (data == nil)
    return -1;
  if (pci_read_config_bytes(bus, dev, func, offset, raw, sizeof(raw)) < 0)
    return -1;
  *data = (u32int)raw[0] | ((u32int)raw[1] << 8) | ((u32int)raw[2] << 16) |
          ((u32int)raw[3] << 24);
  return 0;
}

int pci_config_read16(u8int bus, u8int dev, u8int func, u8int offset,
                      u16int *data) {
  u8int raw[2];

  if (data == nil)
    return -1;
  if (pci_read_config_bytes(bus, dev, func, offset, raw, sizeof(raw)) < 0)
    return -1;
  *data = (u16int)raw[0] | ((u16int)raw[1] << 8);
  return 0;
}

int pci_config_read8(u8int bus, u8int dev, u8int func, u8int offset,
                     u8int *data) {
  if (data == nil)
    return -1;
  return pci_read_config_bytes(bus, dev, func, offset, data, 1);
}

int pcifamily_refresh(void) {
  char *buf;
  int fd;
  long n;
  int count = 0;
  int saw_data = 0;
  int line_start = 1;
  struct PCIFamilyContext *ctx;

  ctx = global_pci_ctx;
  if (ctx == nil)
    return -1;

  buf = family_alloc_zero(512);
  if (buf == nil)
    return -1;

  fd = sys_open(PCI_BUS_PATH, OREAD);
  if (fd >= 0) {
    while ((n = sys_read(fd, buf, 512)) > 0) {
      long i;

      saw_data = 1;
      for (i = 0; i < n; i++) {
        if (line_start && buf[i] != ' ' && buf[i] != '\t' && buf[i] != '\n' &&
            buf[i] != '\r')
          count++;
        line_start = (buf[i] == '\n');
      }
    }
    sys_close(fd);

    if (n < 0 && !saw_data)
      count = -1;
  } else {
    count = -1;
  }

  if (count < 0)
    count = pci_read_count_from_ctl();

  family_free(buf);

  if (count < 0)
    return -1;

  ctx->device_registry.device_count = count;
  ctx->topology.total_devices = count;
  ctx->stats.devices_discovered = count;
  return count;
}

int pcifamily_device_count(void) {
  if (global_pci_ctx == nil)
    return 0;
  return global_pci_ctx->device_registry.device_count;
}

/*
 * Family Ops
 */

static int pci_family_init_op(struct FamilyExchangePage *family) {
  struct PCIFamilyContext *ctx;

  ctx = family_alloc_zero(sizeof(struct PCIFamilyContext));
  if (!ctx)
    return FAMILY_ENOMEM;

  /* Initialize registry lock */
  // lock_init(&ctx->device_registry.device_registry_lock);

  family->family_specific_ctx = ctx;
  global_pci_ctx = ctx;
  global_pci_family = family;

  /* Set capabilities */
  family->capabilities_mask =
      FAMILY_CAP_MULTIPLE_CHANNELS | FAMILY_CAP_TRANSACTIONS;

  /* Initialize resource pool (using our generic helper or custom) */
  ctx->resource_pool =
      resource_pool_create(MAX_PCI_DEVICES * 6); // Example size

  if (pcifamily_refresh() >= 0)
    print("HAL: PCI family discovered %d devices\n", pcifamily_device_count());
  else
    print("HAL: PCI family discovery unavailable\n");

  return FAMILY_OK;
}

static int pci_family_shutdown_op(struct FamilyExchangePage *family) {
  struct PCIFamilyContext *ctx = family->family_specific_ctx;
  if (ctx) {
    resource_pool_destroy(ctx->resource_pool);
    family_free(ctx);
  }
  if (global_pci_ctx == ctx)
    global_pci_ctx = nil;
  if (global_pci_family == family)
    global_pci_family = nil;
  return FAMILY_OK;
}

struct FamilyOps pci_ops = {
    .init = pci_family_init_op,
    .shutdown = pci_family_shutdown_op,
    // .allocate_channel = pci_allocate_channel, // TODO
};

/*
 * Public Init
 */
int pcifamily_init(void) {
  return family_register(FAMILY_PCI, &pci_ops, "PCI");
}
