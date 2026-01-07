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

Lemma test_dequeue_indices :
  forall r,
  valid_indices r ->
  valid_indices {| data := data r;
                   head := wrap (S (head r));
                   tail := tail r;
                   size := size r;
                   lock := lock r |}.
Proof.
  intros r [Hhead Htail].
  unfold valid_indices. simpl.
  split.
  - apply wrap_bound.
  - exact Htail.
Qed.