#include "dat.h"
#include "devregistry.h"
#include "fns.h"
#include "io.h"
#include "mem.h"
#include "pci.h"
#include "pciframework.h"
#include "portlib.h"
#include "u.h"

enum {
  PciADDR = 0xCF8,
  PciDATA = 0xCFC,

  MSIXCtrl = 0x02,
  MSICtrl = 0x02,
  MSIAddr = 0x04,
  MSIData32 = 0x08,
  MSIData64 = 0x0C,
};

typedef struct Pcisiz Pcisiz;
struct Pcisiz {
  Pcidev *dev;
  vlong siz;
  int bar;
  int typ;
};

static struct {
  Lock lock;
  PCIDriver *drivers;
  int driver_count;
  Device **pci_devices;
  int pci_device_count;
  int pci_device_capacity;
} pci_framework;

static Lock pcicfglock;
static Lock pciscanlock;
static Pcidev *pcilist;
static Pcidev **pcitail = &pcilist;
static int pcicfgmode = -1; /* -1 unknown, 0 unavailable, 1 mode 1 */
static int pciscanned;
static int pcifmtinstalled;

int pcimaxdno = 31;
Pcidev *pciparentdev;

static char *bustypes[] = {
    "CBUSI", "CBUSII", "EISA", "FUTURE", "INTERN", "ISA",
    "MBI",   "MBII",   "MCA",  "MPI",    "MPSA",   "NUBUS",
    "PCI",   "PCMCIA", "TC",   "VL",     "VME",    "XPRESS",
};

static PCIClass pci_classes[] = {
    {0x00, 0x00, 0x00, "Legacy", "Any"},
    {0x01, 0x00, 0x00, "Mass Storage", "SCSI"},
    {0x01, 0x01, 0x00, "Mass Storage", "IDE"},
    {0x01, 0x02, 0x00, "Mass Storage", "Floppy"},
    {0x01, 0x05, 0x00, "Mass Storage", "ATA"},
    {0x01, 0x06, 0x00, "Mass Storage", "SATA"},
    {0x01, 0x80, 0x00, "Mass Storage", "Other"},
    {0x02, 0x00, 0x00, "Network", "Ethernet"},
    {0x02, 0x80, 0x00, "Network", "Other"},
    {0x03, 0x00, 0x00, "Display", "VGA"},
    {0x03, 0x01, 0x00, "Display", "XGA"},
    {0x03, 0x80, 0x00, "Display", "Other"},
    {0x04, 0x00, 0x00, "Multimedia", "Video"},
    {0x04, 0x01, 0x00, "Multimedia", "Audio"},
    {0x04, 0x80, 0x00, "Multimedia", "Other"},
    {0x06, 0x00, 0x00, "Bridge", "Host"},
    {0x06, 0x01, 0x00, "Bridge", "ISA"},
    {0x06, 0x04, 0x00, "Bridge", "PCI"},
    {0x06, 0x80, 0x00, "Bridge", "Other"},
    {0x07, 0x00, 0x00, "Simple Communication", "Serial"},
    {0x07, 0x01, 0x00, "Simple Communication", "Parallel"},
    {0x07, 0x80, 0x00, "Simple Communication", "Other"},
    {0x08, 0x00, 0x00, "Base System", "PIC"},
    {0x08, 0x01, 0x00, "Base System", "DMA"},
    {0x08, 0x02, 0x00, "Base System", "Timer"},
    {0x08, 0x03, 0x00, "Base System", "RTC"},
    {0x08, 0x80, 0x00, "Base System", "Other"},
    {0x09, 0x00, 0x00, "Input", "Keyboard"},
    {0x09, 0x01, 0x00, "Input", "Digitizer"},
    {0x09, 0x02, 0x00, "Input", "Mouse"},
    {0x09, 0x80, 0x00, "Input", "Other"},
    {0x0C, 0x00, 0x00, "Serial Bus", "FireWire"},
    {0x0C, 0x01, 0x00, "Serial Bus", "ACCESS"},
    {0x0C, 0x02, 0x00, "Serial Bus", "SSA"},
    {0x0C, 0x03, 0x00, "Serial Bus", "USB"},
    {0x0C, 0x04, 0x00, "Serial Bus", "Fiber"},
    {0x0C, 0x05, 0x00, "Serial Bus", "SMBus"},
    {0x0C, 0x80, 0x00, "Serial Bus", "Other"},
    {0xFF, 0xFF, 0xFF, "Unknown", "Unknown"},
};

static int pcicfgrw8raw(int, int, int, int);
static int pcicfgrw16raw(int, int, int, int);
static int pcicfgrw32raw(int, int, int, int);
static void pciensurecfginit(void);
static int pciensureenumerated(void);
static Pcidev *pcidevalloc(void);
static void pcisetwin(Pcidev *p, uvlong base, uvlong limit);
static int pcisizcmp(const void *a, const void *b);
static vlong pcimask(vlong v);
static int pcivalidwin(Pcidev *p, uvlong base, uvlong limit);
static int pcivalidbar(Pcidev *p, uvlong bar, vlong size);
static int pciscanconfigured(int bno, Pcidev **list, Pcidev *parent);
static int pci_framework_has_device(Device *dev);
static int ahci_probe(Pcidev *pcidev);
static int ide_probe(Pcidev *pcidev);
static int usb_probe(Pcidev *pcidev);
static int ethernet_probe(Pcidev *pcidev);
static int matchcap(Pcidev *, int, int, int);
static int matchhtcap(Pcidev *, int, int, int);
static int pcimsixdisable(Pcidev *p);
static int pcigetmsi(Pcidev *p);
static int pcigetpmrb(Pcidev *p);
extern int inb(int);
extern ulong inl(int);
extern void outb(int, int);
extern void outl(int, ulong);

int (*pcicfgrw8)(int, int, int, int) = pcicfgrw8raw;
int (*pcicfgrw16)(int, int, int, int) = pcicfgrw16raw;
int (*pcicfgrw32)(int, int, int, int) = pcicfgrw32raw;

