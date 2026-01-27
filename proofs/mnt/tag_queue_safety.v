(** * Tag-Queue Safety - Proves tag reuse race prevention
    *
    * Imports: mnt/types, mnt/tag, mnt/rpc
    * Proves: Tags can only be freed after RPC removed from queue
    *
    * BUG FIX: This proof addresses the tag reuse race in devmnt.c:1288
    * where freetag() is called but the RPC might still be in the queue.
    *)

Require Import mnt.types.
Require Import mnt.tag.
Require Import mnt.rpc.
Require Import Coq.Lists.List.
Import ListNotations.

Open Scope Z_scope.

(* ========================================================================= *)
(* QUEUE STATE MODEL                                                         *)
(* ========================================================================= *)

(** Queue: List of RPCs indexed by tag *)
Definition Queue := list (Z * RpcState).

(** RPC is in queue *)
Definition in_queue (q : Queue) (tag : Z) : Prop :=
  exists r, In (tag, r) q.

(** RPC is not in queue *)
Definition not_in_queue (q : Queue) (tag : Z) : Prop :=
  ~ in_queue q tag.

(** Queue contains unique tags *)
Definition queue_tags_unique (q : Queue) : Prop :=
  forall t1 t2 r1 r2,
  In (t1, r1) q ->
  In (t2, r2) q ->
  t1 = t2 ->
  r1 = r2.

(* ========================================================================= *)
(* MNT STATE WITH QUEUE                                                      *)
(* ========================================================================= *)

(** Combined state: tag allocator + queue *)
Record MntState := mkMntState {
  mnt_tags : TagSet;
  mnt_queue : Queue
}.

(** Well-formed Mnt state *)
Definition Inv_MntWellFormed (st : MntState) : Prop :=
  queue_tags_unique st.(mnt_queue) /\
  (forall tag, in_queue st.(mnt_queue) tag -> tag_allocated st.(mnt_tags) tag).

(* ========================================================================= *)
(* QUEUE OPERATIONS                                                          *)
(* ========================================================================= *)

(** Helper: Filter out matching tag from queue *)
Fixpoint queue_remove_tag (tag : Z) (q : Queue) : Queue :=
  match q with
  | [] => []
  | (t, r) :: rest =>
      if Z.eqb t tag then
        queue_remove_tag tag rest
      else
        (t, r) :: queue_remove_tag tag rest
  end.

(** mntqrm: Remove RPC from queue *)
Inductive QueueRemove (tag : Z) (st1 st2 : MntState) : Prop :=
  | QR_Success :
      Inv_MntWellFormed st1 ->
      in_queue st1.(mnt_queue) tag ->
      st2 = mkMntState
              st1.(mnt_tags)
              (queue_remove_tag tag st1.(mnt_queue)) ->
      QueueRemove tag st1 st2.

(** mntqadd: Add RPC to queue (used by mountrpc) *)
Inductive QueueAdd (tag : Z) (r : RpcState) (st1 st2 : MntState) : Prop :=
  | QA_Success :
      Inv_MntWellFormed st1 ->
      tag_allocated st1.(mnt_tags) tag ->
      not_in_queue st1.(mnt_queue) tag ->
      st2 = mkMntState
              st1.(mnt_tags)
              ((tag, r) :: st1.(mnt_queue)) ->
      QueueAdd tag r st1 st2.

(* ========================================================================= *)
(* SAFE FREE TAG OPERATION                                                  *)
(* ========================================================================= *)

(** FreeTagSafe: Tag can only be freed if NOT in queue *)
Inductive FreeTagSafe (tag : Z) (st1 st2 : MntState) : Prop :=
  | FTS_Success :
      Inv_MntWellFormed st1 ->
      tag_allocated st1.(mnt_tags) tag ->
      not_in_queue st1.(mnt_queue) tag ->  (* CRITICAL PRECONDITION *)
      st2 = mkMntState
              (free_tag st1.(mnt_tags) tag)
              st1.(mnt_queue) ->
      FreeTagSafe tag st1 st2.

