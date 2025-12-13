(** * Pebble Types - Base definitions for Pebble verification
    * 
    * This module defines the core types used across all Pebble proofs.
    * Import this into conservation.v, security.v, etc.
    *)

Require Export Coq.ZArith.ZArith.
Require Export Coq.Bool.Bool.
Require Export Coq.Lists.List.
Require Export Lia.
Export ListNotations.

Open Scope Z_scope.

(* ========================================================================= *)
(* CAPABILITY MODEL                                                          *)
(* ========================================================================= *)

Definition CapId := Z.

(* ========================================================================= *)
(* STATE MODEL (matches PebbleState in pebble.h)                             *)
(* ========================================================================= *)

Record PebbleState := mkPebble {
  (* Physical memory pools *)
  colorless : Z;        (* black_budget - unallocated *)
  black : Z;            (* black_inuse - user allocations *)
  blue : Z;             (* blue_inuse - I/O buffers *)
  red : Z;              (* red_inuse - snapshots *)
  
  (* White token accounting *)
  white_pending : Z;    (* bytes authorized by verified whites *)
  white_verified : Z;   (* count of active white tokens *)
  
  (* Security tracking *)
  live_caps : list CapId;
  freed_caps : list CapId;
  next_cap_id : CapId;
  pending_authorizations : Z;
}.

(* Initial state *)
Definition initial_state (total : Z) : PebbleState :=
  mkPebble total 0 0 0 0 0 [] [] 1 0.

(* ========================================================================= *)
(* HELPER FUNCTIONS                                                          *)
(* ========================================================================= *)

Definition cap_in_list (c : CapId) (l : list CapId) : bool :=
  existsb (Z.eqb c) l.

Definition remove_cap (c : CapId) (l : list CapId) : list CapId :=
  filter (fun x => negb (Z.eqb c x)) l.

(* Key lemma about remove_cap - exported for use in other modules *)
Lemma remove_cap_not_in : forall c l, ~ In c (remove_cap c l).
Proof.
  intros c l.
  unfold remove_cap.
  induction l as [| h t IH].
  - simpl. auto.
  - simpl. destruct (Z.eqb c h) eqn:Heq.
    + simpl. apply IH.
    + simpl. intro H. destruct H.
      * apply Z.eqb_neq in Heq. symmetry in H. contradiction.
      * apply IH. assumption.
Qed.

Lemma remove_cap_preserves_other : forall c c' l,
  c <> c' -> In c' l -> In c' (remove_cap c l).
Proof.
  intros c c' l Hneq Hin.
  unfold remove_cap.
  apply filter_In.
  split.
  - assumption.
  - apply negb_true_iff.
    apply Z.eqb_neq.
    assumption.
Qed.
