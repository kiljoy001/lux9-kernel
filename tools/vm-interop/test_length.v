Require Import Coq.Lists.List.
Require Import Coq.Arith.Arith.

Record TestBuffer := {
  items : list nat
}.

Definition test_empty (r : TestBuffer) : bool :=
  Nat.eqb (length r.(items)) 0.

Check test_empty.