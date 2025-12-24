(** * Mount Tag Allocation - Bitmask safety proofs
    * 
    * Imports: mnt/types
    * Proves: Tag allocation uniqueness, alloc/free inverse
    *)

Require Import mnt.types.

(* ========================================================================= *)
(* TAG ALLOCATION TRANSITIONS                                                *)
(* ========================================================================= *)

(** alloctag: Allocates a free tag *)
Inductive AllocTag (tag : Z) (ts1 ts2 : TagSet) : Prop :=
  | AT_Success :
      Inv_TagValid tag ->
      tag_free ts1 tag ->
      ts2 = alloc_tag ts1 tag ->
      AllocTag tag ts1 ts2.

(** freetag: Frees an allocated tag *)
Inductive FreeTag (tag : Z) (ts1 ts2 : TagSet) : Prop :=
  | FT_Success :
      Inv_TagValid tag ->
      tag_allocated ts1 tag ->
      ts2 = free_tag ts1 tag ->
      FreeTag tag ts1 ts2.

(* ========================================================================= *)
(* TAG ALLOCATION PROOFS                                                     *)
(* ========================================================================= *)

(** Allocating a tag marks it as allocated *)
Theorem alloctag_returns_allocated :
  forall tag ts1 ts2,
  AllocTag tag ts1 ts2 -> tag_allocated ts2 tag.
Proof.
  intros tag ts1 ts2 H. inversion H. subst.
  apply alloc_tag_sets.
Qed.

(** Freeing a tag marks it as free *)
Theorem freetag_clears :
  forall tag ts1 ts2,
  FreeTag tag ts1 ts2 -> tag_free ts2 tag.
Proof.
  intros tag ts1 ts2 H. inversion H. subst.
  apply free_tag_clears.
Qed.

(** Allocation preserves other tags *)
Theorem alloctag_preserves_others :
  forall tag tag' ts1 ts2,
  tag <> tag' ->
  AllocTag tag ts1 ts2 ->
  ts2 tag' = ts1 tag'.
Proof.
  intros tag tag' ts1 ts2 Hneq H.
  inversion H. subst.
  unfold alloc_tag.
  destruct (Z.eqb tag' tag) eqn:E.
  - apply Z.eqb_eq in E. subst. exfalso. apply Hneq. reflexivity.
  - reflexivity.
Qed.

(** Freeing preserves other tags *)
Theorem freetag_preserves_others :
  forall tag tag' ts1 ts2,
  tag <> tag' ->
  FreeTag tag ts1 ts2 ->
  ts2 tag' = ts1 tag'.
Proof.
  intros tag tag' ts1 ts2 Hneq H.
  inversion H. subst.
  unfold free_tag.
  destruct (Z.eqb tag' tag) eqn:E.
  - apply Z.eqb_eq in E. subst. exfalso. apply Hneq. reflexivity.
  - reflexivity.
Qed.

(* ========================================================================= *)
(* INVERSE THEOREMS                                                          *)
(* ========================================================================= *)

(** Alloc then free returns to original state for that tag *)
Theorem alloc_free_inverse :
  forall tag ts1 ts2 ts3,
  AllocTag tag ts1 ts2 ->
  FreeTag tag ts2 ts3 ->
  ts3 tag = ts1 tag.
Proof.
  intros tag ts1 ts2 ts3 Ha Hf.
  inversion Ha as [Hv Hfree Heq2]. subst ts2.
  inversion Hf as [Hv' Halloc Heq3]. subst ts3.
  unfold free_tag, alloc_tag.
  rewrite Z.eqb_refl.
  unfold tag_free in Hfree. symmetry. exact Hfree.
Qed.

(** Free then alloc returns to original state for that tag *)
Theorem free_alloc_inverse :
  forall tag ts1 ts2 ts3,
  FreeTag tag ts1 ts2 ->
  AllocTag tag ts2 ts3 ->
  ts3 tag = ts1 tag.
Proof.
  intros tag ts1 ts2 ts3 Hf Ha.
  inversion Hf as [Hv Halloc Heq2]. subst ts2.
  inversion Ha as [Hv' Hfree Heq3]. subst ts3.
  unfold alloc_tag, free_tag.
  rewrite Z.eqb_refl.
  unfold tag_allocated in Halloc. symmetry. exact Halloc.
Qed.

(** Cannot allocate already allocated tag *)
Theorem no_double_alloc :
  forall tag ts1 ts2,
  tag_allocated ts1 tag ->
  ~ AllocTag tag ts1 ts2.
Proof.
  intros tag ts1 ts2 Halloc Haa.
  inversion Haa.
  unfold tag_allocated in Halloc.
  unfold tag_free in H0.
  rewrite Halloc in H0. discriminate.
Qed.

(** Cannot free already free tag *)
Theorem no_double_free :
  forall tag ts1 ts2,
  tag_free ts1 tag ->
  ~ FreeTag tag ts1 ts2.
Proof.
  intros tag ts1 ts2 Hfree Haf.
  inversion Haf.
  unfold tag_free in Hfree.
  unfold tag_allocated in H0.
  rewrite Hfree in H0. discriminate.
Qed.

(** Tag 0 is always reserved *)
Theorem tag_zero_reserved :
  forall ts,
  ts = initial_mntalloc.(ma_tags) ->
  tag_allocated ts 0.
Proof.
  intros ts H. subst.
  unfold tag_allocated, initial_mntalloc, ma_tags. simpl.
  reflexivity.
Qed.

(** NOTAG is always reserved *)
Theorem notag_reserved :
  forall ts,
  ts = initial_mntalloc.(ma_tags) ->
  tag_allocated ts NOTAG.
Proof.
  intros ts H. subst.
  unfold tag_allocated, initial_mntalloc, ma_tags, NOTAG. simpl.
  reflexivity.
Qed.
