/*
 * TPM Secure Element Family Implementation
 *
 * This implements the SecureElement family interface for TPM devices.
 * Future implementations can add Arm TrustZone, RISC-V PMP, Intel SGX, etc.
 */

#include "crypto.h"
#include "dat.h"
#include "error.h"
#include "fns.h"
#include "mem.h"
#include "portlib.h"
#include "u.h"

#include "../include/family/family.h"
#include "../include/tpm.h"

/* TPM-specific channel structure */
typedef struct TPMChannel TPMChannel;
struct TPMChannel {
  uint64_t channel_id;
  uint32_t permissions;
  uint32_t capabilities;
  TPMContext *tpm_ctx;
  void *channel_specific_data;
};

/* TPM device info structure */
typedef struct TPMDeviceInfo TPMDeviceInfo;
struct TPMDeviceInfo {
  uint32_t device_type;
  uint32_t version;
  uint32_t capabilities;
  int available;
  int hardware_backed;
  int channel_count;
  char description[128];
};

#define DEVICE_TPM 1

/* TPM-specific family context */
typedef struct TPMFamilyContext TPMFamilyContext;
struct TPMFamilyContext {
  TPMContext *tpm_ctx;       /* Hardware TPM context */
  uint32_t detected_version; /* TPM 1.2 or TPM 2.0 */
  uintptr_t tpm_base_addr;   /* TPM hardware base address */
  int tpm_available;         /* Hardware detection status */

  /* Capability flags for this specific TPM */
  uint32_t capabilities;

  /* Statistics */
  uint64_t total_commands_sent;
  uint64_t successful_commands;
  uint64_t failed_commands;

  /* Channel management for TPM operations */
  struct {
    uint64_t next_channel_id;
    void *channels[64]; /* Simple array for now */
    int channel_count;
  } channel_mgr;
};

/* Forward declarations */
static int tpm_family_init(struct FamilyExchangePage *family);
static int tpm_family_shutdown(struct FamilyExchangePage *family);
static int tpm_family_scan_devices(struct FamilyExchangePage *family);
static int tpm_family_allocate_channel(struct FamilyExchangePage *family,
                                       void *device_id, uint32_t permissions,
                                       uint64_t *channel_id);
static int tpm_family_release_channel(struct FamilyExchangePage *family,
                                      uint64_t channel_id);
static int tpm_family_device_get_info(struct FamilyExchangePage *family,
                                      void *device, void *info_buffer,
                                      size_t *info_size);
static uint32_t tpm_family_get_capabilities(struct FamilyExchangePage *family);
static int tpm_family_begin_transaction(struct FamilyExchangePage *family,
                                        uint64_t *tx_id);
static int tpm_family_commit_transaction(struct FamilyExchangePage *family,
                                         uint64_t tx_id);

/* TPM Family Operations Structure */
static struct FamilyOps tpm_family_ops = {
    .family_init = tpm_family_init,
    .family_shutdown = tpm_family_shutdown,
    .scan_devices = tpm_family_scan_devices,
    .allocate_channel = tpm_family_allocate_channel,
    .release_channel = tpm_family_release_channel,
    .device_get_info = tpm_family_device_get_info,
    .get_capabilities = tpm_family_get_capabilities,
    .begin_transaction = tpm_family_begin_transaction,
    .commit_transaction = tpm_family_commit_transaction,
};

/*
 * Initialize TPM Secure Element Family
 */
