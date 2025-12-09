/* Kernel compatibility */
#include "../include/tss2_kernel.h"

/* Kernel compatibility */
#include "../include/tss2_kernel.h"

/* SPDX-License-Identifier: BSD-2-Clause */
/***********************************************************************;
 * Copyright (c) 2015 - 2017, Intel Corporation
 * All rights reserved.
 ***********************************************************************/

#ifdef HAVE_CONFIG_H
#include "config.h" // Provided by tss2_kernel.h // Provided by tss2_kernel.h // IWYU pragma: keep
#endif

#include "../include/tss2_sys.h"      // for TSS2_SYS_CONTEXT, Tss2_Sys_Finalize
#include "../util/aux_util.h" // for UNUSED

void
Tss2_Sys_Finalize(TSS2_SYS_CONTEXT *sysContext) {
    UNUSED(sysContext);
}
