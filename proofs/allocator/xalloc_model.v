(** Abstract model of 9front xalloc hole allocator.
    Uses Z addresses with global min/max bounds to match the real system. *)

From Coq Require Import List ZArith Lia.
Import ListNotations.
Local Open Scope Z_scope.

Section AllocatorModel.

Context (mem_min mem_max : Z).
Hypothesis mem_bounds : mem_min < mem_max.

Record Hole := mkHole { h_start : Z; h_size : Z }.

Definition hole_end h := h_start h + h_size h.

Definition hole_in_bounds h : Prop :=
  mem_min <= h_start h /\
  hole_end h < mem_max /\
  h_size h > 0.

Definition disjoint (h1 h2 : Hole) : Prop :=
  hole_end h1 <= h_start h2 \/ 
  hole_end h2 <= h_start h1.

Lemma disjoint_sym : forall a b, disjoint a b -> disjoint b a.
Proof. unfold disjoint; firstorder lia. Qed.

Definition inside (sub outer : Hole) : Prop :=
  mem_min <= h_start sub /\
  h_start sub >= h_start outer /\
  hole_end sub <= hole_end outer /\
  h_size sub > 0.

Definition valid_holes (hs : list Hole) : Prop :=
  Forall hole_in_bounds hs /\ 
  NoDup hs /\ 
  forall h1 h2, In h1 hs -> In h2 hs -> h1 <> h2 -> disjoint h1 h2.

(** The list of holes is sorted by address. 
    This is maintained by xalloc.c and simplifies disjointness proofs. *)
Inductive sorted : list Hole -> Prop :=
| sorted_nil : sorted []
| sorted_one : forall h, sorted [h]
| sorted_cons : forall h1 h2 tl,
    hole_end h1 <= h_start h2 ->
    sorted (h2 :: tl) ->
    sorted (h1 :: h2 :: tl).

Fixpoint alloc_first_fit (hs : list Hole) (req : Z) : option (Hole * list Hole) :=
  match hs with
  | [] => None
  | h :: tl =>
      if Z.leb req 0 then None
      else if Z.leb req (h_size h) then
        let allocated := mkHole (h_start h) req in
        let remaining_size := h_size h - req in
        let remaining :=
          if remaining_size =? 0 then tl
          else mkHole (h_start h + req) remaining_size :: tl in
        Some (allocated, remaining)
      else
        match alloc_first_fit tl req with
        | None => None
        | Some (alloc, new_tl) => Some (alloc, h :: new_tl)
        end
  end.

(** Theorem: First Fit Correctness *)
Theorem first_fit_complete :
  forall hs req,
    req > 0 ->
    alloc_first_fit hs req = None ->
    forall h', In h' hs -> h_size h' < req.
Proof.
  induction hs as [|h tl IH]; intros req Hreq Halloc h' Hin; simpl in *.
  - intuition.
  - destruct (Z.leb req 0) eqn:Hle. { apply Z.leb_le in Hle. lia. }
    destruct (Z.leb req (h_size h)) eqn:Hle2. { inversion Halloc. }
    destruct (alloc_first_fit tl req) eqn:Htl.
    + destruct p. inversion Halloc.
    + simpl in Hin. destruct Hin as [Heq | Hin_tl].
      * subst. apply Z.leb_gt in Hle2. lia.
      * eapply IH; [apply Hreq | apply Htl | apply Hin_tl].
Qed.

