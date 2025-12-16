/* Crypto Server - Algorithm Abstraction Layer
 * Unified interface for all Supercop algorithms
 */
#ifndef CRYPTO_ABSTRACTION_H
#define CRYPTO_ABSTRACTION_H

#include <stdint.h>
#include <stddef.h>

/* Algorithm categories (matches Supercop structure) */
typedef enum {
	CRYPTO_CAT_HASH = 0,      /* crypto_hash */
	CRYPTO_CAT_AEAD,          /* crypto_aead - Authenticated Encryption (AES-GCM) */
	CRYPTO_CAT_STREAM,        /* crypto_stream - Stream ciphers */
	CRYPTO_CAT_SIGN,          /* crypto_sign - Digital signatures */
	CRYPTO_CAT_AUTH,          /* crypto_auth - MACs */
	CRYPTO_CAT_ENCRYPT,       /* crypto_encrypt - Public-key encryption */
	CRYPTO_CAT_KEM,           /* crypto_kem - Key encapsulation */
	CRYPTO_CAT_DH,            /* crypto_dh - Key exchange */
	CRYPTO_CAT_RNG,           /* crypto_rng - Random number generation */
	CRYPTO_CAT_MAX
} CryptoCategory;

/* Generic crypto context (opaque to users) */
typedef struct CryptoContext CryptoContext;

/* Algorithm descriptor */
typedef struct CryptoAlgorithm {
	const char *name;         /* e.g., "sha256", "aes256gcm" */
	CryptoCategory category;  /* Algorithm category */
	const char *impl;         /* Implementation variant (e.g., "sphlib", "ref") */

	/* Sizes (algorithm-specific) */
	size_t key_bytes;         /* Key size (0 if N/A) */
	size_t nonce_bytes;       /* Nonce/IV size (0 if N/A) */
	size_t output_bytes;      /* Output size (0 if variable) */
	size_t auth_bytes;        /* Authentication tag size (0 if N/A) */

	/* Lifecycle functions */
	int (*init)(CryptoContext *ctx);
	int (*update)(CryptoContext *ctx, const uint8_t *data, size_t len);
	int (*final)(CryptoContext *ctx, uint8_t *output, size_t *outlen);
	void (*cleanup)(CryptoContext *ctx);

	/* AEAD-specific functions (for AES-GCM, ChaCha20-Poly1305) */
	int (*aead_encrypt)(CryptoContext *ctx,
	                    const uint8_t *plaintext, size_t plen,
	                    const uint8_t *key, size_t keylen,
	                    const uint8_t *nonce, size_t noncelen,
	                    const uint8_t *ad, size_t adlen,
	                    uint8_t *ciphertext, size_t *clen,
	                    uint8_t *tag, size_t *taglen);

	int (*aead_decrypt)(CryptoContext *ctx,
	                    const uint8_t *ciphertext, size_t clen,
	                    const uint8_t *key, size_t keylen,
	                    const uint8_t *nonce, size_t noncelen,
	                    const uint8_t *ad, size_t adlen,
	                    const uint8_t *tag, size_t taglen,
	                    uint8_t *plaintext, size_t *plen);

	/* Sign/Verify functions (for Ed25519, etc.) */
	int (*sign)(CryptoContext *ctx,
	            const uint8_t *message, size_t mlen,
	            const uint8_t *secretkey, size_t sklen,
	            uint8_t *signature, size_t *siglen);

	int (*verify)(CryptoContext *ctx,
	              const uint8_t *message, size_t mlen,
	              const uint8_t *publickey, size_t pklen,
	              const uint8_t *signature, size_t siglen);

	/* Stream cipher functions */
	int (*stream_xor)(CryptoContext *ctx,
	                  const uint8_t *input, size_t len,
	                  const uint8_t *key, size_t keylen,
	                  const uint8_t *nonce, size_t noncelen,
	                  uint8_t *output);
} CryptoAlgorithm;

/* Algorithm registry */
typedef struct CryptoRegistry {
	CryptoAlgorithm **algorithms;  /* Array of algorithm descriptors */
	int count;                     /* Number of registered algorithms */
	int capacity;                  /* Array capacity */
} CryptoRegistry;

/* Global registry (populated at startup) */
extern CryptoRegistry crypto_registry;

/* Registry management */
int crypto_registry_init(void);
int crypto_registry_add(CryptoAlgorithm *algo);
CryptoAlgorithm *crypto_registry_find(const char *category, const char *name);
void crypto_registry_cleanup(void);

/* Context management */
CryptoContext *crypto_context_create(CryptoAlgorithm *algo);
void crypto_context_destroy(CryptoContext *ctx);

/* Unified API - dispatches to appropriate algorithm */
int crypto_hash_data(const char *algorithm,
                     const uint8_t *data, size_t len,
                     uint8_t *hash, size_t *hashlen);

int crypto_aead_encrypt_data(const char *algorithm,
                              const uint8_t *plaintext, size_t plen,
                              const uint8_t *key, size_t keylen,
                              const uint8_t *nonce, size_t noncelen,
                              const uint8_t *ad, size_t adlen,
                              uint8_t *ciphertext, size_t *clen,
                              uint8_t *tag, size_t *taglen);

int crypto_aead_decrypt_data(const char *algorithm,
                              const uint8_t *ciphertext, size_t clen,
                              const uint8_t *key, size_t keylen,
                              const uint8_t *nonce, size_t noncelen,
                              const uint8_t *ad, size_t adlen,
                              const uint8_t *tag, size_t taglen,
                              uint8_t *plaintext, size_t *plen);

int crypto_sign_data(const char *algorithm,
                     const uint8_t *message, size_t mlen,
                     const uint8_t *secretkey, size_t sklen,
                     uint8_t *signature, size_t *siglen);

int crypto_verify_signature(const char *algorithm,
                             const uint8_t *message, size_t mlen,
                             const uint8_t *publickey, size_t pklen,
                             const uint8_t *signature, size_t siglen);

/* Streaming API for large data (hash, AEAD) */
CryptoContext *crypto_stream_start(const char *category, const char *algorithm);
int crypto_stream_update(CryptoContext *ctx, const uint8_t *data, size_t len);
int crypto_stream_final(CryptoContext *ctx, uint8_t *output, size_t *outlen);
void crypto_stream_abort(CryptoContext *ctx);

/* Helper: List available algorithms */
int crypto_list_algorithms(CryptoCategory category, char ***names, int *count);

/* Helper: Get algorithm info */
int crypto_algorithm_info(const char *category, const char *name,
                          size_t *key_bytes, size_t *nonce_bytes,
                          size_t *output_bytes);

#endif /* CRYPTO_ABSTRACTION_H */
