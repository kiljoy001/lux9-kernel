/*
 * PCI Family Implementation - Core PCI Device Family
 *
 * Implements the PCI-specific family operations for the PCI device family
 * interface. Handles PCI device discovery, configuration, BAR mapping, and
 * coordination. Supports traditional PCI and PCIe devices with proper
 * transaction support.
 */

#include "dat.h"
#include "exchange.h"
#include "family/family.h"
#include "family/pci_9p.h"
#include "family/pci_family_ops.h"
#include "fns.h"
#include "mem.h"
#include "pageown.h"
#include "pebble.h"
#include "portlib.h"
#include "u.h"
#include <error.h>

/* Simple isspace replacement - no glibc dependency */
#define isspace(c) ((c) == ' ' || (c) == '\t' || (c) == '\n' || (c) == '\r')

/* Forward declarations */
static struct PCIChannel *lookup_pci_channel(struct FamilyExchangePage *family,
                                             uint64_t channel_id);
extern struct PCIFamilyOps pci_family_ops;

/* Global PCI family context */
struct PCIFamilyContext *global_pci_ctx = NULL;
struct FamilyExchangePage *global_pci_family = NULL;

/* PCI standard constants */
#define PCI_CONFIG_ADDRESS_PORT 0xCF8
#define PCI_CONFIG_DATA_PORT 0xCFC

/* PCI command register bits */
#define PCI_COMMAND_IO_SPACE (1 << 0)
#define PCI_COMMAND_MEMORY_SPACE (1 << 1)
#define PCI_COMMAND_BUS_MASTER (1 << 2)
#define PCI_COMMAND_SPECIAL_CYCLES (1 << 3)
#define PCI_COMMAND_MEMORY_WRITE_INVALIDATE (1 << 4)
#define PCI_COMMAND_VGA_PALETTE_SNOOP (1 << 5)
#define PCI_COMMAND_PARITY_ERROR_RESPONSE (1 << 6)
#define PCI_COMMAND_SERR_ENABLE (1 << 8)
#define PCI_COMMAND_BACK_TO_BACK_WRITE (1 << 9)

/* PCI status register bits */
#define PCI_STATUS_CAPABILITY_LIST (1 << 4)
#define PCI_STATUS_66MHZ_CAPABLE (1 << 5)

/* PCI capability IDs */
#define PCI_CAP_ID_PM 0x01      /* Power Management */
#define PCI_CAP_ID_MSI 0x05     /* Message Signaled Interrupts */
#define PCI_CAP_ID_EXPRESS 0x10 /* Express capability */
#define PCI_CAP_ID_MSIX 0x11    /* MSI-X */

/* Helper function to construct PCI address key */
static inline uint32_t pci_address_key(uint16_t domain, uint8_t bus,
                                       uint8_t device, uint8_t function) {
  return (domain << 8) | (bus << 0) | (device << 3) | (function << 0);
}

/* Parse PCI address string like "0000:00:1f.2" */
struct PCIAddress parse_pci_address_string(const char *address_str) {
  struct PCIAddress addr = {0};
  uint16_t domain;
  uint8_t bus, device, function;
  char *parse_end;

  if (!address_str || *address_str == '\0')
    return addr;

  /* Try to parse domain:bus:device.function format (e.g., "0000:00:1f.2") */
  if (strstr(address_str, ":")) {
    domain = strtol(address_str, &parse_end, 16);
    if (*parse_end != ':')
      return addr;
    parse_end++;

    bus = strtol(parse_end, &parse_end, 16);
    if (*parse_end != ':')
      return addr;
    parse_end++;

    device = strtol(parse_end, &parse_end, 16);
    if (*parse_end != '.')
      return addr;
    parse_end++;

    function = strtol(parse_end, &parse_end, 16);
    if (*parse_end != '\0' && !isspace(*parse_end))
      return addr;
  } else {
    /* Legacy format without domain */
    domain = 0;
    bus = strtol(address_str, &parse_end, 16);
    if (*parse_end != ':')
      return addr;
    parse_end++;

    device = strtol(parse_end, &parse_end, 16);
    if (*parse_end != '.')
      return addr;
    parse_end++;

    function = strtol(parse_end, &parse_end, 16);
    if (*parse_end != '\0' && !isspace(*parse_end))
      return addr;
  }

  /* Validate ranges */
  if (domain >= 256 || bus >= 256 || device >= 32 || function >= 8)
    return addr;

  addr.domain = domain;
  addr.bus = bus;
  addr.device = device;
  addr.function = function;
  addr.domain_active = 1;
  addr.domain_bus_dev_func = pci_address_key(domain, bus, device, function);

  return addr;
}

/* Format PCI address back to string */
char *format_pci_address(struct PCIAddress *addr) {
  static char buffer[32]; // Static buffer (thread-local if needed)

  if (!addr || !addr->domain_active) {
    snprint(buffer, sizeof(buffer), "invalid");
    return buffer;
  }

  snprint(buffer, sizeof(buffer), "%04x:%02x:%02x.%x", addr->domain, addr->bus,
          addr->device, addr->function);

  return buffer;
}

