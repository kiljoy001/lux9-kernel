/*
 * PCI Family Operations Header - PCI Device Family Interfaces
 * 
 * Defines PCI-specific family operations, data structures, and interfaces
 * Provides the complete PCI family implementation framework
 * Integrates with exchange pages, pebble system, and borrow checker
 */

#pragma once

#include "../u.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "family.h"
#include "pci.h"
#include "pageown.h"
#include "pebble.h"
#include "exchange.h"
#include <error.h>

/* PCI family constants */
#define PCI_FAMILY_TYPE           FAMILY_PCI
#define PCI_CONFIG_SPACE_SIZE    256
#define PCI_MAX_BARS             6
#define PCI_MAX_FUNCTION         8
#define PCI_MAX_DEVICE           32
#define PCI_MAX_BUS              256
#define PCI_MAX_DOMAIN           256
#define MAX_PCI_DEVICES          2048  // Maximum devices manageable

/* PCI capability flags */
#define PCI_CAP_PCIE_SUPPORT     0x01
#define PCI_CAP_MSI_SUPPORT      0x02
#define PCI_CAP_MSIX_SUPPORT     0x04
#define PCI_CAP_PM_SUPPORT       0x08
#define PCI_CAP_AGP_SUPPORT      0x10
#define PCI_CAP_HOTPLUG_SUPPORT  0x20

/* PCI family capabilities */
#define FAMILY_CAP_PCIE_SUPPORT    0x0100
#define FAMILY_CAP_MSI_SUPPORT     0x0200
#define FAMILY_CAP_MSIX_SUPPORT    0x0400
#define FAMILY_CAP_HOT_PLUG        0x0800
#define FAMILY_CAP_MULTI_DEVICE    0x1000
#define FAMILY_CAP_PERSISTENT_NAMES 0x2000

/* PCI device descriptor */
struct PCIDeviceDescriptor {
    // Device identification
    struct PCIAddress {
        uint8_t domain;              // PCI domain
        uint8_t bus;                 // PCI bus number
        uint8_t device;              // Device number
        uint8_t function;            // Function number
        bool domain_active;          // Validity flag
        uint32_t domain_bus_dev_func; // Composite key
    } address;
    
    // PCI device information
    uint16_t vendor_id;              // Vendor ID
    uint16_t device_id;              // Device ID
    uint8_t class_code;              // Base class
    uint8_t subclass_code;           // Subclass
    uint8_t prog_if;                 // Programming interface
    uint8_t revision;                // Revision
    
    // Device capabilities
    struct {
        bool has_pcie;               // PCIe capability
        bool has_msi;                // MSI support
        bool has_msix;               // MSI-X support
        bool has_pm;                 // Power management
        bool has_acpi;               // ACPI support
        uint8_t pci_version;         // PCI version (2.0, 3.0, 4.0, etc.)
        uint32_t capabilities;       // Bit mask of capabilities
    } capabilities;
    
    // BAR information
    struct {
        uint64_t base_address;       // Base address from config space
        uint64_t size;               // Size in bytes
        uint32_t type;               // Memory, I/O, prefetchable
        uint32_t flags;              // Additional flags
        bool is_64bit;               // 64-bit BAR
        bool is_valid;               // BAR is valid
    } bars[PCI_MAX_BARS];
    
    uint8_t bar_count;               // How many BARs are valid
    uint64_t total_memory_size;      // Total BAR memory size
    
    // Device state (protected by borrow checker)
    enum DeviceState device_state;
    struct Proc* owner_process;      // Current owner process
    uint64_t last_access_time;       // Last access timestamp
    uint32_t access_count;           // Number of accesses
    TransactionID active_transaction; // Current transaction if any
    
    // Channel binding
    ChannelID bound_channel_id;       // Channel ID if bound, CHANNEL_UNBOUND otherwise
    struct PCIChannel* bound_channel; // Back-reference to channel
    
    // Driver registration
    char bound_driver_name[64];       // Name of bound driver process
    struct Process* driver_ctx;       // Driver-specific context
    PebbleHandle* driver_white_token;  // Driver's authorization token
    
