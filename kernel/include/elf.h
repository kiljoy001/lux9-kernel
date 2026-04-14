/* ELF format definitions for x86_64 */
#pragma once

/* ELF magic number */
#define ELF_MAGIC 0x464c457f  /* "\x7fELF" in little-endian */

/* ELF classes */
#define ELFCLASS32 1
#define ELFCLASS64 2

/* ELF data encodings */
#define ELFDATA2LSB 1  /* Little endian */
#define ELFDATA2MSB 2  /* Big endian */

/* ELF types */
#define ET_NONE   0
#define ET_REL    1
#define ET_EXEC   2
#define ET_DYN    3
#define ET_CORE   4

/* ELF machine types */
#define EM_X86_64 62

/* Program header types */
#define PT_NULL    0
#define PT_LOAD    1
#define PT_DYNAMIC 2
#define PT_INTERP  3
#define PT_NOTE    4
#define PT_SHLIB   5
#define PT_PHDR    6
#define PT_TLS     7

/* Program header flags */
#define PF_X 0x1  /* Execute */
#define PF_W 0x2  /* Write */
#define PF_R 0x4  /* Read */

/* ELF64 header */
typedef struct {
	unsigned char e_ident[16];  /* Magic number and other info */
	u16int e_type;              /* Object file type */
	u16int e_machine;           /* Architecture */
	u32int e_version;           /* Object file version */
	u64int e_entry;             /* Entry point virtual address */
	u64int e_phoff;             /* Program header table file offset */
	u64int e_shoff;             /* Section header table file offset */
	u32int e_flags;             /* Processor-specific flags */
	u16int e_ehsize;            /* ELF header size in bytes */
	u16int e_phentsize;         /* Program header table entry size */
	u16int e_phnum;             /* Program header table entry count */
	u16int e_shentsize;         /* Section header table entry size */
	u16int e_shnum;             /* Section header table entry count */
	u16int e_shstrndx;          /* Section header string table index */
} Elf64_Ehdr;

/* ELF64 program header */
typedef struct {
	u32int p_type;              /* Segment type */
	u32int p_flags;             /* Segment flags */
	u64int p_offset;            /* Segment file offset */
	u64int p_vaddr;             /* Segment virtual address */
	u64int p_paddr;             /* Segment physical address */
	u64int p_filesz;            /* Segment size in file */
	u64int p_memsz;             /* Segment size in memory */
	u64int p_align;             /* Segment alignment */
} Elf64_Phdr;

/* Helper macros to read ELF values (handles endianness) */
#define ELF_MAGIC_0 0x7f
#define ELF_MAGIC_1 'E'
#define ELF_MAGIC_2 'L'
#define ELF_MAGIC_3 'F'
