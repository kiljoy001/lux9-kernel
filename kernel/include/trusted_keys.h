/*
 * Trusted Keys for Kernel Crypto
 *
 * Contains baked-in public keys for signature verification.
 */
#pragma once

#include <stdint.h>
#include <stddef.h>

/* Trusted Ed25519 public key for initrd signature verification */
extern const uint8_t trusted_ed25519_pubkey[32];

/* Verify Ed25519 signature using trusted public key */
int verify_initrd_signature(const uint8_t *message, size_t message_len,
                           const uint8_t *signature, size_t signature_len);