    // IRQ information
    uint8_t irq_line;                 // Legacy IRQ line
    uint8_t interrupt_pin;            // Interrupt pin (A-D)
};

/* PCI channel structure */
struct PCIChannel {
    // Channel identification
    ChannelID channel_id;             // Unique channel number within PCI family
    char channel_name[64];            // User-friendly name (eth0, wlan0, etc.)
    
    // Device binding
    struct PCIDeviceDescriptor* bound_device;  // Which PCI device this controls
    uint8_t bus, dev, func;           // Quick access to bound device
    
    // Channel security and permissions
    PebbleHandle* white_token;        // Authorization token
    uint32_t permissions_mask;        // Allowed operations (READ_CONFIG, WRITE_CONFIG, etc.)
    PebbleHandle* white_token_family; // Family-level token for multi-device ops
    
    // Associated resources
    struct {
        struct PCIBarResource* bars[PCI_MAX_BARS];
        struct PCIIrqResource* irqs[8];
        struct PCIDmaResource* dmas[4];
        int bar_count, irq_count, dma_count;
    } resources;
    
    // Channel state
    enum ChannelState state;          // ACTIVE, INACTIVE, ERROR, SUSPENDED
    uint64_t created_at;              // Creation timestamp
    uint64_t last_operation;          // Last operation timestamp
    struct Process* owning_process;   // Process that owns this channel
    
    // Transaction support
    struct {
        TransactionID current_tx;     // Current transaction ID
        struct PCIChannelTransaction* pending_ops[MAX_PENDING_OPS];
        int pending_count;
    } transaction_state;
    
    // Driver context
    struct Process* driver_ctx;
    char driver_process_name[64];
};

/* PCI Resource Pool forward declarations */
struct PCIResourcePool;
struct PCIBarResource;
struct PCIIrqResource;
struct PCIDmaResource;

/* PCI family context */
struct PCIFamilyContext {
    // Device registry
    struct {
        struct PCIDeviceDescriptor* devices[MAX_PCI_DEVICES];
        int device_count;
        Lock device_registry_lock;
        
        // Indexing structures for fast lookup
        struct PCIDeviceDescriptor* pci_address_map[256][32][8];  // bus:dev:func
        struct PCIDeviceDescriptor* vendor_device_map[MAX_VENDOR_DEV_PAIRS];
        int total_devices;
    } device_registry;
    
    // Channel manager for multi-device support
    struct {
        struct PCIChannel channels[MAX_CHANNELS_PER_FAMILY_DEFAULT];
        uint64_t next_channel_id;
        uint64_t active_channel_count;
        Lock channel_lock;
        
        // Allocation strategies
        int (*allocate_by_address)(struct PCIFamilyContext*, struct PCIAddress*, ChannelID*);
        int (*allocate_by_criteria)(struct PCIFamilyContext*, struct PCIDeviceCriteria*, ChannelID*);
        int (*allocate_by_name)(struct PCIFamilyContext*, char*, ChannelID*);
    } channel_mgr;
    
    // Resource pool management
    struct PCIResourcePool* resource_pool;
    
    // Transaction coordination
    struct {
        ChannelID next_transaction_id;
        struct PCITransaction* active_transactions[MAX_TRANSACTIONS_PER_FAMILY_DEFAULT];
        int transaction_count;
        Lock transaction_lock;
    } transaction_mgr;
    
    // Event system
    struct {
        struct PCIEvent event_queue[MAX_PCI_EVENTS];
        int event_head, event_tail;
        uint32_t event_sequence;
        Lock event_lock;
    } event_system;
    
    // PCI topology information
    struct {
        uint8_t primary_bus;          // Primary bus number
        uint8_t max_bus;              // Maximum bus number
        uint32_t total_devices;       // Total devices discovered
        uint64_t domain_count;        // Number of PCI domains
    } topology;
    
    // Statistics
    struct {
        uint64_t devices_discovered;
        uint64_t devices_enabled;
        uint64_t channels_allocated;
        uint64_t transactions_started;
        uint64_t transactions_committed;
        uint64_t transactions_rolled_back;
        uint64_t resource_allocations;
        uint64_t error_count;
        uint64_t total_operations;
    } stats;
};

