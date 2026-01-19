/*
 * PCI Resource Pool Manager - Resource Pool Implementation
 *
 * Manages PCI device resources including BARs, IRQs, DMA regions
 * Provides resource allocation, tracking, and cleanup for PCI channels
 * Integrates with exchange pages and pebble system for security
 */

#include "dat.h"
#include "exchange.h"
#include "family/family.h"
#include "family/pci_family_ops.h"
#include "fns.h"
#include "mem.h"
#include "pageown.h"
#include "pebble.h"
#include "portlib.h"
#include "u.h"
#include <error.h>

/* Pebble granularity for resource calculations */
#ifndef PEBBLE_GRANULARITY
#define PEBBLE_GRANULARITY 4096 /* 4KB pages */
#endif

/* Stub implementations for missing helper functions */
static Proc *current_process(void) { return up; }
static uint64_t now(void) { return fastticks(nil); }
static int exchange_prepare_pci_bar(uint64_t channel_id, uint8_t bar_num,
                                    PebbleHandle *token,
                                    ExchangeHandle *handle) {
  memset(handle, 0, sizeof(*handle));
  return 0;
}
/*@
  @ requires handle == \null || \valid(handle);
  @ assigns *handle;
  @*/
static void exchange_cleanup(ExchangeHandle *handle) {
  /* Clean up exchange page handle - mark as unused */
  if (handle != nil)
    memset(handle, 0, sizeof(*handle));
}
static PebbleHandle *pebble_create_white(int ctx, char *desc, size_t size) {
  return nil;
}
static struct PCIChannel *lookup_pci_channel(struct FamilyExchangePage *family,
                                             uint64_t channel_id) {
  return nil;
}
static void *upamalloc(size_t size, size_t align, int zero) {
  return xalloc_driver(size);
}
static void upafree(uintptr addr, size_t size) { xfree_driver((void *)addr); }
static int exchange_prepare_pci_dma(uint64_t channel_id, uintptr paddr,
                                    size_t size, PebbleHandle *token,
                                    ExchangeHandle *handle) {
  memset(handle, 0, sizeof(*handle));
  return 0;
}

/* Resource pool statistics */
struct ResourcePoolStats {
  uint32_t used_bars, max_bars;
  uint32_t used_irqs, max_irqs;
  uint32_t used_dmas, max_dmas;
  uint64_t total_bar_allocations;
  uint64_t total_irq_allocations;
  uint64_t total_dma_allocations;
  uint64_t total_bar_bytes;
  uint64_t total_dma_bytes;
  uint32_t peak_concurrent_bars;
  uint32_t peak_concurrent_irqs;
  uint32_t peak_concurrent_dmas;
  uint64_t allocation_failures;
};

/* Resource pool types */
enum PCIResourceType {
  PCI_RESOURCE_BAR = 1,
  PCI_RESOURCE_IRQ = 2,
  PCI_RESOURCE_DMA = 3,
  PCI_RESOURCE_MMIO = 4,
  PCI_RESOURCE_CONFIG = 5
};

/* BAR resource descriptor */
struct PCIBarResource {
  uint64_t bar_id;           // 64-bit resource ID
  uint8_t bar_number;        // PCI BAR number (0-5)
  uint64_t physical_address; // Physical address of BAR
  uint64_t size;             // Size of BAR region
  uint32_t flags;            // BAR properties (memory, I/O, etc.)
  bool is_64bit;             // Whether BAR is 64-bit
  bool is_prefetchable;      // Prefetchable memory
  bool is_active;            // Resource currently in use

  // Channel binding
  uint64_t bound_channel_id;        // Channel that owns this BAR
  struct PCIChannel *bound_channel; // Direct channel reference

  // Exchange page handling
  struct ExchangeHandle *exchange_handle; // Exchange page handle
  PebbleHandle *bar_white_token;          // Authorization token for BAR access

  // Security and ownership
  struct Proc *owning_process; // Process that allocated this BAR
  uint64_t allocated_at;       // Allocation timestamp
  uint32_t access_count;       // Number of accesses
};

