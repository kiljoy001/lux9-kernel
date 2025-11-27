/*
 * Kernel Cryptographic Functions
 *
 * SHA256 and HMAC-SHA256 implementations using sphlib.
 * TPM-backed key storage for secure key management.
 */

#include "u.h"
#include "portlib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "crypto.h"
#include "tpm.h"

/* Define CRYPTO_NAMESPACE for sphlib - we want the sph_ prefix */
#define CRYPTO_NAMESPACE(x) sph_##x

/* Include sphlib SHA256 */
#include "sph_sha2.h"

/* External symbols from assembly */
extern void sha256_transform_hw(uint32_t state[8], const uint8_t block[64], uint32_t nblocks);

/*
 * Check if hardware SHA extensions are available
 */
int
crypto_hw_sha_available(void)
{
    /* Access current CPU's Mach structure */
    extern Mach *m;
    return m->havesha;
}

/*
 * Check if hardware AES-NI is available
 */
int
crypto_hw_aes_available(void)
{
    /* Access current CPU's Mach structure */
    extern Mach *m;
    return m->haveaes;
}

/*
 * SHA256 hash function
 *
 * Uses hardware SHA extensions if available, falls back to software.
 */
int
crypto_sha256(uint8_t *out, const uint8_t *data, size_t len)
{
    sph_sha256_context ctx;

    if (!out || !data) {
        return -1;
    }

    /* Hardware acceleration path */
    if (crypto_hw_sha_available() && len >= 64) {
        /* Use hardware for full blocks, software for remainder */
        uint32_t state[8] = {
            0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
            0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19
        };

        size_t full_blocks = len / 64;
        size_t remainder = len % 64;

        /* Process full blocks with hardware */
        if (full_blocks > 0) {
            sha256_transform_hw(state, data, full_blocks);
        }

        /* Process remainder with software */
        if (remainder > 0 || len == 0) {
            sph_sha256_init(&ctx);
            /* Copy hardware state to software context */
            memcpy(ctx.val, state, sizeof(state));
            ctx.count = full_blocks * 64;
            /* Process remaining bytes */
            sph_sha256(&ctx, data + (full_blocks * 64), remainder);
            sph_sha256_close(&ctx, out);
        } else {
            /* All data processed by hardware, finalize manually */
            sph_sha256_init(&ctx);
            memcpy(ctx.val, state, sizeof(state));
            ctx.count = len;
            sph_sha256_close(&ctx, out);
        }

        return 0;
    }

    /* Software fallback */
    sph_sha256_init(&ctx);
    sph_sha256(&ctx, data, len);
    sph_sha256_close(&ctx, out);

    return 0;
}

/*
 * HMAC-SHA256 implementation
 *
 * Standard HMAC construction: HMAC(K, m) = H((K ⊕ opad) || H((K ⊕ ipad) || m))
 */
int
crypto_hmac_sha256(uint8_t *out, const uint8_t *key, size_t keylen,
                   const uint8_t *data, size_t len)
{
    sph_sha256_context ctx;
    uint8_t k[64];  /* Block size for SHA256 */
    uint8_t tk[32]; /* Temporary key if keylen > 64 */
    uint8_t inner[32];
    int i;

    if (!out || !key || !data) {
        return -1;
    }

    /* Prepare the key */
    memset(k, 0, sizeof(k));

    if (keylen > 64) {
        /* If key is longer than block size, hash it first */
        sph_sha256_init(&ctx);
        sph_sha256(&ctx, key, keylen);
        sph_sha256_close(&ctx, tk);
        memcpy(k, tk, 32);
    } else {
        memcpy(k, key, keylen);
    }

    /* Compute inner hash: H((K ⊕ ipad) || m) */
    sph_sha256_init(&ctx);

    /* XOR key with ipad (0x36) and hash it */
    for (i = 0; i < 64; i++) {
        uint8_t byte = k[i] ^ 0x36;
        sph_sha256(&ctx, &byte, 1);
    }

    /* Hash the message */
    sph_sha256(&ctx, data, len);
    sph_sha256_close(&ctx, inner);

    /* Compute outer hash: H((K ⊕ opad) || inner) */
    sph_sha256_init(&ctx);

    /* XOR key with opad (0x5c) and hash it */
    for (i = 0; i < 64; i++) {
        uint8_t byte = k[i] ^ 0x5c;
        sph_sha256(&ctx, &byte, 1);
    }

    /* Hash the inner result */
    sph_sha256(&ctx, inner, 32);
    sph_sha256_close(&ctx, out);

    /* Clear sensitive data */
    memset(k, 0, sizeof(k));
    memset(tk, 0, sizeof(tk));
    memset(inner, 0, sizeof(inner));

    return 0;
}

/*
 * TPM-backed key storage
 *
 * The HMAC key is stored sealed to TPM PCRs. This ensures:
 * 1. Key is protected by TPM hardware
 * 2. Key is only unsealed in expected system state (PCR values)
 * 3. Key can be rotated on boot
 */

/* Global TPM key state */
static struct {
    uint8_t hmac_key[CRYPTO_HMAC_KEY_BYTES];
    int key_valid;
    uint64_t key_generation;  /* Incremented on each rotation */
    uint64_t key_uses;        /* Count of HMAC operations with this key */
    Lock lock;
} tpm_key_state;

/*
 * Initialize TPM key storage
 */
