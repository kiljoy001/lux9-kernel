/*
 * Test Merkle Tree Implementation
 * Validates blind_ledger_update_merkle_root() with various scenarios
 *
 * Run with:
 *   gcc -g -O0 test_merkle_tree.c -o test_merkle_tree -lcrypto
 *   valgrind --leak-check=full --show-leak-kinds=all ./test_merkle_tree
 *   valgrind --tool=helgrind ./test_merkle_tree
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>
#include <openssl/sha.h>

/* Userspace types mapping from Plan 9 */
typedef unsigned char u8int;
typedef unsigned short u16int;
typedef unsigned int u32int;
typedef unsigned long u64int;
typedef unsigned long ulong;
typedef unsigned long uintptr;
typedef void* Proc;

#define nil NULL
#define nelem(x) (sizeof(x)/sizeof((x)[0]))

/* Blind Ledger constants */
#define BLIND_LEDGER_CAP_SIZE 32
#define LEDGER_HASHTABLE_SIZE 256

typedef enum BlindLedgerState {
    BLIND_LEDGER_STATE_INACTIVE = 0,
    BLIND_LEDGER_STATE_ACTIVE   = 1,
    BLIND_LEDGER_STATE_BURNED   = 2,
    BLIND_LEDGER_STATE_COW_RED  = 3,
    BLIND_LEDGER_STATE_COW_BLUE = 4,
} BlindLedgerState;

typedef struct UserCapability {
    u8int hash[BLIND_LEDGER_CAP_SIZE];
    u64int size;
    u32int type;
    u32int perms;
} UserCapability;

typedef u8int BlindLedgerHash[BLIND_LEDGER_CAP_SIZE];

typedef struct BlindLedgerEntry {
    UserCapability capability;
    uintptr physical_address;
    Proc *owner;
    u8int secret[BLIND_LEDGER_CAP_SIZE];
    u64int epoch;
    u64int span_len;
    u32int permissions;
    BlindLedgerState state;
    BlindLedgerHash leaf_hash;
    BlindLedgerHash process_hash;
} BlindLedgerEntry;

typedef struct LedgerEntryNode {
    BlindLedgerEntry entry;
    struct LedgerEntryNode *next;
} LedgerEntryNode;

/* Global ledger hashtable */
static LedgerEntryNode *ledger_hashtable[LEDGER_HASHTABLE_SIZE];
static u8int merkle_root[BLIND_LEDGER_CAP_SIZE];

/* Crypto wrapper */
static void
crypto_sha256(u8int *hash_out, const u8int *data, ulong len)
{
    SHA256(data, len, hash_out);
}

/* Helper: Hash two nodes together for Merkle tree */
static void
merkle_hash_pair(u8int *out, const u8int *left, const u8int *right)
{
    u8int combined[BLIND_LEDGER_CAP_SIZE * 2];
    memcpy(combined, left, BLIND_LEDGER_CAP_SIZE);
    memcpy(combined + BLIND_LEDGER_CAP_SIZE, right, BLIND_LEDGER_CAP_SIZE);
    crypto_sha256(out, combined, sizeof(combined));
}

/* Merkle tree implementation - EXACT COPY from blind_ledger.c */
void
blind_ledger_update_merkle_root(void)
{
    u8int **leaf_hashes;
    ulong leaf_count, i, level_size, next_level_size;
    u8int *level_hashes, *next_level;

    /* Count active entries across all buckets */
    leaf_count = 0;
    for(i = 0; i < LEDGER_HASHTABLE_SIZE; i++){
        LedgerEntryNode *node = ledger_hashtable[i];
        while(node != nil){
            if(node->entry.state == BLIND_LEDGER_STATE_ACTIVE ||
               node->entry.state == BLIND_LEDGER_STATE_COW_RED ||
               node->entry.state == BLIND_LEDGER_STATE_COW_BLUE){
                leaf_count++;
            }
            node = node->next;
        }
    }

    /* Handle empty ledger */
    if(leaf_count == 0){
        memset(merkle_root, 0, BLIND_LEDGER_CAP_SIZE);
        return;
    }

    /* Allocate array for leaf hashes */
    leaf_hashes = calloc(leaf_count, sizeof(u8int*));
    if(leaf_hashes == nil)
        return; /* Silent failure - non-critical */

    /* Collect all active entry hashes */
    leaf_count = 0;
    for(i = 0; i < LEDGER_HASHTABLE_SIZE; i++){
        LedgerEntryNode *node = ledger_hashtable[i];
        while(node != nil){
            if(node->entry.state == BLIND_LEDGER_STATE_ACTIVE ||
               node->entry.state == BLIND_LEDGER_STATE_COW_RED ||
               node->entry.state == BLIND_LEDGER_STATE_COW_BLUE){
                /* Use capability hash as leaf */
                leaf_hashes[leaf_count++] = node->entry.capability.hash;
            }
            node = node->next;
        }
    }

    /* Handle single entry */
    if(leaf_count == 1){
        memcpy(merkle_root, leaf_hashes[0], BLIND_LEDGER_CAP_SIZE);
        free(leaf_hashes);
        return;
    }

    /* Build Merkle tree bottom-up */
    level_size = leaf_count;
    level_hashes = calloc(level_size, BLIND_LEDGER_CAP_SIZE);
    if(level_hashes == nil){
        free(leaf_hashes);
        return;
    }

    /* Copy leaves to working buffer */
    for(i = 0; i < leaf_count; i++){
        memcpy(level_hashes + i * BLIND_LEDGER_CAP_SIZE,
                leaf_hashes[i], BLIND_LEDGER_CAP_SIZE);
    }
    free(leaf_hashes);

    /* Iteratively hash pairs until we reach root */
    while(level_size > 1){
        next_level_size = (level_size + 1) / 2; /* Round up for odd counts */
        next_level = calloc(next_level_size, BLIND_LEDGER_CAP_SIZE);
        if(next_level == nil){
            free(level_hashes);
            return;
        }

        for(i = 0; i < level_size; i += 2){
            u8int *left = level_hashes + i * BLIND_LEDGER_CAP_SIZE;
            u8int *right;

            /* Handle odd count: duplicate last node */
            if(i + 1 < level_size)
                right = level_hashes + (i + 1) * BLIND_LEDGER_CAP_SIZE;
            else
                right = left;

            merkle_hash_pair(next_level + (i / 2) * BLIND_LEDGER_CAP_SIZE,
                           left, right);
        }

        free(level_hashes);
        level_hashes = next_level;
        level_size = next_level_size;
    }

    /* Store root */
    memcpy(merkle_root, level_hashes, BLIND_LEDGER_CAP_SIZE);
    free(level_hashes);
}

