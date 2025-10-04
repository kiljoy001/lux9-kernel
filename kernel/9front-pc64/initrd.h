/* initrd.h - Initial ramdisk support */
#pragma once

/* TAR header format (POSIX ustar) */
struct tar_header {
	char name[100];
	char mode[8];
	char uid[8];
	char gid[8];
	char size[12];
	char mtime[12];
	char checksum[8];
	char typeflag;
	char linkname[100];
	char magic[6];      /* "ustar\0" */
	char version[2];    /* "00" */
	char uname[32];
	char gname[32];
	char devmajor[8];
	char devminor[8];
	char prefix[155];
	char pad[12];
};

/* File entry in initrd */
struct initrd_file {
	char name[256];
	void *data;
	usize size;
	struct initrd_file *next;
};

/* Global initrd file list */
extern struct initrd_file *initrd_root;
extern void *initrd_base;
extern usize initrd_size;

/* Functions */
void initrd_init(void *addr, usize len);
void* initrd_find(const char *path);
usize initrd_filesize(const char *path);
int initrd_read(const char *path, void *buf, usize offset, usize len);
void initrd_list(void);