/* IRQ resource descriptor */
struct PCIIrqResource {
  uint64_t irq_id;      // 64-bit resource ID
  uint32_t irq_vector;  // IRQ vector number
  uint32_t irq_line;    // Physical IRQ line
  uint8_t trigger_type; // Edge/level trigger
  uint8_t polarity;     // High/low polarity
  bool is_msi;          // MSI/MSI-X IRQ
  bool is_active;       // Resource currently in use

  // Channel binding
  uint64_t bound_channel_id;        // Channel that owns this IRQ
  struct PCIChannel *bound_channel; // Direct channel reference

  // Security and ownership
  struct Proc *owning_process; // Process that allocated this IRQ
  uint64_t allocated_at;       // Allocation timestamp
  uint32_t interrupt_count;    // Number of interrupts delivered
};

/* DMA resource descriptor */
struct PCIDmaResource {
  uint64_t dma_id;           /* 64-bit resource ID */
  uint64_t physical_address; /* Physical DMA address */
  uint64_t size;             /* Size of DMA region */
  uint32_t alignment;        /* Alignment requirements */
  int is_coherent;           /* Cache-coherent DMA */
  int is_active;             /* Resource currently in use */

  /* Channel binding */
  uint64_t bound_channel_id;        /* Channel that owns this DMA */
  struct PCIChannel *bound_channel; /* Direct channel reference */

  /* Exchange page for zero-copy DMA */
  struct ExchangeHandle *exchange_handle; /* Exchange page handle */
  PebbleHandle *dma_white_token; /* Authorization token for DMA access */

  /* DMA tracking */
  struct Proc *owning_process; /* Process that allocated this DMA */
  uint64_t allocated_at;       /* Allocation timestamp */
  uint64_t bytes_transferred;  /* Total bytes transferred */
  uint32_t transfer_count;     /* Number of DMA transfers */
};

/* PCI Resource Pool Manager */
struct PCIResourcePool {
  // Resource arrays
  struct PCIBarResource *bar_resources;
  struct PCIIrqResource *irq_resources;
  struct PCIDmaResource *dma_resources;

  // Resource counts
  uint32_t max_bars;
  uint32_t max_irqs;
  uint32_t max_dmas;
  uint32_t used_bars;
  uint32_t used_irqs;
  uint32_t used_dmas;

  // Resource allocation tracking
  uint64_t next_bar_id;
  uint64_t next_irq_id;
  uint64_t next_dma_id;

  // Lock for resource management
  Lock resource_lock;

  // Statistics
  struct {
    uint64_t total_bar_allocations;
    uint64_t total_irq_allocations;
    uint64_t total_dma_allocations;
    uint64_t total_bar_bytes;
    uint64_t total_dma_bytes;
    uint64_t peak_concurrent_bars;
    uint64_t peak_concurrent_irqs;
    uint64_t peak_concurrent_dmas;
    uint64_t allocation_failures;
  } stats;

  // Security and pebble integration
  PebbleHandle *pool_white_token; // Pool-wide authorization token
  uint32_t total_budget;          // Resource budget for pebbles
  uint32_t used_budget;           // Currently used budget
};

/* Initialize PCI resource pool */
/*@
  @ requires family == \null || \valid(family);
  @ assigns family->resource_pool;
  @*/