PCIDriver ahci_driver = {
    .name = "ahci",
    .probe = ahci_probe,
    .vendor_id = 0,
    .device_id = 0,
    .base_class = 0x01,
    .sub_class = 0x06,
};

PCIDriver ide_driver = {
    .name = "ide",
    .probe = ide_probe,
    .vendor_id = 0,
    .device_id = 0,
    .base_class = 0x01,
    .sub_class = 0x01,
};

PCIDriver usb_driver = {
    .name = "usb",
    .probe = usb_probe,
    .vendor_id = 0,
    .device_id = 0,
    .base_class = 0x0C,
    .sub_class = 0x03,
};

PCIDriver ethernet_driver = {
    .name = "ethernet",
    .probe = ethernet_probe,
    .vendor_id = 0,
    .device_id = 0,
    .base_class = 0x02,
    .sub_class = 0x00,
};

int tbdffmt(Fmt *fmt) {
  int tbdf, type;

  switch (fmt->r) {
  default:
    return fmtstrcpy(fmt, "(tbdffmt)");
  case 'T':
    tbdf = va_arg(fmt->args, int);
    if (tbdf == BUSUNKNOWN)
      return fmtstrcpy(fmt, "unknown");
    type = BUSTYPE(tbdf);
    if (type >= 0 && type < nelem(bustypes))
      return fmtprint(fmt, "%s.%d.%d.%d", bustypes[type], BUSBNO(tbdf),
                      BUSDNO(tbdf), BUSFNO(tbdf));
    return fmtprint(fmt, "%d.%d.%d.%d", type, BUSBNO(tbdf), BUSDNO(tbdf),
                    BUSFNO(tbdf));
  }
}

static void
pciensurefmt(void)
{
  if (pcifmtinstalled)
    return;
  fmtinstall('T', tbdffmt);
  pcifmtinstalled = 1;
}

void
pcicfginit(void)
{
  int n;

  if (pcicfgmode != -1)
    return;

  pciensurefmt();

  n = inl(PciADDR);
  if ((n & 0x7F000000) == 0) {
    outl(PciADDR, 0x80000000);
    outb(PciADDR + 3, 0);
    if (inl(PciADDR) == 0x80000000) {
      ioalloc(PciADDR, 4, 0, "pcicfg.addr");
      ioalloc(PciDATA, 4, 0, "pcicfg.data");
      pcicfgmode = 1;
      pcimaxdno = 31;
    }
  }
  outl(PciADDR, n);

  if (pcicfgmode == -1)
    pcicfgmode = 0;
}

static void
pciensurecfginit(void)
{
  if (pcicfgmode == -1)
    pcicfginit();
}

static int
pcicfgrw8raw(int tbdf, int rno, int data, int read)
{
  int o;

  if (pcicfgmode != 1)
    return -1;

  o = rno & 0x03;
  rno &= ~0x03;
  outl(PciADDR, 0x80000000 | BUSBDF(tbdf) | rno);
  if (read)
    data = inb(PciDATA + o);
  else
    outb(PciDATA + o, data);
  outl(PciADDR, 0);
  return data;
}

static int
pcicfgrw16raw(int tbdf, int rno, int data, int read)
{
  int o;

  if (pcicfgmode != 1)
    return -1;

  o = rno & 0x02;
  rno &= ~0x03;
  outl(PciADDR, 0x80000000 | BUSBDF(tbdf) | rno);
  if (read)
    data = ins(PciDATA + o);
  else
    outs(PciDATA + o, data);
  outl(PciADDR, 0);
  return data;
}

static int
pcicfgrw32raw(int tbdf, int rno, int data, int read)
{
  if (pcicfgmode != 1)
    return -1;

  rno &= ~0x03;
  outl(PciADDR, 0x80000000 | BUSBDF(tbdf) | rno);
  if (read)
    data = inl(PciDATA);
  else
    outl(PciDATA, data);
  outl(PciADDR, 0);
  return data;
}

static Pcidev *
pcidevalloc(void)
{
  Pcidev *p;

  p = xalloc_resident(sizeof(*p));
  if (p == nil)
    panic("pci: no memory for Pcidev");
  memset(p, 0, sizeof(*p));
  return p;
}

void
pcidevfree(Pcidev *p)
{
  Pcidev **l;

  if (p == nil)
    return;

  while (p->bridge != nil)
    pcidevfree(p->bridge);

  if (p->parent != nil) {
    for (l = &p->parent->bridge; *l != nil; l = &(*l)->link) {
      if (*l == p) {
        *l = p->link;
        break;
      }
    }
  }
  for (l = &pcilist; *l != nil; l = &(*l)->list) {
    if (*l == p) {
      if ((*l = p->list) == nil)
        pcitail = l;
      break;
    }
  }
  xfree_resident(p);
}

int
pcicfgr8(Pcidev *p, int rno)
{
  int data;

  if (p == nil)
    return -1;
  pciensurecfginit();
  if (pcicfgmode != 1)
    return -1;

  ilock(&pcicfglock);
  pciparentdev = p->parent;
  data = pcicfgrw8(p->tbdf, rno, 0, 1);
  iunlock(&pcicfglock);
  return data;
}

void
pcicfgw8(Pcidev *p, int rno, int data)
{
  if (p == nil)
    return;
  pciensurecfginit();
  if (pcicfgmode != 1)
    return;

  ilock(&pcicfglock);
  pciparentdev = p->parent;
  pcicfgrw8(p->tbdf, rno, data, 0);
  iunlock(&pcicfglock);
}

int
pcicfgr16(Pcidev *p, int rno)
{
  int data;

  if (p == nil)
    return -1;
  pciensurecfginit();
  if (pcicfgmode != 1)
    return -1;

  ilock(&pcicfglock);
  pciparentdev = p->parent;
  data = pcicfgrw16(p->tbdf, rno, 0, 1);
  iunlock(&pcicfglock);
  return data;
}

