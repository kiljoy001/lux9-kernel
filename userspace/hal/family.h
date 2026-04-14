/*
 * Family Interface - Device Family Abstraction Layer (Userspace HAL)
 *
 * Provides common interface for device families (PCI, USB, I2C, etc.)
 * Implements channel management, resource pooling, and coordination.
 */

#pragma once

#include <libc.h>
#include <u.h>

extern int pebble_alloc(ulong size, void **addr);
extern int pebble_free(void *addr);

static inline void *family_alloc_zero(ulong size) {
  void *p;

  if (pebble_alloc(size, &p) < 0)
    return nil;
  memset(p, 0, size);
  return p;
}

static inline void family_free(void *p) {
  if (p != nil)
    pebble_free(p);
}

/* Family type identifiers */
enum DeviceFamily {
  FAMILY_NONE = 0,
  FAMILY_PCI = 1,
  FAMILY_USB = 2,
  FAMILY_I2C = 3,
  FAMILY_SPI = 4,
  FAMILY_DMA = 5,
  FAMILY_IRQ = 6,
  FAMILY_SECURE_ELEMENT = 7,
  FAMILY_PROC = 8,
  FAMILY_MAX
};

/* Family capabilities flags */
#define FAMILY_CAP_MULTIPLE_CHANNELS (1 << 0)
#define FAMILY_CAP_TRANSACTIONS (1 << 1)
#define FAMILY_CAP_HOT_PLUG (1 << 2)
#define FAMILY_CAP_EVENTS (1 << 3)
#define FAMILY_CAP_MULTI_DEVICE (1 << 4)
#define FAMILY_CAP_PERSISTENT_NAMES (1 << 5)
#define FAMILY_CAP_CRYPTO (1 << 6)
#define FAMILY_CAP_RANDOM (1 << 7)
#define FAMILY_CAP_ATTESTATION (1 << 8)
#define FAMILY_CAP_SECURE_STORAGE (1 << 9)
#define DEFAULT_CHANNELS_PER_FAMILY 1048576UL

/* Channel states */
enum ChannelState {
  CHANNEL_INACTIVE = 0,
  CHANNEL_ACTIVE = 1,
  CHANNEL_ERROR = 2,
  CHANNEL_SUSPENDED = 3
};

/* Device state */
enum DeviceState {
  DEVICE_INIT = 0,
  DEVICE_CONFIGURING = 1,
  DEVICE_ACTIVE = 2,
  DEVICE_ERROR = 3,
  DEVICE_SUSPENDED = 4,
  DEVICE_REMOVED = 5
};

/* Error codes */
enum FamilyError {
  FAMILY_OK = 0,
  FAMILY_EINVAL = -1,
  FAMILY_ENOTFOUND = -2,
  FAMILY_EPERM = -3,
  FAMILY_EBUSY = -4,
  FAMILY_ENOMEM = -5,
  FAMILY_ETIMEDOUT = -6,
  FAMILY_ECONFLICT = -7,
  FAMILY_ETRANSACTION = -8,
  FAMILY_ECHANNEL_EXHAUSTED = -9,
  FAMILY_FAMILY_LIMIT_EXCEEDED = -10
};

/* Forward declarations */
typedef struct FamilyExchangePage FamilyExchangePage;
typedef struct ChannelManager ChannelManager;
typedef struct ResourcePool ResourcePool;
typedef struct FamilyOps FamilyOps;

/* Operations table for family implementations */
struct FamilyOps {
  int (*init)(FamilyExchangePage *family);
  int (*shutdown)(FamilyExchangePage *family);
  int (*allocate_channel)(FamilyExchangePage *family, void *device_id,
                          u32int permissions, u64int *channel_id);
  int (*release_channel)(FamilyExchangePage *family, u64int channel_id);
  int (*device_get_info)(FamilyExchangePage *family, void *device,
                         void *info_buffer, usize *info_size);
  u32int (*get_capabilities)(FamilyExchangePage *family);

  /* Optional 9P operations */
  // Walk, Stat, Open, Read, Write, Close moved to standard 9P server loop in
  // HAL
};

/* Base family structure */
struct FamilyExchangePage {
  int family_type;
  int family_version;
  char family_name[32];
  u32int capabilities_mask;

  void *family_specific_ctx;
  FamilyOps *ops;

  // Stats and state
  int state;
};

/* Public API */
int family_register(int type, FamilyOps *ops, char *name);
int family_unregister(int type);
FamilyExchangePage *family_lookup(int type);
void family_init_registry(void);
int family_registry_count(void);
int family_registry_snapshot(char *buf, int nbuf);
ResourcePool *resource_pool_create(int count);
void resource_pool_destroy(ResourcePool *p);
