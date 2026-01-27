/* domtree_verify.c - Frama-C verification of domtree functions
 *
 * Based on proofs/relooper/domtree_spec.v Coq specification
 * Tests key property: back edge detection for loop headers
 */

typedef unsigned int u32int;
typedef unsigned char u8int;

#define MAX_BLOCKS 64
#define MAX_DOM_CHILDREN 16

typedef struct {
  u32int id;
  u32int start_offset;
  u32int end_offset;
  u32int succ[MAX_DOM_CHILDREN];
  u32int n_succ;
  u32int pred[MAX_DOM_CHILDREN];
  u32int n_pred;
} dt_basic_block_t;

typedef struct {
  u32int block_id;
  u32int idom;
  u32int children[MAX_DOM_CHILDREN];
  u32int n_children;
  u32int rpo;
  u8int is_loop_header;
  u8int is_merge_node;
} domtree_node_t;

typedef struct {
  dt_basic_block_t blocks[MAX_BLOCKS];
  u32int n_blocks;
} dt_cfg_t;

typedef struct {
  domtree_node_t nodes[MAX_BLOCKS];
  u32int n_nodes;
  u32int root;
} domtree_t;

/*
 * @coq_proof: proofs/relooper/domtree_spec.v
 * @theorem: back_edge_characterization
 *   - Edge (src, dst) is back edge iff rpo(dst) <= rpo(src)
 * @theorem: loop_header_iff_back_edge
 *   - Block is loop header iff it has a back edge targeting it
 */
/*@
  requires \valid(cfg) && \valid(tree);
  requires cfg->n_blocks <= MAX_BLOCKS;
  requires cfg->n_blocks > 0;
  requires tree->n_nodes == cfg->n_blocks;
  requires \forall integer i; 0 <= i < cfg->n_blocks ==>
           cfg->blocks[i].n_pred <= MAX_DOM_CHILDREN;

  assigns tree->nodes[0..cfg->n_blocks-1].is_loop_header;
  assigns tree->nodes[0..cfg->n_blocks-1].is_merge_node;

  // Post: if any predecessor has rpo >= this block's rpo, it's a loop header
  behavior back_edge_detection:
    ensures \forall integer i; 0 <= i < cfg->n_blocks ==>
      (\forall integer p; 0 <= p < cfg->blocks[i].n_pred ==>
        cfg->blocks[i].pred[p] < cfg->n_blocks ==>
        tree->nodes[cfg->blocks[i].pred[p]].rpo >= tree->nodes[i].rpo ==>
        tree->nodes[i].is_loop_header == 1);


  // Post: if marked as loop header, there must be a back edge targeting it
behavior loop_header_implies_back_edge:
  ensures \forall integer i; 0 <= i < cfg->n_blocks ==>
    tree->nodes[i].is_loop_header == 1 ==>
      \exists integer p;
0 <= p < cfg->blocks[i].n_pred && cfg->blocks[i].pred[p] < cfg->n_blocks &&
    tree->nodes[cfg->blocks[i].pred[p]].rpo >= tree->nodes[i].rpo;
*/
void domtree_mark_special_nodes(dt_cfg_t *cfg, domtree_t *tree) {
  /*@ loop invariant 0 <= i <= cfg->n_blocks;
      loop assigns i, tree->nodes[0..cfg->n_blocks-1].is_loop_header,
                   tree->nodes[0..cfg->n_blocks-1].is_merge_node;
      loop variant cfg->n_blocks - i;
  */
  for (u32int i = 0; i < cfg->n_blocks; i++) {
    dt_basic_block_t *b = &cfg->blocks[i];
    domtree_node_t *n = &tree->nodes[i];

    u32int forward_in = 0;

    /*@ loop invariant 0 <= p <= b->n_pred;
        loop assigns p, forward_in, n->is_loop_header;
        loop variant b->n_pred - p;
    */
    for (u32int p = 0; p < b->n_pred; p++) {
      u32int pred = b->pred[p];

      // Per Coq theorem: back edge iff pred_rpo >= n_rpo
      if (tree->nodes[pred].rpo < n->rpo) {
        forward_in++;
      } else {
        n->is_loop_header = 1;
      }
    }

    n->is_merge_node = (forward_in >= 2) ? 1 : 0;
  }
}
