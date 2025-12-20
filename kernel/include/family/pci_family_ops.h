/*
 * PCI Family Interface - PCI Device Family Implementation
 * 
 * Provides PCI-specific implementation of the family interface.
 * Handles PCI device discovery, configuration space access, BAR mapping,
 * and coordination with other families (DMA, IRQ families).
 */

#pragma once

#include <stdbool.h>
#include "family.h"

/* PCI constants */
#define PCI_FAMILY_TYPE           FAMILY_PCI
#define MAX_PCI_DEVICES 1024                /* Maximum devices we can track */
#define MAX_PCI_BARS 64
#define MAX_PCI_IRQS 64
#define MAX_PCI_DMAS 64
#define MAX_PCI_BARS_PER_CHANNEL 6
#define MAX_PCI_IRQS_PER_CHANNEL 8
#define MAX_PCI_DMAS_PER_CHANNEL 4
#define PCI_CONFIG_SPACE_SIZE 256              /* Standard PCI config space size */
#define PCIExtended_CONFIG_SPACE_SIZE 4096    /* Extended config space size */
#define PCIBAR_COUNT 6                         /* Number of PCI BARs per device */
#define MAX_PCI_DOMAINS 256                     /* Maximum PCI domains supported */

/* Forward declarations for types referenced in the API */
struct PCIBusTopology;
struct PCIBarResource;
struct PCIIrqResource;
struct PCIDmaResource;
struct PCIChannelBarResource;
struct PCIChannelIrqResource;
struct PCIChannelDmaResource;
struct PCIChannelManager;
struct ExchangeHandle;
struct PCIResourcePool;

/* PCI family capability flags (distinct from base family caps) */
#define PCIFAMILY_CAP_PCIE_SUPPORT    0x0100
#define PCIFAMILY_CAP_MSI_SUPPORT     0x0200
#define PCIFAMILY_CAP_MSIX_SUPPORT    0x0400

/* PCI device identification structure */
struct PCIAddress {
    uint16_t domain;      /* PCI domain */
    uint8_t domain_active; /* Whether domain is active */
    uint8_t bus;         /* PCI bus number */
    uint8_t device;       /* Device number */
    uint8_t function;     /* Function number */
    
    /* Quick access */
    uint32_t domain_bus_dev_func; /* Combined key for quick lookup */
};

/* PCI device descriptor */
struct PCIDeviceDescriptor {
    /* Device identification */
    struct PCIAddress address;
    
    /* PCI vendor and device information */
    uint16_t vendor_id;
    uint16_t device_id;
    uint16_t subsystem_vendor;
    uint16_t subsystem_device;
    uint8_t class_code;          /* Base class */
    uint8_t subclass_code;         /* Subclass */
    uint8_t prog_if;              /* Programming interface */
    uint8_t revision;             /* Revision ID */
    char class_desc[64];          /* Human-readable class description */
    
    /* PCI capabilities */
    struct {
        bool has_pcie;               /* PCIe capability */
        bool has_msi;                /* MSI support */
        bool has_msix;               /* MSI-X support */
        bool has_pm;                 /* Power management */
        bool has_acpi;               /* ACPI support */
        bool has_virtio;             /* VirtIO capabilities */
        uint8_t pcie_version;        /* PCIe version (2.0, 3.0, 4.0, etc.) */
        uint32_t pcie_max_payload;    /* Maximum payload size */
        uint32_t msi_count;          /* MSI message count */
        uint32_t msix_count;         /* MSI-X message count */
    } capabilities;
    
    /* BAR information */
    struct {
        uint32_t base_address;      /* Base address from config space */
        uint64_t size;              /* Size in bytes */
        uint32_t type;              /* Memory, I/O, prefetchable flag */
        uint32_t flags;             /* Additional flags */
        bool is_64bit;              /* 64-bit BAR */
        bool is_valid;              /* BAR is valid/implemented */
    } bars[6];                      /* PCI supports up to 6 BARs */
    
    /* Device state (protected by borrow checker) */
    enum DeviceState device_state;
    uint64_t last_access_time;      /* Last access timestamp */
    uint32_t access_count;          /* Number of operations */
    bool has_active_config;         /* Whether device has active blue/red config */
    
    /* Channel binding */
    uint64_t bound_channel_id;      /* Channel ID if bound */
    void* bound_channel;            /* Back-reference to channel */
    
    /* Power management */
    struct {
        uint8_t current_power_state;
        uint32_t capabilities_mask;
        uint64_t power_state_timestamp;
    } power_state;
    
    /* Driver registration */
    char bound_driver_name[64];     /* Name of bound driver process */
    void* driver_ctx;               /* Driver-specific context */
    struct PebbleHandle* driver_white_token; /* Driver's authorization token */
};

struct PCIChannelManager {
    uint64_t next_channel_id;
    uint32_t max_channels;
    struct PCIChannel* channel_pool;
    struct PCIChannel** channel_table;
    Lock channel_lock;
    struct FamilyExchangePage* family;
};

