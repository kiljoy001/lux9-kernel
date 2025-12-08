#include	"u.h"
#include "portlib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include <error.h>

#include "clr/clr-kernel/clr_kernel_architecture.h" // For clr_kernel_system_t etc.
#include "clr/clr-kernel/clr_kernel.h" // For clr_kernel_init and other CLR kernel APIs

// Global CLR system instance
clr_kernel_system_t *g_clr_system = NULL;
Lock clr_system_lock;

// Qid Enums for /dev/clr filesystem hierarchy
enum {
	Qdir,			// /dev/clr
	Qcontrol,		// /dev/clr/control
	Qstats,			// /dev/clr/stats
	Qstatus,		// /dev/clr/status
	Qconfig,		// /dev/clr/config

	QassembliesDir,		// /dev/clr/assemblies
	QassembliesNew,		// /dev/clr/assemblies/new
	Qassembly,		// /dev/clr/assemblies/<assembly_id>
	QassemblyLoad,		// /dev/clr/assemblies/<assembly_id>/load
	QassemblyUnload,		// /dev/clr/assemblies/<assembly_id>/unload
	QassemblyMetadata,	// /dev/clr/assemblies/<assembly_id>/metadata
	QassemblyTypesDir,	// /dev/clr/assemblies/<assembly_id>/types
	QassemblyMethodsDir,	// /dev/clr/assemblies/<assembly_id>/methods
	QassemblyMethod,	// /dev/clr/assemblies/<assembly_id>/methods/<method_id>
	QassemblyMethodExecute,	// /dev/clr/assemblies/<assembly_id>/methods/<method_id>/execute
	QassemblyMethodSignature,	// /dev/clr/assemblies/<assembly_id>/methods/<method_id>/signature
	QassemblyMethodBytecode,	// /dev/clr/assemblies/<assembly_id>/methods/<method_id>/bytecode

	QtaskletsDir,		// /dev/clr/tasklets
	QtaskletsNew,		// /dev/clr/tasklets/new
	Qtasklet,		// /dev/clr/tasklets/<tasklet_id>
	QtaskletControl,		// /dev/clr/tasklets/<tasklet_id>/control
	QtaskletStatus,		// /dev/clr/tasklets/<tasklet_id>/status
	QtaskletDebug,		// /dev/clr/tasklets/<tasklet_id>/debug
	QtaskletStats,		// /dev/clr/tasklets/<tasklet_id>/stats
	QtaskletInput,		// /dev/clr/tasklets/<tasklet_id>/input
	QtaskletOutput,		// /dev/clr/tasklets/<tasklet_id>/output

	QchannelsDir,		// /dev/clr/channels
	QchannelsNew,		// /dev/clr/channels/new
	Qchannel,		// /dev/clr/channels/<channel_id>
	QchannelSend,		// /dev/clr/channels/<channel_id>/send
	QchannelRecv,		// /dev/clr/channels/<channel_id>/recv
	QchannelStatus,		// /dev/clr/channels/<channel_id>/status

	QmemoryDir,		// /dev/clr/memory
	QmemoryHeapUsage,	// /dev/clr/memory/heap_usage
	QmemoryGcStatus,		// /dev/clr/memory/gc_status
	QmemoryPebbleSnapshot,	// /dev/clr/memory/pebble_snapshot
};

// Main Dirtab for /dev/clr
// This defines the static entries in the root of /dev/clr
static Dirtab clrdir[] = {
	".",			{Qdir, 0, QTDIR},	0,			DMDIR|0555,
	"control",		{Qcontrol},		0,			0220,
	"stats",		{Qstats},			0,			0444,
	"status",		{Qstatus},			0,			0444,
	"config",		{Qconfig},			0,			0664,
	"assemblies",	{QassembliesDir, 0, QTDIR},	0,		DMDIR|0555,
	"tasklets",		{QtaskletsDir, 0, QTDIR},	0,		DMDIR|0555,
	"channels",		{QchannelsDir, 0, QTDIR},	0,		DMDIR|0555,
	"memory",		{QmemoryDir, 0, QTDIR},	0,		DMDIR|0555,
};

