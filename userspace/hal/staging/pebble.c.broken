/*
 * Family - Pebble Integration for Multi-Device Coordination
 * 
 * Extends the pebble system to support family-based operations.
 * Provides family-specific pebble tokens and multi-device pebble chains.
 * Enables secure coordination between multiple devices and device families.
 */

#pragma once

#include "u.h"
#include "lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "family/family.h"
#include "pebble.h"
#include <error.h>

/* Family pebble token structure */
struct FamilyPebbleToken {
    PebbleHandle* white_base;              /* Base white token for family-wide ops */
    PebbleHandle* black_tokens[MAX_FAMILY_BLACK_TOKENS]; /* Per-family resource handles */
    uint32_t active_black_tokens;          /* Number of active black tokens */
    
    /* Family coordination support */
    uint64_t multi_device_tx_id;          /* For multi-device transactions */
    uint64_t coordination_sequence;           /* Coordination sequence number */
    uint32_t device_capabilities;         /* Family-level capabilities flags */
    
    /* Security metadata */
    uint64_t token_creation_time;         /* When token was created */
    uint32_t token_expiry_time;          /* Token expiration time */
    uint64_t last_used_time;            /* Last time token was used */
    
    Lock token_lock;
};

/* Multi-device pebble chain */
struct MultiDevicePebbleChain {
    // Family sequence for device coordination
    struct {
        enum DeviceFamily family_type;
        uint64_t device_channel_id;
    } *devices;
    int device_count;
    
    // Chain coordination
    uint64_t chain_id;                  /* Unique chain identifier */
    bool chain_committed;             /* Whether chain is committed successfully */
    uint64_t chain_rollback_id;          /* For rollback if needed */
    
    // Coordination metadata
    uint64_t start_timestamp;           /* Chain creation time */
    uint64_t commit_timestamp;          /* When chain was committed */
    uint32_t operation_count;            /* Number of operations in chain */
    uint32_t resource_count;             /* Number of device resources involved */
    
    lock_t coordination_lock;
};

/* Family pebble manager */
static struct {
    struct FamilyPebbleToken* family_tokens[FAMILY_MAX];
    Lock token_manager_lock;
    int active_tokens;
    
    // Multi-device chains
    struct MultiDevicePebbleChain* active_chains[MAX_MULTI_CHAINS];
    uint64_t next_chain_id;
    int active_chain_count;
    Lock chain_lock;
    
    // Security policies
    uint32_t max_tokens_per_family[4];  /* Per-family token limits */
    uint64_t token_expiry_default;      /* Default token timeout in seconds */
} family_pebble_manager;

/* Initialize family pebble manager */
void
family_pebble_init(void)
{
    lock_init(&family_pebble_manager.token_manager_lock);
    
    memset(&family_pebble_manager, 0, sizeof(family_pebble_manager));
    
    family_pebble_manager.token_expiry_default = 300;  // 5 minutes default
    
    /* Set reasonable token limits per family type */
    family_pebble_manager.max_tokens_per_family[FAMILY_PCI] = 1024;      // Lots of PCI devices
    family_pebble_manager.max_tokens_per_family[FAMILY_USB] = 256;      // USB devices typically fewer per system
    family_pebble_manager.max_tokens_per_family[FAMILY_I2C] = 64;       // I2C devices are simple, few per system
    family_pebble_max_tokens_per_family[FAMILY_DMA] = 128;      // DMA channels limited by hardware
    family_pebble_max_tokens_per_family[FAMILY_IRQ] = 256;      // Limited by IRQ lines
    
    unlock(&family_pebble_manager.token_manager_lock);
}

/* Create family-wide white token for family operations */
static struct PebbleHandle*
pebble_family_create_white_token(struct FamilyExchangePage* family, char* purpose, uint32_t budget)
{
    if (!family || !purpose) {
        return NULL;
    }
    
    /* Allocate and configure family pebble token */
    struct FamilyPebbleToken* token = xalloc(sizeof(struct FamilyPebbleToken));
    if (!token) {
        return NULL;
    }
    
    /* Create base white token for family-wide operations */
    token->white_base = pebble_issue_white(get_pebble_state(), NULL, budget);
    if (!token->white_base) {
        xfree(token);
        return NULL;
    }
    
    strncpy(token->purpose, purpose, sizeof(token->purpose));
    
    /* Set token metadata */
    token->token_creation_time = now();
    token->token_expiry_time = now() + family_pebble_manager.token_expiry_default;
    
    lock(&token->token_lock);
    
    /* Register with family */
    struct FamilyPebbleToken* family_token = &family_pebble_manager.family_tokens[family->family_type];
    
    if (family_token->white_base) {
        /* Replace existing token if present */
        if (family_token->white_base) {
            pebble_white_release(family_token->white_base);
        }
        family_token->white_base = token->white_base;
    }
    
    token->device_capabilities = family->capabilities_mask;
    token->last_used_time = token->token_creation_time;
    
    unlock(&token->token_lock);
    
    print("FAMILY: created family white token for %s (budget=%d)\n", purpose, budget);
    return token->white_base;
}