/* PCI family operations structure */
struct PCIFamilyOps {
    // Base family operations
    int (*family_init)(struct FamilyExchangePage* family);
    int (*family_shutdown)(struct FamilyExchangePage* family);
    int (*family_suspend)(struct FamilyExchangePage* family);
    int (*family_resume)(struct FamilyExchangePage* family);
    
    // Bus and device management
    int (*scan_pci_buses)(struct PCIFamilyContext* ctx);
    int (*discover_device)(struct PCIFamilyContext* ctx, struct PCIAddress* addr, struct PCIDeviceDescriptor** device);
    int (*remove_device)(struct PCIFamilyContext* ctx, struct PCIAddress* addr);
    int (*enable_device)(struct PCIFamilyContext* ctx, struct PCIDeviceDescriptor* device);
    int (*disable_device)(struct PCIFamilyContext* ctx, struct PCIDeviceDescriptor* device);
    
    // Channel management
    int (*allocate_channel)(struct PCIFamilyContext* ctx, struct PCIAddress* addr, uint32_t permissions, ChannelID* channel_id);
    int (*release_channel)(struct PCIFamilyContext* ctx, ChannelID channel_id);
    int (*lookup_channel)(struct PCIFamilyContext* ctx, ChannelID channel_id, struct PCIChannel** channel);
    
    // Configuration operations with transaction support
    int (*begin_config_transaction)(struct PCIFamilyContext* ctx, ChannelID channel_id, TransactionID* tx_id);
    int (*read_config)(struct PCIFamilyContext* ctx, ChannelID channel_id, uint8_t offset, uint32_t size, void* data, TransactionID tx_id);
    int (*write_config)(struct PCIFamilyContext* ctx, ChannelID channel_id, uint8_t offset, uint32_t size, void* data, TransactionID tx_id);
    int (*commit_config_transaction)(struct PCIFamilyContext* ctx, ChannelID channel_id, TransactionID tx_id);
    int (*rollback_config_transaction)(struct PCIFamilyContext* ctx, ChannelID channel_id, TransactionID tx_id);
    
    // BAR and mapping operations
    int (*map_bar)(struct PCIFamilyContext* ctx, ChannelID channel_id, uint8_t bar_num, uint32_t size, uintptr* virtual_addr);
    int (*unmap_bar)(struct PCIFamilyContext* ctx, ChannelID channel_id, uint8_t bar_num);
    int (*get_bar_info)(struct PCIFamilyContext* ctx, ChannelID channel_id, uint8_t bar_num, struct PCIBarInfo* info);
    
    // IRQ and DMA coordination
    int (*allocate_irq)(struct PCIFamilyContext* ctx, ChannelID channel_id, uint32_t preferred_irq, struct PCIIrqSetup** irq_setup);
    int (*release_irq)(struct PCIFamilyContext* ctx, ChannelID channel_id, uint32_t irq_handle);
    int (*allocate_dma_region)(struct PCIFamilyContext* ctx, ChannelID channel_id, uint32_t size, uint32_t alignment, struct PCIDmaRegion** dma_region);
    int (*release_dma_region)(struct PCIFamilyContext* ctx, ChannelID channel_id, uint32_t dma_handle);
    
    // Multi-device coordination
    int (*begin_multi_device_transaction)(struct PCIFamilyContext* ctx, ChannelID channel_ids[], int count, MultiTransactionID* multi_tx_id);
    int (*commit_multi_device_transaction)(struct PCIFamilyContext* ctx, MultiTransactionID multi_tx_id);
    int (*rollback_multi_device_transaction)(struct PCIFamilyContext* ctx, MultiTransactionID multi_tx_id);
    
    // Event system
    int (*subscribe_events)(struct PCIFamilyContext* ctx, struct Process* subscriber, uint32_t event_mask);
    int (*unsubscribe_events)(struct PCIFamilyContext* ctx, struct Process* subscriber);
    int (*notify_event)(struct PCIFamilyContext* ctx, enum PCIEventType event_type, void* event_data);
    
    // 9P interface
    int (*handle_9p_request)(struct FamilyExchangePage* family, struct Fcall* fc, struct Chan* chan);
    
