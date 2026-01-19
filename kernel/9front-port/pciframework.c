/*
 * Unified PCI framework implementation
 * Provides structured PCI device management and driver registration
 */

#include "u.h"
/* #include "lib.h" */
#include "dat.h"
#include "devregistry.h"
#include "fns.h"
#include "mem.h"
#include "pci.h"
#include "pciframework.h"

/* Global PCI framework state */
static struct {
  Lock lock;               /* Framework lock */
  PCIDriver *drivers;      /* Registered drivers */
  int driver_count;        /* Number of drivers */
  Device **pci_devices;    /* Registered PCI devices */
  int pci_device_count;    /* Number of PCI devices */
  int pci_device_capacity; /* Capacity of device array */
} pci_framework;

/* PCI class information */
static PCIClass pci_classes[] = {
    {0x00, 0x00, 0x00, "Legacy", "Any"},
    {0x01, 0x00, 0x00, "Mass Storage", "SCSI"},
    {0x01, 0x01, 0x00, "Mass Storage", "IDE"},
    {0x01, 0x02, 0x00, "Mass Storage", "Floppy"},
    {0x01, 0x05, 0x00, "Mass Storage", "ATA"},
    {0x01, 0x06, 0x00, "Mass Storage", "SATA"},
    {0x01, 0x80, 0x00, "Mass Storage", "Other"},
    {0x02, 0x00, 0x00, "Network", "Ethernet"},
    {0x02, 0x80, 0x00, "Network", "Other"},
    {0x03, 0x00, 0x00, "Display", "VGA"},
    {0x03, 0x01, 0x00, "Display", "XGA"},
    {0x03, 0x80, 0x00, "Display", "Other"},
    {0x04, 0x00, 0x00, "Multimedia", "Video"},
    {0x04, 0x01, 0x00, "Multimedia", "Audio"},
    {0x04, 0x80, 0x00, "Multimedia", "Other"},
    {0x06, 0x00, 0x00, "Bridge", "Host"},
    {0x06, 0x01, 0x00, "Bridge", "ISA"},
    {0x06, 0x04, 0x00, "Bridge", "PCI"},
    {0x06, 0x80, 0x00, "Bridge", "Other"},
    {0x07, 0x00, 0x00, "Simple Communication", "Serial"},
    {0x07, 0x01, 0x00, "Simple Communication", "Parallel"},
    {0x07, 0x80, 0x00, "Simple Communication", "Other"},
    {0x08, 0x00, 0x00, "Base System", "PIC"},
    {0x08, 0x01, 0x00, "Base System", "DMA"},
    {0x08, 0x02, 0x00, "Base System", "Timer"},
    {0x08, 0x03, 0x00, "Base System", "RTC"},
    {0x08, 0x80, 0x00, "Base System", "Other"},
    {0x09, 0x00, 0x00, "Input", "Keyboard"},
    {0x09, 0x01, 0x00, "Input", "Digitizer"},
    {0x09, 0x02, 0x00, "Input", "Mouse"},
    {0x09, 0x80, 0x00, "Input", "Other"},
    {0x0C, 0x00, 0x00, "Serial Bus", "FireWire"},
    {0x0C, 0x01, 0x00, "Serial Bus", "ACCESS"},
    {0x0C, 0x02, 0x00, "Serial Bus", "SSA"},
    {0x0C, 0x03, 0x00, "Serial Bus", "USB"},
    {0x0C, 0x04, 0x00, "Serial Bus", "Fiber"},
    {0x0C, 0x05, 0x00, "Serial Bus", "SMBus"},
    {0x0C, 0x80, 0x00, "Serial Bus", "Other"},
    {0xFF, 0xFF, 0xFF, "Unknown", "Unknown"}};

/* Initialize PCI framework */
/*@
  @ assigns \nothing;
  @*/
void pci_framework_init(void) {
  memset(&pci_framework, 0, sizeof(pci_framework));

  /* Initialize device array */
  pci_framework.pci_device_capacity = 64;
  pci_framework.pci_devices =
      malloc(sizeof(Device *) * pci_framework.pci_device_capacity);
  if (pci_framework.pci_devices == nil) {
    pci_framework.pci_device_capacity = 0;
  }

  if (0)
    print("pci_framework: initialized\n");
}

/* Match PCI device against criteria */
int pci_match_device(Pcidev *pcidev, ushort vendor_id, ushort device_id,
                     uchar base_class, uchar sub_class) {
  if (pcidev == nil) {
    return 0;
  }

  /* Match vendor ID if specified */
  if (vendor_id != 0 && pcidev->vid != vendor_id) {
    return 0;
  }

  /* Match device ID if specified */
  if (device_id != 0 && pcidev->did != device_id) {
    return 0;
  }

  /* Match base class if specified */
  if (base_class != 0xFF && pcidev->ccrb != base_class) {
    return 0;
  }

  /* Match sub class if specified */
  if (sub_class != 0xFF && pcidev->ccru != sub_class) {
    return 0;
  }

  return 1;
}

