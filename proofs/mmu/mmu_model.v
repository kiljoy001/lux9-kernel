(** Abstract MMU/segment model for 9front pc64 port.
    Focus: user/kernel separation, alignment, and non-overlap of mappings. *)

From Coq Require Import List Arith Lia.
Import ListNotations.

(* Constants taken from the C side (mem.h) but kept abstract. *)
Parameter KZERO BY2PG : nat.
Axiom BY2PG_pos : BY2PG > 0.

Definition addr := nat.

Record pte := {
  va : addr;
  pa : addr;
  user : bool;
  writable : bool
}.

Definition page_range (a : addr) : addr * addr := (a, a + BY2PG).

Definition disjoint_range (a b : addr) : Prop :=
  a + BY2PG <= b \/ b + BY2PG <= a.

Definition page_aligned (a : addr) : Prop := a mod BY2PG = 0.

Definition canonical_user (a : addr) : Prop := a < KZERO.
Definition canonical_kernel (a : addr) : Prop := KZERO <= a.

Definition wf_entry (e : pte) : Prop :=
  page_aligned (va e) /\
  page_aligned (pa e) /\
  (user e = true -> canonical_user (va e)) /\
  (user e = false -> canonical_kernel (va e)).

Definition pairwise_disjoint (l : list pte) : Prop :=
  forall e1 e2,
    In e1 l -> In e2 l -> e1 <> e2 ->
    disjoint_range (va e1) (va e2).

Definition wf (pt : list pte) : Prop :=
  Forall wf_entry pt /\ pairwise_disjoint pt.

Definition disjoint_from_va (a : addr) (e : pte) : Prop :=
  disjoint_range a (va e).

Lemma Forall_In : forall {A} (P : A -> Prop) (l : list A) (x : A),
  Forall P l -> In x l -> P x.
Proof.
  intros A P l; induction l; simpl; intros x HForall Hin; try tauto.
  inversion HForall; subst.
  destruct Hin as [Hin | Hin]; subst; auto.
Qed.

Lemma pairwise_tail :
  forall h t, pairwise_disjoint (h :: t) -> pairwise_disjoint t.
Proof.
  unfold pairwise_disjoint; intros h t Hpair e1 e2 Hin1 Hin2 Hneq.
  apply Hpair; try (right; assumption); assumption.
Qed.

Lemma disjoint_sym : forall a b, disjoint_range a b -> disjoint_range b a.
Proof.
  unfold disjoint_range; firstorder lia.
Qed.

Lemma disjoint_self : forall a, ~ disjoint_range a a.
Proof.
  unfold disjoint_range; intros a [H|H].
  all: assert (BY2PG > 0) by apply BY2PG_pos; lia.
Qed.

Section MapUser.
  Variable pt : list pte.

  Inductive map_user : addr -> addr -> list pte -> Prop :=
  | MapUser :
      forall v p,
        canonical_user v ->
        page_aligned v ->
        page_aligned p ->
        (forall e, In e pt -> disjoint_from_va v e) ->
        map_user v p ({| va := v; pa := p; user := true; writable := true |} :: pt).
End MapUser.

Lemma map_user_preserves_wf :
  forall pt v p pt',
    wf pt ->
    map_user pt v p pt' ->
    wf pt'.
Proof.
  intros pt v p pt' [Hwf Hdisj] Hmap.
  inversion Hmap; subst; clear Hmap.
  split.
  - constructor.
    + repeat split; try assumption.
      * simpl; intros _; assumption.
      * simpl; intros Hfalse; discriminate Hfalse.
    + assumption.
  - intros e1 e2 Hin1 Hin2 Hneq.
    simpl in Hin1, Hin2.
    destruct Hin1 as [Hin1 | Hin1], Hin2 as [Hin2 | Hin2]; subst.
    * exfalso; apply Hneq; reflexivity.
    * eapply H2; eauto.
    * apply disjoint_sym; eapply H2; eauto.
    * apply Hdisj; assumption.
Qed.

Lemma wf_user_entries_below_kzero :
  forall pt e,
    wf pt ->
    In e pt ->
    user e = true ->
    va e < KZERO.
Proof.
  intros pt e [Hfor _] Hin Huser.
  pose proof (@Forall_In pte wf_entry pt e Hfor Hin) as Hwfe.
  destruct Hwfe as [_ [_ [Hu _]]]. apply Hu; assumption.
Qed.

Lemma wf_kernel_entries_not_below_kzero :
  forall pt e,
    wf pt ->
    In e pt ->
    user e = false ->
    KZERO <= va e.
Proof.
  intros pt e [Hfor _] Hin Huser.
  pose proof (@Forall_In pte wf_entry pt e Hfor Hin) as Hwfe.
  destruct Hwfe as [_ [_ [_ Hk]]]. apply Hk; assumption.
Qed.

(* A small safety theorem capturing the intended invariant:
   adding a user mapping cannot overlap an existing mapping. *)
Lemma map_user_disjoint_preserves :
  forall pt v p pt' e,
    wf pt ->
    map_user pt v p pt' ->
    In e pt ->
    disjoint_from_va v e.
Proof.
  intros pt v p pt' e _ Hmap Hin.
  inversion Hmap; subst; eauto.
Qed.

Hint Resolve map_user_disjoint_preserves wf_user_entries_below_kzero
     wf_kernel_entries_not_below_kzero : mmu.

(* Canonical-address corollary mirroring the C expectation:
   a well-formed table never mixes user VAs above KZERO. *)
Corollary wf_user_kernel_separation :
  forall pt e1 e2,
    wf pt ->
    In e1 pt -> In e2 pt ->
    user e1 = true -> user e2 = false ->
    va e1 < KZERO /\ KZERO <= va e2.
Proof.
  intros; split; eauto with mmu.
Qed.