(* ========================================================================= *)
(* TAG REUSE RACE BUG MODEL                                                  *)
(* ========================================================================= *)

(** UNSAFE free: Frees tag even if RPC still in queue (THE BUG) *)
Inductive FreeTagUnsafe (tag : Z) (st1 st2 : MntState) : Prop :=
  | FTU_Buggy :
      tag_allocated st1.(mnt_tags) tag ->
      st2 = mkMntState
              (free_tag st1.(mnt_tags) tag)
              st1.(mnt_queue) ->
      FreeTagUnsafe tag st1 st2.

(** Prove: Unsafe free can violate queue invariant *)
Theorem unsafe_free_breaks_invariant :
  exists tag st1 st2,
  Inv_MntWellFormed st1 /\
  in_queue st1.(mnt_queue) tag /\
  FreeTagUnsafe tag st1 st2 /\
  ~ Inv_MntWellFormed st2.
Proof.
  (* Construct a counterexample *)
  exists 5.  (* tag = 5 *)

  (* Initial state: tag 5 allocated and in queue *)
  set (ts1 := alloc_tag empty_tagset 5).
  set (r := mkRpc 5 false false).
  set (q1 := [(5, r)]).
  exists (mkMntState ts1 q1).

  (* Final state: tag 5 freed but still in queue *)
  set (ts2 := free_tag ts1 5).
  exists (mkMntState ts2 q1).

  split; [| split; [| split]].
  - (* st1 is well-formed *)
    split.
    + (* queue_tags_unique *)
      intros t1 t2 r1 r2 H1 H2 Heq.
      destruct H1 as [H1 | []]; destruct H2 as [H2 | []];
      inversion H1; inversion H2; subst; reflexivity.
    + (* tags in queue are allocated *)
      intros tag Hin.
      unfold in_queue in Hin. destruct Hin as [r' Hin'].
      destruct Hin' as [H | []].
      inversion H; subst.
      unfold tag_allocated, ts1.
      apply alloc_tag_sets.

  - (* tag 5 is in queue *)
    unfold in_queue. exists r.
    left. reflexivity.

  - (* Unsafe free happened *)
    apply FTU_Buggy.
    + unfold tag_allocated, ts1.
      apply alloc_tag_sets.
    + reflexivity.

  - (* st2 is NOT well-formed (invariant broken) *)
    intro Hwf.
    destruct Hwf as [_ Halloc].
    assert (Hin: in_queue q1 5).
    { unfold in_queue. exists r. left. reflexivity. }
    specialize (Halloc 5 Hin).
    unfold tag_allocated in Halloc.
    simpl in Halloc.
    unfold ts2 in Halloc.
    unfold free_tag in Halloc.
    rewrite Z.eqb_refl in Halloc.
    discriminate Halloc.
Qed.

(* ========================================================================= *)
(* SAFE FREE PRESERVES INVARIANTS                                           *)
(* ========================================================================= *)

(** Theorem: Safe free preserves well-formedness *)
Theorem safe_free_preserves_invariant :
  forall tag st1 st2,
  FreeTagSafe tag st1 st2 ->
  Inv_MntWellFormed st2.
