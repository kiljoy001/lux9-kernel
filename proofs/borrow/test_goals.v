Require Import ZArith List.
Import ListNotations.
Open Scope Z_scope.

(* Simple test: In p [] should be provably False *)
Lemma in_nil_test : forall (p : Z), In p [] -> False.
Proof. intros p H. destruct H. Qed.

(* Test: In p [x] means p = x *)
Lemma in_singleton : forall (p x : Z), In p [x] -> p = x.
Proof. intros p x H. destruct H as [Heq | HIn]. exact Heq. destruct HIn. Qed.

Print in_nil_test.
Print in_singleton.
