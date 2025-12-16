/* Ed25519 crypto wrapper for CryptoAlgorithm interface */
#include "crypto_abstraction.h"
#include "monocypher/monocypher.h"

static int ed25519_sign(CryptoContext *ctx,
                       const uint8_t *message, size_t mlen,
                       const uint8_t *secretkey, size_t sklen,
                       uint8_t *signature, size_t *siglen);
static int ed25519_verify(CryptoContext *ctx,
                         const uint8_t *message, size_t mlen,
                         const uint8_t *publickey, size_t pklen,
                         const uint8_t *signature, size_t siglen);

CryptoAlgorithm ed25519_algorithm = {
    .name = "ed25519",
    .category = CRYPTO_CAT_SIGN,
    .impl = "monocypher",
    .key_bytes = 32,        /* Secret key size */
    .nonce_bytes = 0,
    .output_bytes = 64,     /* Signature size */
    .auth_bytes = 0,
    .init = NULL,
    .update = NULL,
    .final = NULL,
    .cleanup = NULL,
    .aead_encrypt = NULL,
    .aead_decrypt = NULL,
    .sign = ed25519_sign,
    .verify = ed25519_verify,
    .stream_xor = NULL
};

static int
ed25519_sign(CryptoContext *ctx,
            const uint8_t *message, size_t mlen,
            const uint8_t *secretkey, size_t sklen,
            uint8_t *signature, size_t *siglen)
{
    if (!message || !secretkey || !signature || !siglen)
        return -1;
        
    if (sklen != 32 || *siglen < 64)
        return -1;
        
    crypto_sign(signature, secretkey, message, mlen);
    *siglen = 64;
    
    return 0;
}

static int
ed25519_verify(CryptoContext *ctx,
              const uint8_t *message, size_t mlen,
              const uint8_t *publickey, size_t pklen,
              const uint8_t *signature, size_t siglen)
{
    if (!message || !publickey || !signature)
        return -1;
        
    if (pklen != 32 || siglen != 64)
        return -1;
        
    int result = crypto_check(signature, publickey, message, mlen);
    
    return result == 0 ? 0 : -1;  /* 0 = success, -1 = failure */
}

/* Self-test function */
int
ed25519_selftest(void)
{
    uint8_t sk[32];
    uint8_t pk[32];
    uint8_t signature[64];
    size_t siglen = 64;
    const uint8_t test_message[] = "Ed25519 test message";
    
    /* Generate a key pair */
    crypto_sha512(sk, sk, 32);  /* Use SHA-512 to derive a key for testing */
    crypto_sign_public_key(pk, sk);
    
    /* Sign the message */
    if (ed25519_sign(NULL, test_message, sizeof(test_message)-1, sk, 32, signature, &siglen) != 0)
        return -1;
        
    /* Verify the signature */
    if (ed25519_verify(NULL, test_message, sizeof(test_message)-1, pk, 32, signature, siglen) != 0)
        return -1;
        
    /* Test with invalid signature */
    signature[0] ^= 1;  /* Flip a bit */
    if (ed25519_verify(NULL, test_message, sizeof(test_message)-1, pk, 32, signature, siglen) == 0)
        return -1;  /* Should fail */
        
    return 0;
}