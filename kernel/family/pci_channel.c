/*
 * PCI Channel Management
 * 
 * Implements channel lifecycle management for PCI devices.
 * Handles channel allocation, binding, resource tracking, and cleanup.
 * Supports persistent naming and multiple addressing strategies.
 */

#include "u.h"
#include "portlib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "family/family.h"
#include "family/pci_family_ops.h"
#include "pageown.h"
#include "pebble.h"
#include "exchange.h"
#include <error.h>

/* PCI channel resource structures */
struct PCIChannelBarResource {
    uint8_t bar_number;              /* Which BAR (0-5) */
    ExchangeHandle exchange_handle; /* Exchange page handle for BAR memory */
    uintptr virtual_address;         /* Mapped virtual address if mapped */
    size_t mapped_size;              /* Current mapped size */
    bool is_mapped;
    struct PCIDeviceDescriptor* device;
};

struct PCIChannelIrqResource {
    uint32_t irq_number;              /* Physical IRQ line */
    bool msi_enabled;               /* MSI is enabled */
    uint32_t msi_vector;             /* MSI vector number */
    struct PCIDeviceDescriptor* device;
    struct Process* handler_process;   /* Process handling interrupts */
};

struct PCIChannelDmaResource {
    uint32_t channel_id;              /* DMA channel identifier */
    ExchangeHandle exchange_handle;   /* Exchange page for DMA buffer */
    uintptr physical_address;        /* Physical address for DMA */
    size_t size;                     /* Buffer size */
    struct PCIDeviceDescriptor* device;
};

/* PCI channel structure */
struct PCIChannel {
    /* Base channel information */
    uint64_t channel_id;              /* 64-bit unique channel ID */
    char channel_name[64];             /* Human readable name */
    struct FamilyExchangePage* family; /* Back-reference to family */
    
    /* Device binding */
    struct PCIDeviceDescriptor* bound_device;  /* Which device this controls */
    struct PCIAddress pci_address;               /* Quick access */
    
    /* Security and permissions */
    uint32_t permissions_mask;          /* Allowed operations */
    struct PebbleHandle* white_token;    /* Authentication token */
    struct PebbleHandle* white_token_family; /* Family auth token */
    struct Process* owner_process;      /* Process that owns this channel */
    
    /* Resource associations */
    struct {
        struct PCIChannelBarResource* bars[6]; /* Up to 8 BAR resources */
        struct PCIChannelIrqResource* irqs[8]; /* Up to 8 IRQ resources */
        struct PCIChannelDmaResource* dmas[4]; /* Up to 4 DMA resources */
        int bar_count, irq_count, dma_count;
    } resources;
    
    /* Channel state */
    enum ChannelState state;            /* Current channel state */
    uint64_t created_at;               /* Creation timestamp */
    uint64_t last_operation;          /* Last operation timestamp */
    uint64_t operation_count;          /* Number of operations performed */
    
    /* Transaction support */
    struct {
        bool config_transaction_active;
        uint64_t current_transaction_id;
        void* transaction_state;        /* Transaction-specific state */
    } transaction_state;
    
    /* BAR mapping context */
    struct {
        uintptr bar_addresses[6];      /* Virtual addresses for mapped BARs */
        size_t bar_sizes[6];           /* Size of each BAR mapping */
        bool bar_mapped[6];            /* Whether each BAR is mapped */
    } mapping_ctx;
    
    /* Driver context */
    void* driver_context;             /* Driver-specific context */
    
    /* Statistics */
    struct {
        uint64_t reads, writes;
        uint64_t config_space_access;
        uint64_t bar_access;
        uint64_t irq_notifications;
    } stats;
    
    /* Linking (for channel management) */
    struct PCIChannel* next;
    struct PCIChannel* prev;
};

/* PCI channel manager */
struct PCIChannelManager {
    struct PCIChannel* channels[MAX_CHANNELS_PER_PCI_FAMILY];
    ChannelID_64 channel_ids[MAX_CHANNELS_PER_PCI_FAMILY];  /* Fast lookup by ID */
    struct PCIChannel* free_channels; /* Free list for allocation */
    