void setup_pci_resource_pool(struct FamilyExchangePage *family) {
  if (!family) {
    return;
  }

  struct PCIFamilyContext *ctx =
      (struct PCIFamilyContext *)family->family_specific_ctx;
  if (!ctx) {
    return;
  }

  /* Allocate resource pool structure */
  struct PCIResourcePool *pool = xalloc_driver(sizeof(struct PCIResourcePool));
  if (!pool) {
    print("PCI: failed to allocate resource pool\n");
    return;
  }

  memset(pool, 0, sizeof(struct PCIResourcePool));

  /* Allocate resource arrays */
  pool->max_bars = MAX_PCI_DEVICES * 6; // Up to 6 BARs per device
  pool->max_irqs = MAX_PCI_DEVICES * 8; // Up to 8 IRQs per device
  pool->max_dmas = MAX_PCI_DEVICES * 4; // Up to 4 DMA regions per device

  pool->bar_resources =
      xalloc_driver(sizeof(struct PCIBarResource) * pool->max_bars);
  pool->irq_resources =
      xalloc_driver(sizeof(struct PCIIrqResource) * pool->max_irqs);
  pool->dma_resources =
      xalloc_driver(sizeof(struct PCIDmaResource) * pool->max_dmas);

  if (!pool->bar_resources || !pool->irq_resources || !pool->dma_resources) {
    print("PCI: failed to allocate resource arrays\n");
    if (pool->bar_resources)
      xfree_driver(pool->bar_resources);
    if (pool->irq_resources)
      xfree_driver(pool->irq_resources);
    if (pool->dma_resources)
      xfree_driver(pool->dma_resources);
    xfree_driver(pool);
    return;
  }

  /* Initialize resource arrays */
  memset(pool->bar_resources, 0,
         sizeof(struct PCIBarResource) * pool->max_bars);
  memset(pool->irq_resources, 0,
         sizeof(struct PCIIrqResource) * pool->max_irqs);
  memset(pool->dma_resources, 0,
         sizeof(struct PCIDmaResource) * pool->max_dmas);

  /* Initialize counters */
  pool->next_bar_id = 1;
  pool->next_irq_id = 1;
  pool->next_dma_id = 1;
  memset(&pool->resource_lock, 0, sizeof(Lock));

  /* Set up pebble integration */
  pool->total_budget =
      PEBBLE_DEFAULT_BUDGET / 4; // 25% of total budget for PCI resources
  pool->pool_white_token = pebble_create_white(
      PCI_FAMILY_TYPE, "pci_resource_pool", pool->total_budget);

  /* Attach to family context */
  ctx->resource_pool = pool;

  print(
      "PCI: resource pool initialized - max_bars=%d max_irqs=%d max_dmas=%d\n",
      pool->max_bars, pool->max_irqs, pool->max_dmas);
}

/* Allocate BAR resource from pool */
int allocate_pci_bar_resource(struct FamilyExchangePage *family,
                              struct PCIDeviceDescriptor *device,
                              uint8_t bar_number, uint64_t channel_id,
                              struct PCIBarResource **bar_out) {
  struct PCIFamilyContext *ctx =
      (struct PCIFamilyContext *)family->family_specific_ctx;
  struct PCIResourcePool *pool = ctx->resource_pool;

  if (!family || !device || !pool || bar_out) {
    return -1;
  }

  /* Validate BAR number */
  if (bar_number >= 6 || !device->bars[bar_number].is_valid) {
    return -2; // Invalid BAR
  }

  lock(&pool->resource_lock);

  /* Find free BAR slot */
  int free_slot = -1;
  /*@ loop invariant 0 <= i <= pool->max_bars;
  @ loop assigns i;
  @ loop variant pool->max_bars - i;
  @*/
  for (int i = 0; i < pool->max_bars; i++) {
    if (!pool->bar_resources[i].is_active) {
      free_slot = i;
      break;
    }
  }

  if (free_slot == -1) {
    unlock(&pool->resource_lock);
    pool->stats.allocation_failures++;
    return -3; // No free BAR slots
  }

  /* Initialize BAR resource */
  struct PCIBarResource *bar = &pool->bar_resources[free_slot];
  memset(bar, 0, sizeof(struct PCIBarResource));

  bar->bar_id = pool->next_bar_id++;
  bar->bar_number = bar_number;
  bar->physical_address = device->bars[bar_number].base_address;
  bar->size = device->bars[bar_number].size;
  bar->flags = device->bars[bar_number].type;
  bar->is_64bit = device->bars[bar_number].is_64bit;
  bar->is_prefetchable = (device->bars[bar_number].type & 0x08) != 0;
  bar->is_active = 1;

  /* Bind to channel */
  bar->bound_channel_id = channel_id;
  bar->bound_channel = lookup_pci_channel(family, channel_id);
  bar->owning_process = current_process();
  bar->allocated_at = now();

  /* Create exchange page handle for BAR access */
  if (bar->bound_channel) {
    int result = exchange_prepare_pci_bar(channel_id, bar_number,
                                          bar->bound_channel->white_token,
                                          &bar->exchange_handle);
    if (result != 0) {
      unlock(&pool->resource_lock);
      bar->is_active = 0; // Mark as inactive on failure
      return -4;          // Exchange page creation failed
    }

    /* Create BAR-specific pebble token */
    bar->bar_white_token = pebble_create_white(
        PCI_FAMILY_TYPE, "pci_bar_access", bar->size / PEBBLE_GRANULARITY);
  }

  /* Update statistics */
  pool->used_bars++;
  pool->stats.total_bar_allocations++;
  pool->stats.total_bar_bytes += bar->size;
  if (pool->used_bars > pool->stats.peak_concurrent_bars) {
    pool->stats.peak_concurrent_bars = pool->used_bars;
  }

  unlock(&pool->resource_lock);

  *bar_out = bar;

  print("PCI: allocated BAR%d resource id=%d addr=0x%llx size=%lld for channel "
        "%d\n",
        bar_number, bar->bar_id, bar->physical_address, bar->size, channel_id);

  return 0;
}

