/* wasm_host_fruity.h - Fruity IR Host Bindings for WASM
 *
 * Exposes Fruity IR manipulation functions to WASM modules as Lux Capabilities.
 * Each IR object (module, function, block) is associated with a
 * lux_capability_t.
 */

#ifndef WASM_HOST_FRUITY_H
#define WASM_HOST_FRUITY_H

#include "../include/u.h"
#include "../include/portlib.h"
#include "../include/mem.h"
#include "../include/dat.h"
#include "../include/fns.h"
#include "fruity_ir.h"
#include "wasm_capability_bindings.h"
#include "wasm_runtime/wasm3/wasm3.h"

/* Link Fruity IR functions to WASM module */
M3Result LinkFruity(IM3Module module);

#endif /* WASM_HOST_FRUITY_H */
