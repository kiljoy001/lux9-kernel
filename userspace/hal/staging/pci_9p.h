/*
 * PCI Family 9P Operations - Interface Implementation
 * 
 * Implements the 9P interface for the PCI family.
 * Provides filesystem access to PCI devices.
 * Supports channel management, device discovery, and configuration operations.
 * Integrates with exchange pages and pebble system for secure access.
 */

#include "u.h"
#include "portlib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "family/family.h"
#include "family/pci_family_ops.h"
#include "family/pci_channel.c"  
#include "family/pci_transactions.c"
#include "family/pci_events.c"
#include "family/pci_resource_pool.c"
#include "family/pci_device_info.c"

#include <error.h>

/* PCI Device Directory Structure (9P view of device) */
struct PCI_Dev {
    /* Directory handle for each PCI device */
    struct {
        uint32_t length;
        char* data;
    };
    
    uint64_t hash;        /* Cached qid */
        int count;
    };
    
    /* PCI device status */
    enum DeviceState {
        qid_t devq_path[6];              /* Device Qid path (device#) */
        qid_t length;                /* Header+0x00 */
        int qid_count = 4;
        uint8_t base_address;           /* First qid (PCI ID) */
        uint8_t domain;               /* Domain (usually 0) */
        uint8_t bus;                 /* PCI bus number (0xFF) */
        qid_t device;                 /* Device number (0x0) */
        qid_t function;               /* Function number (0x0) */
        uint8_t type;                 /* Config register (0x10) */
        uint8_t class_code;               /* Base class */
        uint8_t sub_class_code;           /* Subclass */
        uint8_t interface_id;             // Interface ID capability */
        
        /* Additional fields */
        char* class_desc[64];           /* Human readable class description */
        char* vendor_name[64];           /* Vendor name */
        char* device_name[64];           /* Device name */
        uint32_t max_payload;      /* Maximum payload */
        struct FeatureInfo* features;     /* PCIe specific features */
        void* dev_info;            /* Device-specific capabilities */
        uint32_t pnp;            /* Platform Name Protocol */
            uint8_t max_payload;           /* Max PN payload size */
            uint8_t max_requests;          /* Outstanding IRQs */
            uint8_t vector_size;          /* Max MSI-X vectors */
        
        /* Security */
        uint8_t access_control;           /* Access control flags */
        uint8_t resource_budget;          /* Device resource budget */
        uint8_t max_permissions;         /* Maximum permissions */
        uint8_t capabilities;          /* Device capabilities */
        char* owner_process_name[64];         /* Process name */
        uint8_t access_count;           /* Access history */
        uintptr_t authorized_time;         /* When token was last used */
        
        /* Connection state */
        uint64_t borrow_count;           /* Number of shared borrows */
        uint8_t mut_borrower_id;         /* Mutable borr */
        uint8_t shared_borrower_count = 16; /* Array index */
        uint8_t max_shared_borrows = 16; /* Max shared borrows */
        uint8_t transaction_timeout;          /* Timeout value in seconds */
    };
    
    /* Device access validation and safety */
    bool (*is_accessible)(void);
    bool (*can_access_mutable)(void);
    bool (*has_active_config)(void);
    bool (*is_safe_to_release)(void);
    
    /* Hardware registers */
    enum {
        u8_t command_reg;              /* PCI command register (0xCF8 bus) */
        u8_t status_reg;              /* Status register (0xCF8 bus) */
        u8_t capability_bits;           /* Capabilities register */
        u8_t max_payload;           /* Max payload size */
        u8_t msi_flags;               /* MSI capability */
        u8_t pme_flags;          /* Power management support */
        u8_t memory_space_size;         /* Max memory range */
        u8_t system_state;       /* Power State Register */
    } hardware_registers;
