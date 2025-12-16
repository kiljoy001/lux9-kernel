/*
 * HAL Exchange Server - Zero-Copy 9P via Exchange Pages
 *
 * Uses Singularity-style exchange pages for zero-copy 9P message passing:
 * - Client prepares Exchange Page with 9P request
 * - HAL processes request in-place
 * - Response written back to same Exchange Page
 * - No network sockets, no copying, pure shared memory
 *
 * This is a test implementation that uses standard C for portability.
 * Production version will use Lux9 kernel Exchange API.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>

/* Plan 9 style types for compatibility */
typedef unsigned char uchar;
typedef unsigned short ushort;
typedef unsigned int uint;
typedef unsigned long ulong;
typedef unsigned long long uvlong;

#define nil NULL

/* Exchange Page Layout - 4KB */
#define EXCHANGE_PAGE_SIZE 4096
#define MAX_MESSAGE_SIZE 8192  /* 2 pages */

typedef struct ExchangeControl ExchangeControl;
struct ExchangeControl {
	/* Control area - first cache line */
	volatile uint32_t client_seq;     /* Client sequence number */
	volatile uint32_t server_seq;     /* Server sequence number */
	volatile uint32_t request_len;    /* Request message length */
	volatile uint32_t response_len;   /* Response message length */
	volatile uint8_t  status;         /* 0=idle, 1=request_ready, 2=response_ready */
	uint8_t  pad[47];                 /* Pad to 64 bytes */

	/* Request buffer - next 2KB */
	uint8_t request[2048];

	/* Response buffer - remaining 2KB */
	uint8_t response[2048];
};

/* 9P Message Types */
enum {
	Tversion = 100, Rversion,
	Tauth = 102, Rauth,
	Tattach = 104, Rattach,
	Terror = 106, Rerror,
	Tflush = 108, Rflush,
	Twalk = 110, Rwalk,
	Topen = 112, Ropen,
	Tcreate = 114, Rcreate,
	Tread = 116, Rread,
	Twrite = 118, Rwrite,
	Tclunk = 120, Rclunk,
	Tremove = 122, Rremove,
	Tstat = 124, Rstat,
	Twstat = 126, Rwstat,
};

/* QID types */
#define QTDIR    0x80
#define QTFILE   0x00

typedef struct Qid Qid;
struct Qid {
	uint8_t  type;
	uint32_t vers;
	uint64_t path;
};

/* HAL File System */
typedef struct HalNode HalNode;
struct HalNode {
	char name[256];
	Qid qid;
	uint32_t perm;
	uint64_t length;
	int isdir;

	enum {
		HAL_ROOT,
		HAL_PCI_DIR,
		HAL_PCI_DEVICE,
		HAL_PCI_CONFIG,
		HAL_PCI_BAR,
		HAL_PCI_CTL,
	} type;

	struct {
		uint bus;
		uint device;
		uint function;
		int config_fd;
	} pci;

	HalNode *parent;
	HalNode *children;
	HalNode *next;
};

typedef struct Fid Fid;
struct Fid {
	uint32_t fid;
	HalNode *node;
	int omode;
	uint64_t offset;
	Fid *next;
};

typedef struct HalServer HalServer;
struct HalServer {
	HalNode *root;
	Fid *fids;
	uint64_t next_qid_path;
};

static HalServer server;

/* Shared memory allocation (simulates Exchange Page) */
static ExchangeControl *shared_exch = NULL;

/* ===== Helper Functions ===== */

static uint32_t
gbit32(uchar *p)
{
	return p[0] | (p[1]<<8) | (p[2]<<16) | (p[3]<<24);
}

static void
pbit32(uchar *p, uint32_t v)
{
	p[0] = v; p[1] = v >> 8; p[2] = v >> 16; p[3] = v >> 24;
}

static uint16_t
gbit16(uchar *p)
{
	return p[0] | (p[1]<<8);
}

static void
pbit16(uchar *p, uint16_t v)
{
	p[0] = v; p[1] = v >> 8;
}

static uint8_t
gbit8(uchar *p)
{
	return p[0];
}

static void
pbit8(uchar *p, uint8_t v)
{
	p[0] = v;
}

/* ===== 9P Message Parsing ===== */

