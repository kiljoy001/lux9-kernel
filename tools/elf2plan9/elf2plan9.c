/*
 * elf2plan9.c - Convert ELF64 executable to Plan 9 a.out format
 *
 * Usage: elf2plan9 <input.elf> <output.out>
 *
 * This tool converts a static ELF64 executable (like bflat output)
 * to Plan 9's a.out format for execution on 9front.
 */

#include <arpa/inet.h> /* For htonl (big-endian conversion) */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ELF64 header structures */
typedef struct {
  unsigned char e_ident[16];
  uint16_t e_type;
  uint16_t e_machine;
  uint32_t e_version;
  uint64_t e_entry;
  uint64_t e_phoff;
  uint64_t e_shoff;
  uint32_t e_flags;
  uint16_t e_ehsize;
  uint16_t e_phentsize;
  uint16_t e_phnum;
  uint16_t e_shentsize;
  uint16_t e_shnum;
  uint16_t e_shstrndx;
} Elf64_Ehdr;

typedef struct {
  uint32_t p_type;
  uint32_t p_flags;
  uint64_t p_offset;
  uint64_t p_vaddr;
  uint64_t p_paddr;
  uint64_t p_filesz;
  uint64_t p_memsz;
  uint64_t p_align;
} Elf64_Phdr;

/* ELF constants */
#define EI_MAG0 0
#define ELFMAG0 0x7f
#define ELFMAG1 'E'
#define ELFMAG2 'L'
#define ELFMAG3 'F'
#define ELFCLASS64 2
#define PT_LOAD 1
#define PF_X 0x1
#define PF_W 0x2
#define PF_R 0x4

/* Plan 9 a.out header */
typedef struct {
  uint32_t magic;
  uint32_t text;
  uint32_t data;
  uint32_t bss;
  uint32_t syms;
  uint32_t entry;
  uint32_t spsz;
  uint32_t pcsz;
} Plan9Exec;

/* Plan 9 magic numbers */
#define I_MAGIC ((0x00008000) | 11) /* i386 */
#define S_MAGIC ((0x00008000) | 26) /* amd64 (6l uses this) */

/* Write 32-bit big-endian value */
static void write_be32(FILE *f, uint32_t v) {
  uint32_t be = htonl(v);
  fwrite(&be, 4, 1, f);
}

/* Write 64-bit big-endian value */
static void write_be64(FILE *f, uint64_t v) {
  uint32_t high = htonl(v >> 32);
  uint32_t low = htonl(v & 0xFFFFFFFF);
  fwrite(&high, 4, 1, f);
  fwrite(&low, 4, 1, f);
}

/* Read entire file into buffer */
static unsigned char *read_file(const char *path, size_t *size) {
  FILE *f = fopen(path, "rb");
  if (!f)
    return NULL;

  fseek(f, 0, SEEK_END);
  *size = ftell(f);
  fseek(f, 0, SEEK_SET);

  unsigned char *buf = malloc(*size);
  if (!buf) {
    fclose(f);
    return NULL;
  }

  fread(buf, 1, *size, f);
  fclose(f);
  return buf;
}

int main(int argc, char **argv) {
  if (argc != 3) {
    fprintf(stderr, "Usage: %s <input.elf> <output.out>\n", argv[0]);
    return 1;
  }

  const char *infile = argv[1];
  const char *outfile = argv[2];

  size_t elfsize;
  unsigned char *elfdata = read_file(infile, &elfsize);
  if (!elfdata) {
    fprintf(stderr, "Error: cannot read %s\n", infile);
    return 1;
  }

  Elf64_Ehdr *ehdr = (Elf64_Ehdr *)elfdata;
  if (ehdr->e_ident[EI_MAG0] != ELFMAG0 || ehdr->e_ident[1] != ELFMAG1 ||
      ehdr->e_ident[2] != ELFMAG2 || ehdr->e_ident[3] != ELFMAG3) {
    fprintf(stderr, "Error: %s is not an ELF file\n", infile);
    free(elfdata);
    return 1;
  }

  /* Find text extent */
  Elf64_Phdr *phdrs = (Elf64_Phdr *)(elfdata + ehdr->e_phoff);
  uintptr_t min_text = ~0ULL;
  uintptr_t max_text = 0;

  uintptr_t min_data = ~0ULL;
  uintptr_t max_data = 0;
  uintptr_t max_data_mem = 0;

  for (int i = 0; i < ehdr->e_phnum; i++) {
    Elf64_Phdr *ph = &phdrs[i];
    if (ph->p_type != PT_LOAD)
      continue;

    if (ph->p_flags & PF_X) {
      if (ph->p_vaddr < min_text)
        min_text = ph->p_vaddr;
      if (ph->p_vaddr + ph->p_filesz > max_text)
        max_text = ph->p_vaddr + ph->p_filesz;
    } else if (ph->p_flags & PF_W) {
      // Check if this is merged with text (OMAGIC)
      if (ph->p_vaddr >= min_text && ph->p_vaddr < max_text)
        continue;

      if (ph->p_vaddr < min_data)
        min_data = ph->p_vaddr;
      if (ph->p_vaddr + ph->p_filesz > max_data)
        max_data = ph->p_vaddr + ph->p_filesz;
      if (ph->p_vaddr + ph->p_memsz > max_data_mem)
        max_data_mem = ph->p_vaddr + ph->p_memsz;
    }
  }

  uint64_t text_size = (max_text > min_text) ? max_text - min_text : 0;
  uint64_t data_size = (max_data > min_data) ? max_data - min_data : 0;
  uint64_t bss_size = (max_data_mem > max_data) ? max_data_mem - max_data : 0;

  // Create output buffer for text
  unsigned char *text_buf = calloc(1, text_size);

  // Fill text buffer from segments
  for (int i = 0; i < ehdr->e_phnum; i++) {
    Elf64_Phdr *ph = &phdrs[i];
    if (ph->p_type != PT_LOAD)
      continue;
    if (ph->p_flags & PF_X) {
      uint64_t offset = ph->p_vaddr - min_text;
      memcpy(text_buf + offset, elfdata + ph->p_offset, ph->p_filesz);
    }
  }

  FILE *outf = fopen(outfile, "wb");
  if (!outf)
    return 1;

  /* Write Plan 9 header (40 bytes for S_MAGIC) */
  write_be32(outf, S_MAGIC);             /* magic */
  write_be32(outf, (uint32_t)text_size); /* text size */
  write_be32(outf, (uint32_t)data_size); /* data size */
  write_be32(outf, (uint32_t)bss_size);  /* bss size */
  write_be32(outf, 0);                   /* syms */
  write_be32(outf,
             (uint32_t)ehdr->e_entry); /* entry (32-bit part) - ignored? */
  write_be32(outf, 0);                 /* spsz */
  write_be32(outf, 0);                 /* pcsz */

  /* Extended header: entry point (64-bit) */
  write_be64(outf, (uint64_t)ehdr->e_entry);

  fwrite(text_buf, 1, text_size, outf);

  // Write data if separate
  if (data_size > 0 && min_data != ~0ULL) {
    // Not implementing split data copy yet for simplicity, assuming
    // OMAGIC/merged
  }

  fclose(outf);
  printf("Converted %s -> %s (Extended Header)\n", infile, outfile);
  printf("  Entry: 0x%lx\n", (unsigned long)ehdr->e_entry);
  free(elfdata);
  free(text_buf);
  return 0;
}