void
pcicfgw16(Pcidev *p, int rno, int data)
{
  if (p == nil)
    return;
  pciensurecfginit();
  if (pcicfgmode != 1)
    return;

  ilock(&pcicfglock);
  pciparentdev = p->parent;
  pcicfgrw16(p->tbdf, rno, data, 0);
  iunlock(&pcicfglock);
}

int
pcicfgr32(Pcidev *p, int rno)
{
  int data;

  if (p == nil)
    return -1;
  pciensurecfginit();
  if (pcicfgmode != 1)
    return -1;

  ilock(&pcicfglock);
  pciparentdev = p->parent;
  data = pcicfgrw32(p->tbdf, rno, 0, 1);
  iunlock(&pcicfglock);
  return data;
}

void
pcicfgw32(Pcidev *p, int rno, int data)
{
  if (p == nil)
    return;
  pciensurecfginit();
  if (pcicfgmode != 1)
    return;

  ilock(&pcicfglock);
  pciparentdev = p->parent;
  pcicfgrw32(p->tbdf, rno, data, 0);
  iunlock(&pcicfglock);
}

vlong
pcibarsize(Pcidev *p, int rno)
{
  vlong size;
  int v;

  if (p == nil)
    return 0;
  pciensurecfginit();
  if (pcicfgmode != 1)
    return 0;

  ilock(&pcicfglock);
  pciparentdev = p->parent;

  v = pcicfgrw32(p->tbdf, rno, 0, 1);
  pcicfgrw32(p->tbdf, rno, -1, 0);
  size = (int)pcicfgrw32(p->tbdf, rno, 0, 1);
  pcicfgrw32(p->tbdf, rno, v, 0);

  if (rno == PciEBAR0 || rno == PciEBAR1)
    size &= ~0x7FFLL;
  else if (v & 1)
    size = (short)size & ~0x3LL;
  else {
    size &= ~0xFLL;
    if (size >= 0 && (v & 7) == 4 && rno < PciBAR0 + 4 * (nelem(p->mem) - 1)) {
      rno += 4;
      v = pcicfgrw32(p->tbdf, rno, 0, 1);
      pcicfgrw32(p->tbdf, rno, -1, 0);
      size |= (vlong)pcicfgrw32(p->tbdf, rno, 0, 1) << 32;
      pcicfgrw32(p->tbdf, rno, v, 0);
    }
  }

  iunlock(&pcicfglock);

  size = -size;
  if (size < 0 || (size & (size - 1)) != 0)
    return 0;
  return size;
}

void
pcisetbar(Pcidev *p, int rno, uvlong bar)
{
  if (p == nil)
    return;
  pciensurecfginit();
  if (pcicfgmode != 1)
    return;

  ilock(&pcicfglock);
  pciparentdev = p->parent;
  pcicfgrw32(p->tbdf, rno, bar, 0);
  if ((bar & 7) == 4 && rno >= PciBAR0 &&
      rno < PciBAR0 + 4 * (nelem(p->mem) - 1))
    pcicfgrw32(p->tbdf, rno + 4, bar >> 32, 0);
  iunlock(&pcicfglock);
}

static void
pcisetwin(Pcidev *p, uvlong base, uvlong limit)
{
  if (p == nil)
    return;
  pciensurecfginit();
  if (pcicfgmode != 1)
    return;

  ilock(&pcicfglock);
  pciparentdev = p->parent;
  if (base & 1) {
    pcicfgrw16(p->tbdf, PciIBR, (limit & 0xF000) | ((base & 0xF000) >> 8), 0);
    pcicfgrw32(p->tbdf, PciIUBR, (limit & 0xFFFF0000) | (base >> 16), 0);
  } else if (base & 8) {
    pcicfgrw32(p->tbdf, PciPMBR,
               (limit & 0xFFF00000) | ((base & 0xFFF00000) >> 16), 0);
    pcicfgrw32(p->tbdf, PciPUBR, base >> 32, 0);
    pcicfgrw32(p->tbdf, PciPULR, limit >> 32, 0);
  } else {
    pcicfgrw32(p->tbdf, PciMBR,
               (limit & 0xFFF00000) | ((base & 0xFFF00000) >> 16), 0);
  }
  iunlock(&pcicfglock);
}

static int
pcisizcmp(const void *a, const void *b)
{
  const Pcisiz *aa, *bb;

  aa = a;
  bb = b;
  if (aa->siz > bb->siz)
    return 1;
  if (aa->siz < bb->siz)
    return -1;
  return 0;
}

static vlong
pcimask(vlong v)
{
  uvlong mask;

  for (mask = 1ULL << 63; mask != 0; mask >>= 1) {
    if (mask & v)
      break;
  }
  mask--;
  if (v & mask) {
    v |= mask;
    v++;
  }
  return v;
}