static int
parse_9p_header(uchar *msg, uint *type, uint *tag, uint *fid)
{
	*type = gbit8(msg + 4);
	*tag = gbit16(msg + 5);

	/* FID location depends on message type */
	switch(*type) {
	case Tattach:
		*fid = gbit32(msg + 7);
		break;
	case Twalk:
		*fid = gbit32(msg + 7);
		break;
	case Topen:
	case Tread:
	case Twrite:
	case Tclunk:
		*fid = gbit32(msg + 7);
		break;
	default:
		*fid = 0;
	}

	return 0;
}

/* ===== 9P Handlers ===== */

static int
handle_tversion(uchar *req, uint req_len, uchar *resp, uint *resp_len)
{
	uint32_t msize;
	uint size;

	(void)req_len;
	msize = gbit32(req + 7);

	/* Build Rversion */
	size = 4 + 1 + 2 + 4 + 2 + 6;  /* size + type + tag + msize + slen + "9P2000" */
	pbit32(resp, size);
	pbit8(resp + 4, Rversion);
	pbit16(resp + 5, gbit16(req + 5));  /* echo tag */
	pbit32(resp + 7, (msize < 8192) ? msize : 8192);
	pbit16(resp + 11, 6);
	memcpy(resp + 13, "9P2000", 6);

	*resp_len = size;
	return 0;
}

static int
handle_tattach(uchar *req, uint req_len, uchar *resp, uint *resp_len)
{
	uint32_t fid;
	Fid *f;
	uint size;

	(void)req_len;
	fid = gbit32(req + 7);

	printf("HAL: Tattach fid=%u\n", fid);

	/* Create FID pointing to root */
	f = calloc(1, sizeof(Fid));
	f->fid = fid;
	f->node = server.root;
	f->omode = -1;
	f->offset = 0;
	f->next = server.fids;
	server.fids = f;

	/* Build Rattach: size + type + tag + qid(13 bytes) */
	size = 4 + 1 + 2 + 13;
	pbit32(resp, size);
	pbit8(resp + 4, Rattach);
	pbit16(resp + 5, gbit16(req + 5));  /* echo tag */
	pbit8(resp + 7, server.root->qid.type);
	pbit32(resp + 8, server.root->qid.vers);
	pbit32(resp + 12, (uint32_t)server.root->qid.path);
	pbit32(resp + 16, (uint32_t)(server.root->qid.path >> 32));

	*resp_len = size;
	return 0;
}

static int
handle_9p_message(uchar *req, uint req_len, uchar *resp, uint *resp_len)
{
	uint type, tag, fid;

	if(req_len < 7) {
		/* Invalid message - build Rerror */
		pbit32(resp, 4 + 1 + 2 + 2 + 11);
		pbit8(resp + 4, Rerror);
		pbit16(resp + 5, 0);  /* NOTAG */
		pbit16(resp + 7, 11);
		memcpy(resp + 9, "invalid msg", 11);
		*resp_len = 4 + 1 + 2 + 2 + 11;
		return -1;
	}

	parse_9p_header(req, &type, &tag, &fid);

	printf("HAL: Processing 9P message: type=%u tag=%u fid=%u\n", type, tag, fid);

	switch(type) {
	case Tversion:
		return handle_tversion(req, req_len, resp, resp_len);

	case Tattach:
		return handle_tattach(req, req_len, resp, resp_len);

	case Tclunk:
		/* Simple Rclunk response */
		pbit32(resp, 4 + 1 + 2);
		pbit8(resp + 4, Rclunk);
		pbit16(resp + 5, tag);
		*resp_len = 7;
		return 0;

	default:
		/* Unsupported - return Rerror */
		printf("HAL: Unsupported message type %u\n", type);
		pbit32(resp, 4 + 1 + 2 + 2 + 19);
		pbit8(resp + 4, Rerror);
		pbit16(resp + 5, tag);
		pbit16(resp + 7, 19);
		memcpy(resp + 9, "not yet implemented", 19);
		*resp_len = 4 + 1 + 2 + 2 + 19;
		return 0;
	}
}

/* ===== Exchange Page Communication ===== */

static int
hal_serve_exchange_page(ExchangeControl *exch)
{
	uint32_t last_client_seq = 0;
	uint resp_len;

	printf("HAL: Serving Exchange Page at %p\n", (void*)exch);

	for(;;) {
		/* Wait for new request */
		while(exch->client_seq == last_client_seq) {
			usleep(100);  /* 100 microseconds */
		}

		/* New request available */
		last_client_seq = exch->client_seq;

		if(exch->status != 1) {
			printf("HAL: Warning - unexpected status %u\n", exch->status);
			continue;
		}

		printf("HAL: Processing request #%u (len=%u)\n",
		       exch->client_seq, exch->request_len);

		/* Process 9P message */
		resp_len = 0;
		handle_9p_message(exch->request, exch->request_len,
		                  exch->response, &resp_len);

		/* Write response */
		exch->response_len = resp_len;
		__sync_synchronize();  /* Memory barrier */
		exch->status = 2;      /* Response ready */
		exch->server_seq++;

		printf("HAL: Response ready (len=%u, seq=%u)\n",
		       resp_len, exch->server_seq);
	}

	return 0;
}

