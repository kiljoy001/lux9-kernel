Require Import Coq.Lists.List.
Import ListNotations.

Lemma repeat_length : forall {A : Type} (x : A) (n : nat),
  length (List.repeat x n) = n.
Proof.
  intros. induction n; simpl; auto.
Qed.

Definition test_data := List.repeat None 256.

Lemma test : length test_data = 256.
Proof.
  unfold test_data.
  rewrite repeat_length.
  reflexivity.
Qed.
