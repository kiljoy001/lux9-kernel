/*
 * test_ahci.c - Minimal AHCI driver test using SIP devices
 *
 * Tests the complete SIP stack:
 * 1. PCI enumeration to find AHCI
 * 2. Reading BAR addresses
 * 3. DMA buffer allocation
 * 4. MMIO register access
 * 5. IRQ registration
 */

#include <u.h>
#include <libc.h>

/* AHCI register offsets */
enum {
	AHCI_CAP	= 0x00,		/* Host Capabilities */
	AHCI_GHC	= 0x04,		/* Global Host Control */
	AHCI_IS		= 0x08,		/* Interrupt Status */
	AHCI_PI		= 0x0C,		/* Ports Implemented */
	AHCI_VS		= 0x10,		/* Version */
};

/* Find AHCI controller on PCI bus */
static int
find_ahci(char *devname, int devsize, uvlong *baraddr)
{
	int fd;
	char buf[8192];
	int n;
	char *line, *p;
	int found = 0;

	print("test_ahci: Searching for AHCI controller on PCI bus...\n");

	/* Open PCI bus listing */
	fd = open("/dev/pci/bus", OREAD);
	if(fd < 0) {
		print("FAIL: Cannot open /dev/pci/bus: %r\n");
		return -1;
	}

	/* Read all devices */
	n = read(fd, buf, sizeof(buf)-1);
	close(fd);

	if(n < 0) {
		print("FAIL: Cannot read PCI bus: %r\n");
		return -1;
	}
	buf[n] = '\0';

	/* Parse for AHCI controller (class 01.06.01) */
	line = buf;
	while(*line) {
		/* Look for SATA AHCI controller class */
		if(strstr(line, "class=01.06") != nil) {
			/* Extract device name (first field) */
			p = line;
			while(*p && *p != ' ')
				p++;
			*p = '\0';

			strncpy(devname, line, devsize-1);
			devname[devsize-1] = '\0';

			print("  Found AHCI: %s\n", devname);
			found = 1;

			/* Look for bar5 on next lines */
			line = p + 1;
			while(*line && *line != '\n') {
				if(strncmp(line, "  bar5:", 7) == 0) {
					*baraddr = strtoull(line + 13, nil, 0);
					print("  BAR5: 0x%llux\n", *baraddr);
					break;
				}
				while(*line && *line != '\n')
					line++;
				if(*line == '\n')
					line++;
			}
			break;
		}

		/* Next line */
		while(*line && *line != '\n')
			line++;
		if(*line == '\n')
			line++;
	}

	if(!found) {
		print("FAIL: No AHCI controller found\n");
		return -1;
	}

	if(*baraddr == 0) {
		print("FAIL: No BAR5 found for AHCI\n");
		return -1;
	}

	print("OK: Found AHCI at %s, BAR5=0x%llux\n", devname, *baraddr);
	return 0;
}

/* Read AHCI register */
static int
read_ahci_reg(int memfd, uvlong bar, int offset, u32int *value)
{
	if(seek(memfd, bar + offset, 0) < 0) {
		print("FAIL: Cannot seek to 0x%llux: %r\n", bar + offset);
		return -1;
	}

	if(read(memfd, value, 4) != 4) {
		print("FAIL: Cannot read register: %r\n");
		return -1;
	}

	return 0;
}

