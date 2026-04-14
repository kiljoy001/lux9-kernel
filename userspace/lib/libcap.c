/*
 * libcap.c - Capability Token Helper Library Implementation
 *
 * Provides userspace APIs for interacting with resurrection's
 * capability-based /srv namespace.
 */

#include "../../kernel/include/capability.h"
#include "../../kernel/include/libc.h"
#include "../../kernel/include/u.h"

/* Cached resurrection public key */
static u8int resurrection_pubkey[32];
static int pubkey_loaded = 0;

/* Our identity (derived from process info) */
static u8int our_identity[32];
static int identity_loaded = 0;

/* Resurrection file descriptor */
static int resurrection_fd = -1;

enum {
  CAPREQ_ISSUE = 1,
  CAPREQ_REFRESH = 2,
};

#define CAP_SERVICE_NAME_LEN 64

typedef struct CapIssueRequest {
  u32int op;
  char service[CAP_SERVICE_NAME_LEN];
  u8int client_id[CAP_HASH_SIZE];
  u64int flags;
} CapIssueRequest;

typedef struct CapRefreshRequest {
  u32int op;
  CapToken token;
  u8int client_id[CAP_HASH_SIZE];
} CapRefreshRequest;

/*
 * Internal: ensure connection to resurrection
 */
static int ensure_resurrection(void) {
  if (resurrection_fd >= 0)
    return 0;

  resurrection_fd = open("/srv/.resurrection", ORDWR);
  if (resurrection_fd < 0)
    return -1;

  return 0;
}

int cap_get_identity(u8int identity[32]) {
  if (!identity_loaded) {
    int pid = getpid();
    memset(our_identity, 0, sizeof(our_identity));
    our_identity[0] = pid & 0xFF;
    our_identity[1] = (pid >> 8) & 0xFF;
    our_identity[2] = (pid >> 16) & 0xFF;
    our_identity[3] = (pid >> 24) & 0xFF;
    identity_loaded = 1;
  }

  memcpy(identity, our_identity, 32);
  return 0;
}

/*
 * Helper: Request token (common for srv_register and cap_request)
 * mode: 0=Request for self (register), 1=Request for other (cap_request)
 */
static int request_token(const char *service_name, u64int flags,
                         CapToken *out_token) {
  u8int identity[32];
  CapIssueRequest req;

  if (service_name == 0 || out_token == 0)
    return -1;
  if (ensure_resurrection() < 0)
    return -1;

  memset(&req, 0, sizeof(req));
  req.op = CAPREQ_ISSUE;
  cap_get_identity(identity);
  memcpy(req.client_id, identity, sizeof(req.client_id));
  strncpy(req.service, service_name, sizeof(req.service) - 1);
  req.service[sizeof(req.service) - 1] = 0;
  req.flags = flags;

  if (write(resurrection_fd, &req, sizeof(req)) != sizeof(req))
    return -1;
  if (read(resurrection_fd, out_token, sizeof(*out_token)) != sizeof(*out_token))
    return -1;
  return 0;
}

/*
 * srv_register - Register a service with resurrection
 */
int srv_register(const char *name, u64int flags, CapToken *out_token) {
  char path[128];
  int fd;
  int i;

  i = snprint(path, sizeof(path), "/srv/%s", name);
  if (i <= 0 || i >= (int)sizeof(path))
    return -1;
  fd = create(path, ORDWR, 0666);
  if (fd < 0)
    return -1;
  close(fd);

  return request_token(name, flags, out_token);
}

/*
 * srv_deregister - Remove service from /srv
 */
int srv_deregister(const char *name) {
  char path[128];
  int i;

  i = snprint(path, sizeof(path), "/srv/%s", name);
  if (i <= 0 || i >= (int)sizeof(path))
    return -1;
  return remove(path);
}

/*
 * cap_request - Request a token to access a service
 */
int cap_request(const char *service, u64int flags, CapToken *out_token) {
  if (ensure_resurrection() < 0)
    return -1;

  return request_token(service, flags, out_token);
}

/*
 * cap_refresh - Get a new token with current epoch
 */
int cap_refresh(CapToken *tok) {
  CapRefreshRequest req;
  u8int identity[32];

  if (tok == 0)
    return -1;
  if (ensure_resurrection() < 0)
    return -1;

  memset(&req, 0, sizeof(req));
  req.op = CAPREQ_REFRESH;
  memcpy(&req.token, tok, sizeof(req.token));
  cap_get_identity(identity);
  memcpy(req.client_id, identity, sizeof(req.client_id));

  if (write(resurrection_fd, &req, sizeof(req)) != sizeof(req))
    return -1;
  if (read(resurrection_fd, tok, sizeof(*tok)) != sizeof(*tok))
    return -1;
  return 0;
}

int cap_get_resurrection_pubkey(u8int pubkey[32]) {
  int fd;

  if (pubkey_loaded) {
    memcpy(pubkey, resurrection_pubkey, sizeof(resurrection_pubkey));
    return 0;
  }

  fd = open("/srv/.resurrection", OREAD);
  if (fd < 0)
    return -1;
  if (read(fd, resurrection_pubkey, sizeof(resurrection_pubkey)) !=
      sizeof(resurrection_pubkey)) {
    close(fd);
    return -1;
  }
  close(fd);

  pubkey_loaded = 1;
  memcpy(pubkey, resurrection_pubkey, sizeof(resurrection_pubkey));
  return 0;
}

/*
 * cap_verify_client - Verify a client's token
 */
int cap_verify_client(const CapToken *client_tok, const char *our_service) {
  u8int service_hash[32];
  u8int pubkey[32];
  u64int current_epoch;

  cap_hash_service(service_hash, our_service);

  if (cap_get_resurrection_pubkey(pubkey) < 0)
    return -1;

  current_epoch = cap_current_epoch(EPOCH_HOUR);

  return cap_token_verify(client_tok, service_hash, pubkey, current_epoch);
}

/*
 * cap_has_flags - Check if token has required permission flags
 */
int cap_has_flags(const CapToken *tok, u64int required_flags) {
  return (tok->flags & required_flags) == required_flags;
}

/*
 * cap_open - Open a service file and authenticate with token
 */
int cap_open(const char *path, int mode, const CapToken *tok) {
  int fd = open((char *)path, mode);
  if (fd < 0)
    return -1;

  /* Write token to authenticate */
  if (write(fd, (void *)tok, sizeof(CapToken)) != sizeof(CapToken)) {
    close(fd);
    return -1;
  }

  return fd;
}
