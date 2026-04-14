/*
 * TPM/Secure Element Family Implementation - HAL Userspace
 */

#include "family.h"
#include <libc.h>
#include <u.h>

struct FamilyExchangePage *global_tpm_family = nil;

struct TPMFamilyContext {
  int tpm_fd;
  int is_simulated;
};

/*
 * Family Ops
 */

static int tpm_family_init_op(struct FamilyExchangePage *family) {
  struct TPMFamilyContext *ctx;

  ctx = family_alloc_zero(sizeof(struct TPMFamilyContext));
  if (!ctx)
    return FAMILY_ENOMEM;

  /* Try to open TPM device */
  ctx->tpm_fd = -1;
  // ctx->tpm_fd = open("/dev/tpm/0", ORDWR);

  family->family_specific_ctx = ctx;

  /* Set capabilities */
  family->capabilities_mask = FAMILY_CAP_CRYPTO | FAMILY_CAP_RANDOM |
                              FAMILY_CAP_ATTESTATION |
                              FAMILY_CAP_SECURE_STORAGE;

  return FAMILY_OK;
}

static int tpm_family_shutdown_op(struct FamilyExchangePage *family) {
  struct TPMFamilyContext *ctx = family->family_specific_ctx;
  if (ctx) {
    // if(ctx->tpm_fd >= 0) close(ctx->tpm_fd);
    family_free(ctx);
  }
  return FAMILY_OK;
}

static int tpm_get_info(struct FamilyExchangePage *family, void *device,
                        void *buffer, usize *size) {
  // Provide TPM version info etc.
  return FAMILY_OK;
}

struct FamilyOps tpm_ops = {
    .init = tpm_family_init_op,
    .shutdown = tpm_family_shutdown_op,
    .device_get_info = tpm_get_info,
};

/*
 * Public Init
 */
int family_tpm_init(void) {
  return family_register(FAMILY_SECURE_ELEMENT, &tpm_ops, "TPM");
}
