#include <stdio.h>
#include <stdint.h>
#include <string.h>

// Simulate the hash generation
void crypto_sha256(uint8_t *out, const uint8_t *in, size_t len) {
  // Just copy first bytes for debugging
  for (int i = 0; i < 32; i++) {
    out[i] = (i < len) ? in[i] : 0;
  }
}

int main() {
  uint8_t parent_hash[32] = {0xAB, 0xCD, 0xEF};
  uint32_t constraints = 1;
  uint64_t epoch = 1;
  
  uint8_t cap_input[32 + 4 + 8];
  memcpy(cap_input, parent_hash, 32);
  memcpy(cap_input + 32, &constraints, 4);
  memcpy(cap_input + 36, &epoch, 8);
  
  uint8_t child_hash[32];
  crypto_sha256(child_hash, cap_input, sizeof(cap_input));
  
  printf("Child hash (first 8 bytes): ");
  for (int i = 0; i < 8; i++) printf("%02x ", child_hash[i]);
  printf("\n");
  
  // Second derive with same params
  epoch = 1; // Same epoch!
  memcpy(cap_input + 36, &epoch, 8);
  
  uint8_t child2_hash[32];
  crypto_sha256(child2_hash, cap_input, sizeof(cap_input));
  
  printf("Child2 hash: ");
  for (int i = 0; i < 8; i++) printf("%02x ", child2_hash[i]);
  printf("\n");
  
  printf("Same? %s\n", memcmp(child_hash, child2_hash, 32) == 0 ? "YES" : "NO");
  return 0;
}