// Generic device driver initialization
static void
clrinit(void)
{
	ilock(&clr_system_lock);
	if (g_clr_system == NULL) {
		// Initialize the global CLR system
		// Heap size and k-parameter are placeholders for now
		g_clr_system = clr_kernel_init(CLR_DEFAULT_HEAP_SIZE, CLR_DEFAULT_DAG_K);
		if (g_clr_system == NULL) {
			iunlock(&clr_system_lock);
			panic("devclr: failed to initialize CLR kernel system");
		}
		print("devclr: CLR kernel system initialized.\n");
	}
	iunlock(&clr_system_lock);
}

static Chan*
clrattach(char *spec)
{
	return devattach(CLRDEV, spec); // CLRDEV needs to be defined
}

// devgen for /dev/clr
// This function needs to handle both static (clrdir) and dynamic entries (assembly_id, tasklet_id, channel_id)
static int
clrgen(Chan *c, char *name, Dirtab *tab, int ntab, int i, Dir *dp)
{
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

	switch((ulong)c->qid.path){
	case Qdir: // /dev/clr/
		if (strcmp(name, "assemblies") == 0) return devgen(c, name, tab, ntab, i, dp);
		if (strcmp(name, "tasklets") == 0) return devgen(c, name, tab, ntab, i, dp);
		if (strcmp(name, "channels") == 0) return devgen(c, name, tab, ntab, i, dp);
		if (strcmp(name, "memory") == 0) return devgen(c, name, tab, ntab, i, dp);
		if (strcmp(name, "new") == 0) { // For /dev/clr/assemblies/new, /dev/clr/tasklets/new, etc.
			// This is not a static entry, but rather a pseudo-file for creation
			// Need to distinguish parent path or handle this more generically
			break;
		}
		// Fall through to other static or dynamic logic for sub-directories
	
	case QassembliesDir: // /dev/clr/assemblies/
		if (strcmp(name, "new") == 0) {
			qid.path = QassembliesNew; qid.type = QTFILE;
			devdir(c, qid, name, 0, up->user, 0664, dp);
			return 1;
		}
		// Dynamic assembly_id entries will go here
		// For now, no dynamic entries implemented
		break;
	
	case QtaskletsDir: // /dev/clr/tasklets/
		if (strcmp(name, "new") == 0) {
			qid.path = QtaskletsNew; qid.type = QTFILE;
			devdir(c, qid, name, 0, up->user, 0664, dp);
			return 1;
		}
		// Dynamic tasklet_id entries
		break;

	case QchannelsDir: // /dev/clr/channels/
		if (strcmp(name, "new") == 0) {
			qid.path = QchannelsNew; qid.type = QTFILE;
			devdir(c, qid, name, 0, up->user, 0664, dp);
			return 1;
		}
		// Dynamic channel_id entries
		break;

	// Default: No match, return 0
	}
	return 0;
}


static Walkqid*
clrwalk(Chan *c, Chan *nc, char **name, int nname)
{
	return devwalk(c, nc, name, nname, clrdir, nelem(clrdir), clrgen);
}

static int
clrstat(Chan *c, uchar *dp, int n)
{
	return devstat(c, dp, n, clrdir, nelem(clrdir), clrgen);
}

static Chan*
clropen(Chan *c, int omode)
{
	// Ensure CLR system is initialized before opening anything
	qlock(&clr_system_lock);
	if (g_clr_system == NULL) {
		iunlock(&clr_system_lock);
		error("devclr: CLR system not initialized");
	}
	iunlock(&clr_system_lock);

	// Implement specific open logic based on c->qid.path
	switch((ulong)c->qid.path){
	// Case Qcontrol: check write permissions
	case Qcontrol:
	case Qreboot: // Reusing Qreboot for reboot from consdevtab
		if(omode & OREAD)
			error(Eperm);
		break;
	// Read-only files
	case Qstats:
	case Qstatus:
	case QmemoryHeapUsage:
	case QmemoryGcStatus:
	case QmemoryPebbleSnapshot:
		if(omode & OWRITE)
			error(Eperm);
		break;
	}
	
	c = devopen(c, omode, clrdir, nelem(clrdir), clrgen);
	return c;
}

