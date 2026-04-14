#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Include Monocypher directly (it's single file) */
#include "monocypher.c"

void print_hex(const char *label, const uint8_t *data, size_t len) {
  printf("%s: ", label);
  for (size_t i = 0; i < len; i++) {
    printf("%02x", data[i]);
  }
  printf("\n");
}

int parse_hex(const char *hex, uint8_t *bin, size_t len) {
  if (strlen(hex) != len * 2)
    return 0;
  for (size_t i = 0; i < len; i++) {
    char buf[3] = {hex[i * 2], hex[i * 2 + 1], 0};
    char *end;
    bin[i] = (uint8_t)strtoul(buf, &end, 16);
    if (*end != 0)
      return 0;
  }
  return 1;
}

int main(int argc, char **argv) {
  if (argc != 3) {
    fprintf(stderr, "Usage: %s <secret_key_hex> <file_to_sign>\n", argv[0]);
    return 1;
  }

  uint8_t secret_key[32];
  if (!parse_hex(argv[1], secret_key, 32)) {
    fprintf(stderr, "Invalid secret key hex string (must be 64 chars)\n");
    return 1;
  }

  /* Derive public key and full secret key */
  uint8_t public_key[32];
  uint8_t full_secret_key[64];
  uint8_t seed[32];
  /* Monocypher's eddsa_key_pair needs a seed, but here we act as if the input
   * IS the random seed/secret part */
  /* Wait, the input is likely just the seed for Ed25519. Monocypher uses
   * crypto_eddsa_key_pair(sk, pk, seed) */

  crypto_eddsa_key_pair(full_secret_key, public_key, secret_key);

  const char *filename = argv[2];
  FILE *f = fopen(filename, "rb");
  if (!f) {
    perror("fopen");
    return 1;
  }

  fseek(f, 0, SEEK_END);
  long fsize = ftell(f);
  fseek(f, 0, SEEK_SET);

  uint8_t *buf = malloc(fsize);
  if (!buf) {
    perror("malloc");
    fclose(f);
    return 1;
  }

  if (fread(buf, 1, fsize, f) != fsize) {
    perror("fread");
    free(buf);
    fclose(f);
    return 1;
  }
  fclose(f);

  uint8_t signature[64];
  crypto_eddsa_sign(signature, full_secret_key, buf, fsize);

  char sig_filename[1024];
  snprintf(sig_filename, sizeof(sig_filename), "%s.sig", filename);

  FILE *sig_f = fopen(sig_filename, "wb");
  if (!sig_f) {
    perror("fopen sig");
    free(buf);
    return 1;
  }

  if (fwrite(signature, 1, 64, sig_f) != 64) {
    perror("fwrite");
    fclose(sig_f);
    free(buf);
    return 1;
  }

  fclose(sig_f);
  free(buf);

  printf("Signed %s -> %s\n", filename, sig_filename);
  print_hex("Public Key", public_key,
            32); /* Print this so we can put it in kernel */

  return 0;
}
