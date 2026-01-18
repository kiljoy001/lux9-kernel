#include "../dat.h"
#include <stdio.h>
#include <string.h>

extern int tlsh_hash(const u8int *data, u64int len, u8int tlsh_out[35]);
extern int tlsh_distance(const u8int tlsh1[35], const u8int tlsh2[35]);

void print_hex(const u8int *data, int len) {
  for (int i = 0; i < len; i++) {
    printf("%02x", data[i]);
  }
  printf("\n");
}

int main() {
  const char *text1 =
      "This is a sample text for testing the TLSH algorithm. It needs to be "
      "long enough to generate meaningful buckets. TLSH works better with more "
      "data, typically at least 50 bytes, but ideally much more. We are adding "
      "some junk here to make it longer. 1234567890 ABCDEFGHIJKLMNOPQRSTUVWXYZ "
      "abcdefghijklmnopqrstuvwxyz.";
  const char *text2 =
      "This is a sample text for testing the TLSH algorithm. It needs to be "
      "long enough to generate meaningful buckets. TLSH works better with more "
      "data, typically at least 50 bytes, but ideally much more. We are adding "
      "some junk here to make it longer. 1234567890 ABCDEFGHIJKLMNOPQRSTUVWXYZ "
      "abcdefghijklmnopqrstuvwxyz!";
  const char *text3 =
      "Completely different content that should result in a very high TLSH "
      "distance. This text has nothing to do with the first two samples. "
      "Random words: apple, banana, cherry, dog, elephant, forest, galaxy, "
      "horizon, island, jungle.";

  u8int hash1[35], hash2[35], hash3[35];

  if (tlsh_hash((const u8int *)text1, strlen(text1), hash1) != 0) {
    printf("Error hashing text1\n");
    return 1;
  }
  if (tlsh_hash((const u8int *)text2, strlen(text2), hash2) != 0) {
    printf("Error hashing text2\n");
    return 1;
  }
  if (tlsh_hash((const u8int *)text3, strlen(text3), hash3) != 0) {
    printf("Error hashing text3\n");
    return 1;
  }

  printf("Hash 1: ");
  print_hex(hash1, 35);
  printf("Hash 2: ");
  print_hex(hash2, 35);
  printf("Hash 3: ");
  print_hex(hash3, 35);

  int d12 = tlsh_distance(hash1, hash2);
  int d13 = tlsh_distance(hash1, hash3);

  printf("Distance (1, 2): %d\n", d12);
  printf("Distance (1, 3): %d\n", d13);

  if (d12 < 30 && d13 > 100) {
    printf("SUCCESS: TLSH properties verified.\n");
  } else {
    printf("FAILURE: Unexpected TLSH properties.\n");
  }

  return 0;
}
