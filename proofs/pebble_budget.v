(** * Pebble Budget Conservation Verification
    * Models the quantitative accounting in 'kernel/pebble.c'
    * 
    * Key Property: Conservation of Mass
    * The sum of all memory states must remain constant.
    * TotalMemory = FreeBudget + AllocatedBlack
    *)

Require Import Coq.ZArith.ZArith.
Open Scope Z_scope.

(* ========================================================================= *)
(* STATE MODEL *)
(* ========================================================================= *)

Record PebbleState := mkPebble {
  black_budget : Z;   (* Available memory in the pool *)
  black_inuse : Z;    (* Memory currently allocated (Black) *)
  white_pending : Z;  (* Memory reserved by Verified White tokens *)
  white_verified : Z; (* Count of active Verified White tokens *)
}.

(* ========================================================================= *)
(* TRANSITIONS (Modeled from kernel/pebble.c) *)
(* ========================================================================= *)

Inductive VerifyWhite (size : Z) (s1 s2 : PebbleState) : Prop :=
  | Verify_Success :
      size > 0 ->
      s2 = mkPebble 
             s1.(black_budget) 
             s1.(black_inuse) 
             (s1.(white_pending) + size) 
             (s1.(white_verified) + 1) ->
      VerifyWhite size s1 s2.

Inductive BlackAlloc (size : Z) (s1 s2 : PebbleState) : Prop :=
  | Alloc_Success :
      size > 0 ->
      s1.(white_verified) > 0 ->
      s1.(white_pending) >= size ->
      s1.(black_budget) >= size ->
      s2 = mkPebble
             (s1.(black_budget) - size)
             (s1.(black_inuse) + size)
             (s1.(white_pending) - size)
             (s1.(white_verified) - 1) ->
      BlackAlloc size s1 s2.

Inductive BlackAlloc_Rollback (size : Z) (s1 s2 : PebbleState) : Prop :=
  | Alloc_Fail_Rollback :
      let temp_budget := s1.(black_budget) - size in
      let temp_inuse := s1.(black_inuse) + size in
      let temp_pending := s1.(white_pending) - size in
      let temp_verified := s1.(white_verified) - 1 in
      s2 = mkPebble
             (temp_budget + size)
             (temp_inuse - size)
             (temp_pending + size)
             (temp_verified + 1) ->
      BlackAlloc_Rollback size s1 s2.

Inductive BlackFree (size : Z) (s1 s2 : PebbleState) : Prop :=
  | Free_Success :
      size > 0 ->
      s2 = mkPebble
             (s1.(black_budget) + size)
             (s1.(black_inuse) - size)
             s1.(white_pending)
             s1.(white_verified) ->
      BlackFree size s1 s2.

(* ========================================================================= *)
(* INVARIANTS *)
(* ========================================================================= *)

Definition Invariant_Physical_Conservation (s : PebbleState) (initial_mem : Z) : Prop :=
  s.(black_budget) + s.(black_inuse) = initial_mem.

(* ========================================================================= *)
(* PROOFS *)
(* ========================================================================= *)

Theorem Alloc_Preserves_Physical_Mass :
  forall s1 s2 size initial,
  Invariant_Physical_Conservation s1 initial ->
  BlackAlloc size s1 s2 ->
  Invariant_Physical_Conservation s2 initial.
Proof.
  intros s1 s2 size initial Hinv Hstep.
  inversion Hstep. subst.
  unfold Invariant_Physical_Conservation in *.
  simpl.
  rewrite <- Hinv.
  
  (* budget - size + (inuse + size) = budget + inuse *)
  rewrite Z.sub_eq_add_neg.
  rewrite Z.add_assoc.
  rewrite (Z.add_comm (-size) (black_inuse s1 + size)).
  rewrite Z.add_assoc.
  rewrite (Z.add_comm (-size) (black_inuse s1)).
  rewrite <- Z.add_assoc.
  rewrite Z.add_opp_diag_r.
  rewrite Z.add_0_r.
  reflexivity.
Qed.

Theorem Free_Preserves_Physical_Mass :
  forall s1 s2 size initial,
  Invariant_Physical_Conservation s1 initial ->
  BlackFree size s1 s2 ->
  Invariant_Physical_Conservation s2 initial.
Proof.
  intros s1 s2 size initial Hinv Hstep.
  inversion Hstep. subst.
  unfold Invariant_Physical_Conservation in *.
  simpl.
  rewrite <- Hinv.
  
  (* budget + size + (inuse - size) = budget + inuse *)
  rewrite Z.sub_eq_add_neg.
  rewrite Z.add_assoc.
  rewrite (Z.add_comm size (black_inuse s1 + -size)).
  rewrite Z.add_assoc.
  rewrite (Z.add_comm size (black_inuse s1)).
  rewrite <- Z.add_assoc.
  rewrite Z.add_opp_diag_l.
  rewrite Z.add_0_r.
  reflexivity.
Qed.

Theorem Rollback_Is_Identity :
  forall s1 s2 size,
  BlackAlloc_Rollback size s1 s2 ->
  s1 = s2.
Proof.
  intros s1 s2 size Hstep.
  inversion Hstep. subst.
  destruct s1.
  f_equal; simpl; try rewrite Z.sub_eq_add_neg.
  - rewrite Z.add_assoc. rewrite (Z.add_comm (-size) size). rewrite Z.add_opp_diag_l. apply Z.add_0_r.
  - rewrite Z.add_assoc. rewrite (Z.add_comm size (-size)). rewrite Z.add_opp_diag_l. apply Z.add_0_r.
  - rewrite Z.add_assoc. rewrite (Z.add_comm (-size) size). rewrite Z.add_opp_diag_l. apply Z.add_0_r.
  - rewrite Z.add_assoc. rewrite (Z.add_comm (-1) 1). rewrite Z.add_opp_diag_l. apply Z.add_0_r.
Qed.

Theorem Verify_Ignores_Physical_Mass :
  forall s1 s2 size initial,
  Invariant_Physical_Conservation s1 initial ->
  VerifyWhite size s1 s2 ->
  Invariant_Physical_Conservation s2 initial.
Proof.
  intros s1 s2 size initial Hinv Hstep.
  inversion Hstep. subst.
  unfold Invariant_Physical_Conservation in *.
  simpl.
  assumption.
Qed.