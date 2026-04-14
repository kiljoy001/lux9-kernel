/*
 * PCI Family Definitions
 * Reconstructed for HAL Userspace
 */

#pragma once

#include "family.h"
#include <libc.h>
#include <u.h>

#define MAX_PCI_DEVICES 256
#define PCI_CONFIG_SPACE_SIZE 256
#define PCI_SHARP_ROOT "#J"
#define PCI_BUS_PATH "#J/bus"

/* PCI Address */
struct PCIAddress {
  u16int domain;
  u8int bus;
  u8int device;
  u8int function;
  int domain_active;
  u32int domain_bus_dev_func;
};

/* BAR info */
struct PCIBar {
  u32int base_address;
  u64int size;
  u32int type; // 0=mem, 1=io
  int is_64bit;
  int is_valid;
};

/* Forward decl */
struct PCIChannel;

/* Device Descriptor */
struct PCIDeviceDescriptor {
  struct PCIAddress address;
  u16int vendor_id;
  u16int device_id;
  u8int class_code;
  u8int subclass_code;
  u8int revision;
  u8int prog_if;

  struct PCIBar bars[6];

  /* State */
  int device_state;

  /* Binding */
  u64int bound_channel_id;
  struct PCIChannel *bound_channel;

  struct {
    int has_msi;
    int has_msix;
    int has_pcie;
  } capabilities;
};

/* Channel */
struct PCIChannel {
  u64int channel_id;
  struct FamilyExchangePage *family;
  struct PCIDeviceDescriptor *bound_device;
  struct PCIAddress pci_address;
  u32int permissions_mask;
  int state;

  char channel_name[64];
  u64int created_at;
  u64int last_operation;

  /* Channel resources (simplified for now) */
  struct {
    u64int bar_access;
  } stats;

  struct PCIChannel *next; // For free list or lists
};

/* Family Context */
struct PCIFamilyContext {
  struct {
    struct PCIDeviceDescriptor
        *devices[MAX_PCI_DEVICES * 32]; // Hash map or array
    int device_count;
    Lock device_registry_lock;
  } device_registry;

  struct {
    u16int primary_bus;
    u16int max_bus;
    int total_devices;
  } topology;

  struct {
    int devices_discovered;
  } stats;

  struct ResourcePool *resource_pool;
};

/* Permissions */
#define CHANNEL_PERM_READ_CONFIG (1 << 0)
#define CHANNEL_PERM_WRITE_CONFIG (1 << 1)
#define CHANNEL_PERM_MAP_BAR (1 << 2)
#define CHANNEL_PERM_ALLOC_IRQ (1 << 3)
#define CHANNEL_PERM_DMA (1 << 4)

/* Ops */
int pcifamily_init(void);
int pcifamily_refresh(void);
int pcifamily_device_count(void);
int pci_config_read32(u8int bus, u8int dev, u8int func, u8int offset,
                      u32int *data);
int pci_config_read16(u8int bus, u8int dev, u8int func, u8int offset,
                      u16int *data);
int pci_config_read8(u8int bus, u8int dev, u8int func, u8int offset,
                     u8int *data);
