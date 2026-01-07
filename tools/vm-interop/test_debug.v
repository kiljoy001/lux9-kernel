Require Import Coq.Arith.Arith.
Require Import Lia.

Definition WRAPSIZE := 256.

Lemma test : forall n, n < WRAPSIZE -> n <= 255.
Proof.
  intros n H.
  unfold WRAPSIZE in H.
  lia.
Qed.