/* Basic PCI configuration space access functions */
/* Configuration space access helpers (prototypes; implemented elsewhere) */
int pci_config_read8(uint8_t bus, uint8_t dev, uint8_t func, uint8_t offset,
                     uint8_t *data);
int pci_config_read16(uint8_t bus, uint8_t dev, uint8_t func, uint8_t offset,
                      uint16_t *data);
int pci_config_read32(uint8_t bus, uint8_t dev, uint8_t func, uint8_t offset,
                      uint32_t *data);
int pci_config_write8(uint8_t bus, uint8_t dev, uint8_t func, uint8_t offset,
                      uint8_t data);
int pci_config_write16(uint8_t bus, uint8_t dev, uint8_t func, uint8_t offset,
                       uint16_t data);
int pci_config_write32(uint8_t bus, uint8_t dev, uint8_t func, uint8_t offset,
                       uint32_t data);

/* Helper prototypes; implementations provided later or in other units */
void setup_pci_channel_manager(struct FamilyExchangePage *family);
void setup_pci_resource_pool(struct FamilyExchangePage *family);
void setup_pci_event_system(struct FamilyExchangePage *family);
void setup_pci_transaction_manager(struct FamilyExchangePage *family);
void update_pci_family_stats(struct FamilyExchangePage *family);
int notify_pci_device_removed(struct FamilyExchangePage *family,
                              struct PCIDeviceDescriptor *dev);
uint32_t device_can_access_flags(struct PCIDeviceDescriptor *dev);
struct PCIChannel *allocate_pci_channel_struct(
    struct FamilyExchangePage *family, uint64_t channel_id,
    struct PCIDeviceDescriptor *dev, uint32_t permissions);
int validate_channel_operation_permission(void *proc,
                                          struct PCIChannel *channel);
void *current_process(void);
void cleanup_pci_channel_resources(struct PCIChannel *channel);
void free_pci_channel_struct(struct PCIChannel *channel);
struct PCIChannel *lookup_channel_by_id(struct FamilyExchangePage *family,
                                        uint64_t channel_id);
const char *device_state_to_string(enum DeviceState state);

/* Read PCI configuration space */
static int read_pci_config_space(struct PCIDeviceDescriptor *dev, void *buffer,
                                 size_t size) {
  uint8_t *buf = (uint8_t *)buffer;

  if (!dev || !buffer || size == 0 || size > PCI_CONFIG_SPACE_SIZE)
    return -1;

  /* Read standard configuration space */
  for (size_t offset = 0; offset < size; offset += 4) {
    uint32_t word;
    int result = pci_config_read32(dev->address.bus, dev->address.device,
                                   dev->address.function, offset, &word);
    if (result != 0) {
      return result;
    }

    /* Handle partial word at end */
    if (offset + 4 <= size) {
      *((uint32_t *)(buf + offset)) = word;
    } else {
      uint8_t remaining = size - offset;
      for (int i = 0; i < remaining; i++) {
        buf[offset + i] = (word >> (i * 8)) & 0xFF;
      }
    }
  }

  return 0;
}

/* Write PCI configuration space */
static int write_pci_config_space(struct PCIDeviceDescriptor *dev, void *buffer,
                                  size_t size) {
  uint8_t *buf = (uint8_t *)buffer;

  if (!dev || !buffer || size == 0 || size > PCI_CONFIG_SPACE_SIZE)
    return -1;

  /* Write standard configuration space */
  for (size_t offset = 0; offset < size; offset += 4) {
    uint32_t word;

    /* Handle partial word at end */
    if (offset + 4 <= size) {
      word = *((uint32_t *)(buf + offset));
    } else {
      uint8_t remaining = size - offset;
      uint32_t old_word;

      /* Read-modify-write partial dword */
      int result = pci_config_read32(dev->address.bus, dev->address.device,
                                     dev->address.function, offset, &old_word);
      if (result != 0) {
        return result;
      }

      word = old_word;
      for (int i = 0; i < remaining; i++) {
        word &= ~(0xFF << (i * 8));
        word |= (buf[offset + i] << (i * 8));
      }
    }

    int result = pci_config_write32(dev->address.bus, dev->address.device,
                                    dev->address.function, offset, word);
    if (result != 0) {
      return result;
    }
  }

  return 0;
}