/* PCI family context */
struct PCIFamilyContext {
    /* PCI topology information */
    struct {
        uint8_t primary_bus;       /* Primary bus number */
        uint8_t max_bus;            /* Maximum bus number */
        struct PCIBusTopology* tree; /* Device tree structure */
        uint32_t domain_number;     /* PCI domain (for multi-domain systems) */
        uint32_t total_devices;     /* Total discovered devices */
    } topology;
    
    /* Device registry */
    struct {
        struct PCIDeviceDescriptor* devices[MAX_PCI_DEVICES];
        int device_count;
        Lock device_registry_lock;
        
        /* Fast lookup structures */
        struct PCIDeviceDescriptor* domain_map[256][32][8]; /* domain:bus:dev:func */
        uint8_t domain_allocated[256];    /* Which domains are active */
    } device_registry;
    
    /* Resource management */
    struct PCIResourcePool* resource_pool;  /* PCI resource pool manager */
    
    /* Configuration space access */
    struct {
        int (*read_config_byte)(uint8_t bus, uint8_t dev, uint8_t func, uint8_t offset, uint8_t* data);
        int (*read_config_word)(uint8_t bus, uint8_t dev, uint8_t func, uint8_t offset, uint16_t* data);
        int (*read_config_dword)(uint8_t bus, uint8_t dev, uint8_t func, uint8_t offset, uint32_t* data);
        
        int (*write_config_byte)(uint8_t bus, uint8_t dev, uint8_t func, uint8_t offset, uint8_t data);
        int (*write_config_word)(uint8_t bus, uint8_t dev, uint8_t func, uint8_t offset, uint16_t data);
        int (*write_config_dword)(uint8_t bus, uint8_t dev, uint8_t func, uint8_t offset, uint32_t data);
        
        int (*lock_config_space)(uint8_t bus, uint8_t dev, uint8_t func);
        int (*unlock_config_space)(uint8_t bus, uint8_t dev, uint8_t func);
    } config_ops;
    
    /* PCI family statistics */
    struct {
        uint64_t devices_discovered;
        uint64_t devices_enabled;
        uint64_t config_space_reads;
        uint64_t config_space_writes;
        uint64_t bar_mappings;
        uint64_t irq_allocations;
        uint64_t error_count;
        
        /* Hardware-specific statistics */
        uint64_t pcie_transactions;
        uint64_t msi_interrupts;
        uint64_t power_state_changes;
    } stats;
};

/* PCI channel structure (extends base channel with PCI-specific data) */
struct PCIChannel {
    /* Base channel information (family manager handles this) */
    uint64_t channel_id;              /* Base channel identifier */
    char channel_name[64];             /* User-friendly name */
    struct PCIChannelManager* channel_manager; /* Manager owning this channel */
    
    /* PCI-specific binding */
    struct PCIDeviceDescriptor* bound_device;  /* Which device this controls */
    struct PCIAddress pci_address;               /* Quick access to device address */
    
    /* PCI resource associations */
    struct {
        struct PCIChannelBarResource* bars[MAX_PCI_BARS_PER_CHANNEL];
        struct PCIChannelIrqResource* irqs[MAX_PCI_IRQS_PER_CHANNEL];
        struct PCIChannelDmaResource* dmas[MAX_PCI_DMAS_PER_CHANNEL];
        int bar_count, irq_count, dma_count;
    } resources;
    
    /* Security and permissions (inherited from base) */
    uint32_t permissions_mask;          /* What operations allowed */
    struct PebbleHandle* white_token;    /* Authentication token */
    struct PebbleHandle* white_token_family; /* Family-level token */
    
    /* PCI-specific state */
    struct {
        bool config_transaction_active;
        uint64_t current_transaction_id;
    } transaction_state;
    
    /* BAR mapping context */
    struct {
        uintptr bar_mappings[6];      /* Virtual addresses for mapped BARs */
        uintptr bar_addresses[6];     /* Alias for bar_mappings */
        size_t bar_sizes[6];           /* Size of each BAR mapping */
        bool bar_mapped[6];            /* Whether each BAR is mapped */
    } mapping_ctx;

    /* Additional fields for channel management */
    struct FamilyExchangePage* family;  /* Back-reference to family */
    enum ChannelState state;            /* Current channel state */
    uint64_t created_at;                /* Creation timestamp */
    uint64_t last_operation;            /* Last operation timestamp */
    uint64_t operation_count;           /* Number of operations */
    struct Proc* owner_process;         /* Process that owns this channel */
    struct PCIChannel* next;            /* For free list */

    /* Channel statistics */
    struct {
        uint64_t reads, writes;
        uint64_t config_space_access;
        uint64_t bar_access;
        uint64_t irq_notifications;
    } stats;
};

/* PCI family operation functions (C-friendly wrapper around FamilyOps) */
struct PCIFamilyOps {
    struct FamilyOps base;
    
    /* PCI-specific implementations of base operations */
    int (*enable_device)(void* device, void* channel);
    int (*disable_device)(void* device, void* channel);
    int (*get_device_info)(void* device, void* buffer, size_t* size);
    
    /* PCI configuration space operations */
    int (*begin_config_read)(void* device, void* channel);
    int (*begin_config_write)(void* device, void* channel);
    int (*cancel_config_transaction)(void* device, void* channel);
    
