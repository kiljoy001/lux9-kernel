/*
 * Unified PCI framework for microkernel architecture
 * Provides structured PCI device management and driver registration
 */

#ifndef _PCIFRAMEWORK_H_
#define _PCIFRAMEWORK_H_

#include "u.h"
#include "devregistry.h"

/* Forward declarations */
typedef struct Pcidev Pcidev;

/* PCI device class information */
typedef struct PCIClass PCIClass;
struct PCIClass {
    uchar base_class;
    uchar sub_class;
    uchar prog_if;
    char *class_name;
    char *sub_class_name;
};

/* PCI driver registration */
typedef struct PCIDriver PCIDriver;
struct PCIDriver {
    char name[32];              /* Driver name */
    int (*probe)(Pcidev *pcidev);  /* Probe function */
    int (*attach)(Device *dev);    /* Attach function */
    int (*detach)(Device *dev);    /* Detach function */
    int (*configure)(Device *dev); /* Configuration function */
    
    /* Device matching */
    ushort vendor_id;           /* Specific vendor ID (0 = any) */
    ushort device_id;           /* Specific device ID (0 = any) */
    uchar base_class;           /* Base class (0xFF = any) */
    uchar sub_class;            /* Sub class (0xFF = any) */
    
    PCIDriver *next;            /* Next driver in registry */
};

/* PCI framework functions */
void pci_framework_init(void);
int pci_framework_enumerate(void);
int pci_framework_register_driver(PCIDriver *driver);
int pci_framework_unregister_driver(PCIDriver *driver);
void pci_framework_list_drivers(void (*print_func)(char*, ...));
void pci_framework_list_devices(void (*print_func)(char*, ...));

/* Device matching helpers */
int pci_match_device(Pcidev *pcidev, ushort vendor_id, ushort device_id, 
                     uchar base_class, uchar sub_class);
PCIDriver* pci_find_driver_for_device(Pcidev *pcidev);
Device* pci_framework_register_device(Pcidev *pcidev);

/* Class information */
PCIClass* pci_get_class_info(uchar base_class, uchar sub_class);
char* pci_class_name(uchar base_class, uchar sub_class);

/* Standard PCI device matching */
extern PCIDriver ahci_driver;
extern PCIDriver ide_driver;
extern PCIDriver usb_driver;
extern PCIDriver ethernet_driver;

#endif /* _PCIFRAMEWORK_H_ */