    // Resource management
    int (*configure_channel_limits)(struct FamilyExchangePage* family);
    uint32_t (*get_capabilities)(struct FamilyExchangePage* family);
};

/* PCI transaction structure */
struct PCITransaction {
    TransactionID transaction_id;
    ChannelID channel_id;
    enum {
        PCI_TX_INIT = 0,
        PCI_TX_ACTIVE = 1,
        PCI_TX_COMMITTED = 2,
        PCI_TX_ROLLED_BACK = 3,
        PCI_TX_ERROR = 4
    } state;
    
    // PCI transaction specifics
    struct {
        uint32_t config_offset;
        uint32_t config_size;
        void* original_data;
        void* new_data;
        bool config_modified;
        bool bars_modified[PCI_MAX_BARS];
        bool irq_modified;
    } pci_context;
    
    // Rollback information
    void* rollback_data;
    size_t rollback_size;
    
    // Timestamps
    uint64_t created_at;
    uint64_t timeout_at;
    
    // Owner process
    struct Process* owner_process;
};

/* PCI event structure */
struct PCIEvent {
    uint32_t event_type;
    uint64_t timestamp;
    uint32_t sequence;
    struct PCIAddress device_address;
    
    union {
        struct {
            uint16_t vendor_id;
            uint16_t device_id;
            uint8_t class_code;
            uint8_t subclass_code;
        } device_arrived;
        
        struct {
            uint32_t reason;
            uint32_t error_code;
        } device_error;
        
        struct {
            ChannelID channel_id;
            uint32_t operation_type;
        } channel_event;
        
        struct {
            MultiTransactionID transaction_id;
            int channel_count;
            bool committed;
        } multi_device_event;
    } event_data;
};

/* Resource pool statistics */
struct ResourcePoolStats {
    uint32_t used_bars, max_bars;
    uint32_t used_irqs, max_irqs;
    uint32_t used_dmas, max_dmas;
    
    uint64_t total_bar_allocations;
    uint64_t total_irq_allocations;
    uint64_t total_dma_allocations;
    uint64_t total_bar_bytes;
    uint64_t total_dma_bytes;
    uint64_t peak_concurrent_bars;
    uint64_t peak_concurrent_irqs;
    uint64_t peak_concurrent_dmas;
    uint64_t allocation_failures;
};

/* PCI function declarations */

// Core PCI family functions
int pcifamily_init(void);
int pci_init(void);
int pci_shutdown(void);

// PCI address parsing
struct PCIAddress parse_pci_address_string(const char* address_str);
char* format_pci_address(struct PCIAddress* addr);

// Device discovery and management
int scan_pci_bus(struct PCIFamilyContext* ctx, uint16_t bus);
struct PCIDeviceDescriptor* pci_lookup_device(struct FamilyExchangePage* family, struct PCIAddress* addr);

// Channel management
int pci_allocate_channel(struct FamilyExchangePage* family, void* device_id, uint32_t permissions, uint64_t* channel_id);
int pci_release_channel(struct FamilyExchangePage* family, uint64_t channel_id);
struct PCIChannel* lookup_pci_channel(struct FamilyExchangePage* family, uint64_t channel_id);
void setup_pci_channel_manager(struct FamilyExchangePage* family);

// Resource pool management
void setup_pci_resource_pool(struct FamilyExchangePage* family);
int allocate_pci_bar_resource(struct FamilyExchangePage* family, struct PCIDeviceDescriptor* device, uint8_t bar_number, uint64_t channel_id, struct PCIBarResource** bar_out);
int release_pci_bar_resource(struct FamilyExchangePage* family, struct PCIBarResource* bar);
int allocate_pci_irq_resource(struct FamilyExchangePage* family, struct PCIDeviceDescriptor* device, uint32_t irq_vector, uint8_t trigger_type, uint8_t polarity, uint64_t channel_id, struct PCIIrqResource** irq_out);
int release_pci_irq_resource(struct FamilyExchangePage* family, struct PCIIrqResource* irq);
int allocate_pci_dma_resource(struct FamilyExchangePage* family, struct PCIDeviceDescriptor* device, uint64_t size, uint32_t alignment, bool coherent, uint64_t channel_id, struct PCIDmaResource** dma_out);
int release_pci_dma_resource(struct FamilyExchangePage* family, struct PCIDmaResource* dma);
int cleanup_channel_resources(struct FamilyExchangePage* family, uint64_t channel_id);
void get_pci_resource_pool_stats(struct FamilyExchangePage* family, struct ResourcePoolStats* stats);
int shutdown_pci_resource_pool(struct FamilyExchangePage* family);