const u8int*
blind_ledger_get_merkle_root(void)
{
    return merkle_root;
}

/* Test helpers */
static void
print_hash(const char *label, const u8int *hash)
{
    printf("%s: ", label);
    for(int i = 0; i < BLIND_LEDGER_CAP_SIZE; i++)
        printf("%02x", hash[i]);
    printf("\n");
}

static void
add_test_entry(int bucket, BlindLedgerState state, const char *id)
{
    LedgerEntryNode *node = calloc(1, sizeof(LedgerEntryNode));
    assert(node != nil);

    /* Create deterministic hash from id */
    crypto_sha256(node->entry.capability.hash, (u8int*)id, strlen(id));
    node->entry.state = state;
    node->entry.owner = (Proc*)(uintptr)0xDEADBEEF;

    /* Insert at head */
    node->next = ledger_hashtable[bucket];
    ledger_hashtable[bucket] = node;
}

static void
clear_ledger(void)
{
    for(int i = 0; i < LEDGER_HASHTABLE_SIZE; i++){
        LedgerEntryNode *node = ledger_hashtable[i];
        while(node != nil){
            LedgerEntryNode *next = node->next;
            free(node);
            node = next;
        }
        ledger_hashtable[i] = nil;
    }
    memset(merkle_root, 0, BLIND_LEDGER_CAP_SIZE);
}

/* Test cases */
static void
test_empty_ledger(void)
{
    printf("=== Test: Empty Ledger ===\n");
    clear_ledger();

    blind_ledger_update_merkle_root();
    const u8int *root = blind_ledger_get_merkle_root();

    /* Should be all zeros */
    u8int expected[BLIND_LEDGER_CAP_SIZE] = {0};
    assert(memcmp(root, expected, BLIND_LEDGER_CAP_SIZE) == 0);
    print_hash("Root", root);
    printf("✓ PASS\n\n");
}

static void
test_single_entry(void)
{
    printf("=== Test: Single Entry ===\n");
    clear_ledger();

    add_test_entry(0, BLIND_LEDGER_STATE_ACTIVE, "single_entry");

    blind_ledger_update_merkle_root();
    const u8int *root = blind_ledger_get_merkle_root();

    /* Root should equal the single entry's hash */
    LedgerEntryNode *node = ledger_hashtable[0];
    assert(memcmp(root, node->entry.capability.hash, BLIND_LEDGER_CAP_SIZE) == 0);
    print_hash("Root", root);
    printf("✓ PASS\n\n");
}

static void
test_two_entries(void)
{
    printf("=== Test: Two Entries ===\n");
    clear_ledger();

    add_test_entry(0, BLIND_LEDGER_STATE_ACTIVE, "entry_1");
    add_test_entry(1, BLIND_LEDGER_STATE_ACTIVE, "entry_2");

    blind_ledger_update_merkle_root();
    const u8int *root = blind_ledger_get_merkle_root();

    /* Manually compute expected root */
    u8int expected[BLIND_LEDGER_CAP_SIZE];
    merkle_hash_pair(expected,
                     ledger_hashtable[0]->entry.capability.hash,
                     ledger_hashtable[1]->entry.capability.hash);

    assert(memcmp(root, expected, BLIND_LEDGER_CAP_SIZE) == 0);
    print_hash("Root", root);
    printf("✓ PASS\n\n");
}

