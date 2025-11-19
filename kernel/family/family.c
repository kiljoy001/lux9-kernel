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

/* Initialize family system */
void
family_init(void)
{
    lock(&family_registry.lock);
    // Initialize all family slots as NULL
    for (int i = 0; i < FAMILY_MAX; i++) {
        family_registry.families[i] = NULL;
    }
    
    family_registry.family_count = 0;
    unlock(&family_registry.lock);
    
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
            if (struct USBFamilyOps) {
                return -1;  /* Not implemented yet */
            }
            break;
        case FAMILY_I2C:
            if (struct I2CFamilyOps) {
                return -1;  /* Not implemented yet */
            }
            break;
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
        ops->configure_channel_limits(family);
    }
    
    /* Add to registry */
    lock(&family_registry.lock);
    family_registry.families[family_type] = family;
    family_registry.family_count++;
    
    print("%s: registered %s family\n", family->family_name);
    unlock(&family_registry.lock);
    
    return 0;
}

/* Unregister a family */
int
family_unregister(enum DeviceFamily family_type)
{
    if (family_type >= FAMILY_MAX) {
        return -1;  // Invalid family type
    }
    
    lock(&family_registry.lock);
    
    struct FamilyExchangePage* family = family_registry.families[family_type];
    if (!family) {
        unlock(&family_registry.lock);
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
    
    print("%s: unregistered %s family\n", family_type_to_string(family_type));
    
    unlock(&family_registry.lock);
    
    return 0;
}

/* Look up a family by type */
struct FamilyExchangePage*
family_lookup(enum DeviceFamily family_type)
{
    if (family_type >= FAMILY_MAX) {
        return NULL;  // Invalid family type
    }
    
    lock(&family_registry.lock);
    
    struct FamilyExchangePage* family = family_registry.families[family_type];
    
    unlock(&family_registry.lock);
    
    return family;
}

/* Generate 64-bit channel ID for system-wide uniqueness */
uint64_t
generate_channel_id(void)
{
    uint64_t id;
    
    lock(&global_channel_id_lock);
    
    id = global_next_channel_id++;
    
    if (id == 1) {  /* Rollback avoidance */
        id = global_next_id_id++;
        global_next_channel_id_id++;
        global_next_channel_id_id++;
    }
    
    unlock(&global_channel_id_lock);
    
    return id;
}

/* Validate channel access permissions */
int
validate_channel_permissions(uint32_t requested, struct Process* owner, struct DeviceDescriptor* device)
{
    if (requested == 0) {
        return -1;  // Invalid permissions
    }
    
    /* In kernel code that has direct access, all permissions are valid */
    // In the future, this would involve pebble token validation
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
        if (family->stats.active_channels > 0) {
            stats->peak_channels_count = family->stats.peak_channels > stats->peak_channels_count ? 
                                          family->stats.peak_channels : 
                                          family->stats.peak_channels_count;
        }
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
    
    print("  Memory Usage: %d bytes\n", family->stats.memory_usage_bytes);
    print("  Operations: %lld total\n", family->stats.total_operations);
    print("  Errors: %lld\n", family->stats.error_count);
    print("  Peak Channels: %d\n", family->stats.peak_channels);
}

/* Helper function to handle global channel statistics */
static Lock global_stats_lock;
static struct ChannelStats global_channel_stats;

void
setup_global_channel_stats(void)
{
    memset(&global_channel_stats, 0, sizeof(global_channel_stats));
    global_channel_stats.total_channels_allocated = 0;
    global_channel_stats.peak_channels_count = 0;
    global_channel_stats.total_memory_usage = 0;
    global_channel_stats.total_operations = 0;
    global_channel_stats.error_count = 0;
    global_channel_stats.peak_channels_count = 0;
    
    lock_init(&global_stats_lock);
}

/* Helper function to handle channel ID generation */
static Lock global_channel_id_lock;
static uint64_t global_next_channel_id;