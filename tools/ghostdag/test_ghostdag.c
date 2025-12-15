/*
 * GHOSTDAG Test Harness
 *
 * Tests the real k-cluster algorithm with multiple subsystems.
 */

#include "ghostdag.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define NUM_SUBSYSTEMS 4
#define MSGS_PER_SUBSYSTEM 50

int main(int argc, char **argv) {
  GhostDAG dag;
  uint64_t order;
  int i, sub;
  uint8_t k = 3;

  if (argc > 1)
    k = atoi(argv[1]);

  printf("=== GHOSTDAG Test ===\n");
  printf("K parameter: %d\n", k);
  printf("Subsystems: %d\n", NUM_SUBSYSTEMS);
  printf("Messages per subsystem: %d\n\n", MSGS_PER_SUBSYSTEM);

  /* Initialize */
  ghostdag_init(&dag, k);

  /* Simulate parallel message submission from multiple subsystems */
  srand(time(NULL));

  for (i = 0; i < MSGS_PER_SUBSYSTEM; i++) {
    /* Each iteration, all subsystems submit one message */
    for (sub = 0; sub < NUM_SUBSYSTEMS; sub++) {
      order = ghostdag_order(&dag, sub);
      if (order > 0) {
        printf("Subsystem %d: msg ordered at #%lu\n", sub, order);
      } else {
        printf("Subsystem %d: msg DROPPED (RED)\n", sub);
      }
    }
  }

  printf("\n=== Statistics ===\n");
  uint64_t total, blue, red;
  ghostdag_stats(&dag, &total, &blue, &red);
  printf("Total: %lu\n", total);
  printf("Blue (ordered): %lu (%.1f%%)\n", blue, 100.0 * blue / total);
  printf("Red (dropped): %lu (%.1f%%)\n", red, 100.0 * red / total);

  printf("\n=== DAG Dump (last entries) ===\n");
  ghostdag_dump(&dag);

  return 0;
}