Proof.
  intros tag st1 st2 H.
  inversion H; subst.
  destruct H0 as [Huniq Halloc].
  split.
  - (* queue_tags_unique preserved *)
    exact Huniq.
  - (* tags in queue still allocated *)
    intros tag' Hin.
    simpl in Hin.
    assert (Halloc': tag_allocated (mnt_tags st1) tag').
    { apply Halloc. exact Hin. }
    unfold tag_allocated in *.
    simpl.
    unfold free_tag.
    destruct (Z.eqb tag' tag) eqn:E.
    + (* tag' = tag: contradiction with not_in_queue *)
      apply Z.eqb_eq in E. subst.
      exfalso.
      apply H2. exact Hin.
    + (* tag' <> tag: preserved *)
      exact Halloc'.
Qed.

(* ========================================================================= *)
(* CORRECT mntfree SEQUENCE                                                  *)
(* ========================================================================= *)

(** Theorem: Queue remove followed by safe free is correct *)
Theorem mntqrm_then_freetag_safe :
  forall tag st1 st2 st3,
  Inv_MntWellFormed st1 ->
  QueueRemove tag st1 st2 ->
  FreeTagSafe tag st2 st3 ->
  Inv_MntWellFormed st3.
Proof.
  intros tag st1 st2 st3 Hwf1 Hqrm Hfree.
  eapply safe_free_preserves_invariant.
  exact Hfree.
Qed.

(** Helper lemma: queue_remove_tag removes the tag *)
Lemma queue_remove_tag_not_in :
  forall tag r q,
  ~ In (tag, r) (queue_remove_tag tag q).
Proof.
  intros tag r q.
  induction q as [| [t r'] rest IH].
  - simpl. intro H. exact H.
  - simpl.
    destruct (Z.eqb t tag) eqn:E.
    + (* t = tag: removed, not in rest *)
      exact IH.
    + (* t <> tag: not in head, not in tail *)
      intro H.
      destruct H as [Heq | Hin].
      * inversion Heq; subst.
        apply Z.eqb_neq in E.
        contradiction.
      * contradiction.
Qed.

(** Theorem: Queue remove makes tag not in queue *)
Theorem queue_remove_clears :
  forall tag st1 st2,
  QueueRemove tag st1 st2 ->
  not_in_queue st2.(mnt_queue) tag.
Proof.
  intros tag st1 st2 H.
  inversion H; subst.
  unfold not_in_queue, in_queue.
  intro Hin.
  destruct Hin as [r Hin].
  simpl in Hin.

  apply (queue_remove_tag_not_in tag r (mnt_queue st1)).
  exact Hin.
Qed.

(** Corollary: Queue remove enables safe free *)
Theorem queue_remove_enables_free :
  forall tag st1 st2,
  Inv_MntWellFormed st1 ->
  in_queue st1.(mnt_queue) tag ->
  QueueRemove tag st1 st2 ->
  tag_allocated st2.(mnt_tags) tag /\
  not_in_queue st2.(mnt_queue) tag.
Proof.
  intros tag st1 st2 Hwf Hin Hqrm.
  inversion Hqrm; subst.
  split.
  - (* Tag still allocated after queue remove - tags field unchanged *)
    simpl.
    destruct Hwf as [_ Halloc].
    apply Halloc. exact Hin.
  - (* Tag not in queue after remove *)
    unfold not_in_queue, in_queue.
    intro Hcontra.
    destruct Hcontra as [r Hcontra].
    simpl in Hcontra.
    eapply queue_remove_tag_not_in.
    exact Hcontra.
Qed.

(* ========================================================================= *)
(* CODE FIX SPECIFICATION                                                    *)
(* ========================================================================= *)

(**
 * REQUIRED FIX for devmnt.c:1276-1291 (mntfree function)
 *
 * CURRENT CODE (BUGGY):
 * ```c
 * static void mntfree(Mntrpc *r) {
 *   ...
 *   if (mntalloc.nrpcfree >= 32) {
 *     freetag(r->request.tag);  // <-- BUG: Tag freed while RPC may be in queue
 *     unlock(&mntalloc.lock);
 *     free(r);
 *   }
 * }
 * ```
 *
 * REQUIRED FIX:
 * ```c
 * static void mntfree(Mntrpc *r) {
 *   ...
 *   if (mntalloc.nrpcfree >= 32) {
 *     mntqrm(m, r);             // <-- ADD: Remove from queue FIRST
 *     freetag(r->request.tag);  //     THEN free tag
 *     unlock(&mntalloc.lock);
 *     free(r);
 *   }
 * }
 * ```
 *
 * PROOF OBLIGATION:
 * The implementation must call mntqrm() before freetag() to satisfy
 * the FreeTagSafe precondition: not_in_queue st.(mnt_queue) tag
 *)

Print Assumptions unsafe_free_breaks_invariant.
Print Assumptions safe_free_preserves_invariant.
Print Assumptions mntqrm_then_freetag_safe.
