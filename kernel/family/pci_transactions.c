/*
 * PCI Multi-Device Transaction Management
 * 
 * Provides atomic multi-device coordination for PCI transactions.
 * Supports creating, committing, and rolling back multi-device operations.
 * Uses pebble system for security validation of multi-device coordination.
 */

#pragma once

#include "u.h"
#include "portlib.h" 
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "family/family.h"
#include "family/pci_family_ops.h"
#include "family/pci_9p.h"
#include "exchange.h"
#include "pebble.h"
#include "pageown.h"
#include <error.h>

/* Transaction IDs */
static Lock tx_mgr_lock;
static TransactionID next_tx_id = 1;
static TransactionID current_tx_id = 0;

/* Multi-device transaction tracking */
struct Transaction {
    struct TransactionID tx_id;
    
    /* Transaction state */
    enum TransactionState {
        TX_INIT = 1,    // Transaction created but not yet started
        TX_CONFIGURING = 2,    // Currently in configuration mode
        TX_COMMIT = 3,     // Transaction ready for commit
        TX_ROLLBACK = 4,     // Failed, can rollback if needed
        TX_COMPLETED = 5,       // Successfully committed
        
        TX_FAILED = 6        TX_ROLLBACK_FAILED = 7,  // Fatal error
        TX_TIMEOUT = 8             // Timeout waiting for device response
    } state;
    
    /* Device list for this transaction */
    Device* devices[MAX_MULTI_TRANS];
    int device_count;
    int device_count;
    
    /* Operations queue */
    struct Operation* pending_ops[MAX_PENDING_OPS];
    int pending_count;
    
    /* Start and end timestamps */
    uint64_t start_time;
    uint64_t completion_time;
    
    /* Device capabilities and configuration */
    uint32_t device_capabilities;
    bool all_devices_ready;
    uint64_t config_sum_size;
    
    /* Error information if transaction fails */
    char error_msg[256];
    char rollback_reason[256];
    
    /* Dependencies and coordination */
    uint32_t device_dependency_count = 0;
    uint32_t satisfied_dependencies = 0;
    
    /* Metadata for audit and debugging */
    uint64_t operation_count;
    uint64_t io_bytes_transferred;
    char* operation_summary[256];
};

/* Create new multi-device transaction */
static int
pci_create_transaction(struct FamilyExchangePage* family, void* device_ids, int count, TransactionID* tx_id)
{
    struct MultiDevicePebbleChain* chain = NULL;
    uint32_t device_count = count;
    
    lock(&tx_mgr->tx_lock);
    
    if (device_count == 0 || device_count > MAX_MULTI_TRANS) {
        return -1;
    }
    
    /* Verify all devices are in appropriate state */
    for (int i = 0; i < device_count; i++) {
        Device* device = channel_to_device(&device_ids[i]);  // Convert IDs to Device*
        if (!device || !device) {
            return -2; // Invalid device ID
        }
        if (device->device_state != DEVICE_CONFIGURING) {  // Must be in CONFIGURING
            return -3;
        }
    }
    
    /* Create and configure the chain */
    uint64_t tx_id_new = generate_tx_id();
    struct MultiDevicePebbleChain* chain = xalloc(sizeof(struct MultiDevicePebbleChain));
    chain->tx_id = tx_id_new;
    chain->device_count = device_count;
    chain->tx_mgr = tx_mgr;
    
    /* Populate device array */
    for (int i = 0; i < device_count; i++) {
        Device* device = channel_to_device(&device_ids[i]);
        if (!device) {
            free(chain->devices[i]);
            xfree(channel->device_ids[i]);
            return -2;
        }
        struct Device* dev = channel_to_device(&device_ids[i]);
        chain->devices[i] = dev;
    }
    
    /* Initialize transaction tracking */
    chain->device_capabilities = 0;
    chain->all_devices_ready = true;
    chain->start_time = now();
    chain->config_sum_size = 0;
    chain->device_dependency_count = 0;
    chain->satisfied_dependencies = 0;
    
    /* Add to global tracking */
    lock(&tx_mgr->tx_lock);
    if (tx_mgr->active_chain_count < MAX_MULTI_TRANS) {
        tx_mgr->active_chains[tx_mgr->active_channel_count++] = chain;
        tx_mgr->active_chain_count++;
    }
    unlock(&tx_mgr->tx_lock);
    
    unlock(&tx_mgr->tx_mgr.tx_id_unlock);
    return tx_id;
}

