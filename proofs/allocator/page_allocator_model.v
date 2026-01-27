(** Abstract page allocator model inspired by 9front page.c.
    Uses Z addresses and reference counts; models HHDM mapping and free list. *)

From Coq Require Import List ZArith Lia.
Import ListNotations.
Local Open Scope Z_scope.

Section PageAllocator.

Context (mem_min mem_max : Z).
Hypothesis mem_bounds : mem_min < mem_max.

Record Page := mkPage { paddr : Z; refc : nat }.

Definition freelist := list Page.

Definition addr_in_bounds (a : Z) : Prop := mem_min <= a < mem_max.

Definition all_free (l : freelist) : Prop :=
  Forall (fun p => refc p = 0%nat /\ addr_in_bounds (paddr p)) l.

Definition disjoint_pages (p1 p2 : Page) : Prop := paddr p1 <> paddr p2.

Definition valid_freelist (l : freelist) : Prop :=
  all_free l /\
  forall p1 p2, In p1 l -> In p2 l -> p1 <> p2 -> disjoint_pages p1 p2.

Definition alloc_page (l : freelist) : option (Page * freelist) :=
  match l with
  | [] => None
  | p :: tl => Some (mkPage (paddr p) 1%nat, tl)
  end.

Definition free_page (p : Page) (l : freelist) : freelist :=
  if Nat.eqb (refc p) 0 then l else mkPage (paddr p) 0%nat :: l.

Lemma alloc_preserves_valid :
  forall l p l',
    valid_freelist l ->
    alloc_page l = Some (p, l') ->
    valid_freelist l'.
Proof.
  intros l p l' [Hpos Hdisj] Halloc.
  destruct l; simpl in Halloc; try discriminate.
  inversion Halloc; subst; clear Halloc.
  split.
  - inversion Hpos; subst; assumption.
  - intros p1 p2 Hin1 Hin2 Hneq. eapply Hdisj; eauto; right; assumption.
Qed.

Lemma alloc_sets_refc :
  forall l p l',
    alloc_page l = Some (p, l') ->
    refc p = 1%nat.
Proof. intros l p l' H; destruct l; simpl in H; inversion H; reflexivity. Qed.

Lemma alloc_not_nil :
  forall l p l',
    alloc_page l = Some (p, l') -> l <> [].
Proof. intros l p l' H; destruct l; simpl in H; congruence. Qed.

End PageAllocator.