/* Scan a single PCI bus */
static int scan_pci_bus(struct PCIFamilyContext *ctx, uint16_t bus) {
  int device_count = 0;

  if (!ctx || bus >= 256)
    return -1;

  for (uint8_t device = 0; device < 32; device++) {
    for (uint8_t function = 0; function < 8; function++) {
      uint16_t vendor_id;
      uint32_t device_id;

      /* Read vendor ID */
      if (pci_config_read16(bus, device, function, 0x00, &vendor_id) != 0)
        continue;

      /* Check if device exists */
      if (vendor_id == 0xFFFF)
        continue; // No device at this function

      /* Read device ID */
      if (pci_config_read16(bus, device, function, 0x02, &device_id) != 0)
        continue;

      /* Create device descriptor */
      struct PCIDeviceDescriptor *dev =
          malloc(sizeof(struct PCIDeviceDescriptor));
      if (!dev)
        break;

      memset(dev, 0, sizeof(struct PCIDeviceDescriptor));
      dev->address.domain = 0; // Assume domain 0 for now
      dev->address.bus = bus;
      dev->address.device = device;
      dev->address.function = function;
      dev->address.domain_active = 1;
      dev->address.domain_bus_dev_func =
          pci_address_key(0, bus, device, function);

      dev->vendor_id = vendor_id;
      dev->device_id = device_id;

      /* Read basic device information */
      if (pci_config_read8(bus, device, function, 0x08, &dev->class_code) !=
              0 ||
          pci_config_read8(bus, device, function, 0x09, &dev->subclass_code) !=
              0 ||
          pci_config_read8(bus, device, function, 0x3A, &dev->revision) != 0 ||
          pci_config_read8(bus, device, function, 0x3B, &dev->prog_if) != 0) {
        free(dev);
        continue;
      }

      /* Read BARs */
      for (int bar_num = 0; bar_num < 6; bar_num++) {
        uint32_t bar_addr;

        if (pci_config_read32(bus, device, function, 0x10 + (bar_num * 4),
                              &bar_addr) != 0) {
          continue;
        }

        if (bar_addr == 0) {
          dev->bars[bar_num].is_valid = 0;
          continue;
        }

        dev->bars[bar_num].base_address = bar_addr;
        dev->bars[bar_num].is_valid = 1;
        dev->bars[bar_num].is_64bit = (bar_addr & 0x04) != 0;
        dev->bars[bar_num].type = (bar_addr & 0x01) ? 1 : 0; // 0=memory, 1=I/O

        if (dev->bars[bar_num].type == 0) { // Memory BAR
          uint32_t size_bits = (bar_addr >> 2) & 0x3F;
          if (size_bits == 0) {
            dev->bars[bar_num].size = 0; // Invalid size
          } else {
            dev->bars[bar_num].size = (1ULL << size_bits);
          }
        }
      }

      /* Add to registry */
      lock(&ctx->device_registry.device_registry_lock);

      uint32_t key = dev->address.domain_bus_dev_func;
      if (ctx->device_registry.device_count < MAX_PCI_DEVICES) {
        ctx->device_registry.devices[key] = dev;
        ctx->device_registry.device_count++;
      }

      ctx->topology.total_devices++;

      unlock(&ctx->device_registry.device_registry_lock);

      device_count++;

      /* Update statistics */
      ctx->stats.devices_discovered++;

      print("PCI: found device %04x:%02x:%02x.%x (vendor=%04x device=%04x "
            "class=%02x.%02x)\n",
            dev->address.domain, dev->address.bus, dev->address.device,
            dev->address.function, dev->vendor_id, dev->device_id,
            dev->class_code, dev->subclass_code);
    }
  }

  return device_count;
}

/* Initialize PCI family system */
int pcifamily_init(void) {
  if (global_pci_family) {
    print("PCI: family already initialized\n");
    return 0;
  }

  /* Allocate PCI family context */
  global_pci_ctx = xalloc(sizeof(struct PCIFamilyContext));
  if (!global_pci_ctx) {
    print("PCI: failed to allocate PCI context\n");
    return -1;
  }

  memset(global_pci_ctx, 0, sizeof(struct PCIFamilyContext));

  /* Initialize device registry */
  global_pci_ctx->topology.primary_bus = 0;
  global_pci_ctx->topology.max_bus = 0xFF; /* Start with max bus value */

  /* Scan primary domain */
  print("PCI: scanning primary domain\n");
  int devices_found = scan_pci_bus(global_pci_ctx, 0);
  print("PCI: found %d devices on primary domain\n", devices_found);

  /* Register PCI family with family system */
  if (family_register(FAMILY_PCI, &pci_family_ops.base, "PCI") != 0) {
    print("PCI: failed to register PCI family\n");
    xfree(global_pci_ctx);
    global_pci_ctx = NULL;
    return -1;
  }

  global_pci_family = family_lookup(FAMILY_PCI);
  if (!global_pci_family) {
    print("PCI: failed to lookup registered PCI family\n");
    return -1;
  }

  /* Set PCI family specific context */
  global_pci_family->family_specific_ctx = global_pci_ctx;
  global_pci_family->max_channels = 1048576; /* 1M channels with 64-bit IDs */

  print("PCI: PCI family initialized successfully\n");
  return 0;
}

/* Initialize PCI family instance */
static int pci_family_init(struct FamilyExchangePage *family) {
  if (!family) {
    return -1;
  }

  /* Initialize family-specific contexts */
  struct PCIFamilyContext *ctx =
      (struct PCIFamilyContext *)family->family_specific_ctx;
  if (!ctx && global_pci_ctx) {
    family->family_specific_ctx = global_pci_ctx;
    ctx = global_pci_ctx;
  }
  if (!ctx)
    return -1;

  /* Set family type and capabilities */
  family->family_type = FAMILY_PCI;
  family->capabilities_mask = FAMILY_CAP_MULTIPLE_CHANNELS |
                              FAMILY_CAP_TRANSACTIONS | FAMILY_CAP_HOT_PLUG |
                              FAMILY_CAP_EVENTS | FAMILY_CAP_MULTI_DEVICE |
                              FAMILY_CAP_PERSISTENT_NAMES;

  setup_pci_channel_manager(family);
  setup_pci_resource_pool(family);
  setup_pci_event_system(family);
  setup_pci_transaction_manager(family);

  /* Initial device count update */
  lock(&family->family_lock);
  family->stats.peak_channels = 0;
  family->stats.active_channels = 0;
  family->stats.total_channels_created = 0;
  unlock(&family->family_lock);

  print("PCI: family instance initialized\n");
  return 0;
}

