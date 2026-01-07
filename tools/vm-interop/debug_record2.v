Require Import Coq.Lists.List.
Require Import Coq.Arith.Arith.

Record TestRing := {
  commands : list nat;
  head : nat
}.

Definition test_fn (r : TestRing) : nat :=
  length (commands r).

Check test_fn.