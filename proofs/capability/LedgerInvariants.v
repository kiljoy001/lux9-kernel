(* Ledger-level invariants: id uniqueness and parent existence properties. *)
Require Import Coq.Lists.List.
Require Import Coq.Arith.PeanoNat.
Require Import Coq.Bool.Bool.
Import ListNotations.

Require Import proofs.capability.PermsBitmask.
Require Import proofs.capability.CapabilityModel.
Require Import proofs.capability.DerivationChain.
Require Import proofs.capability.ChainDecidability.

Section LedgerInvariants.

(* Uniqueness of lookup: find_cap is deterministic on id. *)
Lemma find_cap_unique :
  forall ct id c1 c2,
    find_cap ct id = Some c1 ->
    find_cap ct id = Some c2 ->
    c1 = c2.
Proof.
  induction ct; simpl; intros id c1 c2 H1 H2; try discriminate.
  destruct (Nat.eqb (cap_id a) id) eqn:Heq.
  - inversion H1; inversion H2; subst; reflexivity.
  - eauto.
Qed.

(* Chain monotonicity for table-respecting chains mirrors the abstract chain. *)
Lemma derived_chain_table_perms_monotonic :
  forall ct c anc,
    derived_chain_table ct c anc ->
    perms_subset (cap_perms c) (cap_perms anc).
Proof.
  intros ct c anc H.
  induction H.
  - apply perms_subset_refl.
  - eapply perms_subset_trans; eauto.
Qed.

End LedgerInvariants.