/* Shutdown PCI family */
static int pci_family_shutdown(struct FamilyExchangePage *family) {
  if (!family)
    return 0;

  print("PCI: shutting down PCI family\n");

  /* Cleanup all channels */
  struct PCIFamilyContext *pci_ctx =
      (struct PCIFamilyContext *)family->family_specific_ctx;
  if (pci_ctx) {
    /* Iterate all devices and release their channels */
    for (int i = 0; i < MAX_PCI_DEVICES; i++) {
      struct PCIDeviceDescriptor *dev = pci_ctx->device_registry.devices[i];
      if (dev && dev->bound_channel_id != 0) {
        pci_release_channel(family, dev->bound_channel_id);
      }
    }
  }

  /* Cleanup device registry */
  struct PCIFamilyContext *ctx =
      (struct PCIFamilyContext *)family->family_specific_ctx;
  if (ctx) {
    for (int i = 0; i < MAX_PCI_DEVICES; i++) {
      if (ctx->device_registry.devices[i]) {
        xfree(ctx->device_registry.devices[i]);
        ctx->device_registry.devices[i] = NULL;
      }
    }
    xfree(ctx);
    family->family_specific_ctx = NULL;
  }

  return 0;
}

/* Scan PCI for devices */
static int pci_scan_devices(struct FamilyExchangePage *family) {
  struct PCIFamilyContext *ctx =
      (struct PCIFamilyContext *)family->family_specific_ctx;
  int total_devices_found = 0;

  if (!ctx) {
    return -1;
  }

  lock(&ctx->device_registry.device_registry_lock);

  /* Scan all buses up to max bus */
  for (uint8_t bus = 0; bus <= ctx->topology.max_bus; bus++) {
    int devices_on_bus = scan_pci_bus(ctx, bus);
    if (devices_on_bus > 0) {
      total_devices_found += devices_on_bus;
    }
  }

  unlock(&ctx->device_registry.device_registry_lock);

  update_pci_family_stats(family);

  print("PCI: total devices found: %d\n", total_devices_found);
  return total_devices_found;
}

/* Discover device by address */
static int pci_discover_device(struct FamilyExchangePage *family,
                               void *device_id, void **device_out) {
  struct PCIFamilyContext *ctx =
      (struct PCIFamilyContext *)family->family_specific_ctx;
  struct PCIAddress *addr = (struct PCIAddress *)device_id;

  if (!ctx || !addr || !device_out) {
    return -1;
  }

  /* Find device in registry */
  uint32_t key = addr->domain_bus_dev_func;
  struct PCIDeviceDescriptor *dev = ctx->device_registry.devices[key];

  if (!dev || !dev->address.domain_active) {
    return -2; // Device not found
  }

  *device_out = dev;
  return 0;
}

/* Remove device from registry */
static int pci_remove_device(struct FamilyExchangePage *family,
                             void *device_id) {
  struct PCIFamilyContext *ctx =
      (struct PCIFamilyContext *)family->family_specific_ctx;
  struct PCIAddress *addr = (struct PCIAddress *)device_id;

  if (!ctx || !addr) {
    return -1;
  }

  uint32_t key = addr->domain_bus_dev_func;
  struct PCIDeviceDescriptor *dev = ctx->device_registry.devices[key];

  if (!dev) {
    return -2; // Device not found
  }

  /* If device has active channel, release it first */
  if (dev->bound_channel_id != 0) {
    pci_release_channel(family, dev->bound_channel_id);
  }

  lock(&ctx->device_registry.device_registry_lock);

  xfree(dev);
  ctx->device_registry.devices[key] = NULL;
  ctx->topology.total_devices--;

  unlock(&ctx->device_registry.device_registry_lock);

  /* Notify interested parties about device removal */
  notify_pci_device_removed(family, dev);

  return 0;
}

