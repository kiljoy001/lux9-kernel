#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

#define CDROM_SECTOR_SIZE 2048

int main(int argc, char **argv) {
    if(argc != 2) {
        fprintf(stderr, "Usage: %s <iso_file>\n", argv[0]);
        return 1;
    }

    int fd = open(argv[1], O_RDONLY);
    if(fd < 0) {
        perror("open");
        return 1;
    }

    // Seek to LBA 0x11 (Boot Record Volume Descriptor)
    off_t offset = 0x11 * CDROM_SECTOR_SIZE;
    if(lseek(fd, offset, SEEK_SET) != offset) {
        perror("lseek");
        close(fd);
        return 1;
    }

    unsigned char buffer[CDROM_SECTOR_SIZE];
    if(read(fd, buffer, CDROM_SECTOR_SIZE) != CDROM_SECTOR_SIZE) {
        perror("read");
        close(fd);
        return 1;
    }

    printf("Boot Record Volume Descriptor at LBA 0x11:\n");
    printf("First byte: 0x%02x (expected 0x00)\n", buffer[0]);
    printf("Identifier: '%.5s' (expected 'CD001')\n", &buffer[1]);
    printf("Version: 0x%02x (expected 0x01)\n", buffer[6]);
    printf("System ID: '%.32s'\n", &buffer[7]);

    // Check if it matches El Torito
    if(buffer[0] == 0 && memcmp(&buffer[1], "CD001", 5) == 0 && buffer[6] == 1) {
        if(memcmp(&buffer[7], "EL TORITO SPECIFICATION", 23) == 0) {
            printf("✓ Valid El Torito boot record found!\n");

            // Get boot catalog location
            unsigned int catalog_lba = *(unsigned int*)&buffer[0x47];
            printf("Boot catalog at LBA: 0x%x\n", catalog_lba);
        } else {
            printf("✗ Not an El Torito boot record (system ID mismatch)\n");
        }
    } else {
        printf("✗ Not a valid boot record\n");
    }

    close(fd);
    return 0;
}