    uint64_t next_channel_id;           /* Next channel ID to allocate */
    uint32_t max_channels;             /* Maximum configurable channels */
    uint32_t channel_count;            /* Number of allocated channels */
    
    Lock channel_lock;                  /* Protect channel pool */
    
    /* Persistent naming */
    struct {
        struct PersistentName* names[MAX_PERSISTENT_NAMES];
        int name_count;
    } name_registry;
    
    /* Allocation strategies */
    int (*allocate_by_address)(struct PCIDeviceDescriptor* dev, uint64_t* channel_id);
    int (*allocate_by_name)(char* device_name, uint64_t* channel_id);
    int (*allocate_auto)(struct PCIDeviceDescriptor* dev, void* criteria, uint64_t* channel_id);
} pci_channel_allocators[4];
};

/* Persistent device names */
struct PersistentName {
    char name[PERSISTENT_NAME_LENGTH];
    uint64_t channel_id;
    struct PCIDeviceDescriptor* bound_device;  /* Reference to bound device */
    uint64_t last_seen;                /* Last time device was seen */
    bool currently_active;          /* Whether device is present */
    struct PersistentName* next;
};

/* Generate 64-bit channel ID */
static uint64_t
generate_pci_channel_id(void)
{
    static uint64_t next_id = 1;  /* IDs start from 1 */
    uint64_t new_id;
    
    lock(&global_channel_id_lock);
    
    new_id = next_id++;
    
    if (new_id == 1) {  /* Rollback avoidance */
        new_id = next_id++;
        next_id++;
    }
    
    unlock(&global_channel_id_lock);
    
    return new_id;
}

/* Find PCI channel by ID (O(1) lookup) */
static struct PCIChannel*
lookup_pci_channel_by_id(struct PCIChannelManager* mgr, uint64_t channel_id)
{
    if (!mgr || channel_id == 0 || channel_id >= mgr->max_channels) {
        return NULL;
    }
    
    if (mgr->channel_ids[channel_id] == NULL) {
        return NULL;  // Not found
    }
    
    return mgr->channel_ids[channel_id];
}

/* Find free channel */
static struct PCIChannel*
allocate_pci_channel_struct(struct FamilyExchangePage* family, uint64_t channel_id, 
                           struct PCIDeviceDescriptor* device, uint32_t permissions)
{
    struct PCIEChannelManager* mgr = (struct PCIEChannelManager*)family->channel_mgr;
    
    if (!mgr || channel_id == 0 || channel_id >= mgr->max_channels) {
        return NULL;
    }
    
    lock(&mgr->channel_lock);
    
    /* Check if channel ID is available */
    if (mgr->channel_ids[channel_id] != NULL) {
        unlock(&mgr->channel_lock);
        return NULL;  // Channel ID already in use
    }
    
    /* Allocate channel structure from free pool */
    struct PCIChannel* channel = NULL;
    if (mgr->free_channels) {
        channel = mgr->free_channels;
        mgr->free_channels = channel->next;
    } else {
        /* No free channels, allocate new one */
        channel = xalloc(sizeof(struct PCIChannel));
        if (!channel) {
            unlock(&mgr->channel_lock);
            return NULL;
        }
    }
    
    /* Initialize channel */
    memset(channel, 0, sizeof(struct PCIChannel));
    channel->channel_id = channel_id;
    channel->family = family;
    channel->bound_device = device;
    channel->pci_address = device->address;
    channel->permissions_mask = permissions;
    channel->owner_process = current_process;
    channel->state = CHANNEL_ACTIVE;
    channel->created_at = now;
    channel->last_operation = now;
    
    /* Set default name */
    snprint(channel->channel_name, sizeof(channel->channel_name), 
            "pci_%04x_%02x_%02x.%x_ch%lu",
            device->address.domain, device->address.bus, 
            device->address.device, device->address.function, channel_id);
    
    /* Bind to device */
    device->bound_channel_id = channel_id;
    device->bound_channel = channel;
    
    /* Add to channel table */
    mgr->channel_ids[channel_id] = channel;
    mgr->channel_count++;
    
