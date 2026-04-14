/*
 * Test program for Phase 2 device registry and PCI framework
 */

#include "../kernel/include/u.h"
#include "../kernel/include/lib.h"
#include "../kernel/include/dat.h"
#include "../kernel/include/fns.h"
#include "../kernel/include/pci.h"
#include "../kernel/include/devregistry.h"
#include "../kernel/include/pciframework.h"

void
test_devregistry(void)
{
    print("=== Testing Device Registry ===\n");
    
    /* Initialize frameworks */
    devregistry_init();
    pci_framework_init();
    
    print("Device registry initialized\n");
    
    /* Test PCI framework enumeration */
    int count = pci_framework_enumerate();
    print("Found %d PCI devices\n", count);
    
    /* List drivers and devices */
    pci_framework_list_drivers(print);
    pci_framework_list_devices(print);
    
    /* List all devices in registry */
    devregistry_list(print);
    
    print("=== Device Registry Test Complete ===\n");
}

int
main(void)
{
    test_devregistry();
    return 0;
}