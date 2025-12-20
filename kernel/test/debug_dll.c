/* debug_dll.c - Debug .NET DLL parsing */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#define READ_UINT16(ptr) (*(uint16_t*)(ptr))
#define READ_UINT32(ptr) (*(uint32_t*)(ptr))

int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <dll>\n", argv[0]);
        return 1;
    }

    FILE *f = fopen(argv[1], "rb");
    if (!f) {
        perror("fopen");
        return 1;
    }

    fseek(f, 0, SEEK_END);
    size_t size = ftell(f);
    fseek(f, 0, SEEK_SET);

    uint8_t *data = malloc(size);
    fread(data, 1, size, f);
    fclose(f);

    printf("File size: %zu bytes\n\n", size);

    // DOS header
    if (data[0] != 'M' || data[1] != 'Z') {
        fprintf(stderr, "Not a DOS/PE file\n");
        return 1;
    }

    uint32_t pe_offset = READ_UINT32(&data[0x3C]);
    printf("PE offset: 0x%x\n", pe_offset);

    // PE signature
    uint32_t pe_sig = READ_UINT32(&data[pe_offset]);
    printf("PE signature: 0x%08x (should be 0x00004550)\n", pe_sig);

    // COFF header
    uint8_t *coff = &data[pe_offset + 4];
    uint16_t machine = READ_UINT16(coff);
    uint16_t num_sections = READ_UINT16(coff + 2);
    uint16_t opt_hdr_size = READ_UINT16(coff + 16);

    printf("Machine: 0x%x\n", machine);
    printf("Number of sections: %d\n", num_sections);
    printf("Optional header size: %d\n\n", opt_hdr_size);

    // Optional header
    uint8_t *opt = coff + 20;
    uint16_t magic = READ_UINT16(opt);
    printf("Optional header magic: 0x%x (0x10B=PE32, 0x20B=PE32+)\n", magic);

    // Data directories location depends on PE32 vs PE32+
    uint8_t *data_dirs;
    if (magic == 0x10B) {
        // PE32: data directories at offset 96
        data_dirs = opt + 96;
    } else if (magic == 0x20B) {
        // PE32+: data directories at offset 112
        data_dirs = opt + 112;
    } else {
        fprintf(stderr, "Unknown magic: 0x%x\n", magic);
        return 1;
    }

    printf("\nData Directories:\n");
    for (int i = 0; i < 16; i++) {
        uint32_t rva = READ_UINT32(data_dirs + i*8);
        uint32_t sz = READ_UINT32(data_dirs + i*8 + 4);
        if (rva || sz) {
            printf("  [%2d] RVA=0x%08x Size=0x%08x", i, rva, sz);
            if (i == 14) printf(" <- CLR Runtime Header");
            printf("\n");
        }
    }

    // Check CLR directory
    uint32_t clr_rva = READ_UINT32(data_dirs + 14*8);
    uint32_t clr_size = READ_UINT32(data_dirs + 14*8 + 4);

    if (clr_size == 0) {
        fprintf(stderr, "\nNot a .NET assembly (no CLR directory)\n");
        return 1;
    }

    printf("\nCLR Directory: RVA=0x%x Size=0x%x\n", clr_rva, clr_size);

    // Find section containing CLR RVA
    uint8_t *sections = opt + opt_hdr_size;
    printf("\nSections:\n");
    for (int i = 0; i < num_sections; i++) {
        uint8_t *sec = sections + i * 40;
        char name[9] = {0};
        memcpy(name, sec, 8);
        uint32_t vsize = READ_UINT32(sec + 8);
        uint32_t vaddr = READ_UINT32(sec + 12);
        uint32_t rawsize = READ_UINT32(sec + 16);
        uint32_t rawptr = READ_UINT32(sec + 20);

        printf("  [%d] %-8s VAddr=0x%08x VSize=0x%08x RawPtr=0x%08x RawSize=0x%08x\n",
               i, name, vaddr, vsize, rawptr, rawsize);

        if (clr_rva >= vaddr && clr_rva < vaddr + vsize) {
            uint32_t offset = rawptr + (clr_rva - vaddr);
            printf("       ^ Contains CLR header at file offset 0x%x\n", offset);
        }
    }

    free(data);
    return 0;
}
