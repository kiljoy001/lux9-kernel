/*
 * GHOSTDAG Concurrency Test
 *
 * Simulates concurrent block production to trigger k-cluster rejection.
 */

#include "ghostdag.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * Submit multiple messages that all see the SAME parent (simulating
 * concurrency) This creates a "burst" where all messages are in each other's
 * anticone.
 */
static void submit_concurrent_burst(GhostDAG *dag, int count, int subsystem) {
  /* Save current state */
  uint16_t saved_head = dag->window_head;
  uint16_t saved_count = dag->window_count;
  uint8_t saved_tips_count = dag->tips_count;
  uint16_t saved_tips[GHOSTDAG_MAX_PARENTS];
  memcpy(saved_tips, dag->tip_ids, sizeof(saved_tips));

  int blue = 0, red = 0;
  uint64_t order;

  printf("Submitting burst of %d concurrent messages from subsystem %d\n",
         count, subsystem);

  for (int i = 0; i < count; i++) {
    /* Reset tips to saved state (simulating all seeing same parents) */
    dag->tips_count = saved_tips_count;
    memcpy(dag->tip_ids, saved_tips, sizeof(saved_tips));

    order = ghostdag_order(dag, subsystem);
    if (order > 0) {
      blue++;
    } else {
      red++;
    }
  }

  printf("  Result: %d BLUE, %d RED\n", blue, red);
}

int main(int argc, char **argv) {
  GhostDAG dag;
  uint8_t k = 3;

  if (argc > 1)
    k = atoi(argv[1]);

  printf("=== GHOSTDAG Concurrency Test ===\n");
  printf("K parameter: %d (expect up to K+1 = %d concurrent BLUE)\n\n", k,
         k + 1);

  ghostdag_init(&dag, k);

  /* Test 1: Burst larger than K */
  printf("--- Test 1: Burst of 10 concurrent (k=%d, expect ~%d BLUE) ---\n", k,
         k + 1);
  submit_concurrent_burst(&dag, 10, 0);

  /* Test 2: After burst, sequential should work */
  printf("\n--- Test 2: Sequential after burst ---\n");
  for (int i = 0; i < 5; i++) {
    uint64_t order = ghostdag_order(&dag, 1);
    printf("Sequential msg: order=%lu (%s)\n", order,
           order > 0 ? "BLUE" : "RED");
  }

  /* Test 3: Another burst */
  printf("\n--- Test 3: Another burst of 8 concurrent ---\n");
  submit_concurrent_burst(&dag, 8, 2);

  /* Final stats */
  printf("\n=== Final Statistics ===\n");
  uint64_t total, blue, red;
  ghostdag_stats(&dag, &total, &blue, &red);
  printf("Total: %lu\n", total);
  printf("Blue: %lu (%.1f%%)\n", blue, 100.0 * blue / total);
  printf("Red: %lu (%.1f%%)\n", red, 100.0 * red / total);

  return 0;
}