    unlock(&mgr->channel_lock);
    
    return channel;
}

/* Release PCI channel structure */
static void
free_pci_channel_struct(struct PCIChannel* channel)
{
    struct PCIEChannelManager* mgr = (struct PCIEChannelManager*)channel->family->channel_mgr;
    
    if (!mgr || !channel) {
        return;
    }
    
    lock(&mgr->channel_lock);
    
    /* Unbind from device */
    if (channel->bound_device) {
        channel->bound_device->bound_channel_id = 0;
        channel->bound_device->bound_channel = NULL;
    }
    
    /* Clear channel table entry */
    if (channel->channel_id < mgr->max_channels) {
        mgr->channel_ids[channel->channel_id] = NULL;
    }
    
    mgr->channel_count--;
    
    /* Add back to free pool */
    channel->next = mgr->free_channels;
    mgr->free_channels = channel;
    
    unlock(&mgr->channel_lock);
}

/* Validate channel permissions */
static int
validate_pci_channel_permissions(uint32_t req_perms, struct PCIDeviceDescriptor* dev)
{
    uint32_t device_flags = device_can_access_flags(dev);
    
    /* Check if requested permissions are a subset of device permissions */
    if ((req_perms & ~device_flags) != 0) {
        return -1;  // Permission denied
    }
    
    /* Check if device is in appropriate state for operation */
    if (dev->device_state == DEVICE_ERROR || dev->device_state == DEVICE_REMOVED) {
        return -2;  // Device not available
    }
    
    return 0; /* OK */
}

/* Device access capability flags */
uint32_t
device_can_access_flags(struct PCIDeviceDescriptor* dev)
{
    uint32_t flags = 0;
    
    if (!dev) {
        return flags;
    }
    
    /* All devices can read config space */
    flags |= CHANNEL_PERM_READ_CONFIG;
    
    /* Check if device allows config space writes */
    if (dev->class_code != 0x00) {  // Not a PCI bridge or other restricted class
        flags |= CHANNEL_PERM_WRITE_CONFIG;
    }
    
    /* Check if device has BARs for memory mapping */
    for (int i = 0; i < 6; i++) {
        if (dev->bars[i].is_valid && dev->bars[i].type == 0) {  // Memory BAR
            flags |= CHANNEL_PERM_MAP_BAR;
            break;  // One BAR is sufficient for basic access
        }
    }
    
    /* Check if device supports MSI */
    if (dev->capabilities.has_msi) {
        flags |= CHANNEL_PERM_ALLOC_IRQ;
    }
    
    return flags;
}

/* Allocate channel by device address */
static int
pci_allocate_channel_by_address(struct FamilyExchangePage* family, void* device_id, 
                                  uint32_t permissions, uint64_t* channel_id)
{
    struct PCIEChannelManager* mgr = (struct PCIEChannelManager*)family->channel_mgr;
    struct PCIDeviceDescriptor* dev = NULL;
    
    /* Find device by address */
    if (pci_discover_device(family, device_id, (void**)&dev) != 0) {
        return -2;  // Device not found
    }
    
    /* Validate device access */
    if (validate_pci_channel_permissions(permissions, dev) != 0) {
        return -3;  // Permission denied
    }
    
    /* Allocate channel */
    uint64_t new_channel_id = generate_pci_channel_id();
    struct PCIChannel* channel = allocate_pci_channel_struct(family, new_channel_id, dev, permissions);
    if (!channel) {
        return -4;  // Allocation failed
    }
    
    *channel_id = new_channel_id;
    return 0;
}

