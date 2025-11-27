/*
 * Family - Pebble Integration
 *
 * Integrates the Pebble capability system with the Family device abstraction.
 * Provides secure resource management for device families.
 */

#include "u.h"
#include "portlib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "family/family.h"
#include "pebble.h"
#include "error.h"

/* Maximum tokens per family type */
#define MAX_FAMILY_TOKENS    64

/* Family pebble token tracking */
typedef struct FamilyPebbleToken {
    PebbleWhite* white_token;       /* White capability token */
    void* black_handle;             /* Associated black handle */
    uint64_t channel_id;            /* Channel this token is for */
    uint32_t permissions;           /* Access permissions */
    uint64_t creation_time;         /* When token was created */
    int active;                     /* Is this token active? */
} FamilyPebbleToken;

/* Per-family pebble state */
typedef struct FamilyPebbleState {
    FamilyPebbleToken tokens[MAX_FAMILY_TOKENS];
    int token_count;
    uint64_t total_tokens_created;
    uint64_t total_tokens_freed;
    Lock state_lock;
} FamilyPebbleState;

/* Global family pebble states - one per family type */
static FamilyPebbleState family_pebble_states[FAMILY_MAX];
static int family_pebble_initialized = 0;

/*
 * Initialize pebble support for a family
 */
int
family_pebble_init(struct FamilyExchangePage* family)
{
    FamilyPebbleState* state;
    int i;

    if (!family) {
        return FAMILY_EINVAL;
    }

    /* Initialize global state on first call */
    if (!family_pebble_initialized) {
        for (i = 0; i < FAMILY_MAX; i++) {
            memset(&family_pebble_states[i], 0, sizeof(FamilyPebbleState));
        }
        family_pebble_initialized = 1;
        print("Pebble-Family: Initialized global pebble-family integration\n");
    }

    /* Initialize this family's pebble state */
    if (family->family_type >= FAMILY_MAX) {
        return FAMILY_EINVAL;
    }

    state = &family_pebble_states[family->family_type];
    memset(state, 0, sizeof(FamilyPebbleState));

    /* Create family-wide white token */
    if (pebble_enabled) {
        PebbleState* ps = pebble_state();
        if (ps) {
            family->family_white_token = (struct PebbleHandle*)pebble_issue_white(ps, nil, PEBBLE_DEFAULT_BUDGET);
            if (family->family_white_token) {
                print("Pebble-Family: Created white token for family %s\n", family->family_name);
            }
        }
    }

    print("Pebble-Family: Initialized family %s\n", family->family_name);
    return FAMILY_OK;
}

/*
 * Cleanup pebble support for a family
 */
void
family_pebble_cleanup(struct FamilyExchangePage* family)
{
    FamilyPebbleState* state;
    int i;

    if (!family || family->family_type >= FAMILY_MAX) {
        return;
    }

    state = &family_pebble_states[family->family_type];

    /* Free all active tokens */
    for (i = 0; i < MAX_FAMILY_TOKENS; i++) {
        if (state->tokens[i].active && state->tokens[i].black_handle) {
            pebble_black_free(state->tokens[i].black_handle);
            state->tokens[i].active = 0;
        }
    }

    memset(state, 0, sizeof(FamilyPebbleState));
    print("Pebble-Family: Cleaned up family %s\n", family->family_name);
}

/*
 * Allocate a pebble-protected resource for a channel
 */
