(* Core capability structures and parent-child derivation checks. *)
Require Import Coq.Lists.List.
Require Import Coq.Arith.PeanoNat.
Import ListNotations.

Require Import proofs.capability.PermsBitmask.

Section CapabilityModel.

Record Capability := {
  cap_id : nat;
  cap_perms : nat; (* permission bitmask *)
  cap_parent : option nat
}.

Definition cap_table := list Capability.

(* Lookup by identifier. *)
Fixpoint find_cap (ct : cap_table) (id : nat) : option Capability :=
  match ct with
  | [] => None
  | c :: cs => if Nat.eqb (cap_id c) id then Some c else find_cap cs id
  end.

Lemma find_cap_id :
  forall ct id c,
    find_cap ct id = Some c ->
    cap_id c = id.
Proof.
  induction ct; simpl; intros id c Hfind; try discriminate.
  destruct (Nat.eqb (cap_id a) id) eqn:Heq.
  - inversion Hfind; subst. apply Nat.eqb_eq in Heq; assumption.
  - apply IHct in Hfind; assumption.
Qed.

(* Immediate derivation rule. *)
Inductive derived_from (ct : cap_table) : Capability -> Capability -> Prop :=
| DeriveIntro :
    forall child parent,
      cap_parent child = Some (cap_id parent) ->
      perms_subset (cap_perms child) (cap_perms parent) ->
      derived_from ct child parent.

Lemma derived_perm_monotonic :
  forall ct c p,
    derived_from ct c p ->
    perms_subset (cap_perms c) (cap_perms p).
Proof.
  intros ct c p H.
  inversion H; subst; auto.
Qed.

(* Ledger-style validation: parent must exist and grant a superset of perms. *)
Definition derivation_valid (ct : cap_table) (child : Capability) : Prop :=
  match cap_parent child with
  | None => False
  | Some pid =>
      match find_cap ct pid with
      | None => False
      | Some parent =>
          perms_subset (cap_perms child) (cap_perms parent)
      end
  end.

Lemma derivation_valid_implies_parent_found :
  forall ct child pid,
    cap_parent child = Some pid ->
    derivation_valid ct child ->
    exists parent, find_cap ct pid = Some parent.
Proof.
  intros ct child pid Hparent Hvalid.
  unfold derivation_valid in Hvalid.
  rewrite Hparent in Hvalid.
  destruct (find_cap ct pid) eqn:Hf; try contradiction.
  eauto.
Qed.

Lemma derivation_valid_perm_subset :
  forall ct child pid parent,
    cap_parent child = Some pid ->
    find_cap ct pid = Some parent ->
    derivation_valid ct child ->
    perms_subset (cap_perms child) (cap_perms parent).
Proof.
  intros ct child pid parent Hpar Hfind Hvalid.
  unfold derivation_valid in Hvalid.
  rewrite Hpar, Hfind in Hvalid.
  exact Hvalid.
Qed.

Lemma derivation_invalid_without_parent :
  forall ct child pid,
    cap_parent child = Some pid ->
    find_cap ct pid = None ->
    derivation_valid ct child = False.
Proof.
  intros ct child pid Hpar Hfind.
  unfold derivation_valid.
  rewrite Hpar, Hfind; reflexivity.
Qed.

End CapabilityModel.
