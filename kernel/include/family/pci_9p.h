/*
 * PCI Family 9P Interface - Header File
 *
 * Defines structures and functions for 9P filesystem access to PCI devices.
 */

#pragma once

#include "../types_fwd.h"
#include <stdbool.h>
#include <stdint.h>

/* Forward declarations for additional Plan 9 types */
typedef struct Walkqid Walkqid;
typedef unsigned char uchar;
typedef long long vlong;
struct FamilyExchangePage;
struct PCIDeviceDescriptor;

/* PCI configuration request structure */
struct PCIConfigRequest {
  uint8_t bus;
  uint8_t device;
  uint8_t function;
  uint16_t offset;
  uint16_t length;
  uint8_t operation; /* 0=read, 1=write */
  uint8_t data[256];
};

/* Public API - only functions that need to be called from outside */

/* 9P Operations */
Walkqid *pci_9p_walk(struct FamilyExchangePage *family, Chan *c, Chan *nc,
                     char **name, int nname);
int pci_9p_stat(struct FamilyExchangePage *family, Chan *c, uchar *dp, int n);
Chan *pci_9p_open(struct FamilyExchangePage *family, Chan *c, int omode);
void pci_9p_close(struct FamilyExchangePage *family, Chan *c);
long pci_9p_read(struct FamilyExchangePage *family, Chan *c, void *buf, long n,
                 vlong off);
long pci_9p_write(struct FamilyExchangePage *family, Chan *c, void *buf, long n,
                  vlong off);
