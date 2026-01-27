Require Import Coq.Arith.Arith.
Require Import Lia.

Definition RINGSIZE := 255.
Definition WRAPSIZE := 256.

Lemma test_wrapsize : WRAPSIZE > 0.
Proof.
  unfold WRAPSIZE. lia.
Qed.

Lemma test_ringsize : RINGSIZE < WRAPSIZE.
Proof.
  unfold RINGSIZE, WRAPSIZE. lia.
Qed.

Lemma test_mod : forall n, n mod WRAPSIZE < WRAPSIZE.
Proof.
  intros n.
  apply Nat.mod_upper_bound.
  unfold WRAPSIZE. lia.
Qed.