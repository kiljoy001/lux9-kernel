/*
 * PCI Family Implementation - HAL Userspace
 */

#include "family_pci.h"

struct FamilyExchangePage *global_pci_family = nil;
struct PCIFamilyContext *global_pci_ctx = nil;

/*
 * Configuration Space Access Stubs
 * In a real implementation, these would interact with /dev/pci or similar.
 */
int pci_config_read32(u8int bus, u8int dev, u8int func, u8int offset,
                      u32int *data) {
  // STUB: Read from /dev/pci or similar
  // For now, return error or mock values
  return -1;
}

int pci_config_read16(u8int bus, u8int dev, u8int func, u8int offset,
                      u16int *data) {
  return -1;
}

int pci_config_read8(u8int bus, u8int dev, u8int func, u8int offset,
                     u8int *data) {
  return -1;
}

/*
 * Family Ops
 */

static int pci_family_init_op(struct FamilyExchangePage *family) {
  struct PCIFamilyContext *ctx;

  ctx = malloc(sizeof(struct PCIFamilyContext));
  if (!ctx)
    return FAMILY_ENOMEM;

  memset(ctx, 0, sizeof(struct PCIFamilyContext));

  /* Initialize registry lock */
  // lock_init(&ctx->device_registry.device_registry_lock);

  family->family_specific_ctx = ctx;

  /* Set capabilities */
  family->capabilities_mask =
      FAMILY_CAP_MULTIPLE_CHANNELS | FAMILY_CAP_TRANSACTIONS;

  /* Initialize resource pool (using our generic helper or custom) */
  ctx->resource_pool =
      resource_pool_create(MAX_PCI_DEVICES * 6); // Example size

  return FAMILY_OK;
}

static int pci_family_shutdown_op(struct FamilyExchangePage *family) {
  struct PCIFamilyContext *ctx = family->family_specific_ctx;
  if (ctx) {
    resource_pool_destroy(ctx->resource_pool);
    free(ctx);
  }
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