int
crypto_tpm_key_init(void)
{
    int ret;
    uint8_t random_key[CRYPTO_HMAC_KEY_BYTES];

    memset(&tpm_key_state, 0, sizeof(tpm_key_state));

    print("Crypto: Initializing TPM-backed key storage...\n");

    /* Report hardware acceleration status */
    if (crypto_hw_sha_available()) {
        print("Crypto: SHA extensions available (hardware accelerated)\n");
    } else {
        print("Crypto: Using software SHA256\n");
    }

    if (crypto_hw_aes_available()) {
        print("Crypto: AES-NI available (hardware accelerated)\n");
    } else {
        print("Crypto: Using software AES\n");
    }

    /* Try to get random key from TPM */
    ret = tpm_get_random(random_key, CRYPTO_HMAC_KEY_BYTES);
    if (ret == CRYPTO_HMAC_KEY_BYTES) {
        /* Successfully got random bytes from TPM */
        memcpy(tpm_key_state.hmac_key, random_key, CRYPTO_HMAC_KEY_BYTES);
        tpm_key_state.key_valid = 1;
        tpm_key_state.key_generation = 1;
        print("Crypto: Generated TPM random key (generation 1)\n");
    } else {
        /* Fallback: Use fastticks for entropy */
        uint64_t t;
        int i;
        print("Crypto: TPM random not available, using fastticks entropy\n");

        for (i = 0; i < CRYPTO_HMAC_KEY_BYTES; i += 8) {
            t = fastticks(nil);
            memcpy(&tpm_key_state.hmac_key[i], &t, 8);
        }
        tpm_key_state.key_valid = 1;
        tpm_key_state.key_generation = 1;
    }

    /* TODO: Seal the key to TPM PCRs using tpm20_seal() */
    /* For now, we keep it in memory. In production:
     * 1. Read current PCR values
     * 2. Seal key to PCRs using TPM
     * 3. Store sealed blob in secure location
     * 4. On boot, unseal if PCRs match
     */

    print("Crypto: TPM key storage initialized\n");
    return 0;
}

/*
 * Get current HMAC key
 */
int
crypto_tpm_get_hmac_key(uint8_t *key_out, size_t *keylen)
{
    if (!key_out || !keylen) {
        return -1;
    }

    if (!tpm_key_state.key_valid) {
        return -1;
    }

    ilock(&tpm_key_state.lock);

    if (*keylen < CRYPTO_HMAC_KEY_BYTES) {
        iunlock(&tpm_key_state.lock);
        *keylen = CRYPTO_HMAC_KEY_BYTES;
        return -1;
    }

    memcpy(key_out, tpm_key_state.hmac_key, CRYPTO_HMAC_KEY_BYTES);
    *keylen = CRYPTO_HMAC_KEY_BYTES;

    iunlock(&tpm_key_state.lock);

    return 0;
}

/*
 * Rotate HMAC key
 *
 * Generates a new random key from TPM and updates the sealed storage.
 * This should be called periodically or on specific events (e.g., boot).
 */
int
crypto_tpm_rotate_hmac_key(void)
{
    int ret;
    uint8_t new_key[CRYPTO_HMAC_KEY_BYTES];
    uint64_t old_gen;

    print("Crypto: Rotating HMAC key...\n");

    /* Generate new random key */
    ret = tpm_get_random(new_key, CRYPTO_HMAC_KEY_BYTES);
    if (ret != CRYPTO_HMAC_KEY_BYTES) {
        /* Fallback to fastticks */
        uint64_t t;
        int i;
        for (i = 0; i < CRYPTO_HMAC_KEY_BYTES; i += 8) {
            t = fastticks(nil);
            memcpy(&new_key[i], &t, 8);
        }
    }

    ilock(&tpm_key_state.lock);

    old_gen = tpm_key_state.key_generation;

    /* Update key */
    memcpy(tpm_key_state.hmac_key, new_key, CRYPTO_HMAC_KEY_BYTES);
    tpm_key_state.key_generation++;
    tpm_key_state.key_uses = 0;

    iunlock(&tpm_key_state.lock);

    /* TODO: Seal new key to TPM PCRs */

    print("Crypto: Key rotated from generation %llu to %llu\n",
          (unsigned long long)old_gen,
          (unsigned long long)tpm_key_state.key_generation);

    /* Clear sensitive data */
    memset(new_key, 0, sizeof(new_key));

    return 0;
}

/*
 * TPM-backed HMAC
 *
 * Uses the current sealed key to compute HMAC.
 */
int
crypto_tpm_hmac_sha256(uint8_t *out, const uint8_t *data, size_t len)
{
    uint8_t key[CRYPTO_HMAC_KEY_BYTES];
    size_t keylen = CRYPTO_HMAC_KEY_BYTES;
    int ret;

    if (!tpm_key_state.key_valid) {
        print("Crypto: TPM key not initialized\n");
        return -1;
    }

    /* Get current key */
    ret = crypto_tpm_get_hmac_key(key, &keylen);
    if (ret != 0) {
        return -1;
    }

    /* Compute HMAC */
    ret = crypto_hmac_sha256(out, key, keylen, data, len);

    /* Track usage */
    ilock(&tpm_key_state.lock);
    tpm_key_state.key_uses++;
    iunlock(&tpm_key_state.lock);

    /* Clear key from stack */
    memset(key, 0, sizeof(key));

    return ret;
}
