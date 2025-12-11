#include "u.h"
#include "portlib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
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
  }
}

static long clrread(Chan *c, void *buf, long n, vlong off) {
  // Implement read logic based on c->qid.path
  switch ((ulong)c->qid.path) {
  case Qdir:
    return devdirread(c, buf, n, clrdir, nelem(clrdir), clrgen);
  case Qstats:
    /* Stub: Return placeholder stats */
    snprint(buf, n, "Assemblies Compiled: 0\n");
    return strlen(buf);
  case Qstatus:
    snprint(buf, n, "CLR Kernel Status: Stub\n");
    return strlen(buf);
  case QmemoryHeapUsage:
    snprint(buf, n, "Heap Size: 0MB\nHeap Used: 0KB\n");
    return strlen(buf);
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
  case Qcontrol:
    /* Stub: Accept but ignore control commands */
    return n;

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
        fruity_module_from_cbor((u8int *)a, n, ctx->error, sizeof(ctx->error));
    if (ctx->module == nil) {
      /* Error message already in ctx->error */
      error("devclr: CBOR deserialization failed");
    }
    return n;

  case Qexecute:
    /* Execute raw DLL data through CLR pipeline */
    {
      int result = clr_execute_assembly(a, n);
      if (result < 0)
        error("devclr: assembly execution failed");
      return n;
    }

  case QassembliesNew:
    /* Stub: Write CIL bytecode to load a new assembly */
    return n;

  case QtaskletsNew:
    /* Stub: Write tasklet parameters to create a new tasklet */
    return n;

  case QchannelsNew:
    /* Stub: Write channel parameters to create a new channel */
    return n;

  default:
    error(Egreg);
  }
  return n;
}

Dev clrdevtab = {
    'K', /* Device character for CLR */
    "clr",

    devreset, clrinit,  devshutdown, clrattach, clrwalk,
    clrstat,  clropen,  devcreate,   clrclose,  clrread,
    devbread, clrwrite, devbwrite,   devremove, devwstat,
    devpower,
    devconfig,
};
