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

End ChainDecidability.