/* ===== Filesystem Initialization ===== */

static HalNode*
node_create(const char *name, int isdir)
{
	HalNode *n;

	n = calloc(1, sizeof(HalNode));
	if(!n) return nil;

	strncpy(n->name, name, sizeof(n->name)-1);
	n->qid.type = isdir ? QTDIR : QTFILE;
	n->qid.vers = 0;
	n->qid.path = ++server.next_qid_path;
	n->perm = isdir ? 0755 : 0644;
	n->isdir = isdir;

	return n;
}

static void
node_add_child(HalNode *parent, HalNode *child)
{
	child->parent = parent;
	child->next = parent->children;
	parent->children = child;
}

static void
hal_init_filesystem(void)
{
	HalNode *pci_dir, *pci_dev, *config, *ctl;

	/* Create root */
	server.root = node_create("", 1);
	server.root->type = HAL_ROOT;

	/* Create /hal/pci/ */
	pci_dir = node_create("pci", 1);
	pci_dir->type = HAL_PCI_DIR;
	node_add_child(server.root, pci_dir);

	/* Create example device /hal/pci/00:1f.2/ */
	pci_dev = node_create("00:1f.2", 1);
	pci_dev->type = HAL_PCI_DEVICE;
	pci_dev->pci.bus = 0;
	pci_dev->pci.device = 0x1f;
	pci_dev->pci.function = 2;
	pci_dev->pci.config_fd = -1;
	node_add_child(pci_dir, pci_dev);

	/* Create /hal/pci/00:1f.2/config */
	config = node_create("config", 0);
	config->type = HAL_PCI_CONFIG;
	config->pci = pci_dev->pci;
	config->length = 256;
	node_add_child(pci_dev, config);

	/* Create /hal/pci/00:1f.2/ctl */
	ctl = node_create("ctl", 0);
	ctl->type = HAL_PCI_CTL;
	ctl->pci = pci_dev->pci;
	node_add_child(pci_dev, ctl);

	printf("HAL: Filesystem tree initialized\n");
}

/* ===== Shared Memory Setup (simulates Exchange Page) ===== */

static ExchangeControl*
setup_shared_memory(void)
{
	/* Allocate memory that both server and client can access
	 * In production, this would use kernel Exchange API
	 * For testing, we use a global variable
	 */
	shared_exch = calloc(1, sizeof(ExchangeControl));
	if(!shared_exch) {
		fprintf(stderr, "HAL: Failed to allocate exchange page\n");
		return NULL;
	}

	return shared_exch;
}

/* Export exchange page address for client */
void
hal_get_exchange_page(ExchangeControl **exch_out)
{
	*exch_out = shared_exch;
}

/* ===== Main Entry Point ===== */

int
main(int argc, char *argv[])
{
	ExchangeControl *exch;

	(void)argc;
	(void)argv;

	printf("HAL: Exchange-based 9P Server starting...\n");

	/* Initialize server */
	memset(&server, 0, sizeof(server));
	server.next_qid_path = 0;
	hal_init_filesystem();

	/* Setup exchange page */
	exch = setup_shared_memory();
	if(!exch) {
		fprintf(stderr, "HAL: Failed to setup exchange page\n");
		return 1;
	}

	printf("HAL: Exchange Page at %p (exported globally)\n", (void*)exch);
	printf("HAL: Clients can access via hal_get_exchange_page()\n");
	printf("HAL: Exchange Page Layout:\n");
	printf("     Control:  offset 0x%04lx, size %lu bytes\n",
	       (unsigned long)0, sizeof(ExchangeControl));
	printf("     Request:  offset 0x%04lx, size %lu bytes\n",
	       (unsigned long)((char*)exch->request - (char*)exch),
	       sizeof(exch->request));
	printf("     Response: offset 0x%04lx, size %lu bytes\n",
	       (unsigned long)((char*)exch->response - (char*)exch),
	       sizeof(exch->response));

	/* Serve requests */
	hal_serve_exchange_page(exch);

	return 0;
}
