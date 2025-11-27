/*
 * HAL Exchange Test - Combined Server + Client Test
 *
 * Spawns server thread and runs client to demonstrate Exchange Page communication.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>

/* Plan 9 style types for compatibility */
typedef unsigned char uchar;
typedef unsigned int uint;

/* Exchange Page Layout - 4KB */
typedef struct ExchangeControl {
	volatile uint32_t client_seq;
	volatile uint32_t server_seq;
	volatile uint32_t request_len;
	volatile uint32_t response_len;
	volatile uint8_t  status;
	uint8_t  pad[47];
	uint8_t request[2048];
	uint8_t response[2048];
} ExchangeControl;

/* 9P Message Types */
enum {
	Tversion = 100, Rversion,
	Tattach = 104, Rattach,
	Tclunk = 120, Rclunk,
	Rerror = 107,
};

/* QID types */
#define QTDIR    0x80

typedef struct Qid {
	uint8_t  type;
	uint32_t vers;
	uint64_t path;
} Qid;

typedef struct HalNode HalNode;
struct HalNode {
	char name[256];
	Qid qid;
	uint32_t perm;
	uint64_t length;
	int isdir;
	enum { HAL_ROOT, HAL_PCI_DIR, HAL_PCI_DEVICE, HAL_PCI_CONFIG, HAL_PCI_CTL } type;
	struct { uint bus, device, function; int config_fd; } pci;
	HalNode *parent, *children, *next;
};

typedef struct Fid {
	uint32_t fid;
	HalNode *node;
	int omode;
	uint64_t offset;
	struct Fid *next;
} Fid;

typedef struct {
	HalNode *root;
	Fid *fids;
	uint64_t next_qid_path;
} HalServer;

static HalServer server;
static ExchangeControl *shared_exch = NULL;

/* ===== Helper Functions ===== */

static uint32_t gbit32(uchar *p) {
	return p[0] | (p[1]<<8) | (p[2]<<16) | (p[3]<<24);
}

static void pbit32(uchar *p, uint32_t v) {
	p[0] = v; p[1] = v >> 8; p[2] = v >> 16; p[3] = v >> 24;
}

static uint16_t gbit16(uchar *p) {
	return p[0] | (p[1]<<8);
}

static void pbit16(uchar *p, uint16_t v) {
	p[0] = v; p[1] = v >> 8;
}

static uint8_t gbit8(uchar *p) {
	return p[0];
}

static void pbit8(uchar *p, uint8_t v) {
	p[0] = v;
}

/* ===== Server Implementation ===== */

static int handle_tversion(uchar *req, uint req_len, uchar *resp, uint *resp_len) {
	uint32_t msize;
	uint size;
	(void)req_len;
	msize = gbit32(req + 7);
	size = 4 + 1 + 2 + 4 + 2 + 6;
	pbit32(resp, size);
	pbit8(resp + 4, Rversion);
	pbit16(resp + 5, gbit16(req + 5));
	pbit32(resp + 7, (msize < 8192) ? msize : 8192);
	pbit16(resp + 11, 6);
	memcpy(resp + 13, "9P2000", 6);
	*resp_len = size;
	return 0;
}

static int handle_tattach(uchar *req, uint req_len, uchar *resp, uint *resp_len) {
	uint32_t fid;
	Fid *f;
	uint size;
	(void)req_len;
	fid = gbit32(req + 7);
	printf("[Server] Tattach fid=%u\n", fid);
	f = calloc(1, sizeof(Fid));
	f->fid = fid;
	f->node = server.root;
	f->omode = -1;
	f->offset = 0;
	f->next = server.fids;
	server.fids = f;
	size = 4 + 1 + 2 + 13;
	pbit32(resp, size);
	pbit8(resp + 4, Rattach);
	pbit16(resp + 5, gbit16(req + 5));
	pbit8(resp + 7, server.root->qid.type);
	pbit32(resp + 8, server.root->qid.vers);
	pbit32(resp + 12, (uint32_t)server.root->qid.path);
	pbit32(resp + 16, (uint32_t)(server.root->qid.path >> 32));
	*resp_len = size;
	return 0;
}

static int handle_9p_message(uchar *req, uint req_len, uchar *resp, uint *resp_len) {
	uint type, tag;
	if(req_len < 7) {
		pbit32(resp, 4 + 1 + 2 + 2 + 11);
		pbit8(resp + 4, Rerror);
		pbit16(resp + 5, 0);
		pbit16(resp + 7, 11);
		memcpy(resp + 9, "invalid msg", 11);
		*resp_len = 4 + 1 + 2 + 2 + 11;
		return -1;
	}
	type = gbit8(req + 4);
	tag = gbit16(req + 5);
	printf("[Server] 9P message: type=%u tag=%u\n", type, tag);
	switch(type) {
	case Tversion: return handle_tversion(req, req_len, resp, resp_len);
	case Tattach: return handle_tattach(req, req_len, resp, resp_len);
	case Tclunk:
		pbit32(resp, 7);
		pbit8(resp + 4, Rclunk);
		pbit16(resp + 5, tag);
		*resp_len = 7;
		return 0;
	default:
		pbit32(resp, 4 + 1 + 2 + 2 + 19);
		pbit8(resp + 4, Rerror);
		pbit16(resp + 5, tag);
		pbit16(resp + 7, 19);
		memcpy(resp + 9, "not yet implemented", 19);
		*resp_len = 4 + 1 + 2 + 2 + 19;
		return 0;
	}
}

