#include "u.h"
#include "portlib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "family/family.h"

/* Global lock_init */
void
lock_init(Lock *l)
{
	memset(l, 0, sizeof(Lock));
}

/* ctype support */
unsigned char _ctype[257] = {0};

/* PCI Config wrappers */
/* Defined in pcipc.c */
extern int (*pcicfgrw32)(int, int, int, int);
extern int (*pcicfgrw16)(int, int, int, int);
extern int (*pcicfgrw8)(int, int, int, int);

int
pci_config_read32(uint8_t bus, uint8_t dev, uint8_t func, uint8_t offset, uint32_t* data)
{
	int tbdf = (bus << 16) | (dev << 11) | (func << 8);
	if(pcicfgrw32)
		*data = pcicfgrw32(tbdf, offset, 0, 1);
	else
		*data = 0xFFFFFFFF;
	return 0;
}

int
pci_config_write32(uint8_t bus, uint8_t dev, uint8_t func, uint8_t offset, uint32_t data)
{
	int tbdf = (bus << 16) | (dev << 11) | (func << 8);
	if(pcicfgrw32)
		pcicfgrw32(tbdf, offset, data, 0);
	return 0;
}

int
pci_config_read16(uint8_t bus, uint8_t dev, uint8_t func, uint8_t offset, uint16_t* data)
{
	int tbdf = (bus << 16) | (dev << 11) | (func << 8);
	if(pcicfgrw16)
		*data = pcicfgrw16(tbdf, offset, 0, 1);
	else
		*data = 0xFFFF;
	return 0;
}

int
pci_config_write16(uint8_t bus, uint8_t dev, uint8_t func, uint8_t offset, uint16_t data)
{
	int tbdf = (bus << 16) | (dev << 11) | (func << 8);
	if(pcicfgrw16)
		pcicfgrw16(tbdf, offset, data, 0);
	return 0;
}

int
pci_config_read8(uint8_t bus, uint8_t dev, uint8_t func, uint8_t offset, uint8_t* data)
{
	int tbdf = (bus << 16) | (dev << 11) | (func << 8);
	if(pcicfgrw8)
		*data = pcicfgrw8(tbdf, offset, 0, 1);
	else
		*data = 0xFF;
	return 0;
}

int
pci_config_write8(uint8_t bus, uint8_t dev, uint8_t func, uint8_t offset, uint8_t data)
{
	int tbdf = (bus << 16) | (dev << 11) | (func << 8);
	if(pcicfgrw8)
		pcicfgrw8(tbdf, offset, data, 0);
	return 0;
}

/* Process stub */
void*
current_process(void)
{
	return up;
}

/* Permission stub */
int
validate_channel_operation_permission(void* proc, void* channel)
{
	return 1;
}

/* Stubs for missing family functions */
void setup_pci_event_system(struct FamilyExchangePage* family) {}
void setup_pci_transaction_manager(struct FamilyExchangePage* family) {}
void update_pci_family_stats(struct FamilyExchangePage* family) {}
int notify_pci_device_removed(struct FamilyExchangePage* family, void* dev) { return 0; }
