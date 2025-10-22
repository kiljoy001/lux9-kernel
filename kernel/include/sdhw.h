/* sdhw.h - Hardware function pointers for SD device driver */
#ifndef SDHW_H
#define SDHW_H

/* Function pointer types for I/O operations */
extern int (*sd_inb)(int);
extern void (*sd_outb)(int, int);
extern ulong (*sd_inl)(int);
extern void (*sd_outl)(int, ulong);
extern void (*sd_insb)(int, void*, int);
extern void (*sd_inss)(int, void*, int);
extern void (*sd_outsb)(int, void*, int);
extern void (*sd_outss)(int, void*, int);
extern Pcidev* (*sd_pcimatch)(Pcidev* prev, int vid, int did);
extern void (*sd_microdelay)(int);

/* Initialize function pointers */
void sdhw_init(void);

#endif /* SDHW_H */