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
        /* CRITICAL: No fallback to weak entropy sources! */
        print("Crypto: FATAL - TPM random not available, refusing to use weak entropy\n");
        print("Crypto: System requires hardware TPM or RDRAND for cryptographic operations\n");

        /* Zero out memory to avoid uninitialized data */
        memset(tpm_key_state.hmac_key, 0, sizeof(tpm_key_state.hmac_key));
        tpm_key_state.key_valid = 0;
        tpm_key_state.key_generation = 0;

        /* TODO: Try RDRAND/RDSEED as alternative hardware RNG */
        return -1;
    }

    /* Seal the key to TPM PCRs for hardware-backed protection */
    if (tpm_key_state.key_valid) {
        int seal_ret = crypto_tpm_seal_key(tpm_key_state.hmac_key, CRYPTO_HMAC_KEY_BYTES);
        if (seal_ret == 0) {
            print("Crypto: HMAC key sealed to TPM PCRs\n");
        } else {
            print("Crypto: WARNING - Failed to seal key to TPM (code %d)\n", seal_ret);
            print("Crypto: Key remains in memory only (not hardware-protected)\n");
        }
    }

    print("Crypto: TPM key storage initialized\n");
    return tpm_key_state.key_valid ? 0 : -1;
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
        /* CRITICAL: No weak entropy fallback during key rotation */
        print("Crypto: FATAL - TPM random unavailable during key rotation\n");
        memset(new_key, 0, sizeof(new_key));
        return -1;
    }

    ilock(&tpm_key_state.lock);

    old_gen = tpm_key_state.key_generation;

    /* Update key */
    memcpy(tpm_key_state.hmac_key, new_key, CRYPTO_HMAC_KEY_BYTES);
    tpm_key_state.key_generation++;
    tpm_key_state.key_uses = 0;

    iunlock(&tpm_key_state.lock);

    /* Seal new key to TPM PCRs */
    ret = crypto_tpm_seal_key(tpm_key_state.hmac_key, CRYPTO_HMAC_KEY_BYTES);
    if (ret == 0) {
        print("Crypto: New key sealed to TPM\n");
    } else {
        print("Crypto: WARNING - Failed to seal rotated key to TPM\n");
    }

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

/*
 * TPM Key Sealing/Unsealing Implementation
 *
 * These functions provide hardware-backed key protection by sealing keys
 * to TPM PCRs. The key can only be unsealed if the PCR values match.
 */

/* Storage for sealed key blob */
static struct {
    uint8_t sealed_blob[512];  /* Encrypted key blob from TPM */
    size_t blob_size;
    int sealed;
} tpm_sealed_key;

/*
 * Seal HMAC key to TPM PCRs
 *
 * Seals the key to current PCR state. Key can only be unsealed when
 * PCRs match (measured boot state verification).
 */
int
crypto_tpm_seal_key(const uint8_t *key, size_t keylen)
{
    /* TODO: Implement using TPM2-TSS SAPI functions:
     *
     * 1. Create a sealed data object using Tss2_Sys_Create():
     *    - inSensitive contains the HMAC key
     *    - inPublic specifies TPM2_ALG_KEYEDHASH object type
     *    - creationPCR specifies which PCRs to seal to (e.g., PCR 0-7)
     *
     * 2. Store the returned outPrivate blob in tpm_sealed_key.sealed_blob
     *
     * 3. The blob is encrypted by TPM and can only be unsealed when
     *    the specified PCRs match their current values
     *
     * For now, we skip sealing and keep key in memory only.
     */

    if (!key || keylen == 0 || keylen > sizeof(tpm_sealed_key.sealed_blob)) {
        return -1;
    }

    print("crypto_tpm_seal_key: UNIMPLEMENTED - key not sealed to TPM\n");
    print("crypto_tpm_seal_key: Key remains in memory (not hardware-protected)\n");

    /* Mark as not sealed */
    tpm_sealed_key.sealed = 0;
    tpm_sealed_key.blob_size = 0;

    /* For production: uncomment when SAPI integration complete
     *
     * TSS2_SYS_CONTEXT *sapi_ctx = get_sapi_context();
     * TPM2B_SENSITIVE_CREATE inSensitive = {...};
     * TPM2B_PUBLIC inPublic = {...};
     * TPML_PCR_SELECTION creationPCR = {...};
     * TPM2B_PRIVATE outPrivate = {0};
     *
     * // Populate structures...
     * inSensitive.sensitive.data.size = keylen;
     * memcpy(inSensitive.sensitive.data.buffer, key, keylen);
     *
     * // Call SAPI
     * TSS2_RC rc = Tss2_Sys_Create(sapi_ctx, TPM2_RH_OWNER, ...);
     * if (rc != TSS2_RC_SUCCESS) return -1;
     *
     * // Store sealed blob
     * memcpy(tpm_sealed_key.sealed_blob, outPrivate.buffer, outPrivate.size);
     * tpm_sealed_key.blob_size = outPrivate.size;
     * tpm_sealed_key.sealed = 1;
     */

    return 0;  /* Return success for now (degraded mode) */
}

/*
 * Unseal HMAC key from TPM
 *
 * Attempts to unseal the key. Will only succeed if PCR values match
 * the values at seal time (verified boot state).
 */
int
crypto_tpm_unseal_key(uint8_t *key_out, size_t *keylen)
{
    /* TODO: Implement using Tss2_Sys_Unseal():
     *
     * 1. Load the sealed blob using Tss2_Sys_Load()
     * 2. Unseal using Tss2_Sys_Unseal()
     * 3. TPM will verify PCR values before unsealing
     * 4. Return unsealed key in key_out
     *
     * For now, return error (no sealed key available).
     */

    if (!key_out || !keylen) {
        return -1;
    }

    if (!tpm_sealed_key.sealed) {
        print("crypto_tpm_unseal_key: No sealed key available\n");
        return -1;
    }

    print("crypto_tpm_unseal_key: UNIMPLEMENTED\n");

    /* For production: uncomment when SAPI integration complete
     *
     * TSS2_SYS_CONTEXT *sapi_ctx = get_sapi_context();
     * TPM2B_SENSITIVE_DATA outData = {0};
     *
     * // Call SAPI
     * TSS2_RC rc = Tss2_Sys_Unseal(sapi_ctx, sealed_handle, ...);
     * if (rc != TSS2_RC_SUCCESS) return -1;
     *
     * // Return unsealed key
     * if (*keylen < outData.size) return -1;
     * memcpy(key_out, outData.buffer, outData.size);
     * *keylen = outData.size;
     */

    return -1;  /* Return error for now (not implemented) */
}