void
pcibusmap(Pcidev *root, uvlong *pmema, ulong *pioa, int wrreg)
{
  Pcidev *p;
  Pcisiz *table, *tptr, *mtb, *itb;
  uvlong mema, smema;
  ulong ioa, sioa, v;
  vlong hole, size;
  int ntb, i, rno;

  if (pmema == nil || pioa == nil)
    return;

  ioa = *pioa;
  mema = *pmema;

  ntb = 0;
  for (p = root; p != nil; p = p->link)
    ntb++;
  ntb *= (PciCIS - PciBAR0) / 4;

  table = xalloc_resident((2 * ntb + 1) * sizeof(Pcisiz));
  if (table == nil)
    panic("pcibusmap: can't allocate memory");
  itb = table;
  mtb = table + ntb;

  for (p = root; p != nil; p = p->link) {
    if (p->ccrb == 0x06) {
      if (p->ccru == 0x07) {
        if (pcicfgr32(p, PciBAR0) & 1)
          continue;
        size = pcibarsize(p, PciBAR0);
        if (size == 0)
          continue;
        mtb->dev = p;
        mtb->bar = 0;
        mtb->siz = size;
        mtb->typ = 0;
        mtb++;
        continue;
      }

      if (p->ccru != 0x04 || p->bridge == nil)
        continue;

      sioa = ioa;
      smema = mema;
      pcibusmap(p->bridge, &smema, &sioa, 0);

      hole = pcimask(sioa - ioa);
      if (hole < (1 << 12))
        hole = 1 << 12;
      itb->dev = p;
      itb->bar = -1;
      itb->siz = hole;
      itb->typ = 0;
      itb++;

      hole = pcimask(smema - mema);
      if (hole < (1 << 20))
        hole = 1 << 20;
      mtb->dev = p;
      mtb->bar = -1;
      mtb->siz = hole;
      mtb->typ = 0;
      mtb++;

      size = pcibarsize(p, PciEBAR1);
      if (size != 0) {
        mtb->dev = p;
        mtb->bar = -3;
        mtb->siz = size;
        mtb->typ = 0;
        mtb++;
      }
      continue;
    }

    size = pcibarsize(p, PciEBAR0);
    if (size != 0) {
      mtb->dev = p;
      mtb->bar = -2;
      mtb->siz = size;
      mtb->typ = 0;
      mtb++;
    }

    for (i = 0; i < nelem(p->mem); i++) {
      rno = PciBAR0 + i * 4;
      v = pcicfgr32(p, rno);
      size = pcibarsize(p, rno);
      if (size == 0)
        continue;
      if (v & 1) {
        itb->dev = p;
        itb->bar = i;
        itb->siz = size;
        itb->typ = 1;
        itb++;
      } else {
        mtb->dev = p;
        mtb->bar = i;
        mtb->siz = size;
        mtb->typ = v & 7;
        if (mtb->typ == 4)
          i++;
        mtb++;
      }
    }
  }

  qsort(table, itb - table, sizeof(Pcisiz), pcisizcmp);
  tptr = table + ntb;
  qsort(tptr, mtb - tptr, sizeof(Pcisiz), pcisizcmp);

  for (tptr = table; tptr < itb; tptr++) {
    hole = tptr->siz;
    if (tptr->bar == -1)
      hole = 1 << 12;
    ioa = (ioa + hole - 1) & ~(hole - 1);
    if (wrreg) {
      p = tptr->dev;
      if (tptr->bar == -1) {
        p->ioa.bar = ioa;
        p->ioa.size = tptr->siz;
      } else {
        p->mem[tptr->bar].size = tptr->siz;
        p->mem[tptr->bar].bar = ioa | 1;
        pcisetbar(p, PciBAR0 + tptr->bar * 4, p->mem[tptr->bar].bar);
      }
    }
    ioa += tptr->siz;
  }

  for (tptr = table + ntb; tptr < mtb; tptr++) {
    hole = tptr->siz;
    if (tptr->bar == -1)
      hole = 1 << 20;
    mema = (mema + hole - 1) & ~((uvlong)hole - 1);
    if (wrreg) {
      p = tptr->dev;
      if (tptr->bar == -1) {
        p->mema.bar = mema;
        p->mema.size = tptr->siz;
      } else if (tptr->bar == -2) {
        p->rom.bar = mema | 1;
        p->rom.size = tptr->siz;
        pcisetbar(p, PciEBAR0, p->rom.bar);
      } else if (tptr->bar == -3) {
        p->rom.bar = mema | 1;
        p->rom.size = tptr->siz;
        pcisetbar(p, PciEBAR1, p->rom.bar);
      } else {
        p->mem[tptr->bar].size = tptr->siz;
        p->mem[tptr->bar].bar = mema | tptr->typ;
        pcisetbar(p, PciBAR0 + tptr->bar * 4, p->mem[tptr->bar].bar);
      }
    }
    mema += tptr->siz;
  }

  *pmema = mema;
  *pioa = ioa;
  xfree_resident(table);

  if (!wrreg)
    return;

  for (p = root; p != nil; p = p->link) {
    if (p->bridge == nil) {
      pcienable(p);
      continue;
    }
    pcisetwin(p, p->ioa.bar | 1, p->ioa.bar + p->ioa.size - 1);
    pcisetwin(p, p->mema.bar | 0, p->mema.bar + p->mema.size - 1);
    pcisetwin(p, 0xFFF00000 | 8, 0);
    pcienable(p);
    sioa = p->ioa.bar;
    smema = p->mema.bar;
    pcibusmap(p->bridge, &smema, &sioa, 1);
  }
}

static int
pcivalidwin(Pcidev *p, uvlong base, uvlong limit)
{
  Pcidev *bridge;

  bridge = p->parent;
  if (base & 1) {
    base &= ~3;
    if (base > limit)
      return 0;
    if (bridge == nil)
      return 1;
    return base >= bridge->ioa.bar && limit < bridge->ioa.bar + bridge->ioa.size;
  }

  base &= ~0xFULL;
  if (base > limit)
    return 0;
  if (bridge == nil)
    return 1;
  if (base >= bridge->mema.bar && limit < bridge->mema.bar + bridge->mema.size)
    return 1;
  return base >= bridge->prefa.bar && limit < bridge->prefa.bar + bridge->prefa.size;
}

static int
pcivalidbar(Pcidev *p, uvlong bar, vlong size)
{
  if (bar & 1) {
    bar &= ~3;
    if (bar == 0 || size < 4 || (bar & (size - 1)) != 0)
      return 0;
    return pcivalidwin(p, bar | 1, bar + size - 1);
  }

  bar &= ~0xFULL;
  if (bar == 0 || size < 16 || (bar & (size - 1)) != 0)
    return 0;
  return pcivalidwin(p, bar | 0, bar + size - 1);
}