/* Allocate channel by persistent name */
static int
pci_allocate_channel_by_name(struct FamilyExchangePage* family, char* device_name, 
                              uint32_t permissions, uint64_t* channel_id)
{
    struct PCIEChannelManager* mgr = (struct PCIEChannelManager*)family->channel_manager;
    
    /* Look up device by name */
    struct PersistentName* name_entry = NULL;
    for (int i = 0; i < mgr->name_registry.name_count; i++) {
        if (strcmp(mgr->name_registry.names[i]->name, device_name) == 0) {
            name_entry = mgr->name_registry.names[i];
            break;
        }
    }
    
    if (!name_entry || !name_entry->currently_active) {
        return -2;  // Name not found or device not active
    }
    
    struct PCIDeviceDescriptor* dev = name_entry->bound_device;
    
    /* Re-associate current device if needed */
    if (dev && dev->bound_channel_id == 0) {
        /* Device lost its channel, need to check if it's still valid */
        if (dev->device_state != DEVICE_ACTIVE) {
            return -3;  // Device is not active anymore
        }
    }
    
    /* Validate device access */
    if (validate_pci_channel_permissions(permissions, dev) != 0) {
        return -4;  // Permission denied
    }
    
    /* If device has existing channel, return it */
    if (dev->bound_channel_id != 0) {
        *channel_id = dev->bound_channel_id;
        return 0;  // Reuse existing channel
    }
    
    /* Allocate new channel */
    uint64_t new_channel_id = generate_pci_channel_id();
    struct PCIChannel* channel = allocate_pci_channel_struct(family, new_channel_id, dev, permissions);
    if (!channel) {
        return -5;  // Allocation failed
    }
    
    /* Update persistent name entry */
    name_entry->channel_id = new_channel_id;
    
    *channel_id = new_channel_id;
    return 0;
}

/* Auto-allocate best matching device */
static int
pci_allocate_channel_auto(struct FamilyExchangePage* family, void* device_criteria, 
                            uint32_t permissions, uint64_t* channel_id)
{
    struct PCIDeviceCriteria* criteria = (struct PCIDeviceCriteria*)device_criteria;
    struct PCIEChannelManager* mgr = (struct PCIEChannelManager*)family->channel_manager;
    struct PCIDeviceDescriptor* best_device = NULL;
    int best_score = -1;
    
    /* Scan all devices and find best match */
    for (int i = 0; i < MAX_PCI_DEVICES; i++) {
        struct PCIDeviceDescriptor* dev = NULL;
        
        /* Find device in registry */
        for (int j = 0; j < MAX_PCI_DEVICES; j++) {
            if (mgr->family_ctx->device_registry.devices[j] && 
                mgr->family_ctx->device_registry.devices[j]->address.domain_bus_dev_func == i) {
                dev = mgr->family_ctx->device_registry.devices[j];
                break;
            }
        }
        
        if (!dev || dev->bound_channel_id != 0) {
            continue;  // Device not found or already bound
        }
        
        /* Check if device matches criteria */
        int score = 0;
        
        /* Check class code match */
        if (criteria->class_code != 0xFF && dev->class_code == criteria->class_code) {
            score += criteria->class_match_weight;
        }
        
        /* Check vendor/device match */
        if (criteria->vendor_id != 0xFFFF && dev->vendor_id == criteria->vendor_id) {
            score += criteria->vendor_match_weight;
        }
        if (criteria->device_id != 0xFFFF && dev->device_id == criteria->device_id) {
            score += criteria->device_match_weight;
        }
        
        /* Check capabilities match */
        if (criteria->has_pcie && dev->capabilities.has_pcie) {
            score += criteria->capability_match_weight;
        }
        if (criteria->has_msi && dev->capabilities.has_msi) {
            score += criteria->capability_match_weight;
        }
        
        /* Check if device is available */
        if (validate_pci_channel_permissions(permissions, dev) == 0 && score > best_score) {
            best_device = dev;
            best_score = score;
        }
    }
    
    if (best_score < 0) {
        return -2;  // No matching device found
    }
    
    /* Allocate channel for best device */
    uint64_t new_channel_id = generate_pci_channel_id();
    struct PCIChannel* channel = allocate_pci_channel_struct(family, new_channel_id, best_device, permissions);
    if (!channel) {
        return -3;  // Allocation failed
    }
    
    *channel_id = new_channel_id;
    return 0;
}

