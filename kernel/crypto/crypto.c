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
extern uint64_t rdrand_u64(void);

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
 * Check if hardware RDRAND is available
 */
int
crypto_hw_rdrand_available(void)
{
    /* Access current CPU's Mach structure */
    extern Mach *m;
    return m->haverdrand;
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
        /* TPM unavailable - try RDRAND as hardware RNG fallback */
        print("Crypto: TPM random not available\n");

        if (crypto_hw_rdrand_available()) {
            print("Crypto: Falling back to RDRAND hardware RNG\n");
            int i;
            uint64_t *key_u64 = (uint64_t*)random_key;
            int success = 1;

            for (i = 0; i < CRYPTO_HMAC_KEY_BYTES / 8; i++) {
                key_u64[i] = rdrand_u64();
                if (key_u64[i] == 0) {
                    print("Crypto: RDRAND failed at byte %d\n", i * 8);
                    success = 0;
                    break;
                }
            }

            if (success) {
                memcpy(tpm_key_state.hmac_key, random_key, CRYPTO_HMAC_KEY_BYTES);
                tpm_key_state.key_valid = 1;
                tpm_key_state.key_generation = 1;
                print("Crypto: Generated RDRAND key (generation 1)\n");
            } else {
                print("Crypto: FATAL - RDRAND failed\n");
                memset(tpm_key_state.hmac_key, 0, sizeof(tpm_key_state.hmac_key));
                tpm_key_state.key_valid = 0;
                tpm_key_state.key_generation = 0;
                return -1;
            }
        } else {
            /* CRITICAL: No hardware RNG available! */
            print("Crypto: FATAL - No hardware RNG available (TPM or RDRAND)\n");
            print("Crypto: System requires hardware TPM or RDRAND for cryptographic operations\n");

            /* Zero out memory to avoid uninitialized data */
            memset(tpm_key_state.hmac_key, 0, sizeof(tpm_key_state.hmac_key));
            tpm_key_state.key_valid = 0;
            tpm_key_state.key_generation = 0;
            return -1;
        }
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

    /* Generate new random key from TPM */
    ret = tpm_get_random(new_key, CRYPTO_HMAC_KEY_BYTES);
    if (ret != CRYPTO_HMAC_KEY_BYTES) {
        /* TPM unavailable - try RDRAND as fallback */
        print("Crypto: TPM random unavailable during key rotation\n");

        if (crypto_hw_rdrand_available()) {
            print("Crypto: Falling back to RDRAND for key rotation\n");
            int i;
            uint64_t *key_u64 = (uint64_t*)new_key;
            int success = 1;

            for (i = 0; i < CRYPTO_HMAC_KEY_BYTES / 8; i++) {
                key_u64[i] = rdrand_u64();
                if (key_u64[i] == 0) {
                    print("Crypto: RDRAND failed during rotation at byte %d\n", i * 8);
                    success = 0;
                    break;
                }
            }

            if (!success) {
                print("Crypto: FATAL - RDRAND failed during key rotation\n");
                memset(new_key, 0, sizeof(new_key));
                return -1;
            }
        } else {
            /* CRITICAL: No hardware RNG available */
            print("Crypto: FATAL - No hardware RNG available for key rotation\n");
            memset(new_key, 0, sizeof(new_key));
            return -1;
        }
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

/* External TPM2 SAPI functions */
extern int tpm2_create_primary(u32int *handle_out);
extern int tpm2_create(u32int parent_handle, u8int *data, u16int data_len,
                       u8int *private_out, u16int *private_len,
                       u8int *public_out, u16int *public_len);
extern int tpm2_load(u32int parent_handle, u8int *private_blob, u16int private_len,
                     u8int *public_blob, u16int public_len, u32int *handle_out);
extern int tpm2_unseal(u32int item_handle, u8int *data_out, u16int *data_len);

/* Storage for sealed key blob */
static struct {
    uint8_t private_blob[256];  /* Encrypted private blob from TPM */
    uint16_t private_len;
    uint8_t public_blob[256];   /* Public blob */
    uint16_t public_len;
    uint32_t srk_handle;  /* Storage Root Key handle */
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
    int ret;

    if (!key || keylen == 0 || keylen > 128) {
        print("crypto_tpm_seal_key: invalid parameters (keylen=%lu)\n", keylen);
        return -1;
    }

    /* Create Storage Root Key if not already created */
    if (tpm_sealed_key.srk_handle == 0) {
        ret = tpm2_create_primary(&tpm_sealed_key.srk_handle);
        if (ret < 0) {
            print("crypto_tpm_seal_key: failed to create SRK\n");
            return -1;
        }
    }

    /* Seal the key using TPM2_Create */
    ret = tpm2_create(tpm_sealed_key.srk_handle,
                      (u8int*)key, (u16int)keylen,
                      tpm_sealed_key.private_blob, &tpm_sealed_key.private_len,
                      tpm_sealed_key.public_blob, &tpm_sealed_key.public_len);

    if (ret < 0) {
        print("crypto_tpm_seal_key: TPM2_Create failed\n");
        tpm_sealed_key.sealed = 0;
        return -1;
    }

    tpm_sealed_key.sealed = 1;
    print("crypto_tpm_seal_key: Successfully sealed %lu byte key to TPM\n", keylen);

    return 0;
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
    int ret;
    u32int obj_handle;
    u16int unsealed_len;

    if (!key_out || !keylen) {
        return -1;
    }

    if (!tpm_sealed_key.sealed) {
        print("crypto_tpm_unseal_key: No sealed key available\n");
        return -1;
    }

    /* Load the sealed object */
    ret = tpm2_load(tpm_sealed_key.srk_handle,
                    tpm_sealed_key.private_blob, tpm_sealed_key.private_len,
                    tpm_sealed_key.public_blob, tpm_sealed_key.public_len,
                    &obj_handle);

    if (ret < 0) {
        print("crypto_tpm_unseal_key: TPM2_Load failed\n");
        return -1;
    }

    /* Unseal the data */
    ret = tpm2_unseal(obj_handle, (u8int*)key_out, &unsealed_len);

    if (ret < 0) {
        print("crypto_tpm_unseal_key: TPM2_Unseal failed\n");
        return -1;
    }

    *keylen = unsealed_len;
    print("crypto_tpm_unseal_key: Successfully unsealed %d byte key\n", unsealed_len);

    return 0;
}