static int
pciscanconfigured(int bno, Pcidev **list, Pcidev *parent)
{
  Pcidev *p, *head, **tail;
  int dno, fno, hdt, i, l, maxfno, maxubn, rno, sbn, tbdf, ubn;

  maxubn = bno;
  head = nil;
  tail = nil;

  for (dno = 0; dno <= pcimaxdno; dno++) {
    maxfno = 0;
    for (fno = 0; fno <= maxfno; fno++) {
      tbdf = MKBUS(BusPCI, bno, dno, fno);

      ilock(&pcicfglock);
      pciparentdev = parent;
      l = pcicfgrw32(tbdf, PciVID, 0, 1);
      iunlock(&pcicfglock);
      if (l == 0xFFFFFFFF || l == 0)
        continue;

      p = pcidevalloc();
      p->parent = parent;
      p->tbdf = tbdf;
      p->vid = l;
      p->did = l >> 16;

      p->vid = pcicfgr16(p, PciVID);
      p->did = pcicfgr16(p, PciDID);
      if (p->vid == 0xFFFF || p->vid == 0) {
        xfree_resident(p);
        continue;
      }

      p->pcr = pcicfgr16(p, PciPCR);
      p->rid = pcicfgr8(p, PciRID);
      p->ccrp = pcicfgr8(p, PciCCRp);
      p->ccru = pcicfgr8(p, PciCCRu);
      p->ccrb = pcicfgr8(p, PciCCRb);
      p->cls = pcicfgr8(p, PciCLS);
      p->ltr = pcicfgr8(p, PciLTR);
      p->intl = pcicfgr8(p, PciINTL);

      hdt = pcicfgr8(p, PciHDT);
      if (hdt & 0x80)
        maxfno = MaxFNO;

      switch (p->ccrb) {
      case 0x00:
      case 0x01:
      case 0x02:
      case 0x03:
      case 0x04:
      case 0x07:
      case 0x08:
      case 0x09:
      case 0x0A:
      case 0x0B:
      case 0x0C:
      case 0x0D:
      case 0x0E:
      case 0x0F:
      case 0x10:
      case 0x11:
        if ((hdt & 0x7F) != 0)
          break;
        rno = PciBAR0;
        for (i = 0; i < nelem(p->mem); i++) {
          p->mem[i].bar = (ulong)pcicfgr32(p, rno);
          p->mem[i].size = pcibarsize(p, rno);
          if ((p->mem[i].bar & 7) == 4 && i < nelem(p->mem) - 1) {
            rno += 4;
            p->mem[i].bar |= (uvlong)pcicfgr32(p, rno) << 32;
            i++;
            p->mem[i].bar = 0;
            p->mem[i].size = 0;
          }
          rno += 4;
        }
        p->rom.bar = (ulong)pcicfgr32(p, PciEBAR0);
        p->rom.size = pcibarsize(p, PciEBAR0);
        break;
      case 0x06:
        if (p->ccru == 0x07) {
          p->mem[0].bar = (ulong)pcicfgr32(p, PciBAR0);
          p->mem[0].size = pcibarsize(p, PciBAR0);
          break;
        }
        if (p->ccru != 0x04)
          break;
        p->rom.bar = (ulong)pcicfgr32(p, PciEBAR1);
        p->rom.size = pcibarsize(p, PciEBAR1);
        break;
      default:
        break;
      }

      if (head != nil)
        *tail = p;
      else
        head = p;
      tail = &p->link;

      if (pcilist != nil)
        *pcitail = p;
      else
        pcilist = p;
      pcitail = &p->list;
    }
  }

  *list = head;
  for (p = head; p != nil; p = p->link) {
    switch (p->ccrb) {
    case 0x06:
      if (p->ccru == 0x04)
        break;
      /* fall through */
    default:
      for (i = 0; i < nelem(p->mem); i++) {
        if (p->mem[i].size == 0)
          continue;
        if (!pcivalidbar(p, p->mem[i].bar, p->mem[i].size)) {
          if (p->mem[i].bar & 1)
            p->mem[i].bar &= 3;
          else
            p->mem[i].bar &= 0xF;
          pcisetbar(p, PciBAR0 + i * 4, p->mem[i].bar);
        }
      }
      if (p->rom.size) {
        if ((p->rom.bar & 1) == 0 ||
            !pcivalidbar(p, p->rom.bar & ~0x7FFULL, p->rom.size)) {
          p->rom.bar = 0;
          pcisetbar(p, PciEBAR0, p->rom.bar);
        }
      }
      continue;
    }

    if (p->rom.size) {
      if ((p->rom.bar & 1) == 0 ||
          !pcivalidbar(p, p->rom.bar & ~0x7FFULL, p->rom.size)) {
        p->rom.bar = 0;
        pcisetbar(p, PciEBAR1, p->rom.bar);
      }
    }

    sbn = pcicfgr8(p, PciSBN);
    ubn = pcicfgr8(p, PciUBN);
    if (sbn == 0 || ubn == 0 || sbn <= bno || ubn < sbn)
      continue;
    if (ubn > maxubn)
      maxubn = ubn;
    pciscanconfigured(sbn, &p->bridge, p);
  }

  return maxubn;
}

int
pciscan(int bno, Pcidev **list, Pcidev *parent)
{
  return pciscanconfigured(bno, list, parent);
}

static int
pciensureenumerated(void)
{
  Pcidev *root;

  lock(&pciscanlock);
  if (pciscanned) {
    unlock(&pciscanlock);
    return 0;
  }

  pciensurecfginit();
  if (pcicfgmode != 1) {
    unlock(&pciscanlock);
    return -1;
  }

  pcilist = nil;
  pcitail = &pcilist;
  root = nil;
  pciscanconfigured(0, &root, nil);
  pciscanned = 1;
  unlock(&pciscanlock);
  return 0;
}

void
pcibussize(Pcidev *root, uvlong *msize, ulong *iosize)
{
  if (msize == nil || iosize == nil)
    return;
  *msize = 0;
  *iosize = 0;
  pcibusmap(root, msize, iosize, 0);
}

Pcidev *
pcimatch(Pcidev *prev, int vid, int did)
{
  if (pciensureenumerated() < 0)
    return nil;

  if (prev == nil)
    prev = pcilist;
  else
    prev = prev->list;

  while (prev != nil) {
    if ((vid == 0 || prev->vid == vid) && (did == 0 || prev->did == did))
      break;
    prev = prev->list;
  }
  return prev;
}

Pcidev *
pcimatchtbdf(int tbdf)
{
  Pcidev *pcidev;

  if (pciensureenumerated() < 0)
    return nil;

  for (pcidev = pcilist; pcidev != nil; pcidev = pcidev->list) {
    if (pcidev->tbdf == tbdf)
      return pcidev;
  }
  return nil;
}

