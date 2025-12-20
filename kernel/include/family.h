/*
 * Base family interface - Device family abstraction
 * Provides common infrastructure for kernel device families (PCI, USB, I2C, etc.)
 *
 * Device families provide:
 *   - Multi-device channel management
 *   - Protocol-aware device operations
 *   - 9P filesystem interface for userspace drivers
 *   - Pebble-based security with borrow checking
 *   - Exchange page integration for zero-copy operations
 */

#pragma once

#include "../u.h"
#include "../include/mem.h"
#include "../include/lib.h"

// Forward declarations
struct Proc;
struct Chan;
struct Fcall;
struct Dir;
struct PebbleWhite;
struct PebbleBlack;
struct PebbleBlue;
struct PebbleRed;
struct ExchangeHandle;

// 64-bit channel and transaction identifiers
typedef uint64_t ChannelID;
typedef uint64_t TransactionID;

// Special identifiers
#define CHANNEL_ID_INVALID    0ULL
#define CHANNEL_ID_UNBOUND    0xFFFFFFFFFFFFFFFFULL
#define TRANSACTION_ID_INVALID 0ULL

// Device family types
enum FamilyType {
    FAMILY_PCI = 1,
    FAMILY_USB = 2,
    FAMILY_I2C = 3,
    FAMILY_SPI = 4,
    FAMILY_MAX = 255
};

// Channel and device states
enum ChannelState {
    CHANNEL_INACTIVE = 0,
    CHANNEL_ACTIVE = 1,
    CHANNEL_ERROR = 2,
    CHANNEL_SUSPENDED = 3
};

enum DeviceState {
    DEVICE_INIT = 0,
    DEVICE_CONFIGURING = 1,
    DEVICE_ACTIVE = 2,
    DEVICE_ERROR = 3,
    DEVICE_REMOVED = 4
};

// Operation permissions
enum PermissionMask {
    PERM_READ_CONFIG = 0x01,
    PERM_WRITE_CONFIG = 0x02,
    PERM_MAP_BARS = 0x04,
    PERM_ALLOC_IRQ = 0x08,
    PERM_ALLOC_DMA = 0x10,
    PERM_MULTI_DEVICE = 0x20,
    PERM_ADMIN = 0x80
};

// Base family exchange page structure
struct FamilyExchangePage {
    // Channel management
    struct ChannelManager {
        ChannelID next_channel_id;
        uint64_t active_channel_count;
        Lock channel_lock;
        
        // Channel lookup table (indexed by ChannelID)
        struct Channel* channels;
        uint64_t channel_capacity;
    } channel_mgr;
    
    // Family state
    enum FamilyType family_type;
    enum DeviceState state;
    uint64_t created_at;
    uint64_t last_operation;
    
    // Pebble security
    struct PebbleWhite* family_white_token;
    struct PebbleBlack* family_black_handle;
    
    // Transaction management
    struct {
        TransactionID next_transaction_id;
        struct Transaction* active_transactions;
        uint64_t transaction_count;
        Lock transaction_lock;
    } transaction_mgr;
};

// Base channel structure
struct Channel {
    // Channel identification
    ChannelID channel_id;
    char channel_name[64];
    
    // Channel state and permissions
    enum ChannelState state;
    uint32_t permissions_mask;
    
    // Security and ownership
    struct Proc* owning_process;
    struct PebbleWhite* white_token;
    struct PebbleBlack* black_handle;
    
    // Device binding
    struct DeviceDescriptor* bound_device;
    
    // Transaction support
    TransactionID current_transaction;
    
    // Exchange page handles
    struct ExchangeHandle* exchange_handles;
    int exchange_count;
    
    // Timestamps
    uint64_t created_at;
    uint64_t last_operation;
};

// Base device descriptor
struct DeviceDescriptor {
    // Device identification
    struct DeviceAddress {
        uint8_t bus;
        uint8_t device;
        uint8_t function;
        uint8_t domain;
    } address;
    
    // Device information
    uint16_t vendor_id;
    uint16_t device_id;
    uint8_t class_code;
    uint8_t subclass_code;
    uint8_t revision;
    
    // Device state (protected by borrow checker)
    enum DeviceState state;
    struct Proc* owner_process;
    uint64_t last_access_time;
    uint32_t access_count;
    TransactionID active_transaction;
    
    // Channel binding
    ChannelID bound_channel_id;
    struct Channel* bound_channel;
    
    // Security
    struct PebbleWhite* device_white_token;
    char bound_driver_name[64];
};

// Base family operations interface
struct FamilyOps {
    // Family lifecycle
    int (*family_init)(struct FamilyExchangePage* family);
    int (*family_shutdown)(struct FamilyExchangePage* family);
    int (*family_suspend)(struct FamilyExchangePage* family);
    int (*family_resume)(struct FamilyExchangePage* family);
    
    // Device management
    int (*discover_device)(struct FamilyExchangePage* family, void* addr_data, struct DeviceDescriptor** device);
    int (*enable_device)(struct FamilyExchangePage* family, struct DeviceDescriptor* device);
    int (*disable_device)(struct FamilyExchangePage* family, struct DeviceDescriptor* device);
    
    // Channel management
    int (*allocate_channel)(struct FamilyExchangePage* family, void* addr_data, void* criteria, ChannelID* channel_id);
    int (*release_channel)(struct FamilyExchangePage* family, ChannelID channel_id);
    int (*lookup_channel)(struct FamilyExchangePage* family, ChannelID channel_id, struct Channel** channel);
    
