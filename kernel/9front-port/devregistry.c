/*
 * Device registry implementation for microkernel architecture
 * Tracks all devices in the system regardless of where their drivers run
 */

#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "pci.h"
#include "devregistry.h"

/* Global device registry */
static struct {
    Lock lock;              /* Registry lock */
    Device *devices;        /* Linked list of devices */
    ulong next_id;          /* Next device ID */
    int device_count;       /* Total device count */
    int type_count[DEVTYPE_MAX];  /* Count by type */
    int state_count[DEVSTATE_MAX]; /* Count by state */
} devregistry;

/* Initialize the device registry */
void
devregistry_init(void)
{
    memset(&devregistry, 0, sizeof(devregistry));
    devregistry.next_id = 1;
    
    if (0) print("devregistry: initialized\n");
}

/* Generate unique device ID */
static ulong
devregistry_generate_id(void)
{
    return devregistry.next_id++;
}

/* Create a new device structure */
static Device*
devregistry_create_device(DevType type)
{
    Device *dev = malloc(sizeof(Device));
    if (dev == nil) {
        return nil;
    }
    
    memset(dev, 0, sizeof(Device));
    dev->id = devregistry_generate_id();
    dev->type = type;
    dev->state = DEVSTATE_PRESENT;
    dev->created = m->ticks;
    dev->last_seen = m->ticks;
    
    /* Update counts */
    devregistry.type_count[type]++;
    devregistry.state_count[DEVSTATE_PRESENT]++;
    devregistry.device_count++;
    
    return dev;
}

/* Register a PCI device */
Device*
devregistry_register_pci(Pcidev *pcidev)
{
    if (pcidev == nil) {
        return nil;
    }
    
    Device *dev = devregistry_create_device(DEVTYPE_PCI);
    if (dev == nil) {
        return nil;
    }
    
    /* Fill in PCI-specific information */
    dev->location.pci.busno = BUSBNO(pcidev->tbdf);
    dev->location.pci.devno = BUSDNO(pcidev->tbdf);
    dev->location.pci.funcno = BUSFNO(pcidev->tbdf);
    dev->location.pci.pcidev = pcidev;
    
    dev->vendor_id = pcidev->vid;
    dev->device_id = pcidev->did;
    
    /* Create device name */
    snprint(dev->name, sizeof(dev->name), "%04x:%02x:%02x.%d",
            0, dev->location.pci.busno, dev->location.pci.devno, dev->location.pci.funcno);
    
    /* Create description */
    snprint(dev->description, sizeof(dev->description), 
            "PCI device %04x:%04x", dev->vendor_id, dev->device_id);
    
    /* Add to registry */
    lock(&devregistry.lock);
    dev->next = devregistry.devices;
    if (devregistry.devices != nil) {
        devregistry.devices->prev = dev;
    }
    devregistry.devices = dev;
    unlock(&devregistry.lock);
    
    if (0) print("devregistry: registered PCI device %s (VID:%04x DID:%04x)\n",
          dev->name, dev->vendor_id, dev->device_id);
    
    return dev;
}

/* Register a USB device */
Device*
devregistry_register_usb(int busno, int devno, int funcno, ulong vid, ulong pid)
{
    Device *dev = devregistry_create_device(DEVTYPE_USB);
    if (dev == nil) {
        return nil;
    }
    
    /* Fill in USB-specific information */
    dev->location.usb.busno = busno;
    dev->location.usb.devno = devno;
    dev->location.usb.funcno = funcno;
    
    dev->vendor_id = vid;
    dev->device_id = pid;
    
    /* Create device name */
    snprint(dev->name, sizeof(dev->name), "usb-%d-%d.%d",
            busno, devno, funcno);
    
    /* Create description */
    snprint(dev->description, sizeof(dev->description), 
            "USB device %04x:%04x", vid, pid);
    
    /* Add to registry */
    lock(&devregistry.lock);
    dev->next = devregistry.devices;
    if (devregistry.devices != nil) {
        devregistry.devices->prev = dev;
    }
    devregistry.devices = dev;
    unlock(&devregistry.lock);
    
    if (0) print("devregistry: registered USB device %s (VID:%04x PID:%04x)\n",
          dev->name, vid, pid);
    
    return dev;
}

