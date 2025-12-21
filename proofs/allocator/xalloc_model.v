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
  hole_end h <= mem_max /\
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

Lemma inside_preserves_disjoint_left :
  forall sub outer other,
    inside sub outer ->
    disjoint outer other ->
    disjoint sub other.
Proof.
  unfold inside, disjoint, hole_end; intros sub outer other Hinside Hdisj.
  destruct Hinside as [_ [Hge [Hle _]]].
  destruct Hdisj as [Hcase|Hcase]; [left|right]; lia.
Qed.

Lemma inside_not_disjoint :
  forall sub outer,
    inside sub outer ->
    ~ disjoint outer sub.
Proof.
  unfold inside, disjoint, hole_end; intros sub outer Hinside Hdisj.
  destruct Hinside as [_ [Hge [Hle Hsz]]].
  destruct Hdisj as [Hcase|Hcase]; lia.
Qed.

Definition valid_holes (hs : list Hole) : Prop :=
  Forall hole_in_bounds hs /\
  NoDup hs /\
  forall h1 h2, In h1 hs -> In h2 hs -> h1 <> h2 -> disjoint h1 h2.

(** Simple allocation: take from head hole when size fits and request positive. *)
Definition alloc_head (hs : list Hole) (req : Z) : option (Hole * list Hole) :=
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
      else None
  end.

Lemma alloc_preserves_disjoint :
  forall hs req alloc hs',
    valid_holes hs ->
    alloc_head hs req = Some (alloc, hs') ->
    valid_holes hs'.
Proof.
  intros hs req alloc hs' [Hbounds [Hnodup Hdisj]] Hal.
  destruct hs as [|h tl]; simpl in Hal; try discriminate.
  destruct (Z.leb req 0) eqn:Hreqpos; try discriminate.
  destruct (Z.leb req (h_size h)) eqn:Hle; try discriminate.
  apply Z.leb_le in Hle.
  apply Z.leb_gt in Hreqpos.
  inversion Hal; subst alloc hs'; clear Hal.
  inversion_clear Hbounds as [| ? ? Hhbounds Htlbounds].
  inversion_clear Hnodup as [| ? ? Hnotin Hnoduptl].
  destruct Hhbounds as [Hmin [Hmax Hsz]].
  unfold hole_end in Hmax.
  set (rem_sz := h_size h - req).
  destruct (Z.eq_dec rem_sz 0) as [Hz|Hz].
  - split.
    + rewrite Hz; simpl. exact Htlbounds.
    + split.
      * rewrite Hz; simpl. exact Hnoduptl.
      * intros h1 h2 Hin1 Hin2 Hneq.
        rewrite Hz in *; simpl in *.
        eapply Hdisj; eauto.
  - assert (rem_sz > 0) by lia.
    assert (Hzb : rem_sz =? 0 = false) by (apply Z.eqb_neq; exact Hz).
    rewrite Hzb.
    set (rem_h := mkHole (h_start h + req) rem_sz).
    assert (Hinside : inside rem_h h).
    { unfold inside, rem_h, hole_end; simpl; repeat split; try lia. }
    assert (Hneq_hr : h <> rem_h).
    { intro Heq; apply f_equal with (f := h_start) in Heq; subst rem_h; simpl in Heq; lia. }
    assert (Hnotin_rem : ~ In rem_h tl).
    { intro Hin.
      specialize (Hdisj h rem_h (or_introl eq_refl) (or_intror Hin) Hneq_hr).
      apply (inside_not_disjoint _ _ Hinside) in Hdisj; contradiction. }
    split.
    + constructor.
      * unfold hole_in_bounds, rem_h, hole_end; simpl.
        repeat split; try lia.
      * exact Htlbounds.
    + split.
      * constructor.
        -- intro Hin; apply Hnotin_rem in Hin; contradiction.
        -- exact Hnoduptl.
      * intros h1 h2 Hin1 Hin2 Hneq'.
        simpl in Hin1, Hin2.
        destruct Hin1 as [Hin1 | Hin1]; destruct Hin2 as [Hin2 | Hin2]; subst.
        -- exfalso; apply Hneq'; reflexivity.
        -- assert (h <> h2) by (intros Heq; subst; contradiction Hnotin; assumption).
           specialize (Hdisj h h2 (or_introl eq_refl) (or_intror Hin2) H0).
           eapply inside_preserves_disjoint_left; eauto.
        -- assert (h1 <> h) by (intros Heq; subst; contradiction Hnotin; assumption).
           specialize (Hdisj h1 h (or_intror Hin1) (or_introl eq_refl) H0).
           apply disjoint_sym.
           eapply inside_preserves_disjoint_left; eauto.
           apply disjoint_sym; auto.
        -- specialize (Hdisj h1 h2 (or_intror Hin1) (or_intror Hin2) Hneq').
           exact Hdisj.
Qed.

Lemma alloc_head_consumes :
  forall hs req alloc hs',
    alloc_head hs req = Some (alloc, hs') ->
    h_size alloc = req.
Proof.
  intros hs req alloc hs' H.
  destruct hs; simpl in H; try discriminate.
  destruct (Z.leb req 0) eqn:?; try discriminate.
  destruct (Z.leb req (h_size h)) eqn:?; try discriminate.
  inversion H; subst; reflexivity.
Qed.

Lemma alloc_head_start :
  forall hs req alloc hs',
    alloc_head hs req = Some (alloc, hs') ->
    exists h, In h hs /\ h_start alloc = h_start h.
Proof.
  intros hs req alloc hs' H.
  destruct hs; simpl in H; try discriminate.
  destruct (Z.leb req 0) eqn:?; try discriminate.
  destruct (Z.leb req (h_size h)) eqn:?; try discriminate.
  inversion H; subst; clear H.
  exists h; split; [left; reflexivity| reflexivity].
Qed.

End AllocatorModel.