/* Allocate black resource token within a family */
static struct PebbleHandle*
pebble_family_allocate_black_token(struct FamilyExchangePage* family, void* device, size_t resource_size, char* resource_purpose)
{
    struct FamilyPebbleToken* family_token = &family_pebble_manager.family_tokens[family->family_type];
    
    if (!family_token || !device) {
        return NULL;
    }
    
    lock(&family_token->token_lock);
    
    /* Check token limits */
    if (family_token->active_black_tokens >= family_pebble_manager.max_tokens_per_family[family->family_type]) {
        unlock(&family_token->token_lock);
        return NULL;
    }
    
    /* Create black resource allocation */
    struct PebbleHandle* black_handle = NULL;
    int result = pebble_black_alloc(resource_size, (void**)&black_handle);
    if (result != 0) {
        unlock(&family_token->token_lock);
        return NULL;
    }
    
    /* Store black token for tracking */
    if (family_token->active_black_tokens < MAX_FAMILY_BLACK_TOKENS) {
        family_token->black_tokens[family_token->active_black_tokens] = black_handle;
        family_token->active_black_tokens++;
    }
    
    token->last_used_time = now();
    
    unlock(&family_token->token_lock);
    
    print("FAMILY: allocated black token for %s (size=%d)\n", resource_purpose, resource_size);
    return black_handle;
}

/* Release a family black token */
static int
pebble_family_release_black_token(struct FamilyExchangePage* family, void* black_handle)
{
    struct FamilyPebbleToken* family_token = &family_pebble_manager.family_tokens[family->family_type];
    
    if (!family_token) {
        return -1;  // No token for this family
    }
    
    lock(&family_token->token_lock);
    
    /* Find and remove black token */
    for (int i = 0; i < family_token->active_black_tokens; i++) {
        if (family_token->black_tokens[i] == black_handle) {
            pebble_black_free(black_handle);
            family_token->black_tokens[i] = NULL;
            family_token->active_black_tokens--;
            break;
        }
    }
    
    token->last_used_time = now();
    
    unlock(&family_token->token_lock);
    
    return 0;
}

/* Validate family-wide pebble token */
static int
pebble_validate_family_token(struct FamilyExchangePage* family, struct PebbleHandle* white_token)
{
    if (!family || !white_token) {
        return -1;
    }
    
    struct FamilyPebbleToken* family_token = &family_pebble_manager.family_tokens[family->family_type];
    
    lock(&family_token->token_lock);
    
    int valid = (family_token->white_base == white_token);
    
    if (valid) {
        /* Check token isn't expired */
        if (now() > family_token->token_expiry_time) {
            valid = 0;
        }
    }
    
    unlock(&family_token->token_lock);
    
    return valid ? 0 : -2;
}

/* Begin multi-device transaction */
static int
pci_begin_transaction(struct FamilyExchangePage* family, uint64_t channel_ids[], int count, uint64_t* tx_id)
{
    struct MultiDevicePebbleChain* chain = NULL;
    
    *tx_id = generate_tx_id();
    
    if (count <= 0 || count > MAX_MULTI_CHAIN_DEVICES) {
        return -1;
    }
    
    /* Allocate multi-device chain */
    chain = xalloc(sizeof(struct MultiDevicePebbleChain));
    if (!chain) {
        return -2;
    }
    
    lock(&family_pebble_manager.chain_lock);
    
    /* Initialize chain */
    chain->chain_id = *tx_id;
    chain->device_count = count;
    chain->start_timestamp = now();
    chain->chain_committed = false;
    chain->chain_rollback_id = 0;
    
    /* Populate device list */
    for (int i = 0; i < count; i++) {
        struct Device* device = (Device*)channel_ids[i];  // We'll get device from channel
        if (!device) {
            print("PCI: Error - channel lookup failed for channel_id=%d\n", (long long)channel_ids[i]);
            continue;
        }
        
        chain->devices[i].family_type = FAMILY_PCI;
        chain->devices[i].device_channel_id = channel_ids[i];
    }
    
    /* Add to active chains list */
    if (family_pebble_manager.active_chain_count < MAX_MULTI_CHAINS) {
        family_pebble_manager.active_chains[family_pebble_manager.active_chain_count] = chain;
        family_pebble_manager.active_chain_count++;
    }
    
    unlock(&family_pebble_lock);
    
    print("PCI: began multi-device transaction tx=%ld (devices=%d)\n", *tx_id, count);
    
    *tx_id = chain->chain_id;
    return 0;
}