/* Release BAR resource back to pool */
int release_pci_bar_resource(struct FamilyExchangePage *family,
                             struct PCIBarResource *bar) {
  struct PCIFamilyContext *ctx =
      (struct PCIFamilyContext *)family->family_specific_ctx;
  struct PCIResourcePool *pool = ctx->resource_pool;

  if (!family || !bar || !pool) {
    return -1;
  }

  lock(&pool->resource_lock);

  if (!bar->is_active) {
    unlock(&pool->resource_lock);
    return -2; // Resource not active
  }

  /* Clean up exchange page */
  if (bar->exchange_handle) {
    exchange_cleanup(bar->exchange_handle);
    bar->exchange_handle = NULL;
  }

  /* Clean up pebble token */
  if (bar->bar_white_token) {
    pebble_cleanup(bar->bar_white_token);
    bar->bar_white_token = NULL;
  }

  /* Update statistics */
  pool->used_bars--;
  pool->stats.total_bar_bytes -= bar->size;

  /* Mark resource as inactive */
  bar->is_active = 0;
  bar->bound_channel_id = 0;
  bar->bound_channel = NULL;
  bar->owning_process = NULL;

  unlock(&pool->resource_lock);

  print("PCI: released BAR%d resource id=%d\n", bar->bar_number, bar->bar_id);

  return 0;
}

/* Allocate IRQ resource from pool */
int allocate_pci_irq_resource(struct FamilyExchangePage *family,
                              struct PCIDeviceDescriptor *device,
                              uint32_t irq_vector, uint8_t trigger_type,
                              uint8_t polarity, uint64_t channel_id,
                              struct PCIIrqResource **irq_out) {
  struct PCIFamilyContext *ctx =
      (struct PCIFamilyContext *)family->family_specific_ctx;
  struct PCIResourcePool *pool = ctx->resource_pool;

  if (!family || !device || !pool || irq_out) {
    return -1;
  }

  lock(&pool->resource_lock);

  /* Find free IRQ slot */
  int free_slot = -1;
  /*@ loop invariant 0 <= i <= pool->max_irqs;
  @ loop assigns i;
  @ loop variant pool->max_irqs - i;
  @*/
  for (int i = 0; i < pool->max_irqs; i++) {
    if (!pool->irq_resources[i].is_active) {
      free_slot = i;
      break;
    }
  }

  if (free_slot == -1) {
    unlock(&pool->resource_lock);
    pool->stats.allocation_failures++;
    return -2; // No free IRQ slots
  }

  /* Initialize IRQ resource */
  struct PCIIrqResource *irq = &pool->irq_resources[free_slot];
  memset(irq, 0, sizeof(struct PCIIrqResource));

  irq->irq_id = pool->next_irq_id++;
  irq->irq_vector = irq_vector;
  irq->irq_line = irq_vector; /* Use IRQ vector as line */
  irq->trigger_type = trigger_type;
  irq->polarity = polarity;
  irq->is_msi = device->capabilities.has_msi || device->capabilities.has_msix;
  irq->is_active = 1;

  /* Bind to channel */
  irq->bound_channel_id = channel_id;
  irq->bound_channel = lookup_pci_channel(family, channel_id);
  irq->owning_process = current_process();
  irq->allocated_at = now();

  /* Update statistics */
  pool->used_irqs++;
  pool->stats.total_irq_allocations++;
  if (pool->used_irqs > pool->stats.peak_concurrent_irqs) {
    pool->stats.peak_concurrent_irqs = pool->used_irqs;
  }

  unlock(&pool->resource_lock);

  *irq_out = irq;

  print("PCI: allocated IRQ resource id=%d vector=%d line=%d for channel %d\n",
        irq->irq_id, irq->irq_vector, irq->irq_line, channel_id);

  return 0;
}

