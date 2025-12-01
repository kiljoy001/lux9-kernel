/*
 * Family Base Implementation
 * 
 * Core implementation of the family abstraction layer.
 * Handles family registration, lookup, and basic family management.
 */

#include "u.h"
#include "portlib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "family/family.h"
#include <error.h>

/* Global family registry */
struct FamilyRegistry family_registry;
Lock family_registry_lock;
Lock global_channel_id_lock;
static uint64_t global_next_channel_id;

/* Global stats */
static Lock global_stats_lock;
static struct ChannelStats global_channel_stats;

/* static void lock_init(Lock* l) { memset(l, 0, sizeof(Lock)); } - Moved to stubs.c */

/* Optional subsystem stub implementations */
void
init_resource_pool(struct FamilyExchangePage* family)
{
    /* Resource pool initialization handled by family-specific code */
    family->resource_pool = nil;
}

void
init_event_system(struct FamilyExchangePage* family)
{
    /* Event system initialization handled by family-specific code */
    family->event_system = nil;
}

void
init_transaction_manager(struct FamilyExchangePage* family)
{
    /* Transaction manager initialization handled by family-specific code */
    family->tx_mgr = nil;
}

void
cleanup_family_resources(struct FamilyExchangePage* family)
{
    /* Cleanup handled by family-specific shutdown */
    if(family){
        family->resource_pool = nil;
        family->event_system = nil;
        family->tx_mgr = nil;
    }
}

/* Initialize family system */
void
family_init(void)
{
    memset(&family_registry.registry_lock, 0, sizeof(Lock));
    memset(&global_channel_id_lock, 0, sizeof(Lock));
    memset(&global_stats_lock, 0, sizeof(Lock));

    memset(&global_channel_stats, 0, sizeof(global_channel_stats));

    lock(&family_registry.registry_lock);
    // Initialize all family slots as NULL
    for (int i = 0; i < FAMILY_MAX; i++) {
        family_registry.families[i] = NULL;
    }
    
    family_registry.family_count = 0;
    unlock(&family_registry.registry_lock);
    
    /* Initialize global channel ID generator */
    lock(&global_channel_id_lock);
    global_next_channel_id = 1;  /* Start from channel ID 1 */
    unlock(&global_channel_id_lock);
}

/* Register a new family with the system */
int
family_register(enum DeviceFamily family_type, struct FamilyOps* ops, char* name)
{
    if (family_type >= FAMILY_MAX || !name || !ops) {
        return -1;  // Invalid parameters
    }
    
    /* Validate family type */
    switch (family_type) {
        case FAMILY_PCI:
            if (strcmp(name, "PCI") != 0) {
                return -1;
            }
            break;
        case FAMILY_USB:
        case FAMILY_I2C:
            return -1;  /* Not implemented yet */
        default:
            return -1;  // Invalid family type
    }
    
    /* Check if family already registered */
    if (family_registry.families[family_type] != NULL) {
        return -2;  // Family already registered
    }
    
    /* Allocate family structure */
    struct FamilyExchangePage* family = xalloc(sizeof(struct FamilyExchangePage));
    if (!family) {
        return -3;
    }
    
    memset(family, 0, sizeof(struct FamilyExchangePage));
    
    /* Initialize family */
    family->family_type = family_type;
    family->family_version = 1;
    snprint(family->family_name, sizeof(family->family_name), "%s", name);
    
    /* Set base capabilities */
    family->capabilities_mask = FAMILY_CAP_MULTIPLE_CHANNELS;
    
    /* Initialize channel manager */
    family->max_channels = DEFAULT_CHANNELS_PER_FAMILY;
    family->next_channel_id = 1;
    
    /* Initialize resource pool */
    init_resource_pool(family);
    init_event_system(family);
    init_transaction_manager(family);
    
    /* Set family operations */
    family->ops = ops;
    
    /* Call family-specific initialization */
    if (ops->family_init != NULL) {
        int result = ops->family_init(family);
        if (result != 0) {
            xfree(family);
            return result;
        }
    }
    
    /* Configure channel limits */
    if (ops->configure_channel_limits) {
        ops->configure_channel_limits(family, family->max_channels);
    }
    
    /* Add to registry */
    lock(&family_registry.registry_lock);
    family_registry.families[family_type] = family;
    family_registry.family_count++;
    
    print("%s: registered %s family\n", family->family_name, name);
    unlock(&family_registry.registry_lock);
    
    return 0;
}

