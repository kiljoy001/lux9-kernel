/* wasm.h - Unified WASM Header
 *
 * Include this header to get the common WASM runtime interfaces and
 * related kernel declarations in a stable order.
 */

#pragma once

/* 1. Base kernel definitions and contracts */
#include "../include/kernel.h"

/* 2. Core WASM runtime and integration */
#include "wasm_runtime.h"
#include "wasm_capability_bindings.h"
#include "wasm_host_lux9.h"
#include "wasm_host_fruity.h"
#include "wasm_host_fruity_policy.h"
#include "wasm_fileserver.h"
#include "wasm_9p_integration.h"
#include "wasi_lux9_shim.h"

/* 3. Fruity IR */
#include "fruity_types.h"
#include "fruity_opcodes.h"
#include "fruity_ir.h"