/* Release IRQ resource back to pool */
int release_pci_irq_resource(struct FamilyExchangePage *family,
                             struct PCIIrqResource *irq) {
  struct PCIFamilyContext *ctx =
      (struct PCIFamilyContext *)family->family_specific_ctx;
  struct PCIResourcePool *pool = ctx->resource_pool;

  if (!family || !irq || !pool) {
    return -1;
  }

  lock(&pool->resource_lock);

  if (!irq->is_active) {
    unlock(&pool->resource_lock);
    return -2; // Resource not active
  }

  /* Update statistics */
  pool->used_irqs--;

  /* Mark resource as inactive */
  irq->is_active = 0;
  irq->bound_channel_id = 0;
  irq->bound_channel = NULL;
  irq->owning_process = NULL;

  unlock(&pool->resource_lock);

  print("PCI: released IRQ resource id=%d\n", irq->irq_id);

  return 0;
}

/* Allocate DMA resource from pool */
int allocate_pci_dma_resource(struct FamilyExchangePage *family,
                              struct PCIDeviceDescriptor *device, uint64_t size,
                              uint32_t alignment, bool coherent,
                              uint64_t channel_id,
                              struct PCIDmaResource **dma_out) {
  struct PCIFamilyContext *ctx =
      (struct PCIFamilyContext *)family->family_specific_ctx;
  struct PCIResourcePool *pool = ctx->resource_pool;

  if (!family || !device || !pool || dma_out || size == 0) {
    return -1;
  }

  lock(&pool->resource_lock);

  /* Find free DMA slot */
  int free_slot = -1;
  /*@ loop invariant 0 <= i <= pool->max_dmas;
  @ loop assigns i;
  @ loop variant pool->max_dmas - i;
  @*/
  for (int i = 0; i < pool->max_dmas; i++) {
    if (!pool->dma_resources[i].is_active) {
      free_slot = i;
      break;
    }
  }

  if (free_slot == -1) {
    unlock(&pool->resource_lock);
    pool->stats.allocation_failures++;
    return -2; // No free DMA slots
  }

  /* Initialize DMA resource */
  struct PCIDmaResource *dma = &pool->dma_resources[free_slot];
  memset(dma, 0, sizeof(struct PCIDmaResource));

  dma->dma_id = pool->next_dma_id++;
  dma->size = size;
  dma->alignment = alignment;
  dma->is_coherent = coherent;
  dma->is_active = 1;

  /* Allocate physical memory for DMA */
  uintptr dma_phys = (uintptr)upamalloc(size, alignment, 0);
  if (dma_phys == 0) {
    unlock(&pool->resource_lock);
    pool->stats.allocation_failures++;
    return -3; // Memory allocation failed
  }

  dma->physical_address = dma_phys;

  /* Bind to channel */
  dma->bound_channel_id = channel_id;
  dma->bound_channel = lookup_pci_channel(family, channel_id);
  dma->owning_process = current_process();
  dma->allocated_at = now();

  /* Create exchange page for zero-copy DMA */
  if (dma->bound_channel) {
    int result = exchange_prepare_pci_dma(channel_id, dma->physical_address,
                                          size, dma->bound_channel->white_token,
                                          &dma->exchange_handle);
    if (result != 0) {
      upafree(dma->physical_address, size); // Clean up memory
      unlock(&pool->resource_lock);
      dma->is_active = 0;
      return -4; // Exchange page creation failed
    }

    /* Create DMA-specific pebble token */
    dma->dma_white_token = pebble_create_white(
        PCI_FAMILY_TYPE, "pci_dma_access", size / PEBBLE_GRANULARITY);
  }

  /* Update statistics */
  pool->used_dmas++;
  pool->stats.total_dma_allocations++;
  pool->stats.total_dma_bytes += size;
  if (pool->used_dmas > pool->stats.peak_concurrent_dmas) {
    pool->stats.peak_concurrent_dmas = pool->used_dmas;
  }

  unlock(&pool->resource_lock);

  *dma_out = dma;

  print("PCI: allocated DMA resource id=%d addr=0x%llx size=%lld for channel "
        "%d\n",
        dma->dma_id, dma->physical_address, dma->size, channel_id);

  return 0;
}

