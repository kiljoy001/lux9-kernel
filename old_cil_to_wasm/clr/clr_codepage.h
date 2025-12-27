/*
 * CLR Code Page System
 *
 * Manages IL bytecode in exchange pages, decoupled from tasklets.
 * Supports "many-readers-one-writer" via borrow checker.
 */

#ifndef CLR_CODEPAGE_H
#define CLR_CODEPAGE_H

#define CODEPAGE_MAGIC 0x434F4445 /* "CODE" */
#define CODEPAGE_MAX_METHODS 64
#define CODEPAGE_DATA_SIZE 3584 /* 4KB - header overhead */

/* Forward declarations */
typedef struct CodePage CodePage;
typedef struct CodePageHandle CodePageHandle;

/* Method entry in the code page directory */
typedef struct CodePageMethod {
  unsigned int token;  /* Method token */
  unsigned int offset; /* Offset into il_code */
  unsigned int size;   /* IL size for this method */
} CodePageMethod;

/* The actual 4KB page structure (mapped) */
struct CodePage {
  unsigned int magic;
  unsigned int assembly_id;
  unsigned int method_count;
  unsigned int total_il_size;
  unsigned long borrow_key; /* Key for borrow checker */
  CodePageMethod methods[CODEPAGE_MAX_METHODS];
  unsigned char il_code[CODEPAGE_DATA_SIZE];
};

/* Handle to a managed code page */
struct CodePageHandle {
  CodePage *page; /* Direct pointer if mapped */
  void *exchange; /* ExchangeHandle */
  void *cap;      /* UserCapability */
  void *lock;     /* Protection */
  CodePageHandle *next;
};

/* API */
void clr_codepage_init(void);
CodePageHandle *clr_codepage_create(unsigned int assembly_id);
int clr_codepage_add_method(CodePageHandle *h, unsigned int token, void *il,
                            unsigned int size);
void clr_codepage_seal(
    CodePageHandle *h); /* Checksums, makes read-only to writer */
void clr_codepage_destroy(CodePageHandle *h);

/* Borrow API wrappers */
int clr_codepage_acquire_read(CodePageHandle *h, void *proc);
int clr_codepage_release_read(CodePageHandle *h, void *proc);

#endif /* CLR_CODEPAGE_H */
