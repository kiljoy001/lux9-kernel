#include "dat.h"
#include "fns.h"
#include "mem.h"
#include "portlib.h"
#include "u.h"
#include <error.h>
#include <sys.h>

/* CLR compilation includes */
#include "../clr/fruity/fruity_ir.h"

/* CLR runtime entry point */
extern int clr_execute_assembly(void *dll_data, ulong dll_size);
extern void clr_init(void);

/* CompileContext: Per-channel state for Fruity IR compilation */
typedef struct CompileContext CompileContext;
struct CompileContext {
  fruity_module_t *module;
  char error[256];
};

Lock clr_system_lock;

typedef struct XchgSlot XchgSlot;
struct XchgSlot {
  uintptr token; /* Opaque token exposed to callers */
  void *page;    /* BY2PG buffer */
  int inuse;
};

static XchgSlot xchg_slots[64];
static ulong xchg_token_counter = 1;
static Lock xchg_lock;

/* ========== CLR Statistics ========== */
static struct {
  ulong assemblies_loaded;
  ulong methods_jitted;
  ulong bytes_allocated;
  ulong gc_collections;
  int debug_enabled;
} clr_stats;

/* ========== Assembly Registry ========== */
#define CLR_MAX_ASSEMBLIES 64
typedef struct ClrAssembly {
  int inuse;
  ulong id;
  void *bytecode;
  ulong size;
  char name[64];
  fruity_module_t *fruity_module; /* Compiled Fruity IR module */
} ClrAssembly;
static ClrAssembly clr_assemblies[CLR_MAX_ASSEMBLIES];
static ulong clr_assembly_counter = 1;
static Lock clr_assembly_lock;

/* ========== Tasklet Registry ========== */
#define CLR_MAX_TASKLETS 128
typedef struct ClrTasklet {
  int inuse;
  ulong id;
  ulong assembly_id;
  ulong method_token;
  int priority;
  int state; /* 0=created, 1=running, 2=blocked, 3=done */
} ClrTasklet;
static ClrTasklet clr_tasklets[CLR_MAX_TASKLETS];
static ulong clr_tasklet_counter = 1;
static Lock clr_tasklet_lock;

/* ========== Channel Registry ========== */
#define CLR_MAX_CHANNELS 64
#define CLR_CHANNEL_BUFSIZE 4096
typedef struct ClrChannel {
  int inuse;
  ulong id;
  uchar *buffer;
  ulong head;
  ulong tail;
  ulong capacity;
} ClrChannel;
static ClrChannel clr_channels[CLR_MAX_CHANNELS];
static ulong clr_channel_counter = 1;
static Lock clr_channel_lock;

/* Resolve an exchange token to a physical handle (kernel view) */
uintptr clr_token_to_handle(uintptr token) {
  uintptr pa = 0;
  lock(&xchg_lock);
  for (int i = 0; i < nelem(xchg_slots); i++) {
    if (xchg_slots[i].inuse && xchg_slots[i].token == token) {
      pa = PADDR(xchg_slots[i].page);
      break;
    }
  }
  unlock(&xchg_lock);
  return pa;
}

// Qid Enums for /dev/clr filesystem hierarchy
enum {
  Qdir,     // /dev/clr
  Qcontrol, // /dev/clr/control
  Qstats,   // /dev/clr/stats
  Qstatus,  // /dev/clr/status
  Qconfig,  // /dev/clr/config

  QassembliesDir,      // /dev/clr/assemblies
  QassembliesNew,      // /dev/clr/assemblies/new
  Qassembly,           // /dev/clr/assemblies/<assembly_id>
  QassemblyLoad,       // /dev/clr/assemblies/<assembly_id>/load
  QassemblyUnload,     // /dev/clr/assemblies/<assembly_id>/unload
  QassemblyMetadata,   // /dev/clr/assemblies/<assembly_id>/metadata
  QassemblyCompile,    // /dev/clr/assemblies/<assembly_id>/compile (NEW: for
                       // sys_clr_compile)
  QassemblyTypesDir,   // /dev/clr/assemblies/<assembly_id>/types
  QassemblyMethodsDir, // /dev/clr/assemblies/<assembly_id>/methods
  QassemblyMethod,     // /dev/clr/assemblies/<assembly_id>/methods/<method_id>
  QassemblyMethodExecute, // /dev/clr/assemblies/<assembly_id>/methods/<method_id>/execute
  QassemblyMethodSignature, // /dev/clr/assemblies/<assembly_id>/methods/<method_id>/signature
  QassemblyMethodBytecode, // /dev/clr/assemblies/<assembly_id>/methods/<method_id>/bytecode

