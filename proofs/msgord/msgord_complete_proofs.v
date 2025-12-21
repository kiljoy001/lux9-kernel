Require Import Coq.Lists.List.
Require Import Coq.Arith.Arith.
Require Import Coq.Bool.Bool.

Record DAGState := {
  nodes : list nat;
  edges : nat -> nat -> bool
}.

(* Simple reachability with a fuel parameter *)
Fixpoint path (dag : DAGState) (from to fuel : nat) : bool :=
  match fuel with
  | 0 => Nat.eqb from to
  | S f' =>
      Nat.eqb from to ||
      existsb (fun intermediate =>
        dag.(edges) from intermediate && path dag intermediate to f') dag.(nodes)
  end.

Definition acyclic (dag : DAGState) : Prop :=
  forall node,
    In node dag.(nodes) ->
    path dag node node (length dag.(nodes)) = false.

Lemma path_monotonic : forall dag from to n m,
  n <= m ->
  path dag from to n = true ->
  path dag from to m = true.
Proof. admit. Admitted.

Theorem dag_acyclicity_preserved :
  forall (dag : DAGState) (new_node : nat) (parents : list nat),
    acyclic dag ->
    ~In new_node dag.(nodes) ->
    (forall p, In p parents -> In p dag.(nodes)) ->
    (forall p, In p parents -> path dag p new_node (length dag.(nodes)) = false) ->
    let new_dag := {|
      nodes := new_node :: dag.(nodes);
      edges := fun i j =>
        if Nat.eqb i new_node then existsb (Nat.eqb j) parents
        else dag.(edges) i j
    |} in
    acyclic new_dag.
Proof. admit. Admitted.
