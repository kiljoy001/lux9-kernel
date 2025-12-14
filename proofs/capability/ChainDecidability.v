(* Boolean decision procedures for derivation chains, plus links to the
   inductive specifications. *)
Require Import Coq.Lists.List.
Require Import Coq.Arith.PeanoNat.
Require Import Coq.Bool.Bool.

Require Import proofs.capability.PermsBitmask.
Require Import proofs.capability.CapabilityModel.
Require Import proofs.capability.DerivationChain.
Require Import proofs.capability.Decidability.

Section ChainDecidability.

(* A table-respecting chain: every step must resolve a parent via the table,
   and respect the permission subset. This is the constructive counterpart
   of [derived_chain] that captures the lookup side-condition. *)
Inductive derived_chain_table (ct : cap_table) : Capability -> Capability -> Prop :=
| DCT_Refl : forall c, derived_chain_table ct c c
| DCT_Step : forall c p pid g,
    cap_parent c = Some pid ->
    find_cap ct pid = Some p ->
    perms_subset (cap_perms c) (cap_perms p) ->
    derived_chain_table ct p g ->
    derived_chain_table ct c g.

Lemma derived_chain_table_implies_derived_chain :
  forall ct c anc,
    derived_chain_table ct c anc ->
    derived_chain ct c anc.
Proof.
  intros ct c anc H.
  induction H.
  - constructor.
  - pose proof (proofs.capability.CapabilityModel.find_cap_id _ _ _ H0) as Hid.
    eapply DC_Step.
    + apply DeriveIntro.
      * rewrite Hid; exact H.
      * exact H1.
    + exact IHderived_chain_table.
Qed.

(* Bounded boolean search for a derivation chain. Fuel guarantees termination. *)
Fixpoint derived_chainb (fuel : nat) (ct : cap_table) (c anc : Capability) : bool :=
  match fuel with
  | 0 => false
  | S fuel' =>
      if Nat.eqb (cap_id c) (cap_id anc) then true else
      match cap_parent c with
      | None => false
      | Some pid =>
          match find_cap ct pid with
          | None => false
          | Some p =>
              if perms_subsetb (cap_perms c) (cap_perms p)
              then derived_chainb fuel' ct p anc
              else false
          end
      end
  end.

(* Completeness: any table-respecting chain can be witnessed by some fuel. *)
Lemma derived_chain_table_to_bool :
  forall ct c anc,
    derived_chain_table ct c anc ->
    exists fuel, derived_chainb fuel ct c anc = true.
Proof.
  intros ct c anc Hchain.
  induction Hchain.
  - exists 1. simpl. rewrite Nat.eqb_refl; reflexivity.
  - destruct IHHchain as [fuel Hfuel].
    (* If ids already match, we can terminate immediately; otherwise follow the parent. *)
    destruct (Nat.eqb (cap_id c) (cap_id g)) eqn:Heq.
    + exists 1. simpl. rewrite Heq; reflexivity.
    + exists (S fuel).
      simpl. rewrite Heq.
      rewrite H, H0.
      assert (Hperm_true : perms_subsetb (cap_perms c) (cap_perms p) = true).
      { apply perms_subsetb_correct. exact H1. }
      rewrite Hperm_true.
      exact Hfuel.
Qed.

(* Depth as a relation: length of a table-respecting chain. *)
Inductive dct_depth_of (ct : cap_table) : Capability -> Capability -> nat -> Prop :=
| DCT_Depth_Refl : forall c, dct_depth_of ct c c 0
| DCT_Depth_Step : forall c p pid g n,
    cap_parent c = Some pid ->
    find_cap ct pid = Some p ->
    perms_subset (cap_perms c) (cap_perms p) ->
    dct_depth_of ct p g n ->
    dct_depth_of ct c g (S n).

Lemma derived_chain_table_has_depth :
  forall ct c anc,
    derived_chain_table ct c anc ->
    exists n, dct_depth_of ct c anc n.
Proof.
  intros ct c anc H.
  induction H.
  - exists 0; constructor.
  - destruct IHderived_chain_table as [n Hn].
    exists (S n); econstructor; eauto.
Qed.

Lemma dct_depth_fuel_suffices :
  forall ct c anc n,
    dct_depth_of ct c anc n ->
    derived_chainb (S n) ct c anc = true.
Proof.
  induction 1; simpl.
  - rewrite Nat.eqb_refl; reflexivity.
  - destruct (Nat.eqb (cap_id c) (cap_id g)) eqn:Heq; simpl.
    + reflexivity.
    + rewrite H, H0.
      assert (Hperm_true : perms_subsetb (cap_perms c) (cap_perms p) = true) by
          (apply perms_subsetb_correct; exact H1).
      rewrite Hperm_true.
      exact IHdct_depth_of.
Qed.

Corollary derived_chain_table_bool_complete :
  forall ct c anc (h : derived_chain_table ct c anc),
    exists fuel, derived_chainb fuel ct c anc = true.
Proof.
  intros ct c anc h.
  destruct (derived_chain_table_has_depth _ _ _ h) as [n Hn].
  exists (S n).
  apply dct_depth_fuel_suffices; assumption.
Qed.

End ChainDecidability.