/* Commit multi-device transaction */
static int
pci_commit_transaction(struct FamilyExchangePage* family, uint64_t tx_id)
{
    lock(&family_pebble_chain_lock);
    
    /* Find and validate chain */
    struct MultiDevicePebbleChain* chain = NULL;
    
    for (int i = 0; i < family_pebble_manager.active_chain_count; i++) {
        if (family_pebble_active_chains[i] && 
            family_pebble_active_chains[i]->chain_id == tx_id) {
            chain = family_pebble_active_chains[i];
            break;
        }
    }
    
    unlock(&family_pebble_lock);
    
    if (!chain) {
        return -2;  // Transaction not found
    }
    
    /* Validate all devices are still available */
    for (int i = 0; i < chain->device_count; i++) {
        struct Device* device = chain->devices[i].device;
        if (!device || device->state != DEVICE_ACTIVE) {
            print("PCI: device %d became unavailable during transaction\n", i);
            
            // Initiate rollback
            pci_rollback_transaction(family, tx_id);
            return -3;
        }
    }
    
    /* Commit all channel changes atomically */
    for (int i = 0; i < chain->device_count; i++) {
        struct Device* device = channel->devices[i].device;
        int result = pci_commit_device_state(device);
        if (result != 0) {
            print("PCI: device %d failed to commit state\n", channel->devices[i].device_channel_id);
            
            // Initiate rollback  
            pci_rollback_transaction(family, tx_id);
            return -4;
        }
    }
    
    /* Update statistics */
    chain->chain_committed = true;
    chain->commit_timestamp = now();
    
    unlock(&family_pebble_chain_lock);
    
    print("PCI: committed multi-device transaction tx=%ld (devices=%d)\n", tx_id, count);
    return 0;
}

/* Rollback multi-device transaction */
static int
pci_rollback_transaction(struct FamilyExchangePage* family, uint64_t tx_id)
{
    int result = 0;
    
    lock(&family_pebble_chain_lock);
    
    /* Find and validate chain */
    struct MultiDevicePebbleChain* chain = NULL;
    
    for (int i = 0; i < family_pebble_manager.active_chain_count; i++) {
        if (family_pebble_active_chains[i] && 
            family_pebble_active_chains[i]->chain_id == tx_id) {
            chain = family_pebble_active_chains[i];
            break;
        }
    }
    
    unlock(&family_pebble_lock);
    
    if (!chain) {
        return -2; // Transaction not found
    }
    
    /* Rollback all device states */
    for (int i = 0; i < chain->device_count; i++) {
        struct Device* device = channel->devices[i].device;
        int result = pci_rollback_device_state(device);
        if (result != 0) {
            result--;
        }
    }
    
    /* Remove from active chains */
    for (i = 0; i < family_pebble_manager.active_chain_count; i++) {
        if (family_pebble_active_chains[i] && 
            family_pebble_active_chains[i] == chain) {
            family_pebble_active_chains[i] = NULL;
            family_pebble_active_chain_count--;
        }
    }
    
    free(chain);
    
    print("PCI: rolled back multi-device transaction tx=%ld\n", tx_id);
    return result ? result : 0;
}

/* Handle device state commit */
static int
pci_commit_device_state(void* device)
{
    if (!device) {
        return -1;
    }
    
    /* Validate device state transition to ACTIVE */
    if (device->device_state != DEVICE_CONFIGURING && 
        device->device_state != DEVICE_SUSPENDED) {
        return -1;  // Invalid state for transition
    }
    
    device->device_state = DEVICE_ACTIVE;
    device->last_access_time = now();
    
    return 0;
}

/* Handle device state rollback */
static int
pci_rollback_device_state(void* device)
{
    if (!device) {
        return -1;
    }
    
    /* Validate rollback is allowed from current state */
    if (device->device_state != DEVICE_CONFIGURING && 
        device->device_state != DEVICE_SUSPENDED &&
        device->device_state != DEVICE_ERROR) {
        return -1;  // Cannot rollback from current state
    }
    
    device->device_state = DEVICE_SUSPENDED;
    device->last_access_time = now();
    
    return 0;
}