/* Release DMA resource back to pool */
int release_pci_dma_resource(struct FamilyExchangePage *family,
                             struct PCIDmaResource *dma) {
  struct PCIFamilyContext *ctx =
      (struct PCIFamilyContext *)family->family_specific_ctx;
  struct PCIResourcePool *pool = ctx->resource_pool;

  if (!family || !dma || !pool) {
    return -1;
  }

  lock(&pool->resource_lock);

  if (!dma->is_active) {
    unlock(&pool->resource_lock);
    return -2; // Resource not active
  }

  /* Clean up exchange page */
  if (dma->exchange_handle) {
    exchange_cleanup(dma->exchange_handle);
    dma->exchange_handle = NULL;
  }

  /* Clean up pebble token */
  if (dma->dma_white_token) {
    pebble_cleanup(dma->dma_white_token);
    dma->dma_white_token = NULL;
  }

  /* Free physical memory */
  upafree(dma->physical_address, dma->size);

  /* Update statistics */
  pool->used_dmas--;
  pool->stats.total_dma_bytes -= dma->size;

  /* Mark resource as inactive */
  dma->is_active = 0;
  dma->bound_channel_id = 0;
  dma->bound_channel = NULL;
  dma->owning_process = NULL;

  unlock(&pool->resource_lock);

  print("PCI: released DMA resource id=%d\n", dma->dma_id);

  return 0;
}

/* Cleanup all resources for a channel */
int cleanup_channel_resources(struct FamilyExchangePage *family,
                              uint64_t channel_id) {
  struct PCIFamilyContext *ctx =
      (struct PCIFamilyContext *)family->family_specific_ctx;
  struct PCIResourcePool *pool = ctx->resource_pool;

  if (!family || !pool) {
    return -1;
  }

  lock(&pool->resource_lock);

  /* Clean up all BAR resources for this channel */
  /*@ loop invariant 0 <= i <= pool->max_bars;
  @ loop assigns i;
  @ loop variant pool->max_bars - i;
  @*/
  for (int i = 0; i < pool->max_bars; i++) {
    struct PCIBarResource *bar = &pool->bar_resources[i];
    if (bar->is_active && bar->bound_channel_id == channel_id) {
      release_pci_bar_resource(family, bar);
    }
  }

  /* Clean up all IRQ resources for this channel */
  /*@ loop invariant 0 <= i <= pool->max_irqs;
  @ loop assigns i;
  @ loop variant pool->max_irqs - i;
  @*/
  for (int i = 0; i < pool->max_irqs; i++) {
    struct PCIIrqResource *irq = &pool->irq_resources[i];
    if (irq->is_active && irq->bound_channel_id == channel_id) {
      release_pci_irq_resource(family, irq);
    }
  }

  /* Clean up all DMA resources for this channel */
  /*@ loop invariant 0 <= i <= pool->max_dmas;
  @ loop assigns i;
  @ loop variant pool->max_dmas - i;
  @*/
  for (int i = 0; i < pool->max_dmas; i++) {
    struct PCIDmaResource *dma = &pool->dma_resources[i];
    if (dma->is_active && dma->bound_channel_id == channel_id) {
      release_pci_dma_resource(family, dma);
    }
  }

  unlock(&pool->resource_lock);

  print("PCI: cleaned up all resources for channel %d\n", channel_id);
  return 0;
}