void
main(void)
{
	char devname[64];
	uvlong baraddr = 0;
	int memfd, dmafd, irqctlfd;
	u32int cap, ghc, ver, pi;
	char dmabuf[256];
	int n;

	print("\n=== AHCI Driver Integration Test ===\n\n");

	/* Test 1: PCI Enumeration */
	print("Test 1: PCI Enumeration\n");
	if(find_ahci(devname, sizeof(devname), &baraddr) < 0)
		exits("pci");
	print("PASS: PCI enumeration\n\n");

	/* Test 2: MMIO Access */
	print("Test 2: MMIO Register Access\n");
	memfd = open("/dev/mem", OREAD);
	if(memfd < 0) {
		print("FAIL: Cannot open /dev/mem: %r\n");
		exits("mem");
	}
	print("  Opened /dev/mem\n");

	/* Read AHCI Capabilities */
	if(read_ahci_reg(memfd, baraddr, AHCI_CAP, &cap) < 0)
		exits("mmio");
	print("  CAP = 0x%08ux\n", cap);

	/* Read AHCI Version */
	if(read_ahci_reg(memfd, baraddr, AHCI_VS, &ver) < 0)
		exits("mmio");
	print("  Version = 0x%08ux (AHCI %d.%d)\n",
		ver, (ver >> 16) & 0xff, ver & 0xff);

	/* Read Ports Implemented */
	if(read_ahci_reg(memfd, baraddr, AHCI_PI, &pi) < 0)
		exits("mmio");
	print("  Ports Implemented = 0x%08ux\n", pi);

	/* Count ports */
	int nports = 0;
	for(int i = 0; i < 32; i++) {
		if(pi & (1 << i))
			nports++;
	}
	print("  Number of ports: %d\n", nports);

	close(memfd);
	print("PASS: MMIO access\n\n");

	/* Test 3: DMA Buffer Allocation */
	print("Test 3: DMA Buffer Allocation\n");
	dmafd = open("/dev/dma/alloc", ORDWR);
	if(dmafd < 0) {
		print("FAIL: Cannot open /dev/dma/alloc: %r\n");
		exits("dma");
	}
	print("  Opened /dev/dma/alloc\n");

	/* Allocate 4KB buffer (for AHCI command list) */
	if(fprint(dmafd, "size 4096 align 1024") < 0) {
		print("FAIL: Cannot allocate DMA buffer: %r\n");
		exits("dma");
	}
	print("  Requested 4KB DMA buffer\n");

	/* Read back addresses */
	n = read(dmafd, dmabuf, sizeof(dmabuf)-1);
	if(n < 0) {
		print("FAIL: Cannot read DMA addresses: %r\n");
		exits("dma");
	}
	dmabuf[n] = '\0';
	print("  DMA allocation: %s", dmabuf);

	close(dmafd);
	print("PASS: DMA allocation\n\n");

	/* Test 4: IRQ Registration */
	print("Test 4: IRQ Registration\n");

	/* Get IRQ number from PCI device */
	char ctlpath[128];
	snprint(ctlpath, sizeof(ctlpath), "/dev/pci/%s/ctl", devname);
	int pcifd = open(ctlpath, OREAD);
	if(pcifd < 0) {
		print("FAIL: Cannot open %s: %r\n", ctlpath);
		exits("irq");
	}

	char pcibuf[512];
	n = read(pcifd, pcibuf, sizeof(pcibuf)-1);
	close(pcifd);
	if(n < 0) {
		print("FAIL: Cannot read PCI ctl: %r\n");
		exits("irq");
	}
	pcibuf[n] = '\0';

	/* Parse IRQ line */
	int irq = -1;
	char *irqline = strstr(pcibuf, "irq: ");
	if(irqline != nil) {
		irq = atoi(irqline + 5);
		print("  AHCI IRQ: %d\n", irq);
	} else {
		print("FAIL: Cannot find IRQ in PCI info\n");
		exits("irq");
	}

	/* Register for IRQ */
	irqctlfd = open("/dev/irq/ctl", OWRITE);
	if(irqctlfd < 0) {
		print("FAIL: Cannot open /dev/irq/ctl: %r\n");
		exits("irq");
	}

	char irqcmd[64];
	snprint(irqcmd, sizeof(irqcmd), "register %d test_ahci", irq);
	if(fprint(irqctlfd, "%s", irqcmd) < 0) {
		print("FAIL: Cannot register IRQ: %r\n");
		exits("irq");
	}
	print("  Registered for IRQ %d\n", irq);
	close(irqctlfd);

	print("PASS: IRQ registration\n\n");

	/* Summary */
	print("=== Test Summary ===\n");
	print("✅ PCI Enumeration: Found AHCI controller\n");
	print("✅ MMIO Access: Read AHCI registers successfully\n");
	print("✅ DMA Allocation: Allocated physically contiguous buffer\n");
	print("✅ IRQ Registration: Registered for AHCI interrupts\n");
	print("\n");
	print("SUCCESS: All SIP device tests passed!\n");
	print("AHCI controller is ready for full driver integration.\n");

	exits(nil);
}