// Event system management
void setup_pci_event_system(struct FamilyExchangePage* family);
int pci_subscribe_events(struct FamilyExchangePage* family, struct Process* subscriber, uint32_t event_mask);
int pci_unsubscribe_events(struct FamilyExchangePage* family, struct Process* subscriber);
int pci_notify_event(struct FamilyExchangePage* family, enum PCIEventType event_type, void* event_data);
void notify_pci_device_removed(struct FamilyExchangePage* family, struct PCIDeviceDescriptor* device);

// Transaction management
void setup_pci_transaction_manager(struct FamilyExchangePage* family);
int pci_begin_transaction(struct FamilyExchangePage* family, uint64_t channel_id, uint64_t* tx_id);
int pci_commit_transaction(struct FamilyExchangePage* family, uint64_t channel_id, uint64_t tx_id);
int pci_rollback_transaction(struct FamilyExchangePage* family, uint64_t channel_id, uint64_t tx_id);

// Configuration space access with transaction support
int pci_config_read_with_transaction(struct PCIDeviceDescriptor* dev, uint8_t offset, uint32_t size, void* data, uint64_t tx_id);
int pci_config_write_with_transaction(struct PCIDeviceDescriptor* dev, uint8_t offset, uint32_t size, void* data, uint64_t tx_id);

// Device class helpers
const char* pci_class_to_string(uint8_t class_code, uint8_t subclass);
bool pci_is_network_device(struct PCIDeviceDescriptor* dev);
bool pci_is_storage_device(struct PCIDeviceDescriptor* dev);
bool pci_is_display_device(struct PCIDeviceDescriptor* dev);

// Device state helpers
const char* device_state_to_string(enum DeviceState state);
int device_can_access_flags(struct PCIDeviceDescriptor* dev);

// Resource allocation helpers
int validate_channel_permissions(uint32_t requested, uint32_t available);
int validate_channel_operation_permission(struct Process* proc, struct PCIChannel* channel);

// Statistics and debugging
void update_pci_family_stats(struct FamilyExchangePage* family);
struct PCIChannel* allocate_pci_channel_struct(struct FamilyExchangePage* family, uint64_t channel_id, struct PCIDeviceDescriptor* device, uint32_t permissions);
void free_pci_channel_struct(struct PCIChannel* channel);
void cleanup_pci_channel_resources(struct PCIChannel* channel);

// Exchange page integration for PCI resources
int exchange_prepare_pci_bar(uint64_t channel_id, uint8_t bar_num, PebbleHandle* white_token, struct ExchangeHandle* bar_handle);
int exchange_prepare_pci_dma(uint64_t channel_id, uint64_t physical_addr, uint64_t size, PebbleHandle* white_token, struct ExchangeHandle* dma_handle);

// Constants for limits and sizing
#define MAX_PENDING_OPS              16
#define MAX_PCI_EVENTS               1024
#define MAX_VENDOR_DEV_PAIRS         256
#define DEFAULT_CHANNELS_PER_FAMILY  1048576UL

// Error codes
#define PCI_FAMILY_SUCCESS           0
#define PCI_ERROR_INVALID_ARGS       -1
#define PCI_ERROR_NO_MEMORY          -2
#define PCI_ERROR_DEVICE_NOT_FOUND   -3
#define PCI_ERROR_CHANNEL_EXHAUSTED  -4
#define PCI_ERROR_PERMISSION_DENIED  -5
#define PCI_ERROR_TRANSACTION_FAILED -6
#define PCI_ERROR_RESOURCE_BUSY      -7
#define PCI_ERROR_CAPABILITY_NOT_SUPPORTED -8
#define PCI_ERROR_SYSTEM_LIMIT       -9