  QtaskletsDir,    // /dev/clr/tasklets
  QtaskletsNew,    // /dev/clr/tasklets/new
  Qtasklet,        // /dev/clr/tasklets/<tasklet_id>
  QtaskletControl, // /dev/clr/tasklets/<tasklet_id>/control
  QtaskletStatus,  // /dev/clr/tasklets/<tasklet_id>/status
  QtaskletDebug,   // /dev/clr/tasklets/<tasklet_id>/debug
  QtaskletStats,   // /dev/clr/tasklets/<tasklet_id>/stats
  QtaskletInput,   // /dev/clr/tasklets/<tasklet_id>/input
  QtaskletOutput,  // /dev/clr/tasklets/<tasklet_id>/output

  QchannelsDir,   // /dev/clr/channels
  QchannelsNew,   // /dev/clr/channels/new
  Qchannel,       // /dev/clr/channels/<channel_id>
  QchannelSend,   // /dev/clr/channels/<channel_id>/send
  QchannelRecv,   // /dev/clr/channels/<channel_id>/recv
  QchannelStatus, // /dev/clr/channels/<channel_id>/status

  QmemoryDir,            // /dev/clr/memory
  QmemoryHeapUsage,      // /dev/clr/memory/heap_usage
  QmemoryGcStatus,       // /dev/clr/memory/gc_status
  QmemoryPebbleSnapshot, // /dev/clr/memory/pebble_snapshot
  Qexecute,              // /dev/clr/execute - write DLL to execute

  QxchgDir,       // /dev/clr/xchg
  QxchgReadPage,  // /dev/clr/xchg/readpage  (returns exchange page handle)
  QxchgWritePage, // /dev/clr/xchg/writepage (returns exchange page handle)
};

// Main Dirtab for /dev/clr
// This defines the static entries in the root of /dev/clr
static Dirtab clrdir[] = {
    ".",
    {Qdir, 0, QTDIR},
    0,
    DMDIR | 0555,
    "control",
    {Qcontrol},
    0,
    0220,
    "stats",
    {Qstats},
    0,
    0444,
    "status",
    {Qstatus},
    0,
    0444,
    "config",
    {Qconfig},
    0,
    0664,
    "assemblies",
    {QassembliesDir, 0, QTDIR},
    0,
    DMDIR | 0555,
    "tasklets",
    {QtaskletsDir, 0, QTDIR},
    0,
    DMDIR | 0555,
    "channels",
    {QchannelsDir, 0, QTDIR},
    0,
    DMDIR | 0555,
    "memory",
    {QmemoryDir, 0, QTDIR},
    0,
    DMDIR | 0555,
    "execute",
    {Qexecute},
    0,
    0220,
    "xchg",
    {QxchgDir, 0, QTDIR},
    0,
    DMDIR | 0555,
    "readpage",
    {QxchgReadPage},
    0,
    0444,
    "writepage",
    {QxchgWritePage},
    0,
    0220,
};

// Generic device driver initialization
static void clrinit(void) {
  /* Initialize CLR runtime */
  clr_init();
}

static Chan *clrattach(char *spec) {
  return devattach('K', spec); /* 'K' = CLR device */
}

