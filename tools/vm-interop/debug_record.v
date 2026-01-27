Require Import Coq.Lists.List.
Require Import Coq.Arith.Arith.
Require Import Coq.Strings.String.

Record TestRing := {
  cmds : list string;
  head : nat
}.

Definition test_fn (r : TestRing) : nat :=
  length (cmds r).

Check test_fn.