/* Release channel and cleanup resources */
static int
pci_release_channel(struct FamilyExchangePage* family, uint64_t channel_id)
{
    struct PCIChannel* channel = lookup_pci_channel_by_id(family->channel_mgr, channel_id);
    
    if (!channel) {
        return -2;  // Channel not found
    }
    
    /* Validate permissions */
    if (!validate_channel_operation_permission(current_process(), channel)) {
        return -3;  // Permission denied
    }
    
    /* Clean up channel resources */
    for (int i = 0; i < 6; i++) {
        if (channel->resources.bars[i]) {
            cleanup_pci_bar_resource(channel->resources.bars[i]);
            free(channel->resources.bars[i]);
            channel->resources.bars[i] = NULL;
        }
    }
    
    for (int i = 0; i < 8; i++) {
        if (channel->resources.irqs[i]) {
            cleanup_pci_irq_resource(channel->resources.irqs[i]);
            free(channel->resources.irqs[i]);
            channel->resources.irqs[i] = NULL;
        }
    }
    
    for (int i = 0; i < 4; i++) {
        if (channel->resources.dmas[i]) {
            cleanup_pci_dma_resource(channel->resources.dmas[i]);
            free(channel->resources.dmas[i]);
            channel->resources.dmas[i] = NULL;
        }
    }
    
    /* Free channel structure */
    free_pci_channel_struct(channel);
    
    return 0;
}

/* Initialize PCI channel manager */
void
setup_pci_channel_manager(struct FamilyExchangePage* family)
{
    struct PCIEChannelManager* mgr = xalloc(sizeof(struct PCIEChannelManager));
    if (!mgr) {
        return;
    }
    
    memset(mgr, 0, sizeof(struct PCIEChannelManager));
    
    mgr->max_channels = MAX_CHANNELS_PER_PCI_FAMILY;
    mgr->next_channel_id = 1;
    
    /* Initialize channel pool and lookup table */
    mgr->channels = xalloc(sizeof(struct PCIChannel) * mgr->max_channels);
    mgr->channel_ids = xalloc(sizeof(struct PCIChannel*) * mgr->max_channels);
    
    if (!mgr->channels || !mgr->channel_ids) {
        if (mgr->channels) xfree(mgr->channels);
        if (mgr->channel_ids) xfree(mgr->channel_ids);
        xfree(mgr);
        return;
    }
    
    memset(mgr->channels, 0, sizeof(struct PCIChannel) * mgr->max_channels);
    memset(mgr->channel_ids, 0, sizeof(struct PCIChannel*) * mgr->max_channels);
    
    /* Setup allocation strategies */
    mgr->allocate_by_address = pci_allocate_channel_by_address;
    mgr->allocate_by_name = pci_allocate_channel_by_name;
    mgr->allocate_auto = pci_allocate_channel_auto;
    
    lock_init(&mgr->channel_lock);
    
    family->channel_mgr = (struct ChannelManager*)mgr;
    
    print("PCI: channel manager initialized (max_channels=%d)\n", mgr->max_channels);
}

/* Configure channel limits */
static int
pci_configure_channel_limits(struct FamilyExchangePage* family)
{
    struct PCIEChannelManager* mgr = (struct PCIEChannelManager*)family->channel_manager;
    
    if (!mgr) {
        return -1;
    }
    
    /* Dynamic limit adjustment based on system resources */
    uint32_t max_mem_channels = get_max_channels_for_memory(mgr->max_channels);
    uint32_t max_dev_channels = get_max_channels_for_device_count(mgr->family_ctx->device_registry.total_devices);
    uint32_t hard_limit = mgr->max_channels;
    
    uint32_t new_limit = min(min(max_mem_channels, max_dev_channels), hard_limit);
    
    if (new_limit != mgr->max_channels) {
        lock(&mgr->channel_lock);
        
        /* Check if current usage exceeds new limit */
        if (mgr->channel_count > new_limit) {
            print("PCI ERROR: current channel count %d exceeds new limit %d\n",
                   mgr->channel_count, new_limit);
            unlock(&mgr->channel_lock);
            return -1;
        }
        
        mgr->max_channels = new_limit;
        
        unlock(&mgr->channel_lock);
        
        print("PCI: adjusted channel limits to %d\n", new_limit);
    }
    
    return 0;
}