/* Allocate channel for PCI device */
static int pci_allocate_channel(struct FamilyExchangePage *family,
                                void *device_id, uint32_t permissions,
                                uint64_t *channel_id) {
  struct PCIFamilyContext *ctx =
      (struct PCIFamilyContext *)family->family_specific_ctx;
  struct PCIAddress *addr = (struct PCIAddress *)device_id;
  struct PCIDeviceDescriptor *dev;

  if (!ctx || !addr || !channel_id) {
    return -1;
  }

  /* Find device */
  uint32_t key = addr->domain_bus_dev_func;
  dev = ctx->device_registry.devices[key];

  if (!dev || !dev->address.domain_active) {
    return -2; // Device not found
  }

  /* Check if device is available */
  if (dev->bound_channel_id != 0) {
    return -3; // Already bound to channel
  }

  /* Validate permissions */
  if (!validate_channel_permissions(permissions,
                                    device_can_access_flags(dev))) {
    return -4; // Permission denied
  }

  /* Allocate new channel */
  uint64_t new_channel_id = generate_channel_id();
  struct PCIChannel *channel =
      allocate_pci_channel_struct(family, new_channel_id, dev, permissions);

  if (!channel) {
    return -5; // Allocation failed
  }

  /* Bind device to channel */
  dev->bound_channel_id = new_channel_id;
  dev->bound_channel = channel;

  *channel_id = new_channel_id;

  /* Update statistics */
  lock(&family->family_lock);
  family->stats.total_channels_created++;
  family->stats.active_channels++;
  if (family->stats.active_channels > family->stats.peak_channels) {
    family->stats.peak_channels = family->stats.active_channels;
  }
  unlock(&family->family_lock);

  print("PCI: allocated channel %d for device %04x:%02x:%02x.%x\n",
        new_channel_id, addr->domain, addr->bus, addr->device, addr->function);

  return 0;
}

/* Release PCI channel */
int pci_release_channel(struct FamilyExchangePage *family,
                        uint64_t channel_id) {
  struct PCIFamilyContext *ctx =
      (struct PCIFamilyContext *)family->family_specific_ctx;
  struct PCIChannel *channel = lookup_pci_channel(family, channel_id);

  if (!channel) {
    return -2; // Channel not found
  }

  struct PCIDeviceDescriptor *dev = channel->bound_device;
  if (!dev) {
    return -3; // Invalid channel state
  }

  /* Check permissions */
  if (!validate_channel_operation_permission(current_process(), channel)) {
    return -4; // Permission denied
  }

  /* Clean up channel resources */
  cleanup_pci_channel_resources(channel);

  /* Unbind device */
  dev->bound_channel_id = 0;
  dev->bound_channel = NULL;

  /* Free channel */
  free_pci_channel_struct(channel);

  /* Update statistics */
  lock(&family->family_lock);
  family->stats.active_channels--;
  unlock(&family->family_lock);

  print("PCI: released channel %d\n", channel_id);

  return 0;
}

/* Look up channel by ID */
static int pci_lookup_channel(struct FamilyExchangePage *family,
                              uint64_t channel_id, void **channel) {
  if (!family || !channel) {
    return -1;
  }

  struct PCIChannel *pci_channel =
      (struct PCIChannel *)lookup_channel_by_id(family, channel_id);
  if (!pci_channel) {
    return -2;
  }

  *channel = pci_channel;
  return 0;
}

/* Enable PCI device */
static int pci_device_enable(void *device_void, void *channel_void) {
  struct PCIDeviceDescriptor *dev = (struct PCIDeviceDescriptor *)device_void;
  struct PCIChannel *channel = (struct PCIChannel *)channel_void;
  struct FamilyExchangePage *family = channel->channel_manager->family;

  if (!dev || !channel) {
    return -1;
  }

  /* Enable bus master and memory space */
  uint16_t command;
  if (pci_config_read16(dev->address.bus, dev->address.device,
                        dev->address.function, 0x04, &command) != 0) {
    return -2;
  }

  command |= PCI_COMMAND_MEMORY_SPACE | PCI_COMMAND_BUS_MASTER;
  if (pci_config_write16(dev->address.bus, dev->address.device,
                         dev->address.function, 0x04, command) != 0) {
    return -3;
  }

  dev->device_state = DEVICE_ACTIVE;

  /* Update statistics */
  struct PCIFamilyContext *ctx =
      (struct PCIFamilyContext *)family->family_specific_ctx;
  lock(&ctx->device_registry.device_registry_lock);
  ctx->stats.devices_enabled++;
  unlock(&ctx->device_registry.device_registry_lock);

  print("PCI: enabled device %04x:%02x:%02x.%x\n", dev->address.domain,
        dev->address.bus, dev->address.device, dev->address.function);

  return 0;
}

/* Disable PCI device */
static int pci_device_disable(void *device_void, void *channel_void) {
  struct PCIDeviceDescriptor *dev = (struct PCIDeviceDescriptor *)device_void;
  struct PCIChannel *channel = (struct PCIChannel *)channel_void;

  if (!dev || !channel) {
    return -1;
  }

  /* Disable device by clearing command register bits */
  uint16_t command;
  if (pci_config_read16(dev->address.bus, dev->address.device,
                        dev->address.function, 0x04, &command) != 0) {
    return -2;
  }

  command &= ~(PCI_COMMAND_MEMORY_SPACE | PCI_COMMAND_BUS_MASTER);
  if (pci_config_write16(dev->address.bus, dev->address.device,
                         dev->address.function, 0x04, command) != 0) {
    return -3;
  }

  dev->device_state = DEVICE_SUSPENDED;

  /* Update statistics */
  struct PCIFamilyContext *ctx =
      (struct PCIFamilyContext *)
          channel->channel_manager->family->family_specific_ctx;
  lock(&ctx->device_registry.device_registry_lock);
  ctx->stats.devices_enabled--,
      unlock(&ctx->device_registry.device_registry_lock);

  print("PCI: disabled device %04x:%02x:%02x.%x\n", dev->address.domain,
        dev->address.bus, dev->address.device, dev->address.function);

  return 0;
}

