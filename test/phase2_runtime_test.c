/*
 * Test program for Phase 2 device registry and PCI framework
 * This would be called during kernel initialization
 */

#include "../kernel/include/u.h"
#include "../kernel/include/lib.h"
#include "../kernel/include/dat.h"
#include "../kernel/include/fns.h"
#include "../kernel/include/pci.h"
#include "../kernel/include/devregistry.h"
#include "../kernel/include/pciframework.h"

void
test_phase2_initialization(void)
{
    print("=== Phase 2: Interface Stabilization Test ===\n");
    
    /* Initialize frameworks - these would be called in chandevreset() */
    devregistry_init();
    pci_framework_init();
    
    print("Device registry and PCI framework initialized\n");
    
    /* Register standard drivers */
    pci_framework_register_driver(&ahci_driver);
    pci_framework_register_driver(&ide_driver);
    pci_framework_register_driver(&usb_driver);
    pci_framework_register_driver(&ethernet_driver);
    
    print("Standard PCI drivers registered\n");
    
    /* Enumerate PCI devices */
    int count = pci_framework_enumerate();
    print("Found and registered %d PCI devices\n", count);
    
    /* List drivers and devices */
    pci_framework_list_drivers(print);
    pci_framework_list_devices(print);
    
    /* List all devices in registry */
    devregistry_list(print);
    
    print("=== Phase 2 Test Complete ===\n");
}