/* Allocate BAR resource for PCI channel */
static int
pci_channel_allocate_bar_resource(struct PCIChannel* channel, uint8_t bar_num, 
                                 uint32_t size, uintptr* virtual_addr)
{
    if (!channel || !channel->bound_device || bar_num >= 6) {
        return -1;
    }
    
    struct PCIDeviceDescriptor* dev = channel->bound_device;
    struct PCIChannelBarResource* bar_res = channel->resources.bars[bar_num];
    
    /* Check if BAR is valid */
    if (!dev->bars[bar_num].is_valid) {
        return -2;  // BAR not available
    }
    
    /* Check if BAR size is sufficient */
    if (size > dev->bars[bar_num].size) {
        return -3;  // Requested size exceeds BAR size
    }
    
    /* Check permissions */
    if (!(channel->permissions_mask & CHANNEL_PERM_MAP_BAR)) {
        return -4;  // Permission denied
    }
    
    /* Allocate exchange page for BAR memory */
    ExchangeHandle bar_handle;
    int result = exchange_prepare_pages(dev->bars[bar_num].base_address, 
                                 dev->bars[bar_num].size, &bar_handle, 1);
    if (result != 0) {
        return -5;  // Exchange page allocation failed
    }
    
    /* Map the exchange page to userspace */
    uintptr virt_addr;
    result = exchange_accept(bar_handle, (uintptr)*virtual_addr, 
                              PROT_READ|PROT_WRITE, channel->white_token);
    if (result != 0) {
        exchange_cancel(bar_handle);
        return -6;  // Mapping failed
    }
    
    /* Record allocation */
    if (!bar_res) {
        bar_res = xalloc(sizeof(struct PCIChannelBarResource));
        if (!bar_res) {
            exchange_cancel(bar_handle);
            return -7;
        }
    }
    
    bar_res->bar_number = bar_num;
    bar_res->exchange_handle = bar_handle;
    bar_res->virtual_address = virt_addr;
    bar_res->mapped_size = size;
    bar_res->is_mapped = true;
    bar_res->device = dev;
    
    /* Add to channel resource list */
    channel->resources.bars[bar_num] = bar_res;
    channel->resources.bar_count++;
    
    /* Update channel mapping context */
    channel->mapping_ctx.bar_addresses[bar_num] = virt_addr;
    channel->mapping_ctx.bar_sizes[bar_num] = size;
    channel->mapping_ctx.bar_mapped[bar_num] = true;
    channel->last_operation = now();
    channel->stats.bar_access++;
    
    if (virtual_addr) {
        *virtual_addr = virt_addr;
    }
    
    print("PCI: channel %d BAR%d mapped at %p (size=%d)\n",
           channel->channel_id, bar_num, virt_addr, size);
    
    return 0;
}

/* Unmap and release BAR resource */
static int
pci_channel_release_bar_resource(struct PCIChannel* channel, uint8_t bar_num)
{
    if (!channel || bar_num >= 6) {
        return -1;
    }
    
    struct PCIChannelBarResource* bar_res = channel->resources.bars[bar_num];
    
    if (!bar_res) {
        return -2;  // BAR resource not allocated
    }
    
    /* Unmap virtual address */
    if (bar_res->is_mapped) {
        exchange_unmap(bar_res->exchange_handle);
        channel->mapping_ctx.bar_addresses[bar_num] = 0;
        channel->mapping_ctx.bar_sizes[bar_num] = 0;
        channel->mapping_ctx.bar_mapped[bar_num] = false;
    }
    
    /* Cancel exchange page */
    exchange_cancel(bar_res->exchange_handle);
    
    /* Remove from resource list */
    channel->resources.bars[bar_num] = NULL;
    channel->resources.bar_count--;
    
    channel->last_operation = now();
    channel->stats.bar_access++;
    
    free(bar_res);
    
    return 0;
}