static void
clrclose(Chan *c)
{
	// Clean up any state specific to the channel
	// (e.g., if a file holds a reference to a CLR object)
	USED(c);
}

static long
clrread(Chan *c, void *buf, long n, vlong off)
{
	// Implement read logic based on c->qid.path
	switch((ulong)c->qid.path){
	case Qdir:
		return devdirread(c, buf, n, clrdir, nelem(clrdir), clrgen);
	case Qstats:
		// Read global CLR statistics
		snprint(buf, n, "Tasklets Created: %lu\nChannels Created: %lu\n",
			g_clr_system->stats.tasklets_created, g_clr_system->stats.channels_created);
		return strlen(buf);
	case Qstatus:
		snprint(buf, n, "CLR Kernel Status: %s\n", "Running");
		return strlen(buf);
	case QmemoryHeapUsage:
		snprint(buf, n, "Heap Size: %luMB\nHeap Used: %luKB\n",
			g_clr_system->memory.heap_size / (1024*1024),
			g_clr_system->memory.heap_used / 1024);
		return strlen(buf);
	case Qosversion: // Reusing Qosversion from consdevtab
		snprint(buf, n, "CLR Kernel Version: 1.0\n");
		return strlen(buf);
	default:
		error(Egreg);
	}
	return 0;
}

static long
clrwrite(Chan *c, void *va, long n, vlong off)
{
	char *a = va;

	// Implement write logic based on c->qid.path
	switch((ulong)c->qid.path){
	case Qcontrol:
		// Handle global commands like init, shutdown, set_debug_level
		if (strncmp(a, "init", n) == 0) {
			ilock(&clr_system_lock);
			if (g_clr_system == NULL) {
				g_clr_system = clr_kernel_init(CLR_DEFAULT_HEAP_SIZE, CLR_DEFAULT_DAG_K);
				if (g_clr_system == NULL) {
					iunlock(&clr_system_lock);
					error("devclr: failed to initialize CLR kernel system");
				}
				print("devclr: CLR kernel system re-initialized.\n");
			} else {
				print("devclr: CLR kernel system already initialized.\n");
			}
			iunlock(&clr_system_lock);
			return n;
		}
		if (strncmp(a, "shutdown", n) == 0) {
			// TODO: Implement clr_kernel_shutdown() and cleanup
			ilock(&clr_system_lock);
			if (g_clr_system != NULL) {
				// clr_kernel_shutdown(g_clr_system);
				g_clr_system = NULL;
				print("devclr: CLR kernel system shut down.\n");
			}
			iunlock(&clr_system_lock);
			return n;
		}
		error(Ebadctl); // Bad control command
	
	case QassembliesNew:
		// Write CIL bytecode to load a new assembly
		// Will require parsing the bytecode from 'a'
		break;
	
	case QtaskletsNew:
		// Write tasklet parameters to create a new tasklet
		// Will require parsing params from 'a'
		break;

	case QchannelsNew:
		// Write channel parameters to create a new channel
		break;
	
default:
		error(Egreg);
	}
	return n;
}

Dev clrdevtab = {
	CLRDEV,			// Device number, needs to be defined
	"clr",

	devreset,		// Assuming no specific reset
	clrinit,
	devshutdown,	// Assuming no specific shutdown
	clrattach,
	clrwalk,
	clrstat,
	clropen,
	devcreate,		// For dynamic file creation (new assemblies, tasklets, channels)
	clrclose,
	clrread,
	devbread,
	clrwrite,
	devbwrite,
	devremove,		// For removing assemblies, tasklets, channels
	devwstat,
};