Lemma sorted_head_le_gen : forall l, sorted l -> Forall hole_in_bounds l -> forall h tl, l = h::tl -> forall h', In h' l -> h_start h <= h_start h'.
Proof.
  induction 1 as [|h|h1 h2 tl Hle Hsort IHsorted]; intros Hbounds h0 tl0 Heq h' Hin.
  - discriminate.
  - inversion Heq; subst. simpl in Hin. destruct Hin; [subst; apply Z.le_refl|contradiction].
  - inversion Heq; subst. simpl in Hin. destruct Hin as [?|Hin].
    + subst. apply Z.le_refl.
    + inversion Hbounds as [|? ? Hbound Htail].
      assert (start_le: h_start h2 <= h_start h'). { apply IHsorted with (h:=h2) (tl:=tl); auto. }
      (* h_in_bounds h0 -> size > 0 -> start <= end *)
      assert (h_size h0 > 0). { inversion Hbound. lia. }
      assert (h_start h0 <= hole_end h0) by (unfold hole_end; lia).
      (* Hle : hole_end h0 <= h_start h2 *)
      (* Chain: start h0 <= end h0 <= start h2 <= start h' *)
      lia.
Qed.

Lemma sorted_head_le : forall h hs, sorted (h::hs) -> Forall hole_in_bounds (h::hs) -> forall h', In h' (h::hs) -> h_start h <= h_start h'.
Proof.
  intros. eapply sorted_head_le_gen; eauto.
Qed.

Lemma sorted_implies_disjoint : forall hs, 
  sorted hs -> Forall hole_in_bounds hs ->
  forall h1 h2, In h1 hs -> In h2 hs -> h1 <> h2 -> disjoint h1 h2.
Proof.
  induction 1 as [|h|h1 h2 tl Hle Hsorted IH]; intros Hbounds h1' h2' Hin1 Hin2 Hdiff; simpl in Hin1, Hin2.
  - destruct Hin1.
  - destruct Hin1 as [?|?]; [subst|contradiction]. destruct Hin2 as [?|?]; [subst|contradiction]. congruence.
  - inversion Hbounds as [|? ? Hbound1 Hbounds_tl].
    destruct Hin1 as [Heq1|Hin1]; destruct Hin2 as [Heq2|Hin2]; subst; try congruence.
    + assert (h_start h2 <= h_start h2').
      { apply sorted_head_le with (hs := tl); auto. }
      unfold disjoint; left. lia.
    + assert (h_start h2 <= h_start h1').
      { apply sorted_head_le with (hs := tl); auto. }
      unfold disjoint; right. lia.
    + apply IH; auto.
Qed.

Lemma sorted_implies_nodup : forall hs, sorted hs -> Forall hole_in_bounds hs -> NoDup hs.
Proof.
  induction 1 as [|h|h1 h2 tl Hle Hsort IH]; intros Hbounds.
  - constructor.
  - constructor; [auto | constructor].
  - constructor.
    + (* ~ In h1 (h2::tl) *)
      intro Hin.
      inversion Hbounds as [|? ? Hbound1 Hbounds_tl].
      assert (start_le: h_start h2 <= h_start h1).
      { apply sorted_head_le with (hs := tl); auto. }
      assert (h_size h1 > 0). { inversion Hbound1. lia. }
      unfold hole_end in Hle. lia.
    + (* NoDup (h2::tl) *)
      inversion Hbounds as [|? ? Hbound1 Hbounds_tl].
      inversion Hbounds_tl as [|? ? Hbound2 Hbounds_tl2].
      apply IH; auto.
Qed.

Lemma alloc_first_fit_h_start_monotonic : forall hs req alloc hs',
  req > 0 ->
  alloc_first_fit hs req = Some (alloc, hs') ->
  (forall h, In h hs' -> exists h_old, In h_old hs /\ h_start h >= h_start h_old).
Proof.
  (* Complex proof requiring careful IH management - admitted for now *)
Admitted.

Theorem alloc_first_fit_preserves_sorted :
  forall hs req alloc hs',
    sorted hs ->
    req > 0 ->
    alloc_first_fit hs req = Some (alloc, hs') ->
    sorted hs'.
Proof.
  (* Proof has fragile repeat destruct - admitted for now *)
Admitted.

Lemma alloc_first_fit_sound :
  forall hs req alloc hs',
    valid_holes hs ->
    sorted hs ->
    req > 0 ->
    alloc_first_fit hs req = Some (alloc, hs') ->
    h_size alloc = req /\ valid_holes hs' /\ sorted hs'.
Proof.
Admitted.

End AllocatorModel.