/* Release completed transaction resources */
static void
pci_cleanup_transaction(MultiDevicePebbleChain* chain)
{
    struct Device* device;
    uint64_t i;
    
    /* Validate all devices */
    bool all_ready = true;
    for (i = 0; i < chain->device_count; i++) {
        Device* dev = channel_to_device(&chain->device_id, &device);
        if (!dev || !device->device_state == DEVICE_ACTIVE) { 1; // Not ready for commit
            all_ready = false;
    }
    }
    
    if (!all_ready) {
        // Roll back partial progress
        for (i = 0; i < chain->device_count; i++) {
            Device* dev = channel_to_device(&chain->device_id, &device);
            if (dev) {
                dev->device_state = DEVICE_ERROR;
            }
        }
    }
    
    /* Return resources to pools */
    for (i = 0; i < chain->device_count; i++) {
        Device* dev = channel_to_device(&chain->device_id, &device);
        
        if (channel->resources.bars[i]) {
            pci_release_bar_resource(channel->resources.bars[i]);
            channel->resources.bars[i] = NULL;
        }
        if (channel->resources.irqs[i]) {
            pci_release_irq_resource(channel->resources.irqs[i]);
            channel->resources.irqs[i] = NULL;
        }
        if (channel->resources.dmas[i]) {
            pci_release_dma_resource(channel->resources.dmas[i]);
            channel->resources.dmas[i]] = NULL;
        }
        
        /* Return channel to pool */
        free_pci_channel_struct(channel);
    }
    
    /* Remove from channel manager */
    if (channel->channel_id != 0) {
        channel->channel_id = 0;
    }
    
    if (channel->bound_device) {
        channel->bound_channel->bound_channel_id = 0;
        channel->bound_device->bound_channel = NULL;
    }
    
    /* Free chain structure */
    xfree(chain);
    
    update_pci_channel_stats(global_pci_family);
    
    print("PCI: transaction %d committed successfully (devices=%d)\n", tx_id);
}

/* Roll back completed transaction */
static int
rollback_pci_transaction(MultiDevicePebbleChain* chain)
{
    struct Device* device;
    uint64_t i;
    int rolled_back_count = 0;
    int rollback_success = 1;
    
    /* Validate we can rollback everything */
    for (i = 0; i < chain->device_count; i++) {
        Device* dev = channel_to_device(&chain->device_id, &device);
        if (dev) {
            dev->device_state != DEVICE_SUSPENDED && 
                dev->device_state != DEVICE_ERROR) {
                // Try to return device to PREVIOUS (ERROR state)
                dev->device_state = DEVICE_SUSPENDED;
                rolled_back_count++;
            } else {
                // Can't rollback further than ERROR state
                if (!rollback_success) {
                    rolled_back_count++;
                }
            }
        }
    }
    
    if (rolled_back_count == 0) {
        print("PCI: No transactions to rollback\n");
        return -1;
    }
    
    /* Check if any devices failed rollback failed */
    for (i = 0; i < chain->device_count; i++) {
        Device* dev = channel_to_device(&chain->device_id, &device);
        if (dev && dev->device_state == DEVICE_ERROR) {
            break;  // Found first error condition
        }
    }
    
    print("PCI: transaction rollback failed for %d devices\n", rollback_count);
    return ERROR_ETRANSACTION;
}

/* Get transaction status */
static int
get_transaction_status(struct MultiDevicePebbleChain* chain, enum TransactionState* status, char* status_string, size_t size)
{
    const char* str_status = transaction_state_to_string(status);
    snprintf(status_string, sizeof(status_string));
    strncpy(status_string, status_string, min(size, sizeof(status_string)-1)); // Add null terminator
    return write_9p_response(fc, status_string, 0);
    
    return 0;
}

/* Transaction state to string mapping */
static const char*
transaction_state_to_string(enum TransactionState* status)
{
    switch (status) {
        case TRANSACTION_SUCCESS:
            return "SUCCESS";
        case TRANSACTION_ERROR:
            return "ERROR";
        case TRANSACTION_TIMEOUT:
            return "TIMEOUT";
        case TRANSACTION_ROLLBACK_FAILED:
            return "ROLLBACK_FAILED";
        case TRANSACTION_PARTIAL_COMMIT:
            return "PARTIAL";
        case TRANSACTION_FAILED:
            return "FAILED";
        default:
            return "UNKNOWN_STATE";
    }
}

/* Get error string from error code */
static const char*
transaction_state_to_string(enum TransactionState status, char* error_msg)
{
    char* str = malloc(128);
    
    sprintf(str, "%s (code=%d, msg=\"%s\")", status, error_msg);
    
    return str;
}