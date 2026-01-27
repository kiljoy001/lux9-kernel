/*
 * El Torito ISO boot support for 9front VMX
 * Adds ability to boot directly from ISO images without extracting kernel
 */

#include <u.h>
#include <libc.h>

#define ELTORITO_LBA 17
#define SECTOR_SIZE 2048

/* El Torito Boot Record */
typedef struct ElToritoDesc {
	u8int	type;
	char	id[5];		/* "CD001" */
	u8int	version;	/* 1 */
	char	sysid[32];	/* "EL TORITO SPECIFICATION" */
	u8int	unused[32];
	u32int	catalog;	/* Boot catalog sector */
} ElToritoDesc;

/* Boot Catalog Validation Entry */
typedef struct ElToritoValid {
	u8int	header;		/* 1 */
	u8int	platform;	/* 0 = x86 */
	u16int	reserved;
	char	idstring[24];
	u16int	checksum;
	u16int	signature;	/* 0xAA55 */
} ElToritoValid;

/* Boot Catalog Boot Entry */
typedef struct ElToritoBoot {
	u8int	indicator;	/* 0x88 = bootable */
	u8int	media;		/* 0 = no emulation */
	u16int	loadseg;	/* Load segment */
	u8int	systype;
	u8int	reserved1;
	u16int	sectors;	/* Sector count */
	u32int	lba;		/* Starting LBA */
	u8int	reserved2[20];
} ElToritoBoot;

static int
readiso(int fd, u32int lba, void *buf, int nsect)
{
	vlong off;
	
	off = (vlong)lba * SECTOR_SIZE;
	if(seek(fd, off, 0) != off){
		werrstr("seek to LBA %ud failed", lba);
		return -1;
	}
	
	if(readn(fd, buf, nsect * SECTOR_SIZE) != nsect * SECTOR_SIZE){
		werrstr("read %d sectors at LBA %ud failed", nsect, lba);
		return -1;
	}
	
	return 0;
}

/*
 * Find and load El Torito boot image from ISO
 * Returns malloc'd buffer with boot image and size
 */
void*
eltorito_load(char *isofile, long *size)
{
	int fd;
	ElToritoDesc desc;
	ElToritoValid valid;
	ElToritoBoot boot;
	uchar sector[SECTOR_SIZE];
	void *bootimg;
	int bootsize;
	
	fd = open(isofile, OREAD);
	if(fd < 0){
		werrstr("cannot open ISO: %r");
		return nil;
	}
	
	/* Read El Torito descriptor at sector 17 */
	if(readiso(fd, ELTORITO_LBA, &desc, 1) < 0){
		close(fd);
		return nil;
	}
	
	/* Verify El Torito signature */
	if(memcmp(desc.id, "CD001", 5) != 0 || desc.version != 1){
		close(fd);
		werrstr("not a valid ISO9660 image");
		return nil;
	}
	
	if(memcmp(desc.sysid, "EL TORITO SPECIFICATION", 23) != 0){
		close(fd);
		werrstr("ISO is not El Torito bootable");
		return nil;
	}
	
	/* Read boot catalog */
	if(readiso(fd, desc.catalog, sector, 1) < 0){
		close(fd);
		werrstr("cannot read boot catalog at sector %ud", desc.catalog);
		return nil;
	}
	
	/* Parse validation entry */
	memmove(&valid, sector, sizeof(valid));
	if(valid.header != 1 || valid.signature != 0xAA55){
		close(fd);
		werrstr("invalid boot catalog validation entry");
		return nil;
	}
	
	if(valid.platform != 0){
		close(fd);
		werrstr("boot catalog is not for x86 platform");
		return nil;
	}
	
	/* Parse boot entry */
	memmove(&boot, sector + sizeof(valid), sizeof(boot));
	if(boot.indicator != 0x88){
		close(fd);
		werrstr("boot entry is not bootable");
		return nil;
	}
	
	/* Calculate boot image size */
	bootsize = boot.sectors * 512;  /* El Torito uses 512-byte sectors */
	if(bootsize == 0){
		/* Some ISOs don't specify size, try common sizes */
		bootsize = 4 * 512;  /* 2KB floppy emulation */
	}
	
	print("El Torito: boot image at LBA %ud, %d bytes\n", boot.lba, bootsize);
	
	/* Allocate buffer for boot image */
	bootimg = malloc(bootsize);
	if(bootimg == nil){
		close(fd);
		werrstr("cannot allocate %d bytes for boot image", bootsize);
		return nil;
	}
	
	/* Read boot image */
	if(seek(fd, (vlong)boot.lba * SECTOR_SIZE, 0) < 0){
		free(bootimg);
		close(fd);
		werrstr("cannot seek to boot image");
		return nil;
	}
	
	if(readn(fd, bootimg, bootsize) != bootsize){
		free(bootimg);
		close(fd);
		werrstr("cannot read boot image");
		return nil;
	}
	
	close(fd);
	*size = bootsize;
	return bootimg;
}

/*
 * Extract kernel from ISO boot image
 * This is a simplified version - real implementation would need
 * to handle various boot loader formats (ISOLINUX, GRUB, etc.)
 */
void*
extract_kernel(void *bootimg, long bootsize, long *kernsize)
{
	/* 
	 * For now, just return the boot image itself
	 * A full implementation would parse ISOLINUX/GRUB configs
	 * and extract the actual kernel
	 */
	void *kernel = malloc(bootsize);
	if(kernel == nil)
		return nil;
	
	memmove(kernel, bootimg, bootsize);
	*kernsize = bootsize;
	return kernel;
}