/*@
  @ requires family == \null || \valid(family);
  @ assigns family->family_specific_ctx;
  @*/static int tpm_family_init(struct FamilyExchangePage *family) {
  TPMFamilyContext *ctx;
  int ret;

  print("TPM Family: Initializing TPM Secure Element...\n");

  /* Allocate family context */
  ctx = xalloc_driver(sizeof(TPMFamilyContext));
  if (ctx == nil) {
    print("TPM Family: Failed to allocate context\n");
    return FAMILY_ENOMEM;
  }

  memset(ctx, 0, sizeof(TPMFamilyContext));
  family->family_specific_ctx = ctx;

  /* Try to initialize TPM hardware */
  ret = tpm_init(); /* This calls our kernel TPM driver */
  if (ret < 0) {
    print("TPM Family: Hardware initialization failed, using software "
          "fallback\n");
    ctx->tpm_available = 0;
    ctx->tpm_ctx = nil;
  } else {
    print("TPM Family: TPM hardware initialized successfully\n");
    ctx->tpm_available = 1;

    /* Allocate TPM context */
    ctx->tpm_ctx = xalloc_driver(sizeof(TPMContext));
    if (ctx->tpm_ctx == nil) {
      print("TPM Family: Failed to allocate TPM context\n");
      ctx->tpm_available = 0;
    } else {
      memset(ctx->tpm_ctx, 0, sizeof(TPMContext));
      ctx->tpm_ctx->hardware_available = 1;
    }
  }

  /* Determine TPM version and capabilities */
  if (ctx->tpm_available && ctx->tpm_ctx) {
    ctx->detected_version = ctx->tpm_ctx->version;
    ctx->capabilities = FAMILY_CAP_SECURE_STORAGE | FAMILY_CAP_ATTESTATION |
                        FAMILY_CAP_CRYPTO | FAMILY_CAP_RANDOM;
    print("TPM Family: Detected TPM version %d\n", ctx->detected_version);
  } else {
    /* Software fallback capabilities */
    ctx->detected_version = TPM_VERSION_2_0; /* Assume 2.0 for software */
    ctx->capabilities = FAMILY_CAP_SECURE_STORAGE | FAMILY_CAP_CRYPTO;
    print("TPM Family: Using software capabilities\n");
  }

  /* Initialize channel manager */
  ctx->channel_mgr.next_channel_id =
      0x9000000000000000ULL; /* TPM family channel ID prefix */
  ctx->channel_mgr.channel_count = 0;

  family->family_version = 1;
  family->state = FAMILY_READY;

  /* Initialize TPM-backed crypto key storage */
  crypto_tpm_key_init();

  /* Rotate HMAC key on boot for fresh key each boot */
  crypto_tpm_rotate_hmac_key();

  print("TPM Family: Initialization complete\n");
  return FAMILY_OK;
}

/*
 * Scan for TPM devices
 */
/*@
  @ requires family == \null || \valid(family);
  @ assigns *ctx;
  @*/static int tpm_family_scan_devices(struct FamilyExchangePage *family) {
  TPMFamilyContext *ctx = family->family_specific_ctx;

  print("TPM Family: Scanning for TPM devices...\n");

  if (ctx->tpm_available) {
    print("TPM Family: TPM hardware detected and ready\n");
    return 1; /* One device found */
  } else {
    print("TPM Family: No TPM hardware detected\n");
    return 0; /* No devices found */
  }
}

/*
 * Allocate channel for TPM operations
 */
static int tpm_family_allocate_channel(struct FamilyExchangePage *family,
                                       void *device_id, uint32_t permissions,
                                       uint64_t *channel_id) {
  TPMFamilyContext *ctx = family->family_specific_ctx;
  TPMChannel *tpm_channel;

  if (ctx->channel_mgr.channel_count >= 64) {
    print("TPM Family: Channel limit reached\n");
    return FAMILY_ECHANNEL_EXHAUSTED;
  }

  /* Validate permissions */
  if (permissions & CHANNEL_PERM_ADMIN) {
    /* Check if caller has admin rights */
    /* For now, allow all */
  }

  /* Allocate TPM channel structure */
  tpm_channel = xalloc_driver(sizeof(TPMChannel));
  if (tpm_channel == nil) {
    return FAMILY_ENOMEM;
  }

  memset(tpm_channel, 0, sizeof(TPMChannel));
  tpm_channel->permissions = permissions;
  tpm_channel->tpm_ctx = ctx->tpm_ctx;
  tpm_channel->capabilities = ctx->capabilities;

  /* Generate channel ID */
  ctx->channel_mgr.next_channel_id++;
  tpm_channel->channel_id = ctx->channel_mgr.next_channel_id;
  ctx->channel_mgr.channels[ctx->channel_mgr.channel_count] = tpm_channel;
  ctx->channel_mgr.channel_count++;

  *channel_id = tpm_channel->channel_id;

  family->stats.active_channels++;
  family->stats.total_channels_created++;

  print("TPM Family: Allocated channel 0x%llx\n",
        (unsigned long long)*channel_id);
  return FAMILY_OK;
}

/*
 * Release TPM channel
 */
static int tpm_family_release_channel(struct FamilyExchangePage *family,
                                      uint64_t channel_id) {
  TPMFamilyContext *ctx = family->family_specific_ctx;
  int i;

  /* Find and remove channel */
  for (i = 0; i < ctx->channel_mgr.channel_count; i++) {
    TPMChannel *channel = ctx->channel_mgr.channels[i];
    if (channel->channel_id == channel_id) {
      xfree_driver(channel);
      ctx->channel_mgr.channels[i] =
          ctx->channel_mgr.channels[ctx->channel_mgr.channel_count - 1];
      ctx->channel_mgr.channel_count--;

      family->stats.active_channels--;

      print("TPM Family: Released channel 0x%llx\n",
            (unsigned long long)channel_id);
      return FAMILY_OK;
    }
  }

  print("TPM Family: Channel 0x%llx not found\n",
        (unsigned long long)channel_id);
  return FAMILY_ENOTFOUND;
}

/*
 * Get TPM device information
 */
