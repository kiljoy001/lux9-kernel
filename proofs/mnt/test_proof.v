Require Import Coq.Lists.List.
Require Import Coq.Arith.Arith.
Import ListNotations.

Definition PathComponent : Type := nat.
Definition DOT : PathComponent := 0%nat.
Definition DOTDOT : PathComponent := 1%nat.

Goal [] = [DOTDOT] -> False.
Proof.
  intro H.
  inversion H.
Qed.
