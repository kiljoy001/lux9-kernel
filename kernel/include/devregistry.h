/*
 * Device registry for microkernel architecture
 * Tracks all devices in the system regardless of where their drivers run
 */

#ifndef _DEVREGISTRY_H_
#define _DEVREGISTRY_H_

#include "u.h"
/* pci.h should be included through dat.h */

/* Device types */
typedef enum {
    DEVTYPE_UNKNOWN = 0,
    DEVTYPE_PCI,
    DEVTYPE_USB,
    DEVTYPE_PLATFORM,
    DEVTYPE_VIRTUAL,
    DEVTYPE_MAX
} DevType;

/* Device states */
typedef enum {
    DEVSTATE_UNKNOWN = 0,
    DEVSTATE_PRESENT,      /* Device detected */
    DEVSTATE_CONFIGURED,   /* Device configured */
    DEVSTATE_ONLINE,       /* Device driver connected */
    DEVSTATE_OFFLINE,      /* Device driver disconnected */
    DEVSTATE_ERROR,        /* Device in error state */
    DEVSTATE_MAX
} DevState;

/* Forward declaration */
typedef struct Pcidev Pcidev;

/* Device registration structure */
typedef struct Device Device;
struct Device {
    /* Identity */
    ulong id;              /* Unique device ID */
    DevType type;          /* Device type */
    DevState state;        /* Current state */
    
    /* Device information */
    char name[32];         /* Device name (e.g., "0000:00:1f.2") */
    char description[128]; /* Human-readable description */
    ulong vendor_id;       /* Vendor ID (PCI VID, USB VID, etc.) */
    ulong device_id;       /* Device ID (PCI DID, USB PID, etc.) */
    
    /* Device location */
    union {
        struct {
            int busno;
            int devno; 
            int funcno;
            Pcidev *pcidev;    /* Pointer to PCI device structure */
        } pci;
        struct {
            int busno;
            int devno;
            int funcno;
        } usb;
        struct {
            ulong phys_addr;
            ulong size;
        } platform;
    } location;
    
    /* Driver interface */
    char driver_name[32];  /* Name of driver handling this device */
    int driver_pid;        /* PID of driver process (0 if kernel) */
    ulong capabilities;    /* Capabilities provided by this device */
    
    /* Registry bookkeeping */
    ulong created;         /* Creation timestamp */
    ulong last_seen;       /* Last seen timestamp */
    Device *next;          /* Next device in registry */
    Device *prev;          /* Previous device in registry */
};

/* Device registry functions */
void devregistry_init(void);
Device* devregistry_register_pci(Pcidev *pcidev);
Device* devregistry_register_usb(int busno, int devno, int funcno, ulong vid, ulong pid);
Device* devregistry_register_platform(ulong phys_addr, ulong size, ulong vid, ulong pid, char *name);
Device* devregistry_find_by_id(ulong id);
Device* devregistry_find_by_pci(int busno, int devno, int funcno);
Device* devregistry_find_by_name(char *name);
Device* devregistry_find_by_driver(char *driver_name);
int devregistry_unregister(Device *dev);
int devregistry_update_state(Device *dev, DevState new_state);
void devregistry_list(void (*print_func)(char*, ...));

/* Device matching and filtering */
typedef int (*dev_match_fn)(Device *dev, void *arg);
Device* devregistry_find_match(dev_match_fn match_func, void *arg);
int devregistry_count_by_type(DevType type);
int devregistry_count_by_state(DevState state);

/* Capability management */
int devregistry_has_capability(Device *dev, ulong capability);
void devregistry_add_capability(Device *dev, ulong capability);
void devregistry_remove_capability(Device *dev, ulong capability);

/* Driver registration */
int devregistry_register_driver(Device *dev, char *driver_name, int driver_pid);
int devregistry_unregister_driver(Device *dev);

#endif /* _DEVREGISTRY_H_ */