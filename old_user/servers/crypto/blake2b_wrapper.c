/* Blake2b crypto wrapper for CryptoAlgorithm interface */
#include "crypto_abstraction.h"
#include "monocypher/monocypher.h"

static int blake2b_init(CryptoContext *ctx);
static int blake2b_update(CryptoContext *ctx, const uint8_t *data, size_t len);
static int blake2b_final(CryptoContext *ctx, uint8_t *output, size_t *outlen);
static void blake2b_cleanup(CryptoContext *ctx);

typedef struct {
    crypto_blake2b_ctx ctx;
    uint8_t pending[BLAKE2B_BLOCKBYTES];
    size_t pending_count;
    size_t digest_size;
} Blake2bContext;

CryptoAlgorithm blake2b_algorithm = {
    .name = "blake2b",
    .category = CRYPTO_CAT_HASH,
    .impl = "monocypher",
    .key_bytes = 0,
    .nonce_bytes = 0,
    .output_bytes = BLAKE2B_OUTBYTES,
    .auth_bytes = 0,
    .init = blake2b_init,
    .update = blake2b_update,
    .final = blake2b_final,
    .cleanup = blake2b_cleanup,
    .aead_encrypt = NULL,
    .aead_decrypt = NULL,
    .sign = NULL,
    .verify = NULL,
    .stream_xor = NULL
};

static int
blake2b_init(CryptoContext *ctx)
{
    Blake2bContext *b2ctx = malloc(sizeof(Blake2bContext));
    if (!b2ctx)
        return -1;
    
    if (ctx->crypto_ctx)
        free(ctx->crypto_ctx);
        
    ctx->crypto_ctx = b2ctx;
    b2ctx->pending_count = 0;
    b2ctx->digest_size = BLAKE2B_OUTBYTES;
    crypto_blake2b_init(&b2ctx->ctx);
    return 0;
}

static int
blake2b_update(CryptoContext *ctx, const uint8_t *data, size_t len)
{
    if (!ctx->crypto_ctx)
        return -1;
        
    Blake2bContext *b2ctx = ctx->crypto_ctx;
    
    /* Handle pending data from previous update */
    if (b2ctx->pending_count > 0) {
        size_t needed = BLAKE2B_BLOCKBYTES - b2ctx->pending_count;
        size_t copy = len > needed ? needed : len;
        
        memcpy(b2ctx->pending + b2ctx->pending_count, data, copy);
        b2ctx->pending_count += copy;
        data += copy;
        len -= copy;
        
        if (b2ctx->pending_count == BLAKE2B_BLOCKBYTES) {
            crypto_blake2b_update(&b2ctx->ctx, b2ctx->pending, BLAKE2B_BLOCKBYTES);
            b2ctx->pending_count = 0;
        }
    }
    
    /* Process full blocks */
    if (len >= BLAKE2B_BLOCKBYTES) {
        size_t blocks = len / BLAKE2B_BLOCKBYTES;
        crypto_blake2b_update(&b2ctx->ctx, data, blocks * BLAKE2B_BLOCKBYTES);
        data += blocks * BLAKE2B_BLOCKBYTES;
        len -= blocks * BLAKE2B_BLOCKBYTES;
    }
    
    /* Store remaining partial block */
    if (len > 0) {
        memcpy(b2ctx->pending, data, len);
        b2ctx->pending_count = len;
    }
    
    return 0;
}

static int
blake2b_final(CryptoContext *ctx, uint8_t *output, size_t *outlen)
{
    if (!ctx->crypto_ctx || !output || !outlen)
        return -1;
        
    Blake2bContext *b2ctx = ctx->crypto_ctx;
    
    /* Process any remaining partial block */
    if (b2ctx->pending_count > 0) {
        crypto_blake2b_update(&b2ctx->ctx, b2ctx->pending, b2ctx->pending_count);
    }
    
    /* Finalize the hash */
    crypto_blake2b_final(&b2ctx->ctx, output);
    *outlen = b2ctx->digest_size;
    
    /* Wipe sensitive data */
    memset(b2ctx, 0, sizeof(*b2ctx));
    
    return 0;
}

static void
blake2b_cleanup(CryptoContext *ctx)
{
    if (ctx->crypto_ctx) {
        Blake2bContext *b2ctx = ctx->crypto_ctx;
        memset(b2ctx, 0, sizeof(*b2ctx));
        free(b2ctx);
        ctx->crypto_ctx = NULL;
    }
}

/* Self-test function */
int
blake2b_selftest(void)
{
    const uint8_t test_input[] = "BLAKE2b test input";
    const uint8_t expected_hash[] = {
        0x41, 0x24, 0x2a, 0x74, 0x2f, 0xe0, 0xbc, 0x41,
        0xf1, 0x6c, 0x58, 0x79, 0x8a, 0x0d, 0x67, 0x91,
        0x9f, 0xa7, 0x29, 0x4b, 0x2e, 0x2d, 0x45, 0xf4,
        0x9f, 0xc3, 0x0a, 0x4d, 0xf4, 0x8f, 0x15, 0xee
    };
    uint8_t hash[BLAKE2B_OUTBYTES];
    size_t hash_len;
    
    CryptoContext ctx = {0};
    
    if (blake2b_init(&ctx) != 0)
        return -1;
        
    if (blake2b_update(&ctx, test_input, sizeof(test_input)-1) != 0) {
        blake2b_cleanup(&ctx);
        return -1;
    }
    
    if (blake2b_final(&ctx, hash, &hash_len) != 0) {
        blake2b_cleanup(&ctx);
        return -1;
    }
    
    blake2b_cleanup(&ctx);
    
    if (memcmp(hash, expected_hash, sizeof(expected_hash)) != 0)
        return -1;
        
    return 0;
}