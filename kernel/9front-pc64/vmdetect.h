/* VM Detection for 9front kernel */
#pragma once

/* VM type enumeration */
enum {
  VM_NONE = 0,
  VM_KVM,
  VM_QEMU_TCG,
  VM_XEN,
  VM_VMWARE,
  VM_HYPERV,
  VM_BHYVE,
  VM_VIRTUALBOX,
  VM_PARALLELS,
  VM_UNKNOWN = 255,
};

/* VM information structure */
typedef struct VMInfo VMInfo;
struct VMInfo {
  int type;                  /* VM type from enum above */
  char vendor[16];           /* Hypervisor vendor string */
  int detected;              /* Detection completed flag */
  int paravirt;              /* Supports paravirtualization */
  int skip_acpi_timer;       /* Skip ACPI timer calibration */
  int skip_msr_writes;       /* Skip problematic MSR writes */
  int have_kvm_clock;        /* KVM paravirtual clock detected */
  int have_kvm_nop_io_delay; /* KVM recommends 0-cycle IO delay */
  int have_kvm_mmu_op;       /* KVM MMU operations available */
};

/* Global VM info */
extern VMInfo vm_info;

/* Detection function */
void vm_detect(void);
int vm_is_virtual(void);
void vm_apply_workarounds(void);