uchar
pciipin(Pcidev *pci, uchar pin)
{
  uchar intl;

  if (pci == nil)
    pci = pcilist;

  while (pci != nil) {
    if (pcicfgr8(pci, PciINTP) == pin && pci->intl != 0 && pci->intl != 0xFF)
      return pci->intl;
    if (pci->bridge != nil) {
      intl = pciipin(pci->bridge, pin);
      if (intl != 0)
        return intl;
    }
    pci = pci->list;
  }
  return 0;
}

static void
pcilhinv(Pcidev *p)
{
  Pcidev *t;
  int i;

  for (t = p; t != nil; t = t->link) {
    print("%d  %2d/%d %.2ux %.2ux %.2ux %.4ux %.4ux %3d  ", BUSBNO(t->tbdf),
          BUSDNO(t->tbdf), BUSFNO(t->tbdf), t->ccrb, t->ccru, t->ccrp, t->vid,
          t->did, t->intl);
    for (i = 0; i < nelem(t->mem); i++) {
      if (t->mem[i].size == 0)
        continue;
      print("%d:%.8llux %lld ", i, t->mem[i].bar, t->mem[i].size);
    }
    if (t->rom.bar || t->rom.size)
      print("rom:%.8llux %lld ", t->rom.bar, t->rom.size);
    if (t->ioa.bar || t->ioa.size)
      print("ioa:%.8llux-%.8llux %lld ", t->ioa.bar, t->ioa.bar + t->ioa.size,
            t->ioa.size);
    if (t->mema.bar || t->mema.size)
      print("mema:%.8llux-%.8llux %lld ", t->mema.bar,
            t->mema.bar + t->mema.size, t->mema.size);
    if (t->prefa.bar || t->prefa.size)
      print("prefa:%.8llux-%.8llux %lld ", t->prefa.bar,
            t->prefa.bar + t->prefa.size, t->prefa.size);
    if (t->bridge)
      print("->%d", BUSBNO(t->bridge->tbdf));
    print("\n");
  }
  while (p != nil) {
    if (p->bridge != nil)
      pcilhinv(p->bridge);
    p = p->link;
  }
}

void
pcihinv(Pcidev *p)
{
  print("bus dev type     vid  did  intl memory\n");
  pcilhinv(p);
}

void
pcireset(void)
{
  Pcidev *p;

  if (pciensureenumerated() < 0)
    return;
  for (p = pcilist; p != nil; p = p->list) {
    if (p->ccrb == 0x06)
      continue;
    pcidisable(p);
  }
}

void
pcisetioe(Pcidev *p)
{
  if (p == nil)
    return;
  p->pcr |= IOen;
  pcicfgw16(p, PciPCR, p->pcr);
}

void
pciclrioe(Pcidev *p)
{
  if (p == nil)
    return;
  p->pcr &= ~IOen;
  pcicfgw16(p, PciPCR, p->pcr);
}

void
pcisetbme(Pcidev *p)
{
  if (p == nil)
    return;
  p->pcr |= MASen;
  pcicfgw16(p, PciPCR, p->pcr);
}

void
pciclrbme(Pcidev *p)
{
  if (p == nil)
    return;
  p->pcr &= ~MASen;
  pcicfgw16(p, PciPCR, p->pcr);
}

void
pcisetmwi(Pcidev *p)
{
  if (p == nil)
    return;
  p->pcr |= MemWrInv;
  pcicfgw16(p, PciPCR, p->pcr);
}

void
pciclrmwi(Pcidev *p)
{
  if (p == nil)
    return;
  p->pcr &= ~MemWrInv;
  pcicfgw16(p, PciPCR, p->pcr);
}

int
pcienumcaps(Pcidev *p, int (*fmatch)(Pcidev *, int, int, int), int arg)
{
  int i, r, cap, off;

  if (p == nil || fmatch == nil)
    return -1;
  if ((pcicfgr16(p, PciPSR) & (1 << 4)) == 0)
    return -1;

  switch (pcicfgr8(p, PciHDT) & 0x7F) {
  default:
    return -1;
  case 0:
  case 1:
    off = 0x34;
    break;
  case 2:
    off = 0x14;
    break;
  }

  for (i = 48; i--;) {
    off = pcicfgr8(p, off);
    if (off < 0x40 || (off & 3))
      break;
    off &= ~3;
    cap = pcicfgr8(p, off);
    if (cap == 0xFF)
      break;
    r = fmatch(p, cap, off, arg);
    if (r < 0)
      break;
    if (r == 0)
      return off;
    off++;
  }
  return -1;
}

static int
matchcap(Pcidev *, int cap, int, int arg)
{
  return cap != arg;
}

static int
matchhtcap(Pcidev *p, int cap, int off, int arg)
{
  int mask;

  if (cap != PciCapHTC)
    return 1;
  mask = (arg == 0x00 || arg == 0x20) ? 0xE0 : 0xF8;
  cap = pcicfgr8(p, off + 3);
  return (cap & mask) != arg;
}

int
pcicap(Pcidev *p, int cap)
{
  return pcienumcaps(p, matchcap, cap);
}

int
pcihtcap(Pcidev *p, int cap)
{
  return pcienumcaps(p, matchhtcap, cap);
}

static int
pcimsixdisable(Pcidev *p)
{
  int off;

  off = pcicap(p, PciCapMSIX);
  if (off < 0)
    return -1;
  pcicfgw16(p, off + MSIXCtrl, 0);
  return 0;
}

static int
pcigetmsi(Pcidev *p)
{
  if (p->msi != 0)
    return p->msi;
  p->msi = pcicap(p, PciCapMSI);
  return p->msi;
}

