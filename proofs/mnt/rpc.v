(** * Mount RPC Management - Queue and reference counting proofs
    * 
    * Imports: mnt/types
    * Proves: RPC allocation tracking, conservation
    *)

Require Import mnt.types.

(* ========================================================================= *)
(* RPC ALLOCATION TRANSITIONS                                                *)
(* ========================================================================= *)

(** mntralloc: Allocates an RPC structure *)
Inductive MntRalloc (ma1 ma2 : MntAllocState) : Prop :=
  | MR_FromFree :
      ma1.(ma_nrpcfree) > 0 ->
      ma2 = mkMntAlloc 
              ma1.(ma_id) 
              (ma1.(ma_nrpcfree) - 1)
              (ma1.(ma_nrpcused) + 1)
              ma1.(ma_tags) ->
      MntRalloc ma1 ma2
  | MR_NewAlloc :
      ma1.(ma_nrpcfree) = 0 ->
      ma2 = mkMntAlloc
              ma1.(ma_id)
              ma1.(ma_nrpcfree)
              (ma1.(ma_nrpcused) + 1)
              ma1.(ma_tags) ->  (* tag allocated separately *)
      MntRalloc ma1 ma2.

(** mntfree: Frees an RPC structure *)
Inductive MntFree (ma1 ma2 : MntAllocState) : Prop :=
  | MF_ToFree :
      ma1.(ma_nrpcfree) < 32 ->
      ma1.(ma_nrpcused) > 0 ->
      ma2 = mkMntAlloc
              ma1.(ma_id)
              (ma1.(ma_nrpcfree) + 1)
              (ma1.(ma_nrpcused) - 1)
              ma1.(ma_tags) ->
      MntFree ma1 ma2
  | MF_Discard :
      ma1.(ma_nrpcfree) >= 32 ->
      ma1.(ma_nrpcused) > 0 ->
      ma2 = mkMntAlloc
              ma1.(ma_id)
              ma1.(ma_nrpcfree)
              (ma1.(ma_nrpcused) - 1)
              ma1.(ma_tags) ->  (* tag freed separately *)
      MntFree ma1 ma2.

(* ========================================================================= *)
(* RPC COUNTING PROOFS                                                       *)
(* ========================================================================= *)

(** mntralloc increments used count *)
Theorem mntralloc_increments_used :
  forall ma1 ma2,
  MntRalloc ma1 ma2 -> ma2.(ma_nrpcused) = ma1.(ma_nrpcused) + 1.
Proof.
  intros ma1 ma2 H. inversion H; subst; simpl; reflexivity.
Qed.

(** mntfree decrements used count *)
Theorem mntfree_decrements_used :
  forall ma1 ma2,
  MntFree ma1 ma2 -> ma2.(ma_nrpcused) = ma1.(ma_nrpcused) - 1.
Proof.
  intros ma1 ma2 H. inversion H; subst; simpl; reflexivity.
Qed.

(** mntralloc from free list decrements free count *)
Theorem mntralloc_from_free_decrements :
  forall ma1 ma2,
  ma1.(ma_nrpcfree) > 0 ->
  MntRalloc ma1 ma2 ->
  ma2.(ma_nrpcfree) = ma1.(ma_nrpcfree) - 1.
Proof.
  intros ma1 ma2 Hfree H.
  inversion H; subst; simpl.
  - reflexivity.
  - lia.
Qed.

(** mntfree to free list increments free count *)
Theorem mntfree_to_free_increments :
  forall ma1 ma2,
  ma1.(ma_nrpcfree) < 32 ->
  MntFree ma1 ma2 ->
  ma2.(ma_nrpcfree) = ma1.(ma_nrpcfree) + 1.
Proof.
  intros ma1 ma2 Hlt H.
  inversion H; subst; simpl.
  - reflexivity.
  - lia.
Qed.

(** Well-formedness preserved by mntralloc *)
Theorem mntralloc_preserves_wf :
  forall ma1 ma2,
  Inv_WellFormed ma1 -> MntRalloc ma1 ma2 -> Inv_WellFormed ma2.
Proof.
  intros ma1 ma2 [Hcounts Hid] H.
  unfold Inv_WellFormed, Inv_RpcCountsNonNeg, Inv_IdPositive in *.
  inversion H; subst; simpl; lia.
Qed.

(** Well-formedness preserved by mntfree *)
Theorem mntfree_preserves_wf :
  forall ma1 ma2,
  Inv_WellFormed ma1 -> MntFree ma1 ma2 -> Inv_WellFormed ma2.
Proof.
  intros ma1 ma2 [Hcounts Hid] H.
  unfold Inv_WellFormed, Inv_RpcCountsNonNeg, Inv_IdPositive in *.
  inversion H; subst; simpl; lia.
Qed.

(* ========================================================================= *)
(* INVERSE THEOREMS                                                          *)
(* ========================================================================= *)

(** Alloc then free returns used count to original *)
Theorem ralloc_free_inverse_used :
  forall ma1 ma2 ma3,
  MntRalloc ma1 ma2 -> MntFree ma2 ma3 ->
  ma3.(ma_nrpcused) = ma1.(ma_nrpcused).
Proof.
  intros ma1 ma2 ma3 Ha Hf.
  inversion Ha; inversion Hf; subst; simpl; lia.
Qed.

(** Conservation: free + used changes by at most 1 per operation *)
Theorem rpc_delta_bounded :
  forall ma1 ma2,
  MntRalloc ma1 ma2 ->
  (ma2.(ma_nrpcfree) + ma2.(ma_nrpcused)) - 
  (ma1.(ma_nrpcfree) + ma1.(ma_nrpcused)) <= 1.
Proof.
  intros ma1 ma2 H.
  inversion H; subst; simpl; lia.
Qed.

(** ID never changes during RPC operations *)
Theorem rpc_ops_preserve_id :
  forall ma1 ma2,
  MntRalloc ma1 ma2 -> ma2.(ma_id) = ma1.(ma_id).
Proof.
  intros ma1 ma2 H. inversion H; subst; simpl; reflexivity.
Qed.

(** Tags unchanged by RPC alloc (from free list) *)
Theorem mntralloc_preserves_tags :
  forall ma1 ma2,
  MntRalloc ma1 ma2 -> ma2.(ma_tags) = ma1.(ma_tags).
Proof.
  intros ma1 ma2 H. inversion H; subst; simpl; reflexivity.
Qed.

(** Tags unchanged by RPC free (to free list) *)
Theorem mntfree_preserves_tags :
  forall ma1 ma2,
  ma1.(ma_nrpcfree) < 32 ->
  MntFree ma1 ma2 -> ma2.(ma_tags) = ma1.(ma_tags).
Proof.
  intros ma1 ma2 Hlt H.
  inversion H; subst; simpl.
  - reflexivity.
  - lia.
Qed.

(** Double alloc then double free returns to original nrpcused *)
Theorem double_alloc_double_free :
  forall ma1 ma2 ma3 ma4 ma5,
  MntRalloc ma1 ma2 -> MntRalloc ma2 ma3 ->
  MntFree ma3 ma4 -> MntFree ma4 ma5 ->
  ma5.(ma_nrpcused) = ma1.(ma_nrpcused).
Proof.
  intros ma1 ma2 ma3 ma4 ma5 Ha1 Ha2 Hf1 Hf2.
  inversion Ha1; inversion Ha2; inversion Hf1; inversion Hf2;
  subst; simpl; lia.
Qed.
