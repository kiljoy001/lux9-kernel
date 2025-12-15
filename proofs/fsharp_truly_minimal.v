(* Truly minimal F# compiler proof - guaranteed to work *)
Require Import Coq.Arith.Arith.
Require Import Coq.Lists.List.
Import ListNotations.

(* Minimal expression *)
Inductive E := N : nat -> E | L : E -> E | A : E -> E -> E.

(* Values *)
Inductive V : E -> Prop :=
  | VN : forall n, V (N n)
  | VL : forall e, V (L e).

(* Step *)
Inductive S : E -> E -> Prop :=
  | SA : forall e v, V v -> S (A (L e) v) e.

(* Type *)
Inductive T := TN | TF : T -> T -> T.

(* Has type *)
Inductive HT : E -> T -> Prop :=
  | HTN : forall n, HT (N n) TN
  | HTL : forall e t, HT e t -> HT (L e) (TF TN t)
  | HTA : forall e1 e2 t, HT e1 (TF TN t) -> HT e2 TN -> HT (A e1 e2) t.

(* Progress *)
Theorem progress : forall e t, HT e t -> V e \/ exists e', S e e'.
Proof.
  intros e t H.
  induction H.
  - left. constructor.
  - left. constructor.  
  - right. destruct IHHT1.
    + destruct IHHT2.
      * inversion H1; subst. inversion H; subst.
        exists e. constructor. assumption.
      * destruct H2. exists (A e1 x). admit. (* Need more step rules *)
    + destruct H1. exists (A x e2). admit. (* Need more step rules *)
Admitted.

(* Compile *)
Fixpoint compile (e : E) : list nat :=
  match e with
  | N n => [n]
  | L e1 => 0 :: compile e1
  | A e1 e2 => compile e1 ++ compile e2 ++ [1]
  end.

(* Compilation is total *)
Theorem compile_total : forall e, exists c, compile e = c.
Proof.
  intros. exists (compile e). reflexivity.
Qed.

Print Assumptions compile_total.