/* Find driver for PCI device */
PCIDriver *pci_find_driver_for_device(Pcidev *pcidev) {
  PCIDriver *driver;

  if (pcidev == nil) {
    return nil;
  }

  lock(&pci_framework.lock);
  for (driver = pci_framework.drivers; driver != nil; driver = driver->next) {
    if (pci_match_device(pcidev, driver->vendor_id, driver->device_id,
                         driver->base_class, driver->sub_class)) {
      unlock(&pci_framework.lock);
      return driver;
    }
  }
  unlock(&pci_framework.lock);

  return nil;
}

/* Get PCI class information */
PCIClass *pci_get_class_info(uchar base_class, uchar sub_class) {
  for (int i = 0;
       pci_classes[i].base_class != 0xFF || pci_classes[i].sub_class != 0xFF;
       i++) {
    if (pci_classes[i].base_class == base_class &&
        pci_classes[i].sub_class == sub_class) {
      return &pci_classes[i];
    }
  }

  /* Return unknown class */
  return &pci_classes[nelem(pci_classes) - 1];
}

/* Get PCI class name */
char *pci_class_name(uchar base_class, uchar sub_class) {
  PCIClass *class_info = pci_get_class_info(base_class, sub_class);
  return class_info->class_name;
}

/* Register a PCI device with the framework */
Device *pci_framework_register_device(Pcidev *pcidev) {
  if (pcidev == nil) {
    return nil;
  }

  /* Register with device registry */
  Device *dev = devregistry_register_pci(pcidev);
  if (dev == nil) {
    return nil;
  }

  /* Find appropriate driver */
  PCIDriver *driver = pci_find_driver_for_device(pcidev);
  if (driver != nil) {
    /* Update device description with class information */
    PCIClass *class_info = pci_get_class_info(pcidev->ccrb, pcidev->ccru);
    snprint(dev->description, sizeof(dev->description), "%s %s (%04x:%04x)",
            class_info->class_name, class_info->sub_class_name, pcidev->vid,
            pcidev->did);

    /* Register driver if probe succeeds */
    if (driver->probe == nil || driver->probe(pcidev) == 0) {
      devregistry_register_driver(dev, driver->name,
                                  0); /* PID 0 for kernel drivers */

      /* Call attach if provided */
      if (driver->attach != nil) {
        driver->attach(dev);
      }
    }
  } else {
    /* No driver found, update description */
    PCIClass *class_info = pci_get_class_info(pcidev->ccrb, pcidev->ccru);
    snprint(dev->description, sizeof(dev->description),
            "%s %s (%04x:%04x) - No driver", class_info->class_name,
            class_info->sub_class_name, pcidev->vid, pcidev->did);
  }

  /* Add to framework device list */
  lock(&pci_framework.lock);
  if (pci_framework.pci_device_count >= pci_framework.pci_device_capacity) {
    /* Expand array */
    int new_capacity = pci_framework.pci_device_capacity * 2;
    Device **new_array =
        realloc(pci_framework.pci_devices, sizeof(Device *) * new_capacity);
    if (new_array != nil) {
      pci_framework.pci_devices = new_array;
      pci_framework.pci_device_capacity = new_capacity;
    }
  }

  if (pci_framework.pci_device_count < pci_framework.pci_device_capacity) {
    pci_framework.pci_devices[pci_framework.pci_device_count++] = dev;
  }
  unlock(&pci_framework.lock);

  return dev;
}

/* Enumerate all PCI devices */
/*@
  @ assigns \nothing;
  @*/
int pci_framework_enumerate(void) {
  Pcidev *pcidev = nil;
  int count = 0;

  if (0)
    print("pci_framework: enumerating PCI devices\n");

  /* Enumerate all PCI devices */
  while ((pcidev = pcimatch(pcidev, 0, 0)) != nil) {
    Device *dev = pci_framework_register_device(pcidev);
    if (dev != nil) {
      count++;
    }
  }

  if (0)
    print("pci_framework: found %d PCI devices\n", count);

  return count;
}

/* Register a PCI driver */
/*@
  @ requires driver == \null || \valid(driver);
  @ assigns \nothing;
  @*/
int pci_framework_register_driver(PCIDriver *driver) {
  if (driver == nil) {
    return -1;
  }

  lock(&pci_framework.lock);
  driver->next = pci_framework.drivers;
  pci_framework.drivers = driver;
  pci_framework.driver_count++;
  unlock(&pci_framework.lock);

  if (0)
    print("pci_framework: registered driver %s\n", driver->name);

  return 0;
}

/* Unregister a PCI driver */
/*@
  @ requires driver == \null || \valid(driver);
  @ assigns \nothing;
  @*/
int pci_framework_unregister_driver(PCIDriver *driver) {
  if (driver == nil) {
    return -1;
  }

  lock(&pci_framework.lock);

  if (pci_framework.drivers == driver) {
    pci_framework.drivers = driver->next;
  } else {
    PCIDriver *prev = pci_framework.drivers;
    while (prev != nil && prev->next != driver) {
      prev = prev->next;
    }
    if (prev != nil) {
      prev->next = driver->next;
    } else {
      unlock(&pci_framework.lock);
      return -1; /* Driver not found */
    }
  }

  pci_framework.driver_count--;
  unlock(&pci_framework.lock);

  if (0)
    print("pci_framework: unregistered driver %s\n", driver->name);

  return 0;
}