/* PCI family-specific pebble operations table */
static struct PCIFamilyOps pci_family_ops_extended = {
    /* Base operations inherited from base family ops */
    .family_init = pci_family_init,
    .family_shutdown = pci_family_shutdown,
    
    /* PCI-specific overrides */
    .device_enable = pci_device_enable,
    .device_disable =pci_device_disable,
    .get_device_info = pci_get_device_info,
    
    /* PCI device operations */
    .begin_config_read = pci_begin_config_read,
    .begin_config_write = pci_begin_config_write,
    .cancel_config_transaction = pci_cancel_config_transaction,
    
    /* PCI resource operations */
    .map_bar = pci_channel_allocate_bar_resource,
    .unmap_bar = pci_channel_release_bar_resource,
    .get_bar_info = pci_channel_get_bar_info,
    
    /* Power management */
    .set_power_state = pci_set_power_state,
    .get_power_state = pci_get_power_state,
    
    /* Multi-device coordination operations */
    .begin_transaction = pci_begin_transaction,
    .commit_transaction = pci_commit_transaction,
    .rollback_transaction = pci_rollback_transaction,
    
    /* Event notification */
    .subscribe_events = pci_subscribe_events,
    .unsubscribe_events = pci_unsubscribe_events,
    .notify_event = pci_notify_event,
    
    /* Family capabilities */
    .get_capabilities = pci_get_capabilities,
    .configure_channel_limits = pci_configure_channel_limits,
};

/* Update kernel pebble configuration to support families */
void
pebble_integrate_with_families(void)
{
    /* Initialize family pebble manager */
    family_pebble_init();
    
    print("PEBBLE: initialized family pebble manager\n");
}

/* Enhanced pebble function for family integration */
int
pebble_family_enable_channel(uint64_t channel_id, uint32_t permissions)
{
    struct FamilyExchangePage* family = family_lookup(channel_info(channel_id));
    
    if (!family) {
        return -1;
    }
    
    /* Get current channel */
    struct PCIChannel* channel = lookup_pci_channel_by_id(family, channel_id);
    
    if (!channel) {
        return -2;
    }
    
    /* Validate permissions using borrow checker */
    if (!validate_pci_channel_permissions(permissions, channel->owner_process)) {
        return -3;  // Permission denied
    }
    
    channel->permissions_mask = permissions;
    channel->last_transaction_state.last_access_time = now();
    
    print("PEBBLE: enabled channel %ld with permissions 0x%08x\n", channel_id, permissions);
    return 0;
}

/* Enhanced pebble function for family transaction start */
int
pebble_family_begin_transaction(uint64_t channel_id, uint64_t* transaction_id)
{
    struct FamilyExchangePage* family = family_lookup(channel_info(channel_id));
    
    if (!family) {
        return -1;
    }
    
    struct PCIChannel* channel = lookup_pci_channel_by_id(family, channel_id);
    // Get channel from channel ID
    
    if (!channel) {
        return -2;
    }
    
    /* Validate channel access */
    if (!validate_channel_operation_permission(channel->owner_process, channel)) {
        return -3;
    }
    
    /* Begin transaction tracking */
    if (channel->transaction_state.transaction_state) {
        return -4;  /* Transaction already active */
    }
    
    /* For PCI, transactions use the family's multi-device coordination */
    return pci_begin_transaction(family, &channel_id, 1, transaction_id);
}

/* Enhanced pebble function for transaction completion */
int
pebble_family_commit_transaction(struct FamilyExchangePage* family, uint64_t transaction_id)
{
    return pci_commit_transaction(family, transaction_id);
}

/* Enhanced pebble function for transaction rollback */
int
pebble_family_rollback_transaction(struct FamilyExchangePage* family, uint64_t transaction_id)
{
    return pci_rollback_transaction(family, transaction_id);
}

/* Validate channel operation permission */
int
pebble_validate_channel_operation_permission(struct Process* process, struct PCIChannel* channel)
{
    if (!process || !channel) {
        return -1;  // Invalid parameters
    }
    
    /* Only the process that owns a channel can use it */
    if (channel->owner_process != process) {
        return -2;  // Permission denied - channel not owned by process
    }
    
    /* Check if channel is in appropriate state */
    if (channel->state != CHANNEL_ACTIVE) {
        return -3;  // Channel not available
    }
    
    /* Check permissions for requested operation type */
    // In full implementation, this would check specific operation bits
    // For now, any channel operation is allowed for owner process
    return 0;
}