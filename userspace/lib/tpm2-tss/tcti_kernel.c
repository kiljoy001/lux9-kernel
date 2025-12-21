/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Kernel TCTI - bridges SAPI to our TIS driver
 */

#include "include/tss2_kernel.h"
#include "include/tss2_tcti.h"
#include "include/tss2_common.h"

/* Forward declaration of our TIS driver transmit function */
extern int tpm_transmit(u8int *cmd, usize cmd_len, u8int *resp, usize *resp_len);

/*
 * Kernel TCTI context - minimal implementation
 */
typedef struct {
    TSS2_TCTI_COMMON_CONTEXT common;
    u8int response_buffer[4096];
    usize response_len;
} TCTI_KERNEL_CONTEXT;

/*
 * Transmit command to TPM via TIS driver
 */
static TSS2_RC
tcti_kernel_transmit(
    TSS2_TCTI_CONTEXT *tctiContext,
    size_t size,
    const uint8_t *command)
{
    TCTI_KERNEL_CONTEXT *ctx = (TCTI_KERNEL_CONTEXT*)tctiContext;

    if (!tctiContext || !command || size == 0)
        return TSS2_TCTI_RC_BAD_VALUE;

    if (size > 4096)
        return TSS2_TCTI_RC_BAD_VALUE;

    /* Store response buffer size */
    ctx->response_len = sizeof(ctx->response_buffer);

    /* Call our TIS driver */
    if (tpm_transmit((u8int*)command, size, ctx->response_buffer, &ctx->response_len) < 0)
        return TSS2_TCTI_RC_IO_ERROR;

    return TSS2_RC_SUCCESS;
}

/*
 * Receive response from TPM
 */
static TSS2_RC
tcti_kernel_receive(
    TSS2_TCTI_CONTEXT *tctiContext,
    size_t *size,
    uint8_t *response,
    int32_t timeout)
{
    TCTI_KERNEL_CONTEXT *ctx = (TCTI_KERNEL_CONTEXT*)tctiContext;

    UNUSED(timeout); /* We don't use timeouts in kernel */

    if (!tctiContext || !size)
        return TSS2_TCTI_RC_BAD_VALUE;

    /* Return size if response buffer is NULL */
    if (!response) {
        *size = ctx->response_len;
        return TSS2_RC_SUCCESS;
    }

    if (*size < ctx->response_len)
        return TSS2_TCTI_RC_INSUFFICIENT_BUFFER;

    memmove(response, ctx->response_buffer, ctx->response_len);
    *size = ctx->response_len;

    return TSS2_RC_SUCCESS;
}

/*
 * Finalize TCTI context
 */
static void
tcti_kernel_finalize(TSS2_TCTI_CONTEXT *tctiContext)
{
    /* Nothing to do for kernel TCTI */
    UNUSED(tctiContext);
}

/*
 * Cancel command (not supported)
 */
static TSS2_RC
tcti_kernel_cancel(TSS2_TCTI_CONTEXT *tctiContext)
{
    UNUSED(tctiContext);
    return TSS2_TCTI_RC_NOT_IMPLEMENTED;
}

/*
 * Get poll handles (not supported in kernel)
 */
static TSS2_RC
tcti_kernel_get_poll_handles(
    TSS2_TCTI_CONTEXT *tctiContext,
    TSS2_TCTI_POLL_HANDLE *handles,
    size_t *num_handles)
{
    UNUSED(tctiContext);
    UNUSED(handles);
    UNUSED(num_handles);
    return TSS2_TCTI_RC_NOT_IMPLEMENTED;
}

/*
 * Set locality (handled by TIS driver)
 */
static TSS2_RC
tcti_kernel_set_locality(
    TSS2_TCTI_CONTEXT *tctiContext,
    uint8_t locality)
{
    UNUSED(tctiContext);
    UNUSED(locality);
    /* TIS driver handles locality automatically */
    return TSS2_RC_SUCCESS;
}

/*
 * Make ready (no-op for kernel)
 */
static TSS2_RC
tcti_kernel_make_sticky(
    TSS2_TCTI_CONTEXT *tctiContext,
    TPM2_HANDLE *handle,
    uint8_t sticky)
{
    UNUSED(tctiContext);
    UNUSED(handle);
    UNUSED(sticky);
    return TSS2_RC_SUCCESS;
}

/* Global kernel TCTI instance */
static TCTI_KERNEL_CONTEXT kernel_tcti_ctx = {
    .common = {
        .version = { 2, 0 },
        .transmit = tcti_kernel_transmit,
        .receive = tcti_kernel_receive,
        .finalize = tcti_kernel_finalize,
        .cancel = tcti_kernel_cancel,
        .getPollHandles = tcti_kernel_get_poll_handles,
        .setLocality = tcti_kernel_set_locality,
        .makeSticky = tcti_kernel_make_sticky,
    }
};

/*
 * Get kernel TCTI context for SAPI initialization
 */
TSS2_TCTI_CONTEXT*
tcti_kernel_get_context(void)
{
    return (TSS2_TCTI_CONTEXT*)&kernel_tcti_ctx;
}
