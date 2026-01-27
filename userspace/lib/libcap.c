/*
 * libcap.c - Capability Token Helper Library Implementation
 *
 * Provides userspace APIs for interacting with resurrection's
 * capability-based /srv namespace.
 */

#include "../../kernel/include/capability.h"
#include "../../kernel/include/fcall.h"
#include "../../kernel/include/libc.h"
#include "../../kernel/include/u.h"

/* Helper */
static u32int get_u32(uchar *p) {
  return p[0] | (p[1] << 8) | (p[2] << 16) | (p[3] << 24);
}

/* Cached resurrection public key */
static u8int resurrection_pubkey[32];
static int pubkey_loaded = 0;

/* Our identity (derived from process info) */
static u8int our_identity[32];
static int identity_loaded = 0;

/* Resurrection file descriptor */
static int resurrection_fd = -1;

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

/*
 * Internal: send a 9P message to resurrection
 */
/*
 * Internal: send a 9P message to resurrection with optional extension
 */
static int send_9p_ext(Fcall *tx, Fcall *rx, void *ext, int extlen,
                       void *out_ext, int *out_extlen) {
  uchar buf[8192];
  uint n;

  /* Serialize standard part */
  n = convS2M(tx, buf, sizeof(buf));
  if (n == 0)
    return -1;

  /* Append extension */
  if (ext && extlen > 0) {
    if (n + extlen > sizeof(buf))
      return -1;
    memcpy(buf + n, ext, extlen);

    /* Update size */
    u32int size = get_u32(buf); /* using local helper or macros? */
    /* use macros if available or manual */
    buf[0] = (size + extlen);
    buf[1] = (size + extlen) >> 8;
    buf[2] = (size + extlen) >> 16;
    buf[3] = (size + extlen) >> 24;

    n += extlen;
  }

  if (write(resurrection_fd, buf, n) != n)
    return -1;

  n = read(resurrection_fd, buf, sizeof(buf));
  if (n <= 0)
    return -1;

  if (convM2S(buf, n, rx) == 0)
    return -1;

  if (rx->type == Rerror)
    return -1;

  /* Handle response extension (e.g. signature in Rauth) */
  if (out_ext && out_extlen) {
    u32int std_size = sizeS2M(rx); /* Size of standard packet */
    if (n > std_size) {
      int avail = n - std_size;
      /* Extension format: [siglen:4] [sig:siglen] */
      /* But wait, sizeS2M might calculate strict size based on struct? */
      /* Actually convM2S returns len consumed? No, returns size? */
      /* convS2M returns length. sizeS2M returns length. */
      /* We need to assume the buffer matches standard layout until std_size. */

      /* Manual parse from offset? */
      /* Rauth std size: 4+1+2+13 = 20 bytes? */
      /* 4(size) + 1(type) + 2(tag) + 13(qid) = 20. */
      /* So data at buf + 20 */

      if (rx->type == Rauth) {
        /* Hardcoded offset for Rauth extension */
        uint offset = 4 + 1 + 2 + 13;
        if (n > offset + 4) {
          u32int siglen = buf[offset] | (buf[offset + 1] << 8) |
                          (buf[offset + 2] << 16) | (buf[offset + 3] << 24);
          offset += 4;
          if (n >= offset + siglen) {
            if (*out_extlen >= siglen) {
              memcpy(out_ext, buf + offset, siglen);
              *out_extlen = siglen;
            }
          }
        }
      }
    }
  }

  return 0;
}

static int send_9p(Fcall *tx, Fcall *rx) {
  return send_9p_ext(tx, rx, 0, 0, 0, 0);
}

/*
 * cap_get_identity - Get our process identity
 */
extern int cap_hash_client(u8int *hash, int pid,
                           const u8int *secret); // from blind_cap.c

int cap_get_identity(u8int identity[32]) {
  if (!identity_loaded) {
    /* Using dummy secret for now, should come from kernel/env */
    u8int secret[32];
    memset(secret, 0xEE, 32);
    cap_hash_client(our_identity, getpid(), secret);
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
  Fcall tx, rx;
  u8int identity[32];
  CapBlindRequest req;
  CapBlindResponse resp;
  u8int signature[128];
  int siglen = 128;

  cap_get_identity(identity);
  cap_token_init(out_token, service_name, identity, flags);
  cap_blind_prepare(&req, out_token);

  /* Send blind request via Tauth extension */
  tx.type = Tauth;
  tx.tag = 1;
  tx.afid = NOFID; /* We use Tauth to get signature, not actual auth file?
                      resurrection.c uses srv_alloc_fid(afid).
                      Standard 9P uses afid. */
  tx.afid = 999;   /* Dummy AFID */
  tx.uname = (char *)service_name;
  tx.aname = "";

  /* Prepend size to extension? resurrection currently expects RAW struct */
  /* resurrection: size = get_u32(req); if size < pos + sizeof... */
  /* So just passing the struct is fine if we updated srv_handle_auth to expect
   * correct offset? */
  /* resurrection srv_handle_auth expects data at end. */
  /* But my send_9p_ext appends it. */

  /* Extension payload: [len:4] [data...] ? */
  /* resurrection code: u32int size = get_u32(req); */
  /* Wait, get_u32(req) gets the PACKET size. */

  /* resurrection check: if (size < pos + sizeof(CapBlindRequest)) */
  /* So we must ensure total packet size includes payload. send_9p_ext does
   * this. */
  /* And payload must be just the struct. */

  if (send_9p_ext(&tx, &rx, &req, sizeof(req), signature, &siglen) < 0)
    return -1;

  /* Unblind signature */
  memcpy(resp.blind_signature, signature, 64); /* Assume 64 byte sig */
  cap_unblind(out_token, &resp, &req);

  return 0;
}

/*
 * srv_register - Register a service with resurrection
 */
int srv_register(const char *name, u64int flags, CapToken *out_token) {
  Fcall tx, rx;

  if (ensure_resurrection() < 0)
    return -1;

  /* Create entry in /srv */
  tx.type = Tcreate;
  tx.tag = 1;
  tx.fid = 0; /* root */
  tx.name = (char *)name;
  tx.perm = 0666;
  tx.mode = ORDWR;

  if (send_9p(&tx, &rx) < 0)
    return -1;

  /* Request token for ourselves (to prove we are the service) */
  return request_token(name, flags, out_token);
}

/*
 * srv_deregister - Remove service from /srv
 */
int srv_deregister(const char *name) {
  /* Tremove not fully implemented in resurrection yet? Tclunk is. */
  /* resurrection assumes Tremove on file */
  return -1;
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
  return cap_request("", tok->flags,
                     tok); /* Service name lost? CapToken has hash, not name. */
  /* TODO: Store name in CapToken? Or pass name */
  return -1;
}

/*
 * cap_verify_client - Verify a client's token
 */
int cap_verify_client(const CapToken *client_tok, const char *our_service) {
  u8int service_hash[32];
  u8int pubkey[32];
  u64int current_epoch;

  if (cap_hash_service(service_hash, our_service) < 0)
    return -1;

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
  int fd = open(path, mode);
  if (fd < 0)
    return -1;

  /* Write token to authenticate */
  if (write(fd, tok, sizeof(CapToken)) != sizeof(CapToken)) {
    close(fd);
    return -1;
  }

  return fd;
}
