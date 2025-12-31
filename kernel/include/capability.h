/*
 * capability.h - Cryptographic Capability Token Definitions
 *
 * Implements blind signature-based capability tokens with monotonic epochs
 * for the resurrection server's /srv namespace.
 */

#ifndef _CAPABILITY_H_
#define _CAPABILITY_H_

#include "u.h"

/* Token sizes */
#define CAP_HASH_SIZE 32  /* Blake2b-256 */
#define CAP_SIG_SIZE 64   /* Ed25519 signature */
#define CAP_NONCE_SIZE 16 /* Random nonce */
#define CAP_TOKEN_SIZE sizeof(CapToken)

/* Capability flags */
#define CAP_READ (1 << 0)     /* Read access */
#define CAP_WRITE (1 << 1)    /* Write access */
#define CAP_EXEC (1 << 2)     /* Execute/create access */
#define CAP_ADMIN (1 << 3)    /* Administrative access */
#define CAP_DELEGATE (1 << 4) /* Can delegate to others */

/* Epoch types */
#define EPOCH_HOUR 0    /* 1 hour epochs */
#define EPOCH_DAY 1     /* 1 day epochs */
#define EPOCH_SESSION 2 /* Until reboot */

/* Grace window: accept tokens from previous N epochs */
#define EPOCH_GRACE 1

/*
 * CapToken - Cryptographic capability token
 *
 * Issued via blind signature. Contains:
 * - service_hash: identifies which service this grants access to
 * - client_commit: commitment to client identity (unlinkable)
 * - epoch: monotonic timestamp for freshness
 * - flags: permission bits
 * - signature: blind signature from resurrection
 */
typedef struct CapToken {
  u8int service_hash[CAP_HASH_SIZE];  /* H(service_name) */
  u8int client_commit[CAP_HASH_SIZE]; /* H(client_id || nonce) */
  u64int epoch;                       /* Monotonic epoch */
  u64int flags;                       /* Permission flags */
  u8int signature[CAP_SIG_SIZE];      /* Blind signature */
} CapToken;

/*
 * CapBlindRequest - Blinded token request
 *
 * Client creates this, sends to resurrection for signing.
 * Resurrection cannot see the actual content.
 */
typedef struct CapBlindRequest {
  u8int blinded_data[CAP_HASH_SIZE + CAP_HASH_SIZE + 16]; /* Blinded content */
  u8int blinding_factor[32]; /* Client keeps this secret */
} CapBlindRequest;

/*
 * CapBlindResponse - Blinded signature from resurrection
 */
typedef struct CapBlindResponse {
  u8int blind_signature[CAP_SIG_SIZE];
} CapBlindResponse;

/* Epoch management */
u64int cap_current_epoch(int epoch_type);
int cap_epoch_valid(u64int token_epoch, u64int current_epoch, int grace);

/* Token operations */
int cap_token_init(CapToken *tok, const char *service, const u8int *client_id,
                   u64int flags);
int cap_token_verify(const CapToken *tok, const u8int *service_hash,
                     const u8int *pubkey, u64int current_epoch);

/* Blind signature operations */
int cap_blind_prepare(CapBlindRequest *req, const CapToken *tok);
int cap_blind_sign(CapBlindResponse *resp, const CapBlindRequest *req,
                   const u8int *signing_key);
int cap_unblind(CapToken *tok, const CapBlindResponse *resp,
                const CapBlindRequest *req);

/* Utility */
void cap_hash_service(u8int *out, const char *service_name);
void cap_hash_client(u8int *out, const u8int *client_id, const u8int *nonce);

/*
 * Epoch-Bound Key Derivation
 *
 * Keys are derived from a master secret + epoch, providing:
 * - Automatic key rotation each epoch
 * - Single master secret to protect
 * - Atomic revocation by rotating master
 */

/* Master key structure (stored securely in resurrection) */
typedef struct CapMasterKey {
  u8int secret[32];      /* Master secret */
  u64int creation_epoch; /* When this master was created */
} CapMasterKey;

/* Derived keys for a specific epoch */
typedef struct CapEpochKeys {
  u8int signing_seed[32];   /* Ed25519 seed for this epoch */
  u8int signing_pubkey[32]; /* Ed25519 public key */
  u8int hmac_key[32];       /* HMAC key for this epoch */
  u64int epoch;             /* Which epoch these keys are for */
} CapEpochKeys;

/* Derive keys for a specific epoch from master */
int cap_derive_epoch_keys(CapEpochKeys *out, const CapMasterKey *master,
                          u64int epoch);

/* Get current epoch's keys (caches internally) */
int cap_get_current_keys(CapEpochKeys *out, const CapMasterKey *master);

/* Verify a key's epoch is valid */
int cap_keys_valid_for_epoch(const CapEpochKeys *keys, u64int current_epoch);

#endif /* _CAPABILITY_H_ */