static int tpm_family_device_get_info(struct FamilyExchangePage *family,
                                      void *device, void *info_buffer,
                                      size_t *info_size) {
  TPMFamilyContext *ctx = family->family_specific_ctx;
  TPMDeviceInfo *info = (TPMDeviceInfo *)info_buffer;

  if (*info_size < sizeof(TPMDeviceInfo)) {
    *info_size = sizeof(TPMDeviceInfo);
    return FAMILY_EINVAL;
  }

  /* Populate device info */
  memset(info, 0, sizeof(TPMDeviceInfo));
  info->device_type = DEVICE_TPM;
  info->version = ctx->detected_version;
  info->available = ctx->tpm_available;
  info->capabilities = ctx->capabilities;
  info->channel_count = ctx->channel_mgr.channel_count;

  if (ctx->tpm_ctx) {
    info->hardware_backed = ctx->tpm_ctx->hardware_available;
    strncpy(info->description, "TPM 2.0/1.2 Secure Element",
            sizeof(info->description) - 1);
  } else {
    info->hardware_backed = 0;
    strncpy(info->description, "Software Secure Element",
            sizeof(info->description) - 1);
  }

  print("TPM Family: Device info retrieved\n");
  return FAMILY_OK;
}

/*
 * Get TPM family capabilities
 */
/*@
  @ requires family == \null || \valid(family);
  @ assigns *ctx;
  @*/static uint32_t tpm_family_get_capabilities(struct FamilyExchangePage *family) {
  TPMFamilyContext *ctx = family->family_specific_ctx;
  return ctx->capabilities;
}

/*
 * Begin transaction for atomic TPM operations
 */
static int tpm_family_begin_transaction(struct FamilyExchangePage *family,
                                        uint64_t *tx_id) {
  /* For now, transactions are not implemented for TPM */
  /* Future: implement TPM transactions for atomic operations */
  return FAMILY_ETRANSACTION;
}

/*
 * Commit transaction
 */
static int tpm_family_commit_transaction(struct FamilyExchangePage *family,
                                         uint64_t tx_id) {
  return FAMILY_ETRANSACTION;
}

/*
 * TPM Family Shutdown
 */
/*@
  @ requires family == \null || \valid(family);
  @ assigns *ctx, family->family_specific_ctx, family->state;
  @*/static int tpm_family_shutdown(struct FamilyExchangePage *family) {
  TPMFamilyContext *ctx = family->family_specific_ctx;
  int i;

  print("TPM Family: Shutting down...\n");

  /* Release all channels */
  for (i = 0; i < ctx->channel_mgr.channel_count; i++) {
    xfree_driver(ctx->channel_mgr.channels[i]);
  }

  xfree_driver(ctx);
  family->family_specific_ctx = nil;
  family->state = FAMILY_UNINITIALIZED;

  print("TPM Family: Shutdown complete\n");
  return FAMILY_OK;
}

/*
 * Register TPM Secure Element Family
 */
/*@@*/int tpm_family_register(void) {
  int ret;

  print("TPM Family: Registering TPM Secure Element Family...\n");

  ret = family_register(FAMILY_SECURE_ELEMENT, &tpm_family_ops,
                        "TPM Secure Element");
  if (ret != FAMILY_OK) {
    print("TPM Family: Failed to register family: %d\n", ret);
    return ret;
  }

  print("TPM Family: Successfully registered\n");
  return FAMILY_OK;
}

/*
 * Unregister TPM Family
 */
/*@@*/int tpm_family_unregister(void) {
  print("TPM Family: Unregistering...\n");
  return family_unregister(FAMILY_SECURE_ELEMENT);
}

/*
 * Get secure element family handle
 */
struct FamilyExchangePage *secure_element_family_get(void) {
  return family_lookup(FAMILY_SECURE_ELEMENT);
}

/*
 * Create secure element channel for crypto operations
 */
/*@
  @ requires channel_id == \null || \valid(channel_id);
  @ assigns *family;
  @*/int secure_element_create_channel(uint32_t permissions, uint64_t *channel_id) {
  struct FamilyExchangePage *family = secure_element_family_get();
  if (family == nil) {
    return FAMILY_EINVAL;
  }

  return family->ops->allocate_channel(family, nil, permissions, channel_id);
}

/*
 * Get secure element capabilities
 */
/*@
  @ assigns *family;
  @*/uint32_t secure_element_get_capabilities(void) {
  struct FamilyExchangePage *family = secure_element_family_get();
  if (family == nil) {
    return 0;
  }

  return family->ops->get_capabilities(family);
}

/*
 * Secure Element Family Initialization
 *
 * This function initializes and registers the Secure Element family (TPM, etc.)
 * Called from the kernel boot sequence.
 */
