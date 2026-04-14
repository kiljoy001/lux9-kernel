(* Bitmask-based permission model mirroring kernel flags. *)
Require Import Coq.Arith.PeanoNat.
Require Import Coq.NArith.BinNat.
Require Import Coq.Bool.Bool.

Section PermsBitmask.

(* Individual permission bits. *)
Definition perm_read : nat := 1.
Definition perm_write : nat := 2.
Definition perm_exec : nat := 4.
Definition perm_transfer : nat := 8.
Definition perm_grant : nat := 16.

Definition has_perm (mask bit : nat) : bool :=
  Nat.eqb (Nat.land mask bit) bit.

Definition perms_subset (a b : nat) : Prop :=
  forall bit,
    (bit = perm_read \/
     bit = perm_write \/
     bit = perm_exec \/
     bit = perm_transfer \/
     bit = perm_grant) ->
    has_perm a bit = true -> has_perm b bit = true.

Lemma perms_subset_refl : forall m, perms_subset m m.
Proof.
  unfold perms_subset; intros; auto.
Qed.

Lemma perms_subset_trans : forall a b c,
  perms_subset a b -> perms_subset b c -> perms_subset a c.
Proof.
  unfold perms_subset; intros a b c Hab Hbc bit Hbit Hha.
  apply Hbc; auto.
Qed.

(* Simple decidable check over our fixed set of bits. *)
Definition perms_subset_dec (a b : nat) : bool :=
  has_perm a perm_read && has_perm b perm_read &&
  has_perm a perm_write && has_perm b perm_write &&
  has_perm a perm_exec && has_perm b perm_exec &&
  has_perm a perm_transfer && has_perm b perm_transfer &&
  has_perm a perm_grant && has_perm b perm_grant.

End PermsBitmask.
