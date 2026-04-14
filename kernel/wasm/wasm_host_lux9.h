/* wasm_host_lux9.h - Lux9 Kernel Host Functions for WASM
 *
 * Provides kernel-native host imports under the "lux9" namespace.
 */

#ifndef WASM_HOST_LUX9_H
#define WASM_HOST_LUX9_H

#include "wasm_runtime/wasm3/wasm3.h"

/* Link all lux9 namespace host functions to a WASM module */
M3Result LinkLux9(IM3Module module);

#endif /* WASM_HOST_LUX9_H */