/*@@*/void secure_element_init(void) {
  int ret;

  print("Secure Element: Initializing Secure Element family system...\n");

  /* Register the secure element family */
  ret = tpm_family_register();
  if (ret != 0) {
    print("Secure Element: Failed to register secure element family: %d\n",
          ret);
    return;
  }

  print("Secure Element: Secure Element family system initialized "
        "successfully\n");
}

/*
 * Secure element hardware random number generation
 */
/*@
  @ requires buffer == \null || \valid(buffer);
  @ assigns *family, family->family_specific_ctx, tmp[0..];
  @*/int secure_element_get_random(uint8_t *buffer, int len) {
  TPMFamilyContext *ctx;
  struct FamilyExchangePage *family = secure_element_family_get();

  if (family == nil || family->family_specific_ctx == nil) {
    return -1;
  }

  ctx = family->family_specific_ctx;

  if (ctx->tpm_available && (ctx->capabilities & FAMILY_CAP_RANDOM)) {
    /* Use hardware RNG via TPM */
    return tpm_get_random(buffer, len);
  }

  /* Software fallback */
  uint8_t tmp[32];
  int i, bytes_generated = 0;

  for (i = 0; i < len; i += sizeof(tmp)) {
    /* Use kernel entropy pool as fallback */
    int bytes_to_generate = (len - i) > sizeof(tmp) ? sizeof(tmp) : (len - i);
    int ret = tpm_get_random(tmp, bytes_to_generate);
    if (ret > 0) {
      memmove(buffer + i, tmp, ret);
      bytes_generated += ret;
    }
  }

  return bytes_generated;
}

/*
 * Secure element HMAC computation
 */
int secure_element_hmac(uint64_t channel_id, const uint8_t *data, size_t len,
                        uint8_t *hmac_out) {
  TPMFamilyContext *ctx;
  struct FamilyExchangePage *family = secure_element_family_get();
  size_t hmac_len = 32; /* SHA256 output */
  int i;

  if (family == nil || family->family_specific_ctx == nil) {
    return -1;
  }

  ctx = family->family_specific_ctx;

  print("Secure Element: HMAC requested for channel 0x%llx\n",
        (unsigned long long)channel_id);

  /* Use TPM 2.0 HMAC if available */
  if (ctx->tpm_available && ctx->tpm_ctx &&
      ctx->detected_version == TPM_VERSION_2_0) {
    if (tpm20_hmac(ctx->tpm_ctx, ctx->tpm_ctx->hmac_key_handle, (uint8_t *)data,
                   len, hmac_out, &hmac_len) == 0) {
      return (int)hmac_len;
    }
  }

  /* Software fallback - use TPM-backed HMAC-SHA256 from kernel crypto */
  if (crypto_tpm_hmac_sha256(hmac_out, (const uint8_t *)data, len) == 0) {
    print("Secure Element: Used crypto_tpm_hmac_sha256\n");
    return 32;
  }

  print("Secure Element: HMAC failed\n");
  return -1;
}

/*
 * Secure element attestation
 */
int secure_element_attest(uint64_t channel_id, uint8_t *attestation_data,
                          size_t *data_size) {
  TPMFamilyContext *ctx;
  struct FamilyExchangePage *family = secure_element_family_get();

  if (family == nil || family->family_specific_ctx == nil) {
    return -1;
  }

  ctx = family->family_specific_ctx;

  if (ctx->tpm_available && (ctx->capabilities & FAMILY_CAP_ATTESTATION)) {
    /* Use real TPM attestation */
    print("Secure Element: Hardware attestation requested\n");
    /* This would call the TPM driver attestation functions */
    return -1; /* Not implemented yet */
  }

  /* Software fallback attestation using SHA256 */
  uint64_t timestamp = fastticks(nil);
  uint8_t hash[32];

  /* Build attestation structure:
   * Bytes 0-7:   Magic "SEATT" + padding
   * Bytes 8-15:  Timestamp
   * Bytes 16-47: SHA256 hash of the attestation data
   */
  attestation_data[0] = 'S';
  attestation_data[1] = 'E';
  attestation_data[2] = 'A';
  attestation_data[3] = 'T';
  attestation_data[4] = 'T';
  attestation_data[5] = 0;
  attestation_data[6] = 0;
  attestation_data[7] = 0;
  memmove(&attestation_data[8], &timestamp, sizeof(timestamp));

  /* Hash the entire attestation structure using SHA256 */
  if (crypto_sha256(hash, attestation_data, 512) != 0) {
    print("Secure Element: SHA256 failed for attestation\n");
    return -1;
  }

  /* Include the hash in the attestation */
  memmove(&attestation_data[16], hash, 32);

  *data_size = 64;
  print("Secure Element: Software attestation generated (SHA256)\n");
  return 0;
}
