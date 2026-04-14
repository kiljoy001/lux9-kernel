#ifndef PUBKEY_H
#define PUBKEY_H

#include <stdint.h>

/*
 * Public Key for Secure Initrd Verification (EdDSA)
 *
 * Development Secret Key (seed): all zeros (64 hex zeros)
 * 0000000000000000000000000000000000000000000000000000000000000000
 *
 * Generated using: ./userspace/host_sign <seed> <file>
 * Public key output:
 * 19d3d919475deed4696b5d13018151d1af88b2bd3bcff048b45031c1f36d1858
 *
 * IMPORTANT: Replace with TPM-backed key for production!
 */
static const uint8_t internal_pubkey[32] = {
    0x19, 0xd3, 0xd9, 0x19, 0x47, 0x5d, 0xee, 0xd4, 0x69, 0x6b, 0x5d,
    0x13, 0x01, 0x81, 0x51, 0xd1, 0xaf, 0x88, 0xb2, 0xbd, 0x3b, 0xcf,
    0xf0, 0x48, 0xb4, 0x50, 0x31, 0xc1, 0xf3, 0x6d, 0x18, 0x58};

#endif /* PUBKEY_H */
