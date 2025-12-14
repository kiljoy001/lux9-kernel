(* Ledger-level invariants: id uniqueness and parent existence properties. *)
Require Import Coq.Lists.List.
Require Import Coq.Arith.PeanoNat.
Require Import Coq.Bool.Bool.
Import ListNotations.

Require Import proofs.capability.PermsBitmask.
Require Import proofs.capability.CapabilityModel.
Require Import proofs.capability.DerivationChain.
Require Import proofs.capability.ChainDecidability.
Import CapabilityModel.

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

(* Removing a required parent blocks any nontrivial derivation starting from that child. *)
Lemma no_step_if_parent_missing :
  forall ct c pid anc,
    cap_parent c = Some pid ->
    find_cap ct pid = None ->
    anc <> c ->
    derived_chain_table ct c anc -> False.
Proof.
  intros ct c pid anc Hpar Hmiss Hneq Hchain.
  destruct Hchain as [|c' p' pid' g Hparent Hfind Hperm Hrest].
  - contradiction.
  - assert (pid = pid') by congruence.
    subst pid'.
    rewrite Hmiss in Hfind; discriminate.
Qed.

(* Extending the table with a distinct id preserves derivation_valid for the child. *)
Lemma derivation_valid_preserved_by_cons :
  forall ct child extra pid,
    cap_parent child = Some pid ->
    cap_id extra <> pid ->
    derivation_valid ct child ->
    derivation_valid (extra :: ct) child.
Proof.
  intros ct child extra pid Hpar Hneq Hvalid.
  unfold derivation_valid in *.
  rewrite Hpar in *.
  simpl.
  destruct (Nat.eqb (cap_id extra) pid) eqn:Heq.
  - apply Nat.eqb_eq in Heq; congruence.
  - simpl.
    destruct (find_cap ct pid) as [p|] eqn:Hfind; try contradiction.
    exact Hvalid.
Qed.

End LedgerInvariants.
