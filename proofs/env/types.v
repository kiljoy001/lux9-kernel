(** * Environment Device Types - Base definitions for devenv verification
    * 
    * Models the Egrp (environment group) and Evalue (environment value)
    * structures from devenv.c
    *)

Require Export Coq.ZArith.ZArith.
Require Export Coq.Bool.Bool.
Require Export Coq.Lists.List.
Require Export Lia.
Export ListNotations.

Open Scope Z_scope.

(* ========================================================================= *)
(* CONSTANTS FROM devenv.c                                                   *)
(* ========================================================================= *)

Definition Maxenvsize : Z := 1048576.  (* 1 MB *)
Definition Maxvalsize : Z := Maxenvsize / 2.
Definition DELTAENV : Z := 32.
Definition ENVHASH : Z := 31.

(* ========================================================================= *)
(* EVALUE - Environment value entry                                          *)
(* ========================================================================= *)

Record EvalueState := mkEvalue {
  ev_name : Z;        (* hash of name for simplicity *)
  ev_len : Z;         (* length of value *)
  ev_vers : Z;        (* version *)
  ev_path : Z;        (* qid path *)
}.

(* ========================================================================= *)
(* EGRP - Environment group state                                            *)
(* ========================================================================= *)

Record EgrpState := mkEgrp {
  eg_ref : Z;         (* reference count *)
  eg_nent : Z;        (* number of entries *)
  eg_alloc : Z;       (* total allocated bytes *)
  eg_vers : Z;        (* version *)
  eg_path : Z;        (* next qid path *)
  eg_low : Z;         (* low water mark for free slots *)
}.

(* Initial Egrp state (after newegrp or envinit) *)
Definition initial_egrp : EgrpState :=
  mkEgrp 1 0 0 0 0 0.

(* ========================================================================= *)
(* ALLOCATION COMPUTATION                                                    *)
(* ========================================================================= *)

(* Adding an entry increases allocation *)
Definition add_entry_alloc (eg : EgrpState) (size : Z) : EgrpState :=
  mkEgrp eg.(eg_ref) (eg.(eg_nent) + 1) (eg.(eg_alloc) + size)
         (eg.(eg_vers) + 1) (eg.(eg_path) + 1) eg.(eg_low).

(* Removing an entry decreases allocation *)
Definition remove_entry_alloc (eg : EgrpState) (size : Z) : EgrpState :=
  mkEgrp eg.(eg_ref) eg.(eg_nent) (eg.(eg_alloc) - size)
         (eg.(eg_vers) + 1) eg.(eg_path) eg.(eg_low).

(* ========================================================================= *)
(* REFERENCE COUNTING                                                        *)
(* ========================================================================= *)

Definition incr_ref (eg : EgrpState) : EgrpState :=
  mkEgrp (eg.(eg_ref) + 1) eg.(eg_nent) eg.(eg_alloc)
         eg.(eg_vers) eg.(eg_path) eg.(eg_low).

Definition decr_ref (eg : EgrpState) : EgrpState :=
  mkEgrp (eg.(eg_ref) - 1) eg.(eg_nent) eg.(eg_alloc)
         eg.(eg_vers) eg.(eg_path) eg.(eg_low).

(* ========================================================================= *)
(* INVARIANTS                                                                *)
(* ========================================================================= *)

Definition Inv_RefPositive (eg : EgrpState) : Prop :=
  eg.(eg_ref) >= 1.

Definition Inv_AllocBounded (eg : EgrpState) : Prop :=
  eg.(eg_alloc) >= 0 /\ eg.(eg_alloc) <= Maxenvsize.

Definition Inv_EntriesNonNeg (eg : EgrpState) : Prop :=
  eg.(eg_nent) >= 0 /\ eg.(eg_low) >= 0.

Definition Inv_WellFormed (eg : EgrpState) : Prop :=
  Inv_RefPositive eg /\ Inv_AllocBounded eg /\ Inv_EntriesNonNeg eg.

(* ========================================================================= *)
(* BASIC LEMMAS                                                              *)
(* ========================================================================= *)

Lemma initial_egrp_wellformed : Inv_WellFormed initial_egrp.
Proof.
  unfold Inv_WellFormed, Inv_RefPositive, Inv_AllocBounded, Inv_EntriesNonNeg.
  unfold initial_egrp. simpl.
  unfold Maxenvsize. lia.
Qed.

Lemma incr_ref_preserves_positive : forall eg,
  Inv_RefPositive eg -> Inv_RefPositive (incr_ref eg).
Proof.
  intros eg H. unfold Inv_RefPositive, incr_ref in *. simpl. lia.
Qed.

Lemma decr_ref_preserves_positive : forall eg,
  Inv_RefPositive eg -> eg.(eg_ref) > 1 -> Inv_RefPositive (decr_ref eg).
Proof.
  intros eg H Hgt. unfold Inv_RefPositive, decr_ref in *. simpl. lia.
Qed.