/* Get device information */
static int pci_get_device_info(void *device_void, void *buffer, size_t *size) {
  struct PCIDeviceDescriptor *dev = (struct PCIDeviceDescriptor *)device_void;
  char *buf = (char *)buffer;
  size_t used = 0;

  if (!dev || !buffer || !size)
    return -1;

  /* Build device information string */
  used += snprint(buf + used, *size - used, "address:%04x:%02x:%02x.%x\n",
                  dev->address.domain, dev->address.bus, dev->address.device,
                  dev->address.function);

  used += snprint(buf + used, *size - used, "vendor:%04x device:%04x\n",
                  dev->vendor_id, dev->device_id);

  used += snprint(buf + used, *size - used, "class:%02x.%02x revision:%d\n",
                  dev->class_code, dev->subclass_code, dev->revision);

  used += snprint(buf + used, *size - used, "state:%s\n",
                  device_state_to_string(dev->device_state));

  used += snprint(buf + used, *size - used, "bound_channel:%d\n",
                  dev->bound_channel_id);

  *size = used;
  return 0;
}

/* Get family capabilities */
static uint32_t pci_get_capabilities(struct FamilyExchangePage *family) {
  struct PCIFamilyContext *ctx =
      (struct PCIFamilyContext *)family->family_specific_ctx;
  uint32_t caps = 0;

  /* Check for PCIe support */
  caps |= FAMILY_CAP_MULTIPLE_CHANNELS | FAMILY_CAP_TRANSACTIONS |
          FAMILY_CAP_HOT_PLUG;

  /* Check capabilities from device enumeration */
  for (int i = 0; i < ctx->device_registry.device_count; i++) {
    if (i >= MAX_PCI_DEVICES)
      break;

    struct PCIDeviceDescriptor *dev = NULL;
    for (int j = 0; j < MAX_PCI_DEVICES; j++) {
      if (ctx->device_registry.devices[j] &&
          ctx->device_registry.devices[j]->address.domain_bus_dev_func == i) {
        dev = ctx->device_registry.devices[j];
        break;
      }
    }

    if (!dev)
      continue;

    if (dev->capabilities.has_pcie)
      caps |= PCIFAMILY_CAP_PCIE_SUPPORT;
    if (dev->capabilities.has_msi)
      caps |= PCIFAMILY_CAP_MSI_SUPPORT;
    if (dev->capabilities.has_msix)
      caps |= PCIFAMILY_CAP_MSIX_SUPPORT;
  }

  return caps;
}

/* Full implementations for missing functions */
static int pci_family_suspend(struct FamilyExchangePage *family) {
  struct PCIFamilyContext *ctx =
      (struct PCIFamilyContext *)family->family_specific_ctx;
  if (!ctx) {
    return -1;
  }

  lock(&ctx->device_registry.device_registry_lock);

  /* Save state of all active devices */
  for (int i = 0; i < MAX_PCI_DEVICES; i++) {
    struct PCIDeviceDescriptor *dev = ctx->device_registry.devices[i];
    if (!dev || !dev->address.domain_active) {
      continue;
    }

    /* Save device power state */
    uint8_t power_state;
    pci_config_read8(dev->address.bus, dev->address.device,
                     dev->address.function, 0x44,
                     &power_state); /* PM cap offset */
    dev->power_state.current_power_state = power_state;
    dev->power_state.power_state_timestamp = fastticks(nil);

    /* Put device into D3 (low power) state if supported */
    if (dev->capabilities.has_pm) {
      pci_config_write8(dev->address.bus, dev->address.device,
                        dev->address.function, 0x44, 3); /* D3 state */
    }
  }

  unlock(&ctx->device_registry.device_registry_lock);

  print("PCI: family suspended (%d devices)\n", ctx->topology.total_devices);
  return 0;
}

static int pci_family_resume(struct FamilyExchangePage *family) {
  struct PCIFamilyContext *ctx =
      (struct PCIFamilyContext *)family->family_specific_ctx;
  if (!ctx) {
    return -1;
  }

  lock(&ctx->device_registry.device_registry_lock);

  /* Restore state of all active devices */
  for (int i = 0; i < MAX_PCI_DEVICES; i++) {
    struct PCIDeviceDescriptor *dev = ctx->device_registry.devices[i];
    if (!dev || !dev->address.domain_active) {
      continue;
    }

    /* Restore device to previous power state */
    if (dev->capabilities.has_pm) {
      pci_config_write8(dev->address.bus, dev->address.device,
                        dev->address.function, 0x44,
                        dev->power_state.current_power_state);
    }
  }

  unlock(&ctx->device_registry.device_registry_lock);

  print("PCI: family resumed (%d devices)\n", ctx->topology.total_devices);
  return 0;
}

