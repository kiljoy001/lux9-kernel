Require Import Coq.Arith.Arith.
Require Import Lia.

Definition WRAPSIZE := 256.

Lemma test1 : forall n, n < WRAPSIZE -> n <= 255.
Proof.
  intros. unfold WRAPSIZE in H. lia.
Qed.

Lemma test2 : forall n, n < WRAPSIZE -> S n <= WRAPSIZE.
Proof.
  intros. unfold WRAPSIZE in *. lia.
Qed.

Lemma test3 : forall n, n < 256 -> S n <= 256.
Proof.
  intros. lia.
Qed.