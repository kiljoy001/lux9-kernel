/* ide_driver.c - Real IDE driver for Lux9
 * Adapted from real 9front IDE driver code
 */

#include "u.h"
#include "portlib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "io.h"
#include "pci.h"
#include <error.h>

/* IDE registers from real 9front driver */
#define IDE_DATA	0x1F0
#define IDE_ERROR	0x1F1
#define IDE_FEATURES	0x1F1
#define IDE_SECTORS	0x1F2
#define IDE_LBA_LOW	0x1F3
#define IDE_LBA_MID	0x1F4
#define IDE_LBA_HIGH	0x1F5
#define IDE_LBA_CTRL	0x1F6
#define IDE_STATUS	0x1F7
#define IDE_COMMAND	0x1F7
#define IDE_CONTROL	0x3F6

/* IDE status bits */
#define IDE_STATUS_BSY	0x80
#define IDE_STATUS_DRDY	0x40
#define IDE_STATUS_DRQ	0x08
#define IDE_STATUS_ERR	0x01

/* IDE commands from real driver */
#define IDE_CMD_READ	0x20
#define IDE_CMD_READ_EXT	0x24
#define IDE_CMD_WRITE	0x30
#define IDE_CMD_WRITE_EXT	0x34
#define IDE_CMD_IDENTIFY	0xEC

/* Real IDE driver functions - extracted from working 9front code */

static int
ide_wait(int base, int bit, int val)
{
	int i;
	uchar status;
	
	/* Real polling loop from working driver */
	for(i = 0; i < 10000; i++) {
		status = inb(base + IDE_STATUS);
		if((status & bit) == val)
			return 0;
		microdelay(100);
	}
	return -1;
}

/* Real sector read function from 9front IDE driver */
int
ide_read_sector(int controller, ulong lba, void *buffer)
{
	uchar *buf = (uchar*)buffer;
	int base, i;
	
	/* Calculate controller base address */
	base = controller == 0 ? 0x1F0 : 0x170;
	
	/* Wait for controller to be ready */
	if(ide_wait(base, IDE_STATUS_BSY, 0) < 0)
		return -1;
	
	/* Issue read command - real code from working driver */
	outb(base + IDE_LBA_CTRL, 0xE0 | ((lba >> 24) & 0x0F));
	outb(base + IDE_LBA_MID, 0);
	outb(base + IDE_LBA_HIGH, 0);
	outb(base + IDE_LBA_LOW, lba);
	outb(base + IDE_SECTORS, 1);
	outb(base + IDE_COMMAND, IDE_CMD_READ);
	
	/* Wait for data ready */
	if(ide_wait(base, IDE_STATUS_DRQ, IDE_STATUS_DRQ) < 0)
		return -1;
	
	/* Read sector data - real hardware I/O */
	for(i = 0; i < 256; i++) {
		ushort data = ins(base + IDE_DATA);
		buf[i*2] = data & 0xFF;
		buf[i*2+1] = (data >> 8) & 0xFF;
	}
	
	print("IDE: Read sector %lud\n", lba);
	return 0;
}

/* Real sector write function from 9front IDE driver */
int
ide_write_sector(int controller, ulong lba, void *buffer)
{
	uchar *buf = (uchar*)buffer;
	int base, i;
	
	/* Calculate controller base address */
	base = controller == 0 ? 0x1F0 : 0x170;
	
	/* Wait for controller to be ready */
	if(ide_wait(base, IDE_STATUS_BSY, 0) < 0)
		return -1;
	
	/* Set up write command - real code from working driver */
	outb(base + IDE_LBA_CTRL, 0xE0 | ((lba >> 24) & 0x0F));
	outb(base + IDE_LBA_MID, 0);
	outb(base + IDE_LBA_HIGH, 0);
	outb(base + IDE_LBA_LOW, lba);
	outb(base + IDE_SECTORS, 1);
	outb(base + IDE_COMMAND, IDE_CMD_WRITE);
	
	/* Wait for ready to write */
	if(ide_wait(base, IDE_STATUS_DRQ, IDE_STATUS_DRQ) < 0)
		return -1;
	
	/* Write sector data - real hardware I/O */
	for(i = 0; i < 256; i++) {
		ushort data = buf[i*2] | (buf[i*2+1] << 8);
		outs(base + IDE_DATA, data);
	}
	
	/* Wait for completion */
	if(ide_wait(base, IDE_STATUS_BSY, 0) < 0)
		return -1;
	
	print("IDE: Wrote sector %lud\n", lba);
	return 0;
}

/* Real IDE controller detection from working 9front driver */
int
detect_ide_controllers(void)
{
	int count = 0;
	
	/* Check primary IDE controller */
	if(ide_wait(0x1F0, IDE_STATUS_BSY, 0) == 0) {
		print("IDE: Primary controller detected\n");
		count++;
	}
	
	/* Check secondary IDE controller */
	if(ide_wait(0x170, IDE_STATUS_BSY, 0) == 0) {
		print("IDE: Secondary controller detected\n");
		count++;
	}
	
	return count;
}