/* Get resource pool statistics */
void get_pci_resource_pool_stats(struct FamilyExchangePage *family,
                                 struct ResourcePoolStats *stats) {
  struct PCIFamilyContext *ctx =
      (struct PCIFamilyContext *)family->family_specific_ctx;
  struct PCIResourcePool *pool = ctx->resource_pool;

  if (!family || !pool || !stats) {
    return;
  }

  lock(&pool->resource_lock);

  /* Copy statistics */
  stats->used_bars = pool->used_bars;
  stats->max_bars = pool->max_bars;
  stats->used_irqs = pool->used_irqs;
  stats->max_irqs = pool->max_irqs;
  stats->used_dmas = pool->used_dmas;
  stats->max_dmas = pool->max_dmas;

  stats->total_bar_allocations = pool->stats.total_bar_allocations;
  stats->total_irq_allocations = pool->stats.total_irq_allocations;
  stats->total_dma_allocations = pool->stats.total_dma_allocations;
  stats->total_bar_bytes = pool->stats.total_bar_bytes;
  stats->total_dma_bytes = pool->stats.total_dma_bytes;
  stats->peak_concurrent_bars = pool->stats.peak_concurrent_bars;
  stats->peak_concurrent_irqs = pool->stats.peak_concurrent_irqs;
  stats->peak_concurrent_dmas = pool->stats.peak_concurrent_dmas;
  stats->allocation_failures = pool->stats.allocation_failures;

  unlock(&pool->resource_lock);
}

/* Shutdown resource pool */
/*@
  @ requires family == \null || \valid(family);
  @ assigns *family;
  @ ensures \result <= 0;
  @*/
int shutdown_pci_resource_pool(struct FamilyExchangePage *family) {
  struct PCIFamilyContext *ctx =
      (struct PCIFamilyContext *)family->family_specific_ctx;
  struct PCIResourcePool *pool = ctx->resource_pool;

  if (!family || !pool) {
    return -1;
  }

  print("PCI: shutting down resource pool\n");

  lock(&pool->resource_lock);

  /* Clean up all active resources */
  /*@ loop invariant 0 <= i <= pool->max_bars;
  @ loop assigns i;
  @ loop variant pool->max_bars - i;
  @*/
  for (int i = 0; i < pool->max_bars; i++) {
    if (pool->bar_resources[i].is_active) {
      release_pci_bar_resource(family, &pool->bar_resources[i]);
    }
  }

  /*@ loop invariant 0 <= i <= pool->max_irqs;
  @ loop assigns i;
  @ loop variant pool->max_irqs - i;
  @*/
  for (int i = 0; i < pool->max_irqs; i++) {
    if (pool->irq_resources[i].is_active) {
      release_pci_irq_resource(family, &pool->irq_resources[i]);
    }
  }

  /*@ loop invariant 0 <= i <= pool->max_dmas;
  @ loop assigns i;
  @ loop variant pool->max_dmas - i;
  @*/
  for (int i = 0; i < pool->max_dmas; i++) {
    if (pool->dma_resources[i].is_active) {
      release_pci_dma_resource(family, &pool->dma_resources[i]);
    }
  }

  /* Clean up pool pebble token */
  if (pool->pool_white_token) {
    pebble_cleanup(pool->pool_white_token);
    pool->pool_white_token = NULL;
  }

  /* Free resource arrays */
  xfree_driver(pool->bar_resources);
  xfree_driver(pool->irq_resources);
  xfree_driver(pool->dma_resources);

  unlock(&pool->resource_lock);

  /* Free pool structure */
  xfree_driver(pool);
  ctx->resource_pool = NULL;

  print("PCI: resource pool shutdown complete\n");
  return 0;
}