    // Transaction support
    int (*begin_transaction)(struct FamilyExchangePage* family, ChannelID channel_id, TransactionID* tx_id);
    int (*commit_transaction)(struct FamilyExchangePage* family, ChannelID channel_id, TransactionID tx_id);
    int (*rollback_transaction)(struct FamilyExchangePage* family, ChannelID channel_id, TransactionID tx_id);
    
    // 9P interface
    int (*handle_9p_request)(struct FamilyExchangePage* family, struct Fcall* fc, struct Chan* chan);
    
    // Security and pebble operations
    int (*validate_device_access)(struct FamilyExchangePage* family, struct DeviceDescriptor* device, struct Proc* process);
    int (*create_device_pebble)(struct FamilyExchangePage* family, struct DeviceDescriptor* device, struct PebbleWhite** white_token);
    
    // Event system
    int (*subscribe_events)(struct FamilyExchangePage* family, struct Proc* subscriber, uint32_t event_mask);
    int (*notify_event)(struct FamilyExchangePage* family, uint32_t event_type, void* event_data);
};

// Base transaction structure
struct Transaction {
    TransactionID transaction_id;
    ChannelID channel_id;
    enum {
        TX_INIT = 0,
        TX_ACTIVE = 1,
        TX_COMMITTED = 2,
        TX_ROLLED_BACK = 3,
        TX_ERROR = 4
    } state;
    
    // Transaction rollback information
    void* rollback_data;
    size_t rollback_size;
    
    // Timestamps
    uint64_t created_at;
    uint64_t timeout_at;
    
    // Owner process
    struct Proc* owner_process;
};

// Family registration and management
int family_register(enum FamilyType type, struct FamilyOps* ops, struct FamilyExchangePage** family_out);
int family_unregister(struct FamilyExchangePage* family);
int family_lookup(enum FamilyType type, struct FamilyExchangePage** family_out);

// Base channel management
int family_channel_allocate(struct FamilyExchangePage* family, struct DeviceDescriptor* device, uint32_t permissions, ChannelID* channel_id);
int family_channel_release(struct FamilyExchangePage* family, ChannelID channel_id);
int family_channel_lookup(struct FamilyExchangePage* family, ChannelID channel_id, struct Channel** channel);

// Base transaction management
int family_transaction_begin(struct FamilyExchangePage* family, ChannelID channel_id, TransactionID* tx_id);
int family_transaction_commit(struct FamilyExchangePage* family, ChannelID channel_id, TransactionID tx_id);
int family_transaction_rollback(struct FamilyExchangePage* family, ChannelID channel_id, TransactionID tx_id);

// Security and pebble integration for families
int family_create_pebble(struct FamilyExchangePage* family, struct DeviceDescriptor* device, struct PebbleWhite** white_token);
int family_validate_pebble_access(struct FamilyExchangePage* family, struct PebbleWhite* white_token, uint32_t operation, struct DeviceDescriptor* device);

// Exchange page integration for families
int family_exchange_prepare_device_resource(struct FamilyExchangePage* family, ChannelID channel_id, void* resource_data, struct ExchangeHandle* handle);
int family_exchange_accept_device_resource(struct ExchangeHandle* handle, uintptr dest_vaddr, struct PebbleWhite* white_token);

// 9P filesystem interface for families
int family_9p_handle_request(struct FamilyExchangePage* family, struct Fcall* fc, struct Chan* chan);
int family_9p_create_device_path(struct FamilyExchangePage* family, struct DeviceDescriptor* device, struct Chan* parent);
int family_9p_create_channel_path(struct FamilyExchangePage* family, struct Channel* channel, struct Chan* parent);

// Utility functions
uint64_t family_generate_channel_id(struct FamilyExchangePage* family);
uint64_t family_generate_transaction_id(struct FamilyExchangePage* family);
int family_validate_channel_capacity(struct FamilyExchangePage* family);

// Constants and limits
#define MAX_CHANNELS_PER_FAMILY_DEFAULT 1048576ULL  // 1M channels - 64-bit support from start
#define MAX_DEVICES_PER_FAMILY_DEFAULT 65536        // 65K devices
#define MAX_TRANSACTIONS_PER_FAMILY_DEFAULT 1048576 // 1M transactions
#define MAX_CHANNEL_NAME_LENGTH 63
#define MAX_DRIVER_NAME_LENGTH 63

// Error codes
enum FamilyError {
    FAMILY_SUCCESS = 0,
    FAMILY_ERROR_INVALID_ARGS = -1,
    FAMILY_ERROR_NO_MEMORY = -2,
    FAMILY_ERROR_CHANNEL_EXHAUSTED = -3,
    FAMILY_ERROR_DEVICE_NOT_FOUND = -4,
    FAMILY_ERROR_PERMISSION_DENIED = -5,
    FAMILY_ERROR_TRANSACTION_FAILED = -6,
    FAMILY_ERROR_INVALID_STATE = -7,
    FAMILY_ERROR_RESOURCE_BUSY = -8,
    FAMILY_ERROR_CAPABILITY_NOT_SUPPORTED = -9,
    FAMILY_ERROR_SYSTEM_LIMIT = -10
};