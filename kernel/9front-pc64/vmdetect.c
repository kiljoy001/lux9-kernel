/* VM Detection Implementation */
/* #include "../include/u.h" removed */
#define _U_H_
#define nil ((void *)0)

typedef unsigned char uchar;
typedef unsigned short ushort;
typedef unsigned int uint;
typedef unsigned long ulong;
typedef unsigned long long uvlong;
typedef long long vlong;
typedef unsigned long usize;
typedef unsigned long uintptr;
typedef unsigned char u8int;
typedef unsigned short u16int;
typedef unsigned int u32int;
typedef unsigned long long u64int;
typedef signed char s8int;
typedef signed short s16int;
typedef signed int s32int;
typedef signed long long s64int;
typedef __builtin_va_list va_list;

typedef struct Qid Qid;
typedef struct Dir Dir;
typedef struct Waitmsg Waitmsg;
typedef struct Fmt Fmt;
struct Qid {
  uvlong path;
  ulong vers;
  uchar type;
};
struct Dir {
  ushort type;
  uint dev;
  Qid qid;
  ulong mode;
  ulong atime;
  ulong mtime;
  vlong length;
  char *name;
  char *uid;
  char *gid;
  char *muid;
};
#define ERRMAX 128
struct Waitmsg {
  int pid;
  ulong time[3];
  char msg[ERRMAX];
};

#include "vmdetect.h"
#include <string.h>
extern void *memmove(void *, const void *, size_t);
extern void *memset(void *, int, size_t);
extern int snprint(char *, int, char *, ...);
extern size_t strlen(const char *);
extern char *strncpy(char *, const char *, size_t);
extern int strcmp(const char *, const char *);
extern int memcmp(const void *, const void *, size_t);
extern int print(char *, ...);

#include "mem.h"
#include "dat.h"
#include "fns.h"
/* #include "portlib.h" - Removed */

VMInfo vm_info;

/* Check if running under a hypervisor using CPUID */
static int check_hypervisor_cpuid(void) {
  u32int regs[4];

  /* CPUID leaf 1, ECX bit 31 = hypervisor present */
  cpuid(1, 0, regs);
  return (regs[2] & (1U << 31)) != 0;
}

/* Get hypervisor vendor string from CPUID */
static void get_hypervisor_vendor(char *vendor) {
  u32int regs[4];

  /* CPUID leaf 0x40000000 returns hypervisor vendor */
  cpuid(0x40000000, 0, regs);

  /* Vendor string is in EBX, ECX, EDX */
  memmove(vendor + 0, &regs[1], 4);
  memmove(vendor + 4, &regs[2], 4);
  memmove(vendor + 8, &regs[3], 4);
  vendor[12] = '\0';
}

/* Timing-based detection (backup method) */
static int check_vm_timing(void) {
  uvlong start, end, total = 0;
  int i, slow_count = 0;
  u32int dummy[4];

  /* Test CPUID overhead - VMs trap this instruction */
  for (i = 0; i < 20; i++) {
    start = rdtsc();
    cpuid(0, 0, dummy);
    end = rdtsc();

    total = end - start;

    /* On bare metal: ~50-200 cycles
     * On VM: >1000 cycles due to VM exit overhead */
    if (total > 800)
      slow_count++;
  }

  /* If most tests are slow, probably in VM */
  return slow_count > 10;
}

