/* SeaBIOS El Torito boot support header */

/* SeaBIOS's exact El Torito structures */
struct eltorito_s {
    u8int media;
    u8int emulated_drive;
    u32int ilba;
    u16int sector_count;
    u16int load_segment;
    u16int buffer_segment;
    
    /* Geometry */
    struct {
        u16int sptcyl;  /* sectors per track/cylinder */
        u16int cyllow;
        u8int heads;
    } chs;
    
    u8int controller_index;
    u8int device_spec;
    u32int size;
};

/* Function declarations */
int seabios_cdrom_boot(char *isofile, struct eltorito_s *CDEmu, void **bootimg, long *bootsize);
int seabios_is_bootable(char *isofile);