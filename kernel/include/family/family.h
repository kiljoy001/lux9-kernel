/*
 * Family Interface - Device Family Abstraction Layer
 * 
 * Provides common interface for device families (PCI, USB, I2C, etc.)
 * Implements channel management, resource pooling, and coordination.
 * Serves as foundation for family-specific implementations.
 */

#pragma once

#include "dat.h"

/* Family type identifiers */
enum DeviceFamily {
    FAMILY_NONE = 0,
    FAMILY_PCI = 1,
    FAMILY_USB = 2,
    FAMILY_I2C = 3,
    FAMILY_SPI = 4,
    FAMILY_DMA = 5,
    FAMILY_IRQ = 6,
    FAMILY_MAX
};

/* Family capabilities flags */
#define FAMILY_CAP_MULTIPLE_CHANNELS    (1 << 0)
#define FAMILY_CAP_TRANSACTIONS         (1 << 1)
#define FAMILY_CAP_HOT_PLUG            (1 << 2)
#define FAMILY_CAP_EVENTS              (1 << 3)
#define FAMILY_CAP_MULTI_DEVICE        (1 << 4)
#define FAMILY_CAP_PERSISTENT_NAMES    (1 << 5)

/* Channel states */
enum ChannelState {
    CHANNEL_INACTIVE = 0,
    CHANNEL_ACTIVE = 1,
    CHANNEL_ERROR = 2,
    CHANNEL_SUSPENDED = 3
};

/* Channel permissions */
#define CHANNEL_PERM_READ_CONFIG       (1 << 0)
#define CHANNEL_PERM_WRITE_CONFIG      (1 << 1)
#define CHANNEL_PERM_MAP_BAR          (1 << 2)
#define CHANNEL_PERM_ALLOC_IRQ        (1 << 3)
#define CHANNEL_PERM_ALLOC_DMA        (1 << 4)
#define CHANNEL_PERM_ADMIN            (1 << 7)

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
    FAMILY_EINVAL = -1,          /* Invalid parameters */
    FAMILY_ENOTFOUND = -2,       /* Device not found */
    FAMILY_EPERM = -3,           /* Permission denied */
    FAMILY_EBUSY = -4,           /* Resource busy */
    FAMILY_ENOMEM = -5,          /* Out of memory */
    FAMILY_ETIMEDOUT = -6,       /* Operation timeout */
    FAMILY_ECONFLICT = -7,       /* Resource conflict */
    FAMILY_ETRANSACTION = -8,    /* Transaction failed */
    FAMILY_ECHANNEL_EXHAUSTED = -9,
    FAMILY_FAMILY_LIMIT_EXCEEDED = -10
};

/* Forward declarations */
struct FamilyExchangePage;
struct ChannelManager;
struct ResourcePool;
struct EventSystem;
struct TransactionManager;

// Opaque handle for pebble integration
typedef struct PebbleHandle PebbleHandle;

/* Base family exchange page structure */
struct FamilyExchangePage {
    /* Basic identification */
    enum DeviceFamily family_type;
    uint16_t family_version;
    char family_name[32];
    
    /* System integration */
    struct PebbleHandle* family_white_token;  /* Family-wide authentication */
    uint64_t family_chain_verifier;          /* Verify pebble chain integrity */
    
    /* Channel management */
    struct ChannelManager* channel_mgr;
    uint64_t next_channel_id;                 /* 64-bit channel IDs from start */
    uint32_t max_channels;                   /* Configurable limit */
    
    /* Resource coordination */
    struct ResourcePool* resource_pool;
    
    /* Event system */
    struct EventSystem* event_system;
    
    /* Transaction support */
    struct TransactionManager* tx_mgr;
    
    /* State and statistics */
    enum FamilyState {
        FAMILY_UNINITIALIZED = 0,
        FAMILY_INITIALIZING = 1,
        FAMILY_READY = 2,
        FAMILY_ERROR = 3
    } state;
    