/* Register a platform device */
Device*
devregistry_register_platform(ulong phys_addr, ulong size, ulong vid, ulong pid, char *name)
{
    Device *dev = devregistry_create_device(DEVTYPE_PLATFORM);
    if (dev == nil) {
        return nil;
    }
    
    /* Fill in platform-specific information */
    dev->location.platform.phys_addr = phys_addr;
    dev->location.platform.size = size;
    
    dev->vendor_id = vid;
    dev->device_id = pid;
    
    /* Copy name */
    if (name != nil) {
        strncpy(dev->name, name, sizeof(dev->name) - 1);
        dev->name[sizeof(dev->name) - 1] = '\0';
    } else {
        snprint(dev->name, sizeof(dev->name), "platform-%p", phys_addr);
    }
    
    /* Create description */
    snprint(dev->description, sizeof(dev->description), 
            "Platform device %04x:%04x at %p", vid, pid, phys_addr);
    
    /* Add to registry */
    lock(&devregistry.lock);
    dev->next = devregistry.devices;
    if (devregistry.devices != nil) {
        devregistry.devices->prev = dev;
    }
    devregistry.devices = dev;
    unlock(&devregistry.lock);
    
    if (0) print("devregistry: registered platform device %s (VID:%04x PID:%04x)\n",
          dev->name, vid, pid);
    
    return dev;
}

/* Find device by ID */
Device*
devregistry_find_by_id(ulong id)
{
    Device *dev;
    
    lock(&devregistry.lock);
    for (dev = devregistry.devices; dev != nil; dev = dev->next) {
        if (dev->id == id) {
            unlock(&devregistry.lock);
            return dev;
        }
    }
    unlock(&devregistry.lock);
    
    return nil;
}

/* Find PCI device by location */
Device*
devregistry_find_by_pci(int busno, int devno, int funcno)
{
    Device *dev;
    
    lock(&devregistry.lock);
    for (dev = devregistry.devices; dev != nil; dev = dev->next) {
        if (dev->type == DEVTYPE_PCI &&
            dev->location.pci.busno == busno &&
            dev->location.pci.devno == devno &&
            dev->location.pci.funcno == funcno) {
            unlock(&devregistry.lock);
            return dev;
        }
    }
    unlock(&devregistry.lock);
    
    return nil;
}

/* Find device by name */
Device*
devregistry_find_by_name(char *name)
{
    Device *dev;
    
    if (name == nil) {
        return nil;
    }
    
    lock(&devregistry.lock);
    for (dev = devregistry.devices; dev != nil; dev = dev->next) {
        if (strcmp(dev->name, name) == 0) {
            unlock(&devregistry.lock);
            return dev;
        }
    }
    unlock(&devregistry.lock);
    
    return nil;
}

/* Find devices by driver name */
Device*
devregistry_find_by_driver(char *driver_name)
{
    Device *dev;
    
    if (driver_name == nil) {
        return nil;
    }
    
    lock(&devregistry.lock);
    for (dev = devregistry.devices; dev != nil; dev = dev->next) {
        if (strcmp(dev->driver_name, driver_name) == 0) {
            unlock(&devregistry.lock);
            return dev;
        }
    }
    unlock(&devregistry.lock);
    
    return nil;
}

/* Unregister a device */
int
devregistry_unregister(Device *dev)
{
    if (dev == nil) {
        return -1;
    }
    
    lock(&devregistry.lock);
    
    /* Remove from linked list */
    if (dev->prev != nil) {
        dev->prev->next = dev->next;
    } else {
        devregistry.devices = dev->next;
    }
    
    if (dev->next != nil) {
        dev->next->prev = dev->prev;
    }
    
    /* Update counts */
    devregistry.type_count[dev->type]--;
    devregistry.state_count[dev->state]--;
    devregistry.device_count--;
    
    unlock(&devregistry.lock);
    
    if (0) print("devregistry: unregistered device %s\n", dev->name);
    
    free(dev);
    return 0;
}

/* Update device state */
int
devregistry_update_state(Device *dev, DevState new_state)
{
    if (dev == nil) {
        return -1;
    }
    
    lock(&devregistry.lock);
    
    /* Update state counts */
    devregistry.state_count[dev->state]--;
    dev->state = new_state;
    dev->last_seen = m->ticks;
    devregistry.state_count[new_state]++;
    
    unlock(&devregistry.lock);
    
    if (0) print("devregistry: device %s state changed to %d\n", dev->name, new_state);
    
    return 0;
}

