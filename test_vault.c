// Test vault implementation with Monocypher
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

// Include Monocypher from archive
#include "monocypher-4.0.2/src/monocypher.h"

#define VAULT_SIZE (10*1024*1024)  // 10MB test

typedef struct {
    uint8_t *data;
    size_t size;
    int locked;
    uint8_t master_key[32];
    uint8_t salt[16];
    uint8_t nonce[24];
    int initialized;
} TestVault;

static TestVault vault;

// Derive key from password using Argon2id
static int derive_key(const char *password, size_t passlen, uint8_t *salt, uint8_t *key_out) {
    crypto_argon2_config config;
    crypto_argon2_inputs inputs;
    void *work_area;
    size_t work_size;

    config.algorithm = CRYPTO_ARGON2_ID;
    config.nb_blocks = 4096;  // 4MB
    config.nb_passes = 3;
    config.nb_lanes = 1;

    work_size = config.nb_blocks * 1024;
    work_area = malloc(work_size);
    if(!work_area)
        return -1;

    inputs.pass = (uint8_t*)password;
    inputs.pass_size = passlen;
    inputs.salt = salt;
    inputs.salt_size = 16;

    crypto_argon2(key_out, 32, work_area, config, inputs, crypto_argon2_no_extras);

    crypto_wipe(work_area, work_size);
    free(work_area);
    return 0;
}

int main(void) {
    const char *password = "test_password_123";
    uint8_t test_data[] = "Secret vault contents that need protection!";
    size_t data_len = sizeof(test_data);
    uint8_t recovered[100];

    printf("=== Secure Vault Test with Monocypher ===\n\n");

    // 1. Initialize vault
    printf("1. Initializing vault (%d MB)...\n", VAULT_SIZE/(1024*1024));
    vault.data = calloc(1, VAULT_SIZE);
    vault.size = VAULT_SIZE;
    vault.locked = 1;
    vault.initialized = 0;

    // Generate random salt and nonce
    for(int i = 0; i < 16; i++)
        vault.salt[i] = rand() & 0xFF;
    for(int i = 0; i < 24; i++)
        vault.nonce[i] = rand() & 0xFF;

    printf("   ✓ Vault allocated\n");

    // 2. Initialize with password
    printf("\n2. Initializing with password...\n");
    if(derive_key(password, strlen(password), vault.salt, vault.master_key) < 0) {
        printf("   ✗ Key derivation failed\n");
        return 1;
    }
    vault.initialized = 1;
    vault.locked = 0;
    printf("   ✓ Password set, vault unlocked\n");

    // 3. Write test data
    printf("\n3. Writing test data to vault...\n");
    memcpy(vault.data, test_data, data_len);
    printf("   Data written: '%s'\n", vault.data);

    // 4. Lock vault (encrypt)
    printf("\n4. Locking vault (encrypting)...\n");
    crypto_chacha20_x(vault.data, vault.data, vault.size,
                      vault.master_key, vault.nonce, 0);
    vault.locked = 1;
    printf("   ✓ Vault locked\n");
    printf("   Encrypted data (first 44 bytes): ");
    for(int i = 0; i < 44; i++)
        printf("%02x", vault.data[i]);
    printf("\n");

    // 5. Unlock vault (decrypt)
    printf("\n5. Unlocking vault with password...\n");
    uint8_t derived_key[32];
    if(derive_key(password, strlen(password), vault.salt, derived_key) < 0) {
        printf("   ✗ Key derivation failed\n");
        return 1;
    }

    if(crypto_verify32(derived_key, vault.master_key) != 0) {
        printf("   ✗ Incorrect password\n");
        return 1;
    }

    crypto_chacha20_x(vault.data, vault.data, vault.size,
                      vault.master_key, vault.nonce, 0);
    vault.locked = 0;
    printf("   ✓ Vault unlocked\n");

    // 6. Verify data
    printf("\n6. Verifying decrypted data...\n");
    memcpy(recovered, vault.data, data_len);
    if(memcmp(recovered, test_data, data_len) == 0) {
        printf("   ✓ Data matches: '%s'\n", recovered);
    } else {
        printf("   ✗ Data corruption!\n");
        return 1;
    }

    // 7. Test wrong password
    printf("\n7. Testing wrong password...\n");
    const char *wrong_pw = "wrong_password";
    if(derive_key(wrong_pw, strlen(wrong_pw), vault.salt, derived_key) < 0) {
        printf("   ✗ Key derivation failed\n");
        return 1;
    }

    if(crypto_verify32(derived_key, vault.master_key) != 0) {
        printf("   ✓ Wrong password correctly rejected\n");
    } else {
        printf("   ✗ Wrong password accepted (security failure!)\n");
        return 1;
    }

    // 8. Cleanup
    printf("\n8. Wiping vault...\n");
    crypto_wipe(vault.data, vault.size);
    crypto_wipe(vault.master_key, 32);
    free(vault.data);
    printf("   ✓ Vault wiped\n");

    printf("\n=== All tests passed! ===\n");
    return 0;
}