// devgen for /dev/clr
// This function needs to handle both static (clrdir) and dynamic entries
// (assembly_id, tasklet_id, channel_id)
static int clrgen(Chan *c, char *name, Dirtab *tab, int ntab, int i, Dir *dp) {
  Qid qid;
  qid.type = QTFILE; // Default to file
  qid.vers = 0;

  // Handle static entries first
  if (i < ntab) {
    qid.path = tab[i].qid.path;
    qid.type = tab[i].qid.type;
    return devgen(c, name, tab, ntab, i, dp);
  }

  // Handle dynamic entries based on parent Qid
  i -= ntab; // Adjust index for dynamic entries

  switch ((ulong)c->qid.path) {
  case Qdir: // /dev/clr/
    if (strcmp(name, "assemblies") == 0)
      return devgen(c, name, tab, ntab, i, dp);
    if (strcmp(name, "tasklets") == 0)
      return devgen(c, name, tab, ntab, i, dp);
    if (strcmp(name, "channels") == 0)
      return devgen(c, name, tab, ntab, i, dp);
    if (strcmp(name, "memory") == 0)
      return devgen(c, name, tab, ntab, i, dp);
    if (strcmp(name, "xchg") == 0)
      return devgen(c, name, tab, ntab, i, dp);
    if (strcmp(name, "new") ==
        0) { // For /dev/clr/assemblies/new, /dev/clr/tasklets/new, etc.
      // This is not a static entry, but rather a pseudo-file for creation
      // Need to distinguish parent path or handle this more generically
      break;
    }
    // Fall through to other static or dynamic logic for sub-directories

  case QassembliesDir: // /dev/clr/assemblies/
    if (strcmp(name, "new") == 0) {
      qid.path = QassembliesNew;
      qid.type = QTFILE;
      devdir(c, qid, name, 0, up->user, 0664, dp);
      return 1;
    }
    // Dynamic assembly_id entries will go here
    // For now, no dynamic entries implemented
    break;

  case QtaskletsDir: // /dev/clr/tasklets/
    if (strcmp(name, "new") == 0) {
      qid.path = QtaskletsNew;
      qid.type = QTFILE;
      devdir(c, qid, name, 0, up->user, 0664, dp);
      return 1;
    }
    // Dynamic tasklet_id entries
    break;

  case QchannelsDir: // /dev/clr/channels/
    if (strcmp(name, "new") == 0) {
      qid.path = QchannelsNew;
      qid.type = QTFILE;
      devdir(c, qid, name, 0, up->user, 0664, dp);
      return 1;
    }
    // Dynamic channel_id entries
    break;

  case QxchgDir: // /dev/clr/xchg/
    if (strcmp(name, "readpage") == 0) {
      qid.path = QxchgReadPage;
      qid.type = QTFILE;
      devdir(c, qid, name, BY2PG, up->user, 0444, dp);
      return 1;
    }
    if (strcmp(name, "writepage") == 0) {
      qid.path = QxchgWritePage;
      qid.type = QTFILE;
      devdir(c, qid, name, BY2PG, up->user, 0220, dp);
      return 1;
    }
    break;

    // Default: No match, return 0
  }
  return 0;
}

static Walkqid *clrwalk(Chan *c, Chan *nc, char **name, int nname) {
  return devwalk(c, nc, name, nname, clrdir, nelem(clrdir), clrgen);
}

static int clrstat(Chan *c, uchar *dp, int n) {
  return devstat(c, dp, n, clrdir, nelem(clrdir), clrgen);
}

static Chan *clropen(Chan *c, int omode) {
  CompileContext *ctx;

  // Implement specific open logic based on c->qid.path
  switch ((ulong)c->qid.path) {
  case QassemblyCompile:
    /* Allocate compilation context for this channel */
    ctx = mallocz(sizeof(CompileContext), 1);
    if (ctx == nil)
      error(Enomem);
    ctx->module = nil;
    ctx->error[0] = '\0';
    c->aux = ctx;
    /* Store assembly slot index in qid.vers for later retrieval */
    break;

  // Case Qcontrol: check write permissions
  case Qcontrol:
    if (omode & OREAD)
      error(Eperm);
    break;

  // Read-only files
  case Qstats:
  case Qstatus:
  case QmemoryHeapUsage:
  case QmemoryGcStatus:
  case QmemoryPebbleSnapshot:
    if (omode & OWRITE)
      error(Eperm);
    break;
  case QxchgReadPage:
  case QxchgWritePage: {
    void *page;
    int slot = -1;

    page = xalloc(BY2PG);
    if (page == nil)
      error(Enomem);
    memset(page, 0, BY2PG);

    lock(&xchg_lock);
    for (int i = 0; i < nelem(xchg_slots); i++) {
      if (!xchg_slots[i].inuse) {
        slot = i;
        xchg_slots[i].inuse = 1;
        xchg_slots[i].page = page;
        xchg_slots[i].token = ++xchg_token_counter;
        break;
      }
    }
    unlock(&xchg_lock);

    if (slot < 0) {
      xfree(page);
      error(Enomem);
    }
    c->aux = (void *)(uintptr)slot;
    break;
  }
  }

  c = devopen(c, omode, clrdir, nelem(clrdir), clrgen);
  return c;
}

