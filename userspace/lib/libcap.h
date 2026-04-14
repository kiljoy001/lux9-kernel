/*
 * libcap.h - Capability Token Helper Library for Userspace
 *
 * Provides simple APIs for services and clients to work
 * with the resurrection server's capability system.
 */

#ifndef _LIBCAP_H_
#define _LIBCAP_H_

#include "../../kernel/include/capability.h"
#include "../../kernel/include/u.h"

/*
 * Service registration
 */

/* Register this service with resurrection, obtain our token */
int srv_register(const char *name, u64int flags, CapToken *out_token);

/* Deregister service (on clean shutdown) */
int srv_deregister(const char *name);

/*
 * Client access
 */

/* Request a capability token to access a service */
int cap_request(const char *service, u64int flags, CapToken *out_token);

/* Refresh a token (get new epoch) */
int cap_refresh(CapToken *tok);

/*
 * Verification (called by services)
 */

/* Verify a client's token grants access to us */
int cap_verify_client(const CapToken *client_tok, const char *our_service);

/* Check if token has specific flags */
int cap_has_flags(const CapToken *tok, u64int required_flags);

/*
 * Low-level operations
 */

/* Get resurrection's public key (for offline verification) */
int cap_get_resurrection_pubkey(u8int pubkey[32]);

/* Get our own client/service identity */
int cap_get_identity(u8int identity[32]);

#endif /* _LIBCAP_H_ */