/* List all devices */
void
devregistry_list(void (*print_func)(char*, ...))
{
    Device *dev;
    char *type_names[] = {"Unknown", "PCI", "USB", "Platform", "Virtual"};
    char *state_names[] = {"Unknown", "Present", "Configured", "Online", "Offline", "Error"};
    
    if (print_func == nil) {
        print_func = print;
    }
    
    print_func("Device Registry:\n");
    print_func("Total devices: %d\n", devregistry.device_count);
    print_func("----------------------------------------\n");
    
    lock(&devregistry.lock);
    for (dev = devregistry.devices; dev != nil; dev = dev->next) {
        print_func("%s: %s (%s, %s)\n",
                   dev->name,
                   dev->description,
                   dev->type < DEVTYPE_MAX ? type_names[dev->type] : "Invalid",
                   dev->state < DEVSTATE_MAX ? state_names[dev->state] : "Invalid");
        
        if (dev->driver_name[0] != '\0') {
            print_func("  Driver: %s (PID: %d)\n", dev->driver_name, dev->driver_pid);
        }
    }
    unlock(&devregistry.lock);
    
    print_func("----------------------------------------\n");
    print_func("By type: ");
      /*@ loop invariant 0 <= i <= DEVTYPE_MAX;
    @ loop assigns i;
    @ loop variant DEVTYPE_MAX - i;
    @*/
  for (int i = 0; i < DEVTYPE_MAX; i++) {
        if (devregistry.type_count[i] > 0) {
            print_func("%s=%d ", 
                      i < DEVTYPE_MAX ? type_names[i] : "Invalid",
                      devregistry.type_count[i]);
        }
    }
    print_func("\n");
    
    print_func("By state: ");
      /*@ loop invariant 0 <= i <= DEVSTATE_MAX;
    @ loop assigns i;
    @ loop variant DEVSTATE_MAX - i;
    @*/
  for (int i = 0; i < DEVSTATE_MAX; i++) {
        if (devregistry.state_count[i] > 0) {
            print_func("%s=%d ", 
                      i < DEVSTATE_MAX ? state_names[i] : "Invalid",
                      devregistry.state_count[i]);
        }
    }
    print_func("\n");
}

/* Find device matching criteria */
Device*
devregistry_find_match(dev_match_fn match_func, void *arg)
{
    Device *dev;
    
    if (match_func == nil) {
        return nil;
    }
    
    lock(&devregistry.lock);
    for (dev = devregistry.devices; dev != nil; dev = dev->next) {
        if (match_func(dev, arg)) {
            unlock(&devregistry.lock);
            return dev;
        }
    }
    unlock(&devregistry.lock);
    
    return nil;
}

/* Count devices by type */
int
devregistry_count_by_type(DevType type)
{
    if (type >= DEVTYPE_MAX) {
        return -1;
    }
    
    return devregistry.type_count[type];
}

/* Count devices by state */
int
devregistry_count_by_state(DevState state)
{
    if (state >= DEVSTATE_MAX) {
        return -1;
    }
    
    return devregistry.state_count[state];
}

/* Check device capability */
int
devregistry_has_capability(Device *dev, ulong capability)
{
    if (dev == nil) {
        return 0;
    }
    
    return (dev->capabilities & capability) == capability;
}

/* Add capability to device */
void
devregistry_add_capability(Device *dev, ulong capability)
{
    if (dev == nil) {
        return;
    }
    
    lock(&devregistry.lock);
    dev->capabilities |= capability;
    unlock(&devregistry.lock);
}

/* Remove capability from device */
void
devregistry_remove_capability(Device *dev, ulong capability)
{
    if (dev == nil) {
        return;
    }
    
    lock(&devregistry.lock);
    dev->capabilities &= ~capability;
    unlock(&devregistry.lock);
}

/* Register driver for device */
int
devregistry_register_driver(Device *dev, char *driver_name, int driver_pid)
{
    if (dev == nil || driver_name == nil) {
        return -1;
    }
    
    lock(&devregistry.lock);
    strncpy(dev->driver_name, driver_name, sizeof(dev->driver_name) - 1);
    dev->driver_name[sizeof(dev->driver_name) - 1] = '\0';
    dev->driver_pid = driver_pid;
    unlock(&devregistry.lock);
    
    devregistry_update_state(dev, DEVSTATE_ONLINE);
    
    if (0) print("devregistry: registered driver %s for device %s\n", driver_name, dev->name);
    
    return 0;
}

/* Unregister driver from device */
int
devregistry_unregister_driver(Device *dev)
{
    if (dev == nil) {
        return -1;
    }
    
    lock(&devregistry.lock);
    dev->driver_name[0] = '\0';
    dev->driver_pid = 0;
    unlock(&devregistry.lock);
    
    devregistry_update_state(dev, DEVSTATE_PRESENT);
    
    if (0) print("devregistry: unregistered driver from device %s\n", dev->name);
    
    return 0;
}