static int pci_begin_transaction(struct FamilyExchangePage *family,
                                 void *context) {
  if (!family || !context) {
    return -1;
  }

  struct PCIChannel *channel = (struct PCIChannel *)context;
  struct PCIDeviceDescriptor *dev = channel->bound_device;
  if (!dev) {
    return -1;
  }

  /* Check if transaction already in progress */
  if (channel->transaction_state.config_transaction_active) {
    print("PCI: transaction already active on channel %llux\n",
          channel->channel_id);
    return -1;
  }

  /* Mark transaction as active and assign ID */
  channel->transaction_state.config_transaction_active = true;
  channel->transaction_state.current_transaction_id = fastticks(nil);

  /* Lock config space for this device */
  struct PCIFamilyContext *ctx =
      (struct PCIFamilyContext *)family->family_specific_ctx;
  if (ctx && ctx->config_ops.lock_config_space) {
    if (ctx->config_ops.lock_config_space(dev->address.bus, dev->address.device,
                                          dev->address.function) != 0) {
      channel->transaction_state.config_transaction_active = false;
      return -1;
    }
  }

  return 0;
}

static int pci_commit_transaction(struct FamilyExchangePage *family,
                                  void *context) {
  if (!family || !context) {
    return -1;
  }

  struct PCIChannel *channel = (struct PCIChannel *)context;
  struct PCIDeviceDescriptor *dev = channel->bound_device;
  if (!dev) {
    return -1;
  }

  /* Check if transaction is active */
  if (!channel->transaction_state.config_transaction_active) {
    print("PCI: no active transaction to commit on channel %llux\n",
          channel->channel_id);
    return -1;
  }

  /* Unlock config space */
  struct PCIFamilyContext *ctx =
      (struct PCIFamilyContext *)family->family_specific_ctx;
  if (ctx && ctx->config_ops.unlock_config_space) {
    ctx->config_ops.unlock_config_space(dev->address.bus, dev->address.device,
                                        dev->address.function);
  }

  /* Clear transaction state */
  channel->transaction_state.config_transaction_active = false;
  channel->transaction_state.current_transaction_id = 0;

  /* Update device stats */
  dev->last_access_time = fastticks(nil);
  dev->access_count++;

  return 0;
}

static int pci_rollback_transaction(struct FamilyExchangePage *family,
                                    void *context) {
  if (!family || !context) {
    return -1;
  }

  struct PCIChannel *channel = (struct PCIChannel *)context;
  struct PCIDeviceDescriptor *dev = channel->bound_device;
  if (!dev) {
    return -1;
  }

  /* Check if transaction is active */
  if (!channel->transaction_state.config_transaction_active) {
    print("PCI: no active transaction to rollback on channel %llux\n",
          channel->channel_id);
    return -1;
  }

  /* Unlock config space without applying changes */
  struct PCIFamilyContext *ctx =
      (struct PCIFamilyContext *)family->family_specific_ctx;
  if (ctx && ctx->config_ops.unlock_config_space) {
    ctx->config_ops.unlock_config_space(dev->address.bus, dev->address.device,
                                        dev->address.function);
  }

  /* Clear transaction state */
  channel->transaction_state.config_transaction_active = false;
  channel->transaction_state.current_transaction_id = 0;

  print("PCI: transaction rolled back on channel %llux\n", channel->channel_id);
  return 0;
}

static int pci_subscribe_events(struct FamilyExchangePage *family,
                                void *subscriber, uint32_t event_mask) {
  if (!family || !subscriber || event_mask == 0) {
    return -1;
  }

  struct PCIFamilyContext *ctx =
      (struct PCIFamilyContext *)family->family_specific_ctx;
  if (!ctx) {
    return -1;
  }

  /* For now, just validate and accept subscription
   * Event system integration would be added here */

  print("PCI: event subscription registered (mask=%ux)\n", event_mask);
  return 0;
}

static int pci_unsubscribe_events(struct FamilyExchangePage *family,
                                  void *subscriber) {
  if (!family || !subscriber) {
    return -1;
  }

  struct PCIFamilyContext *ctx =
      (struct PCIFamilyContext *)family->family_specific_ctx;
  if (!ctx) {
    return -1;
  }

  /* For now, just validate and accept unsubscription
   * Event system integration would be added here */

  print("PCI: event unsubscription processed\n");
  return 0;
}

static int pci_notify_event(struct FamilyExchangePage *family,
                            uint32_t event_type, void *event_data) {
  if (!family) {
    return -1;
  }

  struct PCIFamilyContext *ctx =
      (struct PCIFamilyContext *)family->family_specific_ctx;
  if (!ctx) {
    return -1;
  }

  /* For now, just validate the event
   * Full event notification to subscribers would be added here */

  if (event_type == 0) {
    return -1;
  }

  /* Events would be dispatched to subscribers here */
  return 0;
}

