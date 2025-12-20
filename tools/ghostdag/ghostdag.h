/*
 * GHOSTDAG - Real DAG-based Consensus (Standalone Version)
 *
 * Pure C implementation for testing outside kernel.
 */

#ifndef GHOSTDAG_H
#define GHOSTDAG_H

#include <stddef.h>
#include <stdint.h>

/* Configuration */
#define GHOSTDAG_K_PARAM 3       /* Anticone tolerance */
#define GHOSTDAG_MAX_PARENTS 8   /* Max parents per block */
#define GHOSTDAG_WINDOW_SIZE 256 /* Sliding window of recent blocks */

/* Block colors for k-cluster */
typedef enum { GHOSTDAG_BLUE = 0, GHOSTDAG_RED = 1 } GhostColor;

/* Forward declarations */
typedef struct DagEntry DagEntry;
typedef struct GhostDAG GhostDAG;

/*
 * DagEntry - Lightweight DAG node for ordering
 */
struct DagEntry {
  uint64_t id;        /* Unique message ID */
  uint64_t order_num; /* Final consensus order (0 = not yet) */

  /* DAG structure (indices into window) */
  uint16_t parent_ids[GHOSTDAG_MAX_PARENTS];
  uint8_t parent_count;
  uint16_t selected_parent_id; /* Parent with max blue_work */

  /* Blue set scoring */
  uint32_t blue_work;  /* Cumulative blue work in past */
  uint16_t blue_score; /* Count of blue blocks in past */
  GhostColor color;

  /* Source subsystem */
  uint8_t subsystem_id;
};

/*
 * GhostDAG - The ordering consensus engine
 */
struct GhostDAG {
  /* Sliding window of recent entries (circular buffer) */
  DagEntry window[GHOSTDAG_WINDOW_SIZE];
  uint16_t window_head;  /* Next write position */
  uint16_t window_count; /* Current entries in window */

  /* Current tips (entries with no children) */
  uint16_t tip_ids[GHOSTDAG_MAX_PARENTS];
  uint8_t tips_count;

  /* Ordering state */
  uint64_t next_id;    /* Next message ID */
  uint64_t next_order; /* Next order number to assign */

  /* Configuration */
  uint8_t k_param;

  /* Statistics */
  uint64_t total_messages;
  uint64_t blue_messages;
  uint64_t red_dropped;
};

/* Core API */
void ghostdag_init(GhostDAG *dag, uint8_t k_param);
uint64_t ghostdag_order(GhostDAG *dag, uint8_t subsystem_id);
int ghostdag_is_ordered(GhostDAG *dag, uint64_t msg_id, uint64_t *order_out);
void ghostdag_stats(GhostDAG *dag, uint64_t *total, uint64_t *blue,
                    uint64_t *red);
void ghostdag_dump(GhostDAG *dag);

#endif /* GHOSTDAG_H */