    struct {
        uint64_t total_channels_created;
        uint64_t active_channels;
        uint64_t peak_channels;
        uint64_t total_operations;
        uint64_t error_count;
        uint64_t memory_usage_bytes;
    } stats;
    
    /* Locking */
    Lock family_lock;
    
    /* Family-specific operations (family implements these) */
    struct FamilyOps* ops;
    void* family_specific_ctx;                /* Family context (PCIFamilyContext, etc.) */
};

/* Base family operations interface */
struct FamilyOps {
    /* Family lifecycle */
    int (*family_init)(struct FamilyExchangePage* family);
    int (*family_shutdown)(struct FamilyExchangePage* family);
    int (*family_suspend)(struct FamilyExchangePage* family);
    int (*family_resume)(struct FamilyExchangePage* family);
    
    /* Device discovery and management */
    int (*scan_devices)(struct FamilyExchangePage* family);
    int (*discover_device)(struct FamilyExchangePage* family, void* device_id, void** device_out);
    int (*remove_device)(struct FamilyExchangePage* family, void* device_id);
    
    /* Channel operations */
    int (*allocate_channel)(struct FamilyExchangePage* family, void* device_id, 
                            uint32_t permissions, uint64_t* channel_id);
    int (*release_channel)(struct FamilyExchangePage* family, uint64_t channel_id);
    int (*lookup_channel)(struct FamilyExchangePage* family, uint64_t channel_id, void** channel);
    
    /* Device-specific operations implemented by family */
    int (*device_enable)(struct FamilyExchangePage* family, void* device, void* channel);
    int (*device_disable)(struct FamilyExchangePage* family, void* device, void* channel);
    int (*device_get_info)(struct FamilyExchangePage* family, void* device, void* info_buffer, size_t* info_size);
    
    /* Multi-device coordination (optional) */
    int (*begin_transaction)(struct FamilyExchangePage* family, uint64_t* tx_id);
    int (*commit_transaction)(struct FamilyExchangePage* family, uint64_t tx_id);
    int (*rollback_transaction)(struct FamilyExchangePage* family, uint64_t tx_id);
    
    /* Event notification */
    int (*subscribe_events)(struct FamilyExchangePage* family, struct Process* subscriber, uint32_t event_mask);
    int (*unsubscribe_events)(struct FamilyExchangePage* family, struct Process* subscriber);
    int (*notify_event)(struct FamilyExchangePage* family, enum EventType event_type, void* event_data);
    
    /* Capabilities and configuration */
    uint32_t (*get_capabilities)(struct FamilyExchangePage* family);
    int (*configure_channel_limits)(struct FamilyExchangePage* family, uint32_t max_channels);
};

/* Global family registry */
struct FamilyRegistry {
    struct FamilyExchangePage* families[FAMILY_MAX];
    Lock registry_lock;
    int family_count;
};

/* Core family API */
void family_init(void);
int family_register(enum DeviceFamily family_type, struct FamilyOps* ops, char* name);
int family_unregister(enum DeviceFamily family_type);
struct FamilyExchangePage* family_lookup(enum DeviceFamily family_type);

/* Channel management API */
int family_allocate_channel_by_address(struct FamilyExchangePage* family, void* device_address, 
                                       uint32_t permissions, uint64_t* channel_id);
int family_allocate_channel_by_name(struct FamilyExchangePage* family, char* device_name, 
                                   uint32_t permissions, uint64_t* channel_id);
int family_allocate_channel_auto(struct FamilyExchangePage* family, void* criteria, 
                                uint32_t permissions, uint64_t* channel_id);

/* Utility functions */
const char* family_type_to_string(enum DeviceFamily type);
enum DeviceFamily string_to_family_type(const char* name);
int validate_channel_permissions(uint32_t requested, uint32_t granted);
uint64_t generate_channel_id(void);

/* Statistics and debugging */
void family_stats(enum DeviceFamily family_type);
void family_dump_channel_info(uint64_t channel_id);