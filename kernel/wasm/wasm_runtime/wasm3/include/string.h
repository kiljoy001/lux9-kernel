#ifndef _WASM_STRING_H
#define _WASM_STRING_H

/* Lux9 Kernel environment already includes portlib.h via -include */
/* We just need to map missing standard functions */

#define memcpy memmove

/* Portlib uses 'long' for size, wasm3 uses 'size_t' (ulong). 
 * This is generally compatible on 64-bit.
 * Portlib uses 'char *' instead of 'const char *'. 
 * We might get warnings but it should link.
 */

#endif