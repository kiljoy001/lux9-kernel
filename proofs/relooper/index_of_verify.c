/* Standalone verification of index_of function for Frama-C
 * Based on proofs/relooper/ramsey_spec.v Coq specification
 */

typedef unsigned int u32int;

#define MAX_CTX_DEPTH 64

typedef enum {
  CTX_IF_THEN_ELSE,
  CTX_LOOP_HEADED_BY,
  CTX_BLOCK_FOLLOWED_BY
} ctx_type_t;

typedef struct {
  ctx_type_t type;
  u32int label;
} ctx_frame_t;

typedef struct {
  ctx_frame_t frames[MAX_CTX_DEPTH];
  u32int depth;
  u32int fallthrough_label;
  unsigned char has_fallthrough;
} translation_ctx_t;

/*
 * Paper lines 27-31: index function
 * Computes the br index for a target label in the context
 *
 * @coq_proof: proofs/relooper/ramsey_spec.v
 * @theorem: index_in_context
 *   - When index returns Some i, then i < length(ctx)
 * @theorem: context_contains_target (inverse)
 *   - When index returns Some i, target exists at some position in ctx
 * @theorem: index_iff_in_context (biconditional)
 *   - index succeeds iff target exists in context with matching label
 */
/*@
  requires \valid(ctx);
  requires ctx->depth <= MAX_CTX_DEPTH;
  assigns \nothing;

  behavior found:
    assumes \exists integer j; 0 <= j < ctx->depth &&
            (ctx->frames[j].type == CTX_BLOCK_FOLLOWED_BY ||
             ctx->frames[j].type == CTX_LOOP_HEADED_BY) &&
            ctx->frames[j].label == label;
    ensures 0 <= \result < (int)ctx->depth;

  behavior not_found:
    assumes \forall integer j; 0 <= j < ctx->depth ==>
            !(ctx->frames[j].type == CTX_BLOCK_FOLLOWED_BY &&
  ctx->frames[j].label == label) &&
            !(ctx->frames[j].type == CTX_LOOP_HEADED_BY && ctx->frames[j].label
  == label); ensures \result == -1;

  complete behaviors;
  disjoint behaviors;
*/
int index_of(u32int label, translation_ctx_t *ctx) {
  /*@ loop invariant 0 <= i+1 <= ctx->depth;
      loop invariant \forall integer k; i < k < ctx->depth ==>
          !(ctx->frames[k].type == CTX_BLOCK_FOLLOWED_BY && ctx->frames[k].label
     == label) &&
          !(ctx->frames[k].type == CTX_LOOP_HEADED_BY && ctx->frames[k].label ==
     label); loop assigns i; loop variant i+1;
  */
  for (int i = (int)ctx->depth - 1; i >= 0; i--) {
    ctx_frame_t *frame = &ctx->frames[i];
    if (frame->type == CTX_BLOCK_FOLLOWED_BY && frame->label == label) {
      return (int)ctx->depth - 1 - i;
    }
    if (frame->type == CTX_LOOP_HEADED_BY && frame->label == label) {
      return (int)ctx->depth - 1 - i;
    }
  }
  return -1;
}