/* Get BAR resource info */
static int
pci_channel_get_bar_info(struct PCIChannel* channel, uint8_t bar_num, void* info_buffer, size_t* info_size)
{
    if (!channel || !channel->bound_device || bar_num >= 6) {
        return -1;
    }
    
    struct PCIChannelBarResource* bar_res = channel->resources.bars[bar_num];
    
    if (!bar_res) {
        return -2;  // BAR not allocated
    }
    
    struct PCIBarInfo* info = (struct PCIBarInfo*)info_buffer;
    
    info->bar_number = bar_num;
    info->base_address = channel->bound_device->bars[bar_num].base_address;
    info->size = channel->bound_device->bars[bar_num].size;
    info->type = channel->bound_device->bars[bar_num].type;
    info->is_64bit = channel->bound_device->bars[bar_num].is_64bit;
    info->is_valid = channel->bound_device->bars[bar_num].is_valid;
    
    info->virtual_address = bar_res->virtual_address;
    info->is_mapped = bar_res->is_mapped;
    info->mapped_size = bar_res->mapped_size;
    
    *info_size = sizeof(struct PCIBarInfo);
    return 0;
}

/* Initialize channel allocation strategies */
void
setup_pci_channel_allocators(struct PCIEChannelManager* mgr)
{
    /* Initialize all allocation strategies */
    mgr->allocate_by_address = pci_allocate_channel_by_address;
    mgr->allocate_by_name = pci_allocate_channel_by_name;
    mgr->allocate_auto = pci_allocate_channel_auto;
}

/* Update channel statistics */
static void
update_pci_channel_stats(struct PCIChannel* channel)
{
    if (!channel) {
        return;
    }
    
    channel->stats.operation_count++;
    channel->last_operation = now();
}

/* Channel statistics and debugging */
void
pci_print_channel_info(uint64_t channel_id)
{
    struct FamilyExchangePage* family = global_pci_family;
    if (!family) {
        print("PCI: PCI family not available\n");
        return;
    }
    
    struct PCIChannel* channel = lookup_pci_channel_by_id(family->channel_mgr, channel_id);
    
    if (!channel) {
        print("PCI: Channel %lld not found\n", (long long)channel_id);
        return;
    }
    
    print("PCI Channel Info:\n");
    print("  Channel ID:      %ldd\n", (long long)channel->channel_id);
    print("  Name:          %s\n", channel->channel_name);
    print("  State:         %s\n", channel_state_to_string(channel->state));
    print("  Permissions:    0x%08x\n", channel->permissions_mask);
    print("  Device:        %04x:%02x:%02x.%x\n", 
           channel->bound_device->address.domain,
           channel->bound_device->address.bus,
           channel->bound_device->address.device,
           channel->bound_device->address.function);
    print("  Owner:         pid %d\n", channel->owner_process ? channel->owner_process->pid : 0);
    
    print("  Resources:\n");
    for (int i = 0; i < 6; i++) {
        if (channel->resources.bars[i]) {
            print("    BAR%d: %p -> %p (size=%d)\n", i, 
                   (void*)channel->resources.bars[i]->base_address,
                   (void*)channel->resources.bars[i]->virtual_address,
                   channel->resources.bars[i]->mapped_size);
        }
    }
    
    for (int i = 0; i < 8; i++) {
        if (channel->resources.irqs[i]) {
            print("    IRQ%d: line %d\n", i, channel->resources.irqs[i]->irq_number);
        }
    }
    
    for (int i = 0; i < 4; i++) {
        if (channel->resources.dmas[i]) {
            print("    DMA%d: %p (size=%d)\n", i,
                   (void*)channel->resources.dmas[i]->physical_address,
                   channel->resources.dmas[i]->size);
        }
    }
    
    print("  Statistics:\n");
    print("  Operations:    %ld\n", channel->stats.operation_count);
    print("  Config Access:  %ld\n", channel->stats.config_space_access);
    print("  BAR Access:    %ld\n", channel->stats.bar_access);
    print("  IRQ Notified: %ld\n", channel->stats.irq_notifications);
    
    print("  Timing:\n");
    print("  Created:       %T\n", channel->created_at);
    print("  Last Op:       %T\n", channel->last_operation);
}