int
family_pebble_allocate_resource(struct FamilyExchangePage* family, uint64_t channel_id,
                                uint32_t permissions, size_t resource_size, void** resource_handle)
{
    FamilyPebbleState* state;
    FamilyPebbleToken* token;
    void* black_handle;
    int i, free_slot;

    if (!family || family->family_type >= FAMILY_MAX || !resource_handle) {
        return FAMILY_EINVAL;
    }

    if (!pebble_enabled) {
        /* Pebbles disabled - just allocate directly */
        *resource_handle = malloc(resource_size);
        return *resource_handle ? FAMILY_OK : FAMILY_ENOMEM;
    }

    state = &family_pebble_states[family->family_type];

    /* Find free token slot */
    free_slot = -1;
    for (i = 0; i < MAX_FAMILY_TOKENS; i++) {
        if (!state->tokens[i].active) {
            free_slot = i;
            break;
        }
    }

    if (free_slot < 0) {
        print("Pebble-Family: No free token slots for family %s\n", family->family_name);
        return FAMILY_EBUSY;
    }

    /* Allocate black pebble resource */
    if (pebble_black_alloc(resource_size, &black_handle) != 0) {
        return FAMILY_ENOMEM;
    }

    /* Set up token */
    token = &state->tokens[free_slot];
    token->black_handle = black_handle;
    token->channel_id = channel_id;
    token->permissions = permissions;
    token->creation_time = fastticks(nil);
    token->active = 1;
    token->white_token = nil;  /* Could create white token here if needed */

    state->token_count++;
    state->total_tokens_created++;

    *resource_handle = black_handle;

    print("Pebble-Family: Allocated pebble resource for channel 0x%llx (size=%lu)\n",
          (unsigned long long)channel_id, (unsigned long)resource_size);

    return FAMILY_OK;
}

/*
 * Free a pebble-protected resource
 */
int
family_pebble_free_resource(struct FamilyExchangePage* family, void* resource_handle)
{
    FamilyPebbleState* state;
    int i;

    if (!family || family->family_type >= FAMILY_MAX || !resource_handle) {
        return FAMILY_EINVAL;
    }

    if (!pebble_enabled) {
        free(resource_handle);
        return FAMILY_OK;
    }

    state = &family_pebble_states[family->family_type];

    /* Find and free token */
    for (i = 0; i < MAX_FAMILY_TOKENS; i++) {
        if (state->tokens[i].active && state->tokens[i].black_handle == resource_handle) {
            pebble_black_free(state->tokens[i].black_handle);
            state->tokens[i].active = 0;
            state->tokens[i].black_handle = nil;
            state->token_count--;
            state->total_tokens_freed++;

            print("Pebble-Family: Freed pebble resource\n");
            return FAMILY_OK;
        }
    }

    print("Pebble-Family: Resource handle not found\n");
    return FAMILY_ENOTFOUND;
}

/*
 * Validate access to a pebble-protected resource
 */
int
family_pebble_validate_access(struct FamilyExchangePage* family, void* resource_handle,
                              uint32_t requested_permissions)
{
    FamilyPebbleState* state;
    int i;

    if (!family || family->family_type >= FAMILY_MAX || !resource_handle) {
        return FAMILY_EINVAL;
    }

    if (!pebble_enabled) {
        return FAMILY_OK;  /* No pebble protection - allow access */
    }

    state = &family_pebble_states[family->family_type];

    /* Find token and check permissions */
    for (i = 0; i < MAX_FAMILY_TOKENS; i++) {
        if (state->tokens[i].active && state->tokens[i].black_handle == resource_handle) {
            if ((state->tokens[i].permissions & requested_permissions) == requested_permissions) {
                return FAMILY_OK;
            } else {
                return FAMILY_EPERM;
            }
        }
    }

    return FAMILY_ENOTFOUND;
}

/*
 * Get statistics for family pebble usage
 */
void
family_pebble_stats(struct FamilyExchangePage* family)
{
    FamilyPebbleState* state;

    if (!family || family->family_type >= FAMILY_MAX) {
        return;
    }

    state = &family_pebble_states[family->family_type];

    print("Pebble-Family Statistics for %s:\n", family->family_name);
    print("  Active tokens: %d\n", state->token_count);
    print("  Total created: %llu\n", (unsigned long long)state->total_tokens_created);
    print("  Total freed: %llu\n", (unsigned long long)state->total_tokens_freed);
}