int
pcimsienable(Pcidev *p, uvlong addr, ulong data)
{
  int off, ok64;

  if (p == nil)
    return -1;

  off = pcigetmsi(p);
  if (off < 0)
    return -1;
  pcimsixdisable(p);
  ok64 = (pcicfgr16(p, off + MSICtrl) & (1 << 7)) != 0;
  pcicfgw32(p, off + MSIAddr, addr);
  if (ok64)
    pcicfgw32(p, off + MSIAddr + 4, addr >> 32);
  pcicfgw16(p, off + (ok64 ? MSIData64 : MSIData32), data);
  pcicfgw16(p, off + MSICtrl, 1);
  return 0;
}

int
pcimsidisable(Pcidev *p)
{
  int off;

  if (p == nil)
    return -1;

  pcimsixdisable(p);
  off = pcigetmsi(p);
  if (off < 0)
    return -1;
  pcicfgw16(p, off + MSICtrl, 0);
  return 0;
}

static int
pcigetpmrb(Pcidev *p)
{
  if (p->pmrb != 0)
    return p->pmrb;
  p->pmrb = pcicap(p, PciCapPMG);
  return p->pmrb;
}

int
pcigetpms(Pcidev *p)
{
  int pmcsr, ptr;

  if (p == nil)
    return -1;

  ptr = pcigetpmrb(p);
  if (ptr < 0)
    return -1;
  pmcsr = pcicfgr16(p, ptr + 4);
  return pmcsr & 0x0003;
}

int
pcisetpms(Pcidev *p, int state)
{
  int ostate, pmc, pmcsr, ptr;

  if (p == nil)
    return -1;

  ptr = pcigetpmrb(p);
  if (ptr < 0)
    return -1;

  pmc = pcicfgr16(p, ptr + 2);
  pmcsr = pcicfgr16(p, ptr + 4);
  ostate = pmcsr & 0x0003;
  pmcsr &= ~0x0003;

  switch (state) {
  default:
    return -1;
  case 0:
    break;
  case 1:
    if (!(pmc & 0x0200))
      return -1;
    break;
  case 2:
    if (!(pmc & 0x0400))
      return -1;
    break;
  case 3:
    break;
  }

  pmcsr |= state;
  pcicfgw16(p, ptr + 4, pmcsr);
  return ostate;
}

void
pcienable(Pcidev *p)
{
  uint pcr;
  int i;

  if (p == nil)
    return;

  pcienable(p->parent);
  switch (pcisetpms(p, 0)) {
  case 2:
  case 3:
    delay(100);
    break;
  }

  if (p->ltr == 0 || p->ltr == 0xFF) {
    p->ltr = 64;
    pcicfgw8(p, PciLTR, p->ltr);
  }
  if (p->cls == 0 || p->cls == 0xFF) {
    p->cls = 64 / 4;
    pcicfgw8(p, PciCLS, p->cls);
  }

  if (p->bridge != nil)
    pcr = IOen | MEMen | MASen;
  else {
    pcr = 0;
    for (i = 0; i < nelem(p->mem); i++) {
      if (p->mem[i].size == 0)
        continue;
      if (p->mem[i].bar & 1)
        pcr |= IOen;
      else
        pcr |= MEMen;
    }
  }

  if ((p->pcr & pcr) != pcr) {
    p->pcr |= pcr;
    pcicfgw32(p, PciPCR, 0xFFFF0000 | p->pcr);
  }
}

void
pcidisable(Pcidev *p)
{
  if (p == nil)
    return;
  pcimsidisable(p);
  pciclrbme(p);
}

int
pci_match_device(Pcidev *pcidev, ushort vendor_id, ushort device_id,
                 uchar base_class, uchar sub_class)
{
  if (pcidev == nil)
    return 0;
  if (vendor_id != 0 && pcidev->vid != vendor_id)
    return 0;
  if (device_id != 0 && pcidev->did != device_id)
    return 0;
  if (base_class != 0xFF && pcidev->ccrb != base_class)
    return 0;
  if (sub_class != 0xFF && pcidev->ccru != sub_class)
    return 0;
  return 1;
}

PCIDriver *
pci_find_driver_for_device(Pcidev *pcidev)
{
  PCIDriver *driver;

  if (pcidev == nil)
    return nil;

  lock(&pci_framework.lock);
  for (driver = pci_framework.drivers; driver != nil; driver = driver->next) {
    if (pci_match_device(pcidev, driver->vendor_id, driver->device_id,
                         driver->base_class, driver->sub_class)) {
      unlock(&pci_framework.lock);
      return driver;
    }
  }
  unlock(&pci_framework.lock);
  return nil;
}

PCIClass *
pci_get_class_info(uchar base_class, uchar sub_class)
{
  int i;

  for (i = 0; pci_classes[i].base_class != 0xFF ||
              pci_classes[i].sub_class != 0xFF; i++) {
    if (pci_classes[i].base_class == base_class &&
        pci_classes[i].sub_class == sub_class)
      return &pci_classes[i];
  }
  return &pci_classes[nelem(pci_classes) - 1];
}

char *
pci_class_name(uchar base_class, uchar sub_class)
{
  return pci_get_class_info(base_class, sub_class)->class_name;
}

static int
pci_framework_has_device(Device *dev)
{
  int i;

  for (i = 0; i < pci_framework.pci_device_count; i++) {
    if (pci_framework.pci_devices[i] == dev)
      return 1;
  }
  return 0;
}

