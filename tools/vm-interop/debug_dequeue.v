Require Import Coq.Lists.List.
Require Import Coq.Arith.Arith.
Require Import Lia.

Definition RINGSIZE := 255.
Definition WRAPSIZE := 256.

Definition wrap (n : nat) : nat := n mod WRAPSIZE.

Lemma wrap_bound : forall n, wrap n < WRAPSIZE.
Proof.
  intros n.
  unfold wrap.
  apply Nat.mod_upper_bound.
  unfold WRAPSIZE. lia.
Qed.

Record RingBuffer := {
  data : list nat;
  head : nat;
  tail : nat;
  size : nat;
  lock : bool
}.

Definition valid_indices (r : RingBuffer) : Prop :=
  r.(head) < WRAPSIZE /\ r.(tail) < WRAPSIZE.

Lemma dequeue_head_valid : forall r,
  valid_indices r ->
  wrap (S r.(head)) < WRAPSIZE.
Proof.
  intros r H.
  apply wrap_bound.
Qed.