    /* PCI BAR operations */
    int (*map_bar)(void* device, void* channel, uint8_t bar_num, size_t size, uintptr* virtual_address);
    int (*unmap_bar)(void* device, void* channel, uint8_t bar_num);
    int (*get_bar_info)(void* device, void* channel, uint8_t bar_num, void* info, size_t* size);
    
    /* PCI power management */
    int (*set_power_state)(void* device, uint8_t power_state);
    int (*get_power_state)(void* device, uint8_t* power_state);
    
    /* PCI IRQ coordination */
    int (*allocate_irq)(void* device, void* channel, uint32_t preferred_irq, void* irq_setup);
    int (*release_irq)(void* device, void* channel, uint32_t irq_handle);
    
    /* PCIe-specific operations */
    int (*enable_pcie_extended_config)(void* device);
    int (*configure_msi)(void* device, void* channel);
    int (*configure_msix)(void* device, void* channel);
};

/* PCI-specific API */
int pcifamily_init(void);
struct PCIDeviceDescriptor* pci_lookup_device(struct PCIFamilyContext* ctx, struct PCIAddress* addr);
int pci_scan_domain(struct PCIFamilyContext* ctx, uint16_t domain);
int pci_read_config_space(struct PCIFamilyContext* ctx, struct PCIDeviceDescriptor* dev, void* buffer, size_t size);
int pci_write_config_space(struct PCIFamilyContext* ctx, struct PCIDeviceDescriptor* dev, void* buffer, size_t size);

/* PCI address utilities */
struct PCIAddress parse_pci_address_string(const char* address_str);
char* format_pci_address(struct PCIAddress* addr);
uint32_t pci_address_to_key(struct PCIAddress* addr);

/* PCI device utilities */
const char* pci_class_to_string(uint8_t class_code, uint8_t subclass);
bool pci_is_bridge_device(struct PCIDeviceDescriptor* dev);
bool pci_is_display_device(struct PCIDeviceDescriptor* dev);
bool pci_is_network_device(struct PCIDeviceDescriptor* dev);
bool pci_is_storage_device(struct PCIDeviceDescriptor* dev);

/* Configuration space access helpers */
int pci_config_read8(uint8_t bus, uint8_t dev, uint8_t func, uint8_t offset, uint8_t* data);
int pci_config_read16(uint8_t bus, uint8_t dev, uint8_t func, uint8_t offset, uint16_t* data);
int pci_config_read32(uint8_t bus, uint8_t dev, uint8_t func, uint8_t offset, uint32_t* data);
int pci_config_write8(uint8_t bus, uint8_t dev, uint8_t func, uint8_t offset, uint8_t data);
int pci_config_write16(uint8_t bus, uint8_t dev, uint8_t func, uint8_t offset, uint16_t data);
int pci_config_write32(uint8_t bus, uint8_t dev, uint8_t func, uint8_t offset, uint32_t data);

/* PCI constants */
#define PCI_FAMILY_TYPE           FAMILY_PCI
#define MAX_PCI_DEVICES 1024                /* Maximum devices we can track */
#define MAX_PCI_BARS 64
#define MAX_PCI_IRQS 64
#define MAX_PCI_DMAS 64
#define MAX_PCI_BARS_PER_CHANNEL 6
#define MAX_PCI_IRQS_PER_CHANNEL 8
#define MAX_PCI_DMAS_PER_CHANNEL 4
#define PCI_CONFIG_SPACE_SIZE 256              /* Standard PCI config space size */
#define PCIExtended_CONFIG_SPACE_SIZE 4096    /* Extended config space size */
#define PCIBAR_COUNT 6                         /* Number of PCI BARs per device */
#define MAX_PCI_DOMAINS 256                     /* Maximum PCI domains supported */

/* Constants */
#define MAX_PCI_DEVICES 1024                /* Maximum devices we can track */
#define PCI_CONFIG_SPACE_SIZE 256              /* Standard PCI config space size */
#define PCIExtended_CONFIG_SPACE_SIZE 4096    /* Extended config space size */
#define PCIBAR_COUNT 6                         /* Number of PCI BARs per device */
#define MAX_PCI_DOMAINS 256                     /* Maximum PCI domains supported */
#define MAX_PCI_BARS 64
#define MAX_PCI_IRQS 64
#define MAX_PCI_DMAS 64
#define MAX_PCI_BARS_PER_CHANNEL 6
#define MAX_PCI_IRQS_PER_CHANNEL 8
#define MAX_PCI_DMAS_PER_CHANNEL 4
#define PCI_FAMILY_TYPE           FAMILY_PCI
/* PCI standard IDs */
#define PCI_STANDARD_ID_VENDOR 0x0000
#define PCI_STANDARD_ID_DEVICE 0x0000

/* PCI class codes */
#define PCI_BASE_CLASS_STORAGE 0x01
#define PCI_BASE_CLASS_NETWORK 0x02
#define PCI_BASE_CLASS_DISPLAY 0x03
#define PCI_BASE_CLASS_MULTIMEDIA 0x04