/* List registered drivers */
/*@
  @ requires  == \null || \valid();
  @ assigns \nothing;
  @*/
void pci_framework_list_drivers(void (*print_func)(char *, ...)) {
  PCIDriver *driver;

  if (print_func == nil) {
    print_func = print;
  }

  print_func("PCI Framework Drivers:\n");
  print_func("Total drivers: %d\n", pci_framework.driver_count);
  print_func("----------------------------------------\n");

  lock(&pci_framework.lock);
  for (driver = pci_framework.drivers; driver != nil; driver = driver->next) {
    print_func("%s: VID:%04x DID:%04x Class:%02x:%02x\n", driver->name,
               driver->vendor_id, driver->device_id, driver->base_class,
               driver->sub_class);
  }
  unlock(&pci_framework.lock);

  print_func("----------------------------------------\n");
}

/* List PCI devices */
/*@
  @ requires  == \null || \valid();
  @ assigns \nothing;
  @*/
void pci_framework_list_devices(void (*print_func)(char *, ...)) {
  if (print_func == nil) {
    print_func = print;
  }

  print_func("PCI Framework Devices:\n");
  print_func("Total PCI devices: %d\n", pci_framework.pci_device_count);
  print_func("----------------------------------------\n");

  lock(&pci_framework.lock);
    /*@ loop invariant 0 <= i <= pci_framework.pci_device_count;
    @ loop assigns i;
    @ loop variant pci_framework.pci_device_count - i;
    @*/
  for (int i = 0; i < pci_framework.pci_device_count; i++) {
    Device *dev = pci_framework.pci_devices[i];
    if (dev != nil) {
      PCIClass *class_info = pci_get_class_info(dev->location.pci.pcidev->ccrb,
                                                dev->location.pci.pcidev->ccru);

      print_func("%s: %s (%04x:%04x) - %s\n", dev->name, dev->description,
                 dev->vendor_id, dev->device_id,
                 dev->driver_name[0] ? dev->driver_name : "No driver");

      /* Print BAR information */
        /*@ loop invariant 0 <= j <= 6;
    @ loop assigns j;
    @ loop variant 6 - j;
    @*/
  for (int j = 0; j < 6; j++) {
        if (dev->location.pci.pcidev->mem[j].size > 0) {
          print_func("  BAR%d: %p (size %lld)\n", j,
                     dev->location.pci.pcidev->mem[j].bar,
                     dev->location.pci.pcidev->mem[j].size);
        }
      }
    }
  }
  unlock(&pci_framework.lock);

  print_func("----------------------------------------\n");
}

/* Example PCI drivers */
/*@
  @ requires pcidev == \null || \valid(pcidev);
  @ assigns \nothing;
  @*/
static int ahci_probe(Pcidev *pcidev) {
  /* Match AHCI controllers */
  if (pcidev->ccrb == 0x01 && pcidev->ccru == 0x06) {
    return 0; /* Match */
  }
  return -1; /* No match */
}

/*@
  @ requires pcidev == \null || \valid(pcidev);
  @ assigns \nothing;
  @*/
static int ide_probe(Pcidev *pcidev) {
  /* Match IDE controllers */
  if (pcidev->ccrb == 0x01 && pcidev->ccru == 0x01) {
    return 0; /* Match */
  }
  return -1; /* No match */
}

/*@
  @ requires pcidev == \null || \valid(pcidev);
  @ assigns \nothing;
  @*/
static int usb_probe(Pcidev *pcidev) {
  /* Match USB controllers */
  if (pcidev->ccrb == 0x0C && pcidev->ccru == 0x03) {
    return 0; /* Match */
  }
  return -1; /* No match */
}

/*@
  @ requires pcidev == \null || \valid(pcidev);
  @ assigns \nothing;
  @*/
static int ethernet_probe(Pcidev *pcidev) {
  /* Match Ethernet controllers */
  if (pcidev->ccrb == 0x02 && pcidev->ccru == 0x00) {
    return 0; /* Match */
  }
  return -1; /* No match */
}

/* Standard PCI drivers */
PCIDriver ahci_driver = {.name = "ahci",
                         .probe = ahci_probe,
                         .vendor_id = 0,
                         .device_id = 0,
                         .base_class = 0x01,
                         .sub_class = 0x06};

PCIDriver ide_driver = {.name = "ide",
                        .probe = ide_probe,
                        .vendor_id = 0,
                        .device_id = 0,
                        .base_class = 0x01,
                        .sub_class = 0x01};

PCIDriver usb_driver = {.name = "usb",
                        .probe = usb_probe,
                        .vendor_id = 0,
                        .device_id = 0,
                        .base_class = 0x0C,
                        .sub_class = 0x03};

PCIDriver ethernet_driver = {.name = "ethernet",
                             .probe = ethernet_probe,
                             .vendor_id = 0,
                             .device_id = 0,
                             .base_class = 0x02,
                             .sub_class = 0x00};