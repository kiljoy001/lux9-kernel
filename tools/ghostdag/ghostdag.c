/*
 * GHOSTDAG - Real DAG-based Consensus (Standalone Version)
 */

#include "ghostdag.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ========================================================================= */
/* Helpers                                                                   */
/* ========================================================================= */

static DagEntry *get_entry(GhostDAG *dag, uint16_t idx) {
  if (idx >= GHOSTDAG_WINDOW_SIZE)
    return NULL;
  return &dag->window[idx];
}

static uint16_t id_to_idx(GhostDAG *dag, uint64_t id) {
  int i;
  for (i = 0; i < dag->window_count; i++) {
    uint16_t idx = (dag->window_head - 1 - i + GHOSTDAG_WINDOW_SIZE) %
                   GHOSTDAG_WINDOW_SIZE;
    if (dag->window[idx].id == id)
      return idx;
  }
  return GHOSTDAG_WINDOW_SIZE;
}

/* Check if entry A is ancestor of entry B (recursive DFS) */
static int is_ancestor(GhostDAG *dag, uint16_t a_idx, uint16_t b_idx) {
  DagEntry *b;
  int i;

  if (a_idx == b_idx)
    return 1;
  if (a_idx >= GHOSTDAG_WINDOW_SIZE || b_idx >= GHOSTDAG_WINDOW_SIZE)
    return 0;

  b = get_entry(dag, b_idx);
  if (b == NULL)
    return 0;

  for (i = 0; i < b->parent_count; i++) {
    if (is_ancestor(dag, a_idx, b->parent_ids[i]))
      return 1;
  }
  return 0;
}

/* Check if two entries are in each other's anticone */
static int in_anticone(GhostDAG *dag, uint16_t a_idx, uint16_t b_idx) {
  return !is_ancestor(dag, a_idx, b_idx) && !is_ancestor(dag, b_idx, a_idx);
}

/* ========================================================================= */
/* Initialization                                                            */
/* ========================================================================= */

void ghostdag_init(GhostDAG *dag, uint8_t k_param) {
  memset(dag, 0, sizeof(GhostDAG));
  dag->k_param = k_param;
  dag->next_id = 1;
  dag->next_order = 1;

  /* Create genesis entry at index 0 */
  dag->window[0].id = 0;
  dag->window[0].order_num = 0;
  dag->window[0].color = GHOSTDAG_BLUE;
  dag->window[0].blue_work = 1;
  dag->window[0].blue_score = 1;
  dag->window_head = 1;
  dag->window_count = 1;

  /* Genesis is initial tip */
  dag->tip_ids[0] = 0;
  dag->tips_count = 1;
}

/* ========================================================================= */
/* Blue Set Selection                                                        */
/* ========================================================================= */

/* Count blue entries in anticone of given entry */
static int blue_anticone_count(GhostDAG *dag, uint16_t entry_idx) {
  int count = 0, i;
  uint16_t other_idx;
  DagEntry *other;

  for (i = 0; i < dag->window_count; i++) {
    other_idx = (dag->window_head - 1 - i + GHOSTDAG_WINDOW_SIZE) %
                GHOSTDAG_WINDOW_SIZE;
    if (other_idx == entry_idx)
      continue;
    other = get_entry(dag, other_idx);
    if (other == NULL || other->color != GHOSTDAG_BLUE)
      continue;
    if (in_anticone(dag, entry_idx, other_idx))
      count++;
  }
  return count;
}

/* Determine color based on k-cluster membership */
static GhostColor determine_color(GhostDAG *dag, uint16_t entry_idx) {
  int blue_ac = blue_anticone_count(dag, entry_idx);
  return (blue_ac <= dag->k_param) ? GHOSTDAG_BLUE : GHOSTDAG_RED;
}

/* ========================================================================= */
/* Ordering API                                                              */
/* ========================================================================= */

uint64_t ghostdag_order(GhostDAG *dag, uint8_t subsystem_id) {
  DagEntry *entry;
  uint16_t new_idx;
  int i;
  uint64_t order;
  uint32_t max_work = 0;
  int best_parent = 0;

  /* Allocate entry in circular buffer */
  new_idx = dag->window_head;
  entry = &dag->window[new_idx];

  /* Initialize entry */
  memset(entry, 0, sizeof(DagEntry));
  entry->id = dag->next_id++;
  entry->subsystem_id = subsystem_id;

  /* Select parents (all current tips) */
  entry->parent_count = 0;
  for (i = 0; i < dag->tips_count && entry->parent_count < GHOSTDAG_MAX_PARENTS;
       i++) {
    entry->parent_ids[entry->parent_count++] = dag->tip_ids[i];
  }

  /* Find selected parent (highest blue_work) */
  for (i = 0; i < entry->parent_count; i++) {
    DagEntry *p = get_entry(dag, entry->parent_ids[i]);
    if (p != NULL && p->blue_work > max_work) {
      max_work = p->blue_work;
      best_parent = entry->parent_ids[i];
    }
  }
  entry->selected_parent_id = best_parent;

  /* Advance window */
  dag->window_head = (dag->window_head + 1) % GHOSTDAG_WINDOW_SIZE;
  if (dag->window_count < GHOSTDAG_WINDOW_SIZE)
    dag->window_count++;

  /* Determine color */
  entry->color = determine_color(dag, new_idx);

  /* Compute scores from selected parent */
  if (entry->parent_count > 0) {
    DagEntry *sp = get_entry(dag, entry->selected_parent_id);
    if (sp != NULL) {
      entry->blue_score = sp->blue_score;
      entry->blue_work = sp->blue_work;
    }
  }

  dag->total_messages++;

  if (entry->color == GHOSTDAG_BLUE) {
    entry->blue_score++;
    entry->blue_work++;
    entry->order_num = dag->next_order++;
    dag->blue_messages++;
    order = entry->order_num;

    /* Update tips: this entry becomes the new tip */
    dag->tips_count = 0;
    dag->tip_ids[dag->tips_count++] = new_idx;
  } else {
    /* RED - dropped, no order number */
    entry->order_num = 0;
    dag->red_dropped++;
    order = 0;
  }

  return order;
}

int ghostdag_is_ordered(GhostDAG *dag, uint64_t msg_id, uint64_t *order_out) {
  uint16_t idx = id_to_idx(dag, msg_id);
  if (idx >= GHOSTDAG_WINDOW_SIZE)
    return 0;
  DagEntry *e = get_entry(dag, idx);
  if (e == NULL || e->order_num == 0)
    return 0;
  if (order_out)
    *order_out = e->order_num;
  return 1;
}

void ghostdag_stats(GhostDAG *dag, uint64_t *total, uint64_t *blue,
                    uint64_t *red) {
  if (total)
    *total = dag->total_messages;
  if (blue)
    *blue = dag->blue_messages;
  if (red)
    *red = dag->red_dropped;
}

void ghostdag_dump(GhostDAG *dag) {
  int i;
  DagEntry *e;

  printf("GHOSTDAG: window=%d tips=%d k=%d\n", dag->window_count,
         dag->tips_count, dag->k_param);
  printf("  Total: %lu, Blue: %lu, Red: %lu\n", dag->total_messages,
         dag->blue_messages, dag->red_dropped);

  for (i = 0; i < dag->window_count; i++) {
    e = &dag->window[i];
    if (e->id == 0 && i > 0)
      continue;
    printf("  [%lu] %s order=%lu bscore=%u bwork=%u subsys=%d\n", e->id,
           e->color == GHOSTDAG_BLUE ? "BLUE" : "RED", e->order_num,
           e->blue_score, e->blue_work, e->subsystem_id);
  }
}