Device *
pci_framework_register_device(Pcidev *pcidev)
{
  Device *dev, **new_array;
  PCIDriver *driver;
  PCIClass *class_info;
  int new_capacity;

  if (pcidev == nil)
    return nil;

  dev = devregistry_find_by_pci(BUSBNO(pcidev->tbdf), BUSDNO(pcidev->tbdf),
                                BUSFNO(pcidev->tbdf));
  if (dev == nil)
    dev = devregistry_register_pci(pcidev);
  if (dev == nil)
    return nil;

  driver = pci_find_driver_for_device(pcidev);
  class_info = pci_get_class_info(pcidev->ccrb, pcidev->ccru);
  if (driver != nil) {
    snprint(dev->description, sizeof(dev->description), "%s %s (%04x:%04x)",
            class_info->class_name, class_info->sub_class_name, pcidev->vid,
            pcidev->did);
    if (driver->probe == nil || driver->probe(pcidev) == 0) {
      devregistry_register_driver(dev, driver->name, 0);
      if (driver->attach != nil)
        driver->attach(dev);
    }
  } else {
    snprint(dev->description, sizeof(dev->description),
            "%s %s (%04x:%04x) - No driver", class_info->class_name,
            class_info->sub_class_name, pcidev->vid, pcidev->did);
  }

  lock(&pci_framework.lock);
  if (!pci_framework_has_device(dev)) {
    if (pci_framework.pci_device_count >= pci_framework.pci_device_capacity) {
      new_capacity = pci_framework.pci_device_capacity ? pci_framework.pci_device_capacity * 2 : 64;
      new_array = xalloc_resident(sizeof(Device *) * new_capacity);
      if (new_array != nil) {
        if (pci_framework.pci_devices != nil) {
          memmove(new_array, pci_framework.pci_devices,
                  sizeof(Device *) * pci_framework.pci_device_count);
          xfree_resident(pci_framework.pci_devices);
        }
        pci_framework.pci_devices = new_array;
        pci_framework.pci_device_capacity = new_capacity;
      }
    }
    if (pci_framework.pci_device_count < pci_framework.pci_device_capacity)
      pci_framework.pci_devices[pci_framework.pci_device_count++] = dev;
  }
  unlock(&pci_framework.lock);

  return dev;
}

void
pci_framework_init(void)
{
  memset(&pci_framework, 0, sizeof(pci_framework));
  pci_framework.pci_device_capacity = 64;
  pci_framework.pci_devices =
      xalloc_resident(sizeof(Device *) * pci_framework.pci_device_capacity);
  if (pci_framework.pci_devices == nil)
    pci_framework.pci_device_capacity = 0;
  pciensurecfginit();
}

int
pci_framework_enumerate(void)
{
  Pcidev *pcidev;
  int count;

  if (pciensureenumerated() < 0)
    return -1;

  count = 0;
  pcidev = nil;
  while ((pcidev = pcimatch(pcidev, 0, 0)) != nil) {
    if (pci_framework_register_device(pcidev) != nil)
      count++;
  }
  return count;
}

int
pci_framework_register_driver(PCIDriver *driver)
{
  PCIDriver *p;

  if (driver == nil)
    return -1;

  lock(&pci_framework.lock);
  for (p = pci_framework.drivers; p != nil; p = p->next) {
    if (p == driver) {
      unlock(&pci_framework.lock);
      return 0;
    }
  }
  driver->next = pci_framework.drivers;
  pci_framework.drivers = driver;
  pci_framework.driver_count++;
  unlock(&pci_framework.lock);
  return 0;
}

int
pci_framework_unregister_driver(PCIDriver *driver)
{
  PCIDriver *prev;

  if (driver == nil)
    return -1;

  lock(&pci_framework.lock);
  if (pci_framework.drivers == driver)
    pci_framework.drivers = driver->next;
  else {
    for (prev = pci_framework.drivers; prev != nil; prev = prev->next) {
      if (prev->next == driver) {
        prev->next = driver->next;
        break;
      }
    }
    if (prev == nil) {
      unlock(&pci_framework.lock);
      return -1;
    }
  }
  pci_framework.driver_count--;
  unlock(&pci_framework.lock);
  return 0;
}

void
pci_framework_list_drivers(void (*print_func)(char *, ...))
{
  PCIDriver *driver;

  if (print_func == nil)
    print_func = print;

  print_func("PCI Framework Drivers:\n");
  print_func("Total drivers: %d\n", pci_framework.driver_count);
  print_func("----------------------------------------\n");

  lock(&pci_framework.lock);
  for (driver = pci_framework.drivers; driver != nil; driver = driver->next) {
    print_func("%s: VID:%04x DID:%04x Class:%02x:%02x\n", driver->name,
               driver->vendor_id, driver->device_id, driver->base_class,
               driver->sub_class);
  }
  unlock(&pci_framework.lock);
  print_func("----------------------------------------\n");
}

void
pci_framework_list_devices(void (*print_func)(char *, ...))
{
  Device *dev;
  int i, j;

  if (print_func == nil)
    print_func = print;

  print_func("PCI Framework Devices:\n");
  print_func("Total PCI devices: %d\n", pci_framework.pci_device_count);
  print_func("----------------------------------------\n");

  lock(&pci_framework.lock);
  for (i = 0; i < pci_framework.pci_device_count; i++) {
    dev = pci_framework.pci_devices[i];
    if (dev == nil || dev->location.pci.pcidev == nil)
      continue;
    print_func("%s: %s (%04x:%04x) - %s\n", dev->name, dev->description,
               dev->vendor_id, dev->device_id,
               dev->driver_name[0] ? dev->driver_name : "No driver");
    for (j = 0; j < 6; j++) {
      if (dev->location.pci.pcidev->mem[j].size > 0)
        print_func("  BAR%d: %p (size %lld)\n", j,
                   dev->location.pci.pcidev->mem[j].bar,
                   dev->location.pci.pcidev->mem[j].size);
    }
  }
  unlock(&pci_framework.lock);
  print_func("----------------------------------------\n");
}

static int
ahci_probe(Pcidev *pcidev)
{
  return (pcidev != nil && pcidev->ccrb == 0x01 && pcidev->ccru == 0x06) ? 0
                                                                          : -1;
}

static int
ide_probe(Pcidev *pcidev)
{
  return (pcidev != nil && pcidev->ccrb == 0x01 && pcidev->ccru == 0x01) ? 0
                                                                          : -1;
}

static int
usb_probe(Pcidev *pcidev)
{
  return (pcidev != nil && pcidev->ccrb == 0x0C && pcidev->ccru == 0x03) ? 0
                                                                          : -1;
}

static int
ethernet_probe(Pcidev *pcidev)
{
  return (pcidev != nil && pcidev->ccrb == 0x02 && pcidev->ccru == 0x00) ? 0
                                                                          : -1;
}
