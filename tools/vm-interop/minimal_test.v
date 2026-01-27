Require Import Coq.Lists.List.
Require Import Coq.Arith.Arith.

Record SimpleRing := {
  data : list nat;
  size : nat
}.

Definition get_length (r : SimpleRing) : nat :=
  length (data r).

Check get_length.