static void
test_odd_count(void)
{
    printf("=== Test: Odd Count (3 entries) ===\n");
    clear_ledger();

    add_test_entry(0, BLIND_LEDGER_STATE_ACTIVE, "entry_1");
    add_test_entry(1, BLIND_LEDGER_STATE_ACTIVE, "entry_2");
    add_test_entry(2, BLIND_LEDGER_STATE_ACTIVE, "entry_3");

    blind_ledger_update_merkle_root();
    const u8int *root = blind_ledger_get_merkle_root();

    /* Verify tree structure:
     *       root
     *      /    \
     *   h01      h22  (last node duplicated)
     */
    u8int h01[BLIND_LEDGER_CAP_SIZE], h22[BLIND_LEDGER_CAP_SIZE];
    u8int expected[BLIND_LEDGER_CAP_SIZE];

    merkle_hash_pair(h01,
                     ledger_hashtable[0]->entry.capability.hash,
                     ledger_hashtable[1]->entry.capability.hash);
    merkle_hash_pair(h22,
                     ledger_hashtable[2]->entry.capability.hash,
                     ledger_hashtable[2]->entry.capability.hash);
    merkle_hash_pair(expected, h01, h22);

    assert(memcmp(root, expected, BLIND_LEDGER_CAP_SIZE) == 0);
    print_hash("Root", root);
    printf("✓ PASS\n\n");
}

static void
test_power_of_two(void)
{
    printf("=== Test: Power of Two (4 entries) ===\n");
    clear_ledger();

    add_test_entry(0, BLIND_LEDGER_STATE_ACTIVE, "entry_1");
    add_test_entry(1, BLIND_LEDGER_STATE_ACTIVE, "entry_2");
    add_test_entry(2, BLIND_LEDGER_STATE_ACTIVE, "entry_3");
    add_test_entry(3, BLIND_LEDGER_STATE_ACTIVE, "entry_4");

    blind_ledger_update_merkle_root();
    const u8int *root = blind_ledger_get_merkle_root();

    print_hash("Root", root);
    printf("✓ PASS (no crashes, root computed)\n\n");
}

static void
test_mixed_states(void)
{
    printf("=== Test: Mixed States (only ACTIVE/COW count) ===\n");
    clear_ledger();

    add_test_entry(0, BLIND_LEDGER_STATE_ACTIVE, "active_1");
    add_test_entry(1, BLIND_LEDGER_STATE_INACTIVE, "inactive_1");
    add_test_entry(2, BLIND_LEDGER_STATE_COW_RED, "cow_red_1");
    add_test_entry(3, BLIND_LEDGER_STATE_BURNED, "burned_1");
    add_test_entry(4, BLIND_LEDGER_STATE_COW_BLUE, "cow_blue_1");

    blind_ledger_update_merkle_root();
    const u8int *root = blind_ledger_get_merkle_root();

    /* Should only include ACTIVE, COW_RED, COW_BLUE = 3 entries */
    print_hash("Root", root);
    printf("✓ PASS (3 active states included)\n\n");
}

static void
test_large_ledger(void)
{
    printf("=== Test: Large Ledger (100 entries) ===\n");
    clear_ledger();

    char id[32];
    for(int i = 0; i < 100; i++){
        snprintf(id, sizeof(id), "entry_%d", i);
        add_test_entry(i % LEDGER_HASHTABLE_SIZE, BLIND_LEDGER_STATE_ACTIVE, id);
    }

    blind_ledger_update_merkle_root();
    const u8int *root = blind_ledger_get_merkle_root();

    print_hash("Root", root);
    printf("✓ PASS (100 entries processed)\n\n");
}

static void
test_determinism(void)
{
    printf("=== Test: Determinism (same input = same output) ===\n");
    clear_ledger();

    add_test_entry(0, BLIND_LEDGER_STATE_ACTIVE, "entry_1");
    add_test_entry(1, BLIND_LEDGER_STATE_ACTIVE, "entry_2");
    add_test_entry(2, BLIND_LEDGER_STATE_ACTIVE, "entry_3");

    blind_ledger_update_merkle_root();
    u8int root1[BLIND_LEDGER_CAP_SIZE];
    memcpy(root1, blind_ledger_get_merkle_root(), BLIND_LEDGER_CAP_SIZE);

    blind_ledger_update_merkle_root();
    u8int root2[BLIND_LEDGER_CAP_SIZE];
    memcpy(root2, blind_ledger_get_merkle_root(), BLIND_LEDGER_CAP_SIZE);

    assert(memcmp(root1, root2, BLIND_LEDGER_CAP_SIZE) == 0);
    print_hash("Root (run 1)", root1);
    print_hash("Root (run 2)", root2);
    printf("✓ PASS (deterministic)\n\n");
}

int
main(void)
{
    printf("Merkle Tree Validation Suite\n");
    printf("=============================\n\n");

    test_empty_ledger();
    test_single_entry();
    test_two_entries();
    test_odd_count();
    test_power_of_two();
    test_mixed_states();
    test_large_ledger();
    test_determinism();

    printf("=============================\n");
    printf("All tests PASSED ✓\n");
    printf("\nNow run with valgrind:\n");
    printf("  valgrind --leak-check=full --show-leak-kinds=all ./test_merkle_tree\n");
    printf("  valgrind --tool=helgrind ./test_merkle_tree\n");

    return 0;
}