static int pci_configure_channel_limits(struct FamilyExchangePage *family,
                                        uint64_t channel_id, void *limits) {
  if (!family || !limits) {
    return -1;
  }

  struct PCIChannel *channel = lookup_pci_channel(family, channel_id);
  if (!channel) {
    print("PCI: channel %llux not found\n", channel_id);
    return -1;
  }

  /* Limits structure would define max BARs, IRQs, DMAs per channel
   * For now, validate that the channel exists and limits are provided */

  if (!channel->bound_device) {
    print("PCI: channel %llux not bound to device\n", channel_id);
    return -1;
  }

  /* Channel limits configuration would be applied here */
  print("PCI: channel limits configured for %llux\n", channel_id);
  return 0;
}

/* Helper to lookup PCI channel by ID */
static struct PCIChannel *lookup_pci_channel(struct FamilyExchangePage *family,
                                             uint64_t channel_id) {
  if (!family || !family->channel_mgr) {
    return NULL;
  }

  struct PCIChannelManager *mgr =
      (struct PCIChannelManager *)family->channel_mgr;

  lock(&mgr->channel_lock);

  /* Search through channel pool for matching ID */
  for (uint32_t i = 0; i < mgr->max_channels; i++) {
    struct PCIChannel *ch = &mgr->channel_pool[i];
    if (ch->channel_id == channel_id && ch->bound_device != NULL) {
      unlock(&mgr->channel_lock);
      return ch;
    }
  }

  unlock(&mgr->channel_lock);
  return NULL;
}

/* PCI family operation table */
struct PCIFamilyOps pci_family_ops = {
    .base =
        {
            /* Base family operations */
            .family_init = pci_family_init,
            .family_shutdown = pci_family_shutdown,
            .family_suspend = pci_family_suspend,
            .family_resume = pci_family_resume,

            /* 9P Interface Hooks */
            .walk = pci_9p_walk,
            .stat = pci_9p_stat,
            .open = pci_9p_open,
            .close = pci_9p_close,
            .read = pci_9p_read,
            .write = pci_9p_write,

            .scan_devices = pci_scan_devices,
            .discover_device = pci_discover_device,
            .remove_device = pci_remove_device,

            .allocate_channel = pci_allocate_channel,
            .release_channel = pci_release_channel,
            .lookup_channel = pci_lookup_channel,

            /* Device-specific operations */
            .device_enable = pci_device_enable,
            .device_disable = pci_device_disable,
            .device_get_info = pci_get_device_info,

            /* Multi-device coordination (implemented in pci_transaction.c) */
            .begin_transaction = pci_begin_transaction,
            .commit_transaction = pci_commit_transaction,
            .rollback_transaction = pci_rollback_transaction,

            /* Event notification (implemented in pci_events.c) */
            .subscribe_events = pci_subscribe_events,
            .unsubscribe_events = pci_unsubscribe_events,
            .notify_event = pci_notify_event,

            .get_capabilities = pci_get_capabilities,
            .configure_channel_limits = pci_configure_channel_limits,
        },
    /* PCI-specific extensions (hooked up to generic operations for now) */
    .enable_device = pci_device_enable,
    .disable_device = pci_device_disable,
    .get_device_info = NULL,
    .begin_config_read = NULL,
    .begin_config_write = NULL,
    .cancel_config_transaction = NULL,
    .map_bar = NULL,
    .unmap_bar = NULL,
    .get_bar_info = NULL,
    .set_power_state = NULL,
    .get_power_state = NULL,
    .allocate_irq = NULL,
    .release_irq = NULL,
    .enable_pcie_extended_config = NULL,
    .configure_msi = NULL,
    .configure_msix = NULL,
};

/* PCI channel manager implementation moved to pci_channel.c */

/* Initialize PCI family system (called from kernel startup) */
void pci_init(void) {
  if (pcifamily_init() != 0) {
    print("PCI: initialization failed\n");
    return;
  }

  print("PCI: PCI family system initialized\n");
}

/* Device class name helpers */
const char *pci_class_to_string(uint8_t class_code, uint8_t subclass) {
  switch (class_code) {
  case 0x01:
    switch (subclass) {
    case 0x00:
      return "PCI Storage";
    case 0x01:
      return "PCI IDE";
    case 0x06:
      return "PCI SATA";
    default:
      return "PCI Storage";
    }
  case 0x02:
    return "PCI Network";
  case 0x03:
    return "PCI Display";
  case 0x04:
    return "PCI Multimedia";
  default:
    return "PCI Unknown";
  }
}

bool pci_is_network_device(struct PCIDeviceDescriptor *dev) {
  return (dev->class_code == 0x02);
}

bool pci_is_storage_device(struct PCIDeviceDescriptor *dev) {
  return (dev->class_code == 0x01);
}

bool pci_is_display_device(struct PCIDeviceDescriptor *dev) {
  return (dev->class_code == 0x03);
}

/* Device state helpers */
const char *device_state_to_string(enum DeviceState state) {
  switch (state) {
  case DEVICE_INIT:
    return "INIT";
  case DEVICE_CONFIGURING:
    return "CONFIGURING";
  case DEVICE_ACTIVE:
    return "ACTIVE";
  case DEVICE_ERROR:
    return "ERROR";
  case DEVICE_SUSPENDED:
    return "SUSPENDED";
  case DEVICE_REMOVED:
    return "REMOVED";
  default:
    return "UNKNOWN";
  }
}