/* Main VM detection function */
void vm_detect(void) {
  char vendor[16];

  memset(&vm_info, 0, sizeof(vm_info));
  vm_info.type = VM_NONE;

  print("\n=== VM Detection ===\n");

  /* Method 1: CPUID hypervisor bit */
  if (!check_hypervisor_cpuid()) {
    print("VM: CPUID hypervisor bit NOT set\n");

    /* Backup: Timing-based detection */
    if (check_vm_timing()) {
      print("VM: Timing suggests virtualization (stealth VM?)\n");
      vm_info.type = VM_UNKNOWN;
    } else {
      print("VM: Running on BARE METAL\n");
      vm_info.type = VM_NONE;
      vm_info.detected = 1;
      return;
    }
  } else {
    print("VM: CPUID hypervisor bit SET - running in VM\n");
  }

  /* Method 2: Get hypervisor vendor string */
  get_hypervisor_vendor(vendor);
  strncpy(vm_info.vendor, vendor, sizeof(vm_info.vendor) - 1);
  print("VM: Hypervisor vendor = '%s'\n", vm_info.vendor);

  /* Identify specific hypervisor */
  if (strcmp(vendor, "KVMKVMKVM\0\0\0") == 0) {
    vm_info.type = VM_KVM;
    print("VM: Detected KVM\n");
    vm_info.paravirt = 1;

    /* Check KVM features (Leaf 0x40000001) */
    u32int kvm_regs[4];
    cpuid(0x40000001, 0, kvm_regs);
    if (kvm_regs[0] & (1 << 0)) { /* KVM_FEATURE_CLOCKSOURCE */
      vm_info.have_kvm_clock = 1;
      print("VM: KVM Clock detected\n");
    }
    if (kvm_regs[0] & (1 << 1)) { /* KVM_FEATURE_NOP_IO_DELAY */
      vm_info.have_kvm_nop_io_delay = 1;
      print("VM: KVM Nop IO Delay recommended\n");
    }
    if (kvm_regs[0] & (1 << 2)) { /* KVM_FEATURE_MMU_OP */
      vm_info.have_kvm_mmu_op = 1;
      print("VM: KVM MMU operations available\n");
    }

  } else if (strcmp(vendor, "TCGTCGTCGTCG") == 0) {
    vm_info.type = VM_QEMU_TCG;
    print("VM: Detected QEMU/TCG (software emulation)\n");

  } else if (memcmp(vendor, "Microsoft Hv", 12) == 0) {
    vm_info.type = VM_HYPERV;
    print("VM: Detected Hyper-V\n");

  } else if (memcmp(vendor, "VMwareVMware", 12) == 0) {
    vm_info.type = VM_VMWARE;
    print("VM: Detected VMware\n");

  } else if (memcmp(vendor, "XenVMMXenVMM", 12) == 0) {
    vm_info.type = VM_XEN;
    print("VM: Detected Xen\n");
    vm_info.paravirt = 1;

  } else if (memcmp(vendor, "VBoxVBoxVBox", 12) == 0) {
    vm_info.type = VM_VIRTUALBOX;
    print("VM: Detected VirtualBox\n");

  } else if (memcmp(vendor, " lrpepyh vr", 12) ==
             0) { /* "prl hyperv " reversed */
    vm_info.type = VM_PARALLELS;
    print("VM: Detected Parallels\n");

  } else if (memcmp(vendor, "bhyve bhyve ", 12) == 0) {
    vm_info.type = VM_BHYVE;
    print("VM: Detected bhyve\n");

  } else {
    vm_info.type = VM_UNKNOWN;
    print("VM: Unknown hypervisor\n");
  }

  vm_info.detected = 1;
  print("===================\n\n");
}

/* Check if running in any VM */
int vm_is_virtual(void) {
  if (!vm_info.detected)
    vm_detect();

  return vm_info.type != VM_NONE;
}

/* Apply VM-specific workarounds */
void vm_apply_workarounds(void) {
  if (!vm_info.detected)
    vm_detect();

  if (vm_info.type == VM_NONE) {
    print("VM Workarounds: Running on bare metal, no workarounds needed\n");
    return;
  }

  print("\n=== VM Workarounds ===\n");
  print("VM: Applying workarounds for VM type %d\n", vm_info.type);

  /* All VMs: Skip problematic MSR writes */
  if (vm_info.type == VM_KVM || vm_info.type == VM_QEMU_TCG ||
      vm_info.type == VM_UNKNOWN) {

    /*
     * KVM generally handles MSRs fine. Allow PAT unless explicitly disabled.
     */
    if (vm_info.type == VM_KVM) {
      print("VM: KVM detected - Enabling MSR writes (including PAT)\n");
      vm_info.skip_msr_writes = 0;
    } else {
      print("VM: Will skip PAT/EFER MSR writes (safeguard for TCG/Unknown)\n");
      vm_info.skip_msr_writes = 1;
    }
  }

  /* All VMs: Skip ACPI timer calibration (causes hangs) */
  print("VM: Will skip ACPI timer calibration\n");
  vm_info.skip_acpi_timer = 1;

  /* Use paravirt clock if available */
  if (vm_info.paravirt) {
    print("VM: Paravirtualization available\n");
  }

  print("======================\n\n");
}