/* Unregister a family */
int
family_unregister(enum DeviceFamily family_type)
{
    if (family_type >= FAMILY_MAX) {
        return -1;  // Invalid family type
    }
    
    lock(&family_registry.registry_lock);
    
    struct FamilyExchangePage* family = family_registry.families[family_type];
    if (!family) {
        unlock(&family_registry.registry_lock);
        return -2;  // Family not registered
    }
    
    /* Call family cleanup */
    if (family->ops && family->ops->family_shutdown) {
        family->ops->family_shutdown(family);
    }
    
    /* Free family resources */
    cleanup_family_resources(family);
    xfree(family);
    
    family_registry.families[family_type] = NULL;
    family_registry.family_count--;
    
    print("%s: unregistered %s family\n", family_type_to_string(family_type), family_type_to_string(family_type));
    
    unlock(&family_registry.registry_lock);
    
    return 0;
}

/* Look up a family by type */
struct FamilyExchangePage*
family_lookup(enum DeviceFamily family_type)
{
    if (family_type >= FAMILY_MAX) {
        return NULL;  // Invalid family type
    }

    lock(&family_registry.registry_lock);

    struct FamilyExchangePage* family = family_registry.families[family_type];

    unlock(&family_registry.registry_lock);

    return family;
}

/* Generate 64-bit channel ID for system-wide uniqueness */
uint64_t
generate_channel_id(void)
{
    uint64_t id;
    
    lock(&global_channel_id_lock);
    
    id = global_next_channel_id++;
    
    unlock(&global_channel_id_lock);
    
    return id;
}

/* Validate channel access permissions */
int
validate_channel_permissions(uint32_t requested, uint32_t granted)
{
    if (requested == 0)
        return -1;  // Invalid permissions
    if ((requested & ~granted) != 0)
        return -1;  // Request exceeds granted mask
    return 0;
}

/* Update system-wide channel statistics */
void
update_channel_stats(struct FamilyExchangePage* family)
{
    /* Update total system statistics */
    struct ChannelStats* stats = &global_channel_stats;
    
    lock(&global_stats_lock);
    
    stats->total_channels_allocated += 1;
    
    if (family) {
        if (family->stats.active_channels > 0 && family->stats.peak_channels > stats->peak_channels)
            stats->peak_channels = family->stats.peak_channels;
        stats->total_memory_usage += family->stats.memory_usage_bytes;
    }
    
    unlock(&global_stats_lock);
}

/* Debugging and statistics */
void
family_stats(enum DeviceFamily family_type)
{
    struct FamilyExchangePage* family = family_lookup(family_type);
    
    print("Family Statistics - %s:\n", family_type_to_string(family_type));
    if (!family) {
        print("  Family not found\n");
        return;
    }
    
    print("  Channels: %d / %d (active/allocated)\n", 
           family->stats.active_channels, family->max_channels);
    
    print("  Memory Usage: %lld bytes\n", family->stats.memory_usage_bytes);
    print("  Operations: %lld total\n", family->stats.total_operations);
    print("  Errors: %lld\n", family->stats.error_count);
    print("  Peak Channels: %d\n", family->stats.peak_channels);
}

/* Helper function to handle global channel statistics */
void
setup_global_channel_stats(void)
{
    memset(&global_channel_stats, 0, sizeof(global_channel_stats));
    global_channel_stats.total_channels_allocated = 0;
    global_channel_stats.peak_channels = 0;
    global_channel_stats.total_memory_usage = 0;
    global_channel_stats.total_operations = 0;
    global_channel_stats.error_count = 0;
    global_channel_stats.peak_channels = 0;
}

/* Convert family type to string */
const char*
family_type_to_string(enum DeviceFamily type)
{
    switch(type){
    case FAMILY_NONE:
        return "NONE";
    case FAMILY_PCI:
        return "PCI";
    case FAMILY_USB:
        return "USB";
    case FAMILY_I2C:
        return "I2C";
    case FAMILY_SPI:
        return "SPI";
    case FAMILY_DMA:
        return "DMA";
    case FAMILY_IRQ:
        return "IRQ";
    default:
        return "UNKNOWN";
    }
}

/* Convert string to family type */
enum DeviceFamily
string_to_family_type(const char* name)
{
    if(!name)
        return FAMILY_NONE;
    if(strcmp(name, "PCI") == 0)
        return FAMILY_PCI;
    if(strcmp(name, "USB") == 0)
        return FAMILY_USB;
    if(strcmp(name, "I2C") == 0)
        return FAMILY_I2C;
    if(strcmp(name, "SPI") == 0)
        return FAMILY_SPI;
    if(strcmp(name, "DMA") == 0)
        return FAMILY_DMA;
    if(strcmp(name, "IRQ") == 0)
        return FAMILY_IRQ;
    return FAMILY_NONE;
}