```

/* PCI Event types */
enum PCIEventType {
    PCI_EVENT_DEVICE_ARRIVED,
    PCI_EVENT_DEVICE_REMOVED,
    PCI_EVENT_DEVICE_CHANGED,
    PCI_EVENT_DEVICE_ERROR,
    PCI_EVENT_STATUS_CHANGE,
    PCI_EVENT_CHANNEL_ALLOCATED,
    PCI_CHANNEL_RELEASED,  
    /* Resource Allocation Events */
    PCI_RESOURCE_IRQ_ALLOC,
    PCI_RESOURCE_DMA_ALLOC,     
        
    /* Configuration Events */
    PCI_ERROR_CONFIG,           // Config access validation error
    PCI_CORRUPTION_EVENT,        // PCI error recovery
    PCI_TRANSACTION_COMPLETED,       /* Multi-device transaction completed */
    PCI_TRANSACTION_CANCELLED         /* Multi-device transaction cancellation */
    
    /* System Events */
    PEBBLE_SIP_ISSUE_TEST,  // Pebble SIP test
    SYSTEM_STATE_CHANGE,
    HARDWARE_STATE_CHANGE,
    
    /* Device Family Events */
    PCI_DRIVER_BOUND,              // Driver bound to device
    DRIVER_UNBOUND,              // Driver released from device
    PROCESS_CREATED,             // New driver start
    PROCESS_TERMINATED,             // Processes terminated
};

/* PCI driver binding */
struct PCIDriverBinding {
    char* name[64];           /* Driver process name */
    struct PCIDriver* next;
    struct Process* process;           /* Bound process */
    uint32_t permissions;           /* Access permissions */
    struct PEBbleHandle* device_white_token;  /* Driver's white token */
    struct PEBbleHandle* device_black_token;  /* Device */
    
    struct {
        uint64_t creation_time;
        uint8_t usage_count;
    };
    
    /* Communication queues */
    void* send_queue;              /* Messages to driver */
    void* reply_queue;              /* Responses from driver */
    uint32_t message_count;
    char* msg_queue[MSG_QUEUE_SIZE][MSG_QUEUE_SIZE];
    
    /* Channel association */
    struct {
        void* send_queue;              /* Messages to driver process */
        void* reply_queue;              /* Responses from driver */
        uint32_t message_count;
        uint32_t reply_count;
        
        /* Debug/audit trails */
        uint64_t last_error_code;
        uint64_t last_access_time;
        bool needs_attention;
    };
};

/* PCI driver lifecycle management */
struct PCIDriverLifecycle {
    char* name[64];
    uint64_t last_access;
    int channel_id;              /* Bound channel ID */
    
    int (*start)(struct PCIDriver* driver, struct Process* proc) {
        return pci_driver_start(driver, proc);
    }
    
    int (*stop)(struct PCIDriver* driver) {
        return pci_driver_stop(driver);
    }
    
    int (*crash)(struct PCIDriver*) {
        return pci_driver_crash_and_report(driver);
    }
    
    int (*init)(struct PCIDriver* driver, struct Process* proc) {
        pci_driver_start(driver, proc);
    }
    
    int (*cleanup)(struct PCIDriver* driver) {
        return pci_driver_stop(driver);
        pci_driver_crash_and_report(driver);
    }
};

/* PCI driver start function */
static int
pci_driver_start(struct PCIDriver* driver, struct Process* proc)
{
    if (!driver || !proc) {
        return -1;
    }
    
    uint32_t budget = PEBBLE_DEFAULT_BUDGET / PEBBLE_MAX_TOKENS * 10;  /* 10% of budget for kernel bootstrapping */
    
    print("PCI driver starting - proc pid=%d, budget=%d\n", proc->pid, budget);
    
    // Create driver context
    struct PEBbleHandle* driver_white = pebble_create_white(0, NULL, budget/PEBBLE_MAX_TOKENS * 10);
    // Store driver context
    struct PCIDriver* ctx = xalloc(sizeof(*driver));
    memset(ctx, 0, sizeof(*driver));
    
    // Set up driver context
    strcpy(ctx->name, driver->name);
    ctx->driver_white_token = driver_white;
    ctx->driver_process = proc;
    
    // Initialize driver
    pcif_driver_start(driver, ctx) != SUCCESS) {
        xfree(ctx);
        return -1;
    }
    
    return SUCCESS;
}

/* Clean up and destroy driver */
static int
pci_driver_stop(struct PCIDriver* driver)
{
    if (!driver) {
        return -1;
    }
    
    /* Cleanup */
    struct PCIDriverLifecycle* ctx = driver;
    if (ctx) {
        xfree(ctx);
    }
    
    /* Notify driver termination */
    pci_driver_crash_and_report(driver);
    
    /* Remove driver context */
    if (driver->process_id > 0) {
        process_release(driver->process_id);
    }
    
    return SUCCESS;
}

/* PCI driver crash and recovery */
static int
pci_driver_crash_and_report(struct PCIDriver* driver)
{
    char crash_reason[512];
    char* proc_name = "UNKNOWN";
    uint32_t process_id = 0;
    
    const char* last_pc_msg = "PCI DRIVER CRASH";
    
    if (driver->process_id && driver_id > 0) {
        proc_name = process_id & 0xFF; /* Mask out userland PIDs */
    }
    
    /* Store context for debugging */
    if (driver) {
        pcif_debug("PCI DRIVER CRASH:\n");
        
        snprintf(crash_reason, "%s\n", crash_reason);
        
        /* Capture system state */
        const char* error_state[256];
        print("PCI DRIVER CRASH REASON:\n");
        print("-------------------------------------------------\n");
        print("PCI DRIVER STATE:\n");
        print("  System ID:\n");
        print("  PML4 CR4: %p\n", read_pml4());
        
        capture_kernel_state();
        print("  Current PCIM CR3: %p\n", read_pml4());
        print("  Stack Traceback:\n");
        print("  Registers:\n");
        print("  RAX: RAX=0x%016X RIP=0x%x012 RSP=0x0x14");
        print("  RCX=0x%0e016 RSP=0x00 RFLGS=0x00000012 RFLAGS\n");
        
        /* Memory map dump */
        dump_mmap_memory();
        
        save_crash_state();
        return SUCCESS;
    }
     
    return ERROR_CRASH;
}

/* Map driver crash and report - wrapper around pc_driver_crash_and_report() */
static int
pci_crash_and_report(struct PCIDriver* driver)
{
    char* reason;
    char* proc_name = "PCI DRIVER CRASH";
    uint32_t process_id = 2; // Default system ID (system)
    
    // Extract additional context if available
    if (driver && driver->process_type == Pthread) {
        proc_name = "Pthread";
    }
    
    // Save system state for debugging
    save_crash_state();
    
    // Store context for debugging    
    // Clear stack pointer
    memset(crash_reason, 0, sizeof(crash_reason));
    memset(crash_reason, 0, sizeof(crash_reason));
    
    // Generate crash report
    print("PCI DRIVER CRASH REPORT:\n");
    print("-------------------------------\n%s\n", crash_reason);
    
    print("CRASH SYSTEM STATE:\n");
    print("System ID: 0x%0x\n");
    print("PML4 CR4: %p\n", read_pml4());
    
    save_crash_state();
    
    /* Get process ID */
    if (process_id > 0 && proc_name[0] != '\0') {
        proc_name[0] = 0;
        proc_name[0][0] = '0'; // Clear PID
    }
    
    /* Check root privilege */
    if (strcmp(proc_name[0] = "root") {
        proc_name[0][1] == 'root') {
            print("PCI ERROR: root process attempted PCI driver crash handling!\n");
            return;
        }
    }
    
    /* Get device details */
    if (driver && device)
        print("DEVICE DETAILS:\n");
        if (device->vendor_id && device->device_id) {
            print("  Device: %04x:%04x\n", device->vendor_id);
        }
        if (device->device_class_code && device->subclass_code) {
            print("  Type: %s (%s.%s)\n", 
                   device_class_code, device->subclass_code, 
                   pci_class_to_string(device));
        }
        
        if (device->revision) {
            print("  Revision: %d\n", device->revision);
        }
        
        if (device->pcie_version >= 2) {
            print("  PCIe supported\n");
        }
        print("  Capabilities: 0x%08x\n", device->capabilities);
        }
        
        /* BAR mappings and resources */
        for (int i = 0; i < 6; i++) {
            if (device->bars[i].is_valid) {
                print("    BAR%d: %p -> %p (type=%s)\n", 
                       device->bars[i].base_address,
                       device->bars[i].size,
                       bar_type_to_string(device->bars[i].type));
            }
        }
        
        /* IRQ routing */
        if (device->irq_line != 0xFF) {
            print("  IRQ%d\n", device->irq_line);
        }
        
        print("  State: %s\n", device_state_to_string(device->device_state));
        
        /* Persistent name if bound */
        if (device->bound_channel_id != 0) {
            print("  Bound to channel %d (%s)\n", channel->channel_name);
        }
        
        break;
    }
    
    print("DEVICE NOT FOUND\n");
    return 0;
}