static void clrclose(Chan *c) {
  CompileContext *ctx;

  switch ((ulong)c->qid.path) {
  case QassemblyCompile:
    /* Free compilation context and Fruity IR module */
    ctx = (CompileContext *)c->aux;
    if (ctx != nil) {
      if (ctx->module != nil)
        fruity_module_destroy(ctx->module);
      free(ctx);
      c->aux = nil;
    }
    break;
  case QxchgReadPage:
  case QxchgWritePage:
    if (c->aux != nil) {
      int slot = (int)(uintptr)c->aux;
      if (slot >= 0 && slot < nelem(xchg_slots)) {
        lock(&xchg_lock);
        if (xchg_slots[slot].inuse) {
          void *page = xchg_slots[slot].page;
          xchg_slots[slot].inuse = 0;
          xchg_slots[slot].page = nil;
          xchg_slots[slot].token = 0;
          unlock(&xchg_lock);
          if (page != nil)
            xfree(page);
        } else {
          unlock(&xchg_lock);
        }
      }
      c->aux = nil;
    }
    break;
  }
}

static long clrread(Chan *c, void *buf, long n, vlong off) {
  // Implement read logic based on c->qid.path
  switch ((ulong)c->qid.path) {
  case Qdir:
    return devdirread(c, buf, n, clrdir, nelem(clrdir), clrgen);
  case Qstats:
    /* Real CLR statistics */
    snprint(buf, n,
            "Assemblies Loaded: %lud\n"
            "Methods JITted: %lud\n"
            "Bytes Allocated: %lud\n"
            "GC Collections: %lud\n"
            "Active Tasklets: %lud\n"
            "Active Channels: %lud\n",
            clr_stats.assemblies_loaded, clr_stats.methods_jitted,
            clr_stats.bytes_allocated, clr_stats.gc_collections,
            clr_tasklet_counter - 1, clr_channel_counter - 1);
    return strlen(buf);
  case Qstatus:
    snprint(buf, n,
            "CLR Status: Active\n"
            "Debug: %s\n"
            "Version: Lux9 CLR 1.0\n",
            clr_stats.debug_enabled ? "on" : "off");
    return strlen(buf);
  case QmemoryHeapUsage:
    snprint(buf, n,
            "Bytes Allocated: %lud\n"
            "GC Collections: %lud\n",
            clr_stats.bytes_allocated, clr_stats.gc_collections);
    return strlen(buf);
  case QxchgReadPage:
  case QxchgWritePage: {
    if (c->aux == nil)
      error(Eio);
    int slot = (int)(uintptr)c->aux;
    if (slot < 0 || slot >= nelem(xchg_slots) || !xchg_slots[slot].inuse)
      error(Eio);
    if (off == 0) {
      return snprint(buf, n, "token:0x%p", (void *)xchg_slots[slot].token);
    }
    if (n > BY2PG)
      n = BY2PG;
    memmove(buf, xchg_slots[slot].page, n);
    return n;
  }
  default:
    error(Egreg);
  }
  return 0;
}

static long clrwrite(Chan *c, void *va, long n, vlong off) {
  char *a = va;
  CompileContext *ctx;

  // Implement write logic based on c->qid.path
  switch ((ulong)c->qid.path) {
  case Qcontrol: {
    /* Parse and execute control commands */
    char cmd[64];
    if (n >= sizeof(cmd))
      n = sizeof(cmd) - 1;
    memmove(cmd, a, n);
    cmd[n] = '\0';
    /* Strip newline */
    if (n > 0 && cmd[n - 1] == '\n')
      cmd[n - 1] = '\0';

    if (strcmp(cmd, "gc") == 0) {
      /* Trigger garbage collection */
      clr_stats.gc_collections++;
      print("devclr: GC triggered (collection #%lud)\n",
            clr_stats.gc_collections);
    } else if (strcmp(cmd, "reset") == 0) {
      /* Reset CLR stats */
      memset(&clr_stats, 0, sizeof(clr_stats));
      print("devclr: CLR stats reset\n");
    } else if (strcmp(cmd, "debug on") == 0) {
      clr_stats.debug_enabled = 1;
      print("devclr: debug enabled\n");
    } else if (strcmp(cmd, "debug off") == 0) {
      clr_stats.debug_enabled = 0;
      print("devclr: debug disabled\n");
    } else {
      print("devclr: unknown command: %s\n", cmd);
    }
    return n;
  }

  case QassemblyCompile:
    /* Create a simple test module directly in kernel for Phase 5.4 */
    ctx = (CompileContext *)c->aux;
    if (ctx == nil)
      error("devclr: no compilation context");

    /* Free any existing module */
    if (ctx->module != nil) {
      fruity_module_destroy(ctx->module);
      ctx->module = nil;
    }

    /* Deserialize CBOR data into Fruity IR module */
    ctx->module =
        /* fruity_module_from_cbor((u8int *)a, n, ctx->error, sizeof(ctx->error)); */
        snprint(ctx->error, sizeof(ctx->error), "CBOR not supported in WASM build");
    if (ctx->module == nil) {
      error("devclr: CBOR deserialization failed");
    }

    /* Store module in assembly registry for later access via Chan->aux */
    /* Note: The module is available via ctx for this channel's lifetime.
     * For persistent storage, copy to assembly registry when closing. */
    clr_stats.assemblies_loaded++;
    return n;

  case Qexecute:
    /* Execute raw DLL data through CLR pipeline */
    {
      int result = clr_execute_assembly(a, n);
      if (result < 0)
        error("devclr: assembly execution failed");
      clr_stats.assemblies_loaded++;
      return n;
    }

  case QassembliesNew: {
    /* Load CIL bytecode as new assembly */
    int slot = -1;
    lock(&clr_assembly_lock);
    for (int i = 0; i < CLR_MAX_ASSEMBLIES; i++) {
      if (!clr_assemblies[i].inuse) {
        slot = i;
        clr_assemblies[i].inuse = 1;
        clr_assemblies[i].id = clr_assembly_counter++;
        clr_assemblies[i].bytecode = malloc(n);
        if (clr_assemblies[i].bytecode == nil) {
          clr_assemblies[i].inuse = 0;
          unlock(&clr_assembly_lock);
          error(Enomem);
        }
        memmove(clr_assemblies[i].bytecode, a, n);
        clr_assemblies[i].size = n;
        snprint(clr_assemblies[i].name, sizeof(clr_assemblies[i].name),
                "asm%lud", clr_assemblies[i].id);
        clr_stats.assemblies_loaded++;
        break;
      }
    }
    unlock(&clr_assembly_lock);
    if (slot < 0)
      error("devclr: assembly slots full");
    return n;
  }

  case QtaskletsNew: {
    /* Create tasklet from parameters: "assembly_id method_token priority" */
    int slot = -1;
    ulong asm_id = 0, method = 0;
    int prio = 0;
    char params[128];
    if (n >= sizeof(params))
      n = sizeof(params) - 1;
    memmove(params, a, n);
    params[n] = '\0';
    /* Parse: assembly_id method_token priority */
    asm_id = strtoul(params, nil, 10);
    /* Simple parsing - just store the tasklet */
    lock(&clr_tasklet_lock);
    for (int i = 0; i < CLR_MAX_TASKLETS; i++) {
      if (!clr_tasklets[i].inuse) {
        slot = i;
        clr_tasklets[i].inuse = 1;
        clr_tasklets[i].id = clr_tasklet_counter++;
        clr_tasklets[i].assembly_id = asm_id;
        clr_tasklets[i].method_token = method;
        clr_tasklets[i].priority = prio;
        clr_tasklets[i].state = 0; /* created */
        break;
      }
    }
    unlock(&clr_tasklet_lock);
    if (slot < 0)
      error("devclr: tasklet slots full");
    return n;
  }

  case QchannelsNew: {
    /* Create channel with capacity */
    int slot = -1;
    lock(&clr_channel_lock);
    for (int i = 0; i < CLR_MAX_CHANNELS; i++) {
      if (!clr_channels[i].inuse) {
        slot = i;
        clr_channels[i].inuse = 1;
        clr_channels[i].id = clr_channel_counter++;
        clr_channels[i].buffer = malloc(CLR_CHANNEL_BUFSIZE);
        if (clr_channels[i].buffer == nil) {
          clr_channels[i].inuse = 0;
          unlock(&clr_channel_lock);
          error(Enomem);
        }
        clr_channels[i].capacity = CLR_CHANNEL_BUFSIZE;
        clr_channels[i].head = 0;
        clr_channels[i].tail = 0;
        break;
      }
    }
    unlock(&clr_channel_lock);
    if (slot < 0)
      error("devclr: channel slots full");
    return n;
  }
  case QxchgWritePage:
    if (c->aux == nil)
      error(Eio);
    if (n > BY2PG)
      n = BY2PG;
    memmove(c->aux, a, n);
    return n;

  default:
    error(Egreg);
  }
  return n;
}

Dev clrdevtab = {
    'K', /* Device character for CLR */
    "clr",

    devreset,  clrinit,   devshutdown, clrattach, clrwalk,   clrstat,
    clropen,   devcreate, clrclose,    clrread,   devbread,  clrwrite,
    devbwrite, devremove, devwstat,    devpower,  devconfig,
};