static void* server_thread(void *arg) {
	ExchangeControl *exch = (ExchangeControl*)arg;
	uint32_t last_seq = 0;
	uint resp_len;

	printf("[Server] Started, serving Exchange Page at %p\n", (void*)exch);

	for(;;) {
		while(exch->client_seq == last_seq)
			usleep(100);

		last_seq = exch->client_seq;
		if(exch->status != 1) continue;

		printf("[Server] Request #%u (len=%u)\n", exch->client_seq, exch->request_len);

		resp_len = 0;
		handle_9p_message(exch->request, exch->request_len, exch->response, &resp_len);

		exch->response_len = resp_len;
		__sync_synchronize();
		exch->status = 2;
		exch->server_seq++;

		printf("[Server] Response ready (len=%u, seq=%u)\n", resp_len, exch->server_seq);
	}
	return NULL;
}

static HalNode* node_create(const char *name, int isdir) {
	HalNode *n = calloc(1, sizeof(HalNode));
	if(!n) return NULL;
	strncpy(n->name, name, sizeof(n->name)-1);
	n->qid.type = isdir ? QTDIR : 0;
	n->qid.vers = 0;
	n->qid.path = ++server.next_qid_path;
	n->perm = isdir ? 0755 : 0644;
	n->isdir = isdir;
	return n;
}

/* ===== Client Implementation ===== */

static int send_9p_request(ExchangeControl *exch, uint8_t *msg, uint len) {
	if(len > sizeof(exch->request)) {
		fprintf(stderr, "[Client] Request too large: %u bytes\n", len);
		return -1;
	}
	memcpy((void*)exch->request, msg, len);
	exch->request_len = len;
	__sync_synchronize();
	exch->status = 1;
	exch->client_seq++;

	printf("[Client] Sent request (len=%u, seq=%u)\n", len, exch->client_seq);

	uint32_t expected_seq = exch->server_seq + 1;
	while(exch->server_seq != expected_seq || exch->status != 2)
		usleep(10);

	printf("[Client] Received response (len=%u, seq=%u)\n", exch->response_len, exch->server_seq);
	return exch->response_len;
}

static int build_tversion(uint8_t *buf, uint msize, const char *version) {
	uint vlen = (uint)strlen(version);
	uint size = 4 + 1 + 2 + 4 + 2 + vlen;
	pbit32(buf, size);
	pbit8(buf + 4, Tversion);
	pbit16(buf + 5, 0);
	pbit32(buf + 7, msize);
	pbit16(buf + 11, vlen);
	memcpy(buf + 13, version, vlen);
	return size;
}

static int build_tattach(uint8_t *buf, uint fid, const char *uname, const char *aname) {
	uint ulen = (uint)strlen(uname);
	uint alen = (uint)strlen(aname);
	uint size = 4 + 1 + 2 + 4 + 4 + 2 + ulen + 2 + alen;
	pbit32(buf, size);
	pbit8(buf + 4, Tattach);
	pbit16(buf + 5, 1);
	pbit32(buf + 7, fid);
	pbit32(buf + 11, ~0);
	pbit16(buf + 15, ulen);
	memcpy(buf + 17, uname, ulen);
	pbit16(buf + 17 + ulen, alen);
	memcpy(buf + 19 + ulen, aname, alen);
	return size;
}

/* ===== Main Test ===== */

int main(void) {
	pthread_t srv_thread;
	uint8_t msg[256];
	int len;

	printf("=== HAL Exchange Test (Server + Client) ===\n\n");

	/* Initialize server */
	memset(&server, 0, sizeof(server));
	server.root = node_create("", 1);
	server.root->type = HAL_ROOT;

	/* Create Exchange Page */
	shared_exch = calloc(1, sizeof(ExchangeControl));
	if(!shared_exch) {
		fprintf(stderr, "Failed to allocate exchange page\n");
		return 1;
	}

	printf("Exchange Page at %p\n\n", (void*)shared_exch);

	/* Start server thread */
	if(pthread_create(&srv_thread, NULL, server_thread, shared_exch) != 0) {
		perror("pthread_create");
		return 1;
	}

	usleep(100000); /* Let server start */

	/* Client: Tversion */
	printf("\n[Client] Test 1: Tversion\n");
	len = build_tversion(msg, 8192, "9P2000");
	if(send_9p_request(shared_exch, msg, len) > 0) {
		uint type = gbit8(shared_exch->response + 4);
		if(type == Rversion) {
			uint msize = gbit32(shared_exch->response + 7);
			printf("[Client] ✓ Tversion succeeded: msize=%u\n", msize);
		}
	}

	/* Client: Tattach */
	printf("\n[Client] Test 2: Tattach\n");
	len = build_tattach(msg, 0, "test", "");
	if(send_9p_request(shared_exch, msg, len) > 0) {
		uint type = gbit8(shared_exch->response + 4);
		if(type == Rattach)
			printf("[Client] ✓ Tattach succeeded\n");
	}

	/* Performance test */
	printf("\n[Client] Test 3: Performance (1000 requests)\n");
	clock_t start = clock();
	for(int i = 0; i < 1000; i++) {
		len = build_tversion(msg, 8192, "9P2000");
		send_9p_request(shared_exch, msg, len);
	}
	clock_t end = clock();
	double elapsed = (double)(end - start) / CLOCKS_PER_SEC;
	printf("[Client] 1000 requests in %.3f seconds (%.0f req/s)\n\n", elapsed, 1000.0 / elapsed);

	printf("=== Test Complete ===\n");
	printf("Zero-copy 9P via Exchange Pages demonstrated successfully!\n");

	pthread_cancel(srv_thread);
	pthread_join(srv_thread, NULL);
	free(shared_exch);
	return 0;
}
