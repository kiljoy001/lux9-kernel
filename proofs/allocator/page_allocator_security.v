(** Security properties for page allocation and exchange.
    Abstract model: free pages are a list of addresses; owned pages are
    address-owner pairs. Proves exclusivity and preservation across
    allocation, free, and ownership transfer. *)

From Coq Require Import List ZArith Lia.
Import ListNotations.
Local Open Scope Z_scope.

Section PageAllocatorSecurity.

Definition addr := Z.
Definition owner := Z.
Definition owned := (addr * owner)%type.

Definition free_list := list addr.
Definition owned_list := list owned.

Definition owned_addrs (ol : owned_list) : list addr :=
  map fst ol.

Definition valid_free (fl : free_list) : Prop := NoDup fl.
Definition valid_owned (ol : owned_list) : Prop := NoDup (owned_addrs ol).

Definition disjoint_free_owned (fl : free_list) (ol : owned_list) : Prop :=
  forall a, In a fl -> ~ In a (owned_addrs ol).

Definition valid_state (fl : free_list) (ol : owned_list) : Prop :=
  valid_free fl /\ valid_owned ol /\ disjoint_free_owned fl ol.

Definition alloc_state (who : owner) (fl : free_list) (ol : owned_list)
  : option (addr * free_list * owned_list) :=
  match fl with
  | [] => None
  | a :: tl => Some (a, tl, (a, who) :: ol)
  end.

Fixpoint remove_owned (a : addr) (who : owner) (ol : owned_list)
  : option owned_list :=
  match ol with
  | [] => None
  | (x, o) :: tl =>
      if Z.eqb x a then
        if Z.eqb o who then Some tl
        else
          match remove_owned a who tl with
          | None => None
          | Some tl' => Some ((x, o) :: tl')
          end
      else
        match remove_owned a who tl with
        | None => None
        | Some tl' => Some ((x, o) :: tl')
        end
  end.

Fixpoint transfer_owned (a : addr) (from to : owner) (ol : owned_list)
  : option owned_list :=
  match ol with
  | [] => None
  | (x, o) :: tl =>
      if Z.eqb x a then
        if Z.eqb o from then Some ((x, to) :: tl)
        else
          match transfer_owned a from to tl with
          | None => None
          | Some tl' => Some ((x, o) :: tl')
          end
      else
        match transfer_owned a from to tl with
        | None => None
        | Some tl' => Some ((x, o) :: tl')
        end
  end.

Lemma owned_addrs_cons :
  forall a o ol, owned_addrs ((a, o) :: ol) = a :: owned_addrs ol.
Proof. reflexivity. Qed.

Lemma valid_free_tail :
  forall a fl, valid_free (a :: fl) -> valid_free fl.
Proof. intros a fl H; inversion H; assumption. Qed.

Lemma disjoint_in_free_not_owned :
  forall fl ol a,
    disjoint_free_owned fl ol ->
    In a fl ->
    ~ In a (owned_addrs ol).
Proof. intros fl ol a H HIn; apply (H a HIn). Qed.

Lemma remove_owned_subset :
  forall a who ol ol' x,
    remove_owned a who ol = Some ol' ->
    In x (owned_addrs ol') ->
    In x (owned_addrs ol).
Proof.
  intros a who ol; induction ol as [|[y o] tl IH]; intros ol' x Hrm Hin.
  - simpl in Hrm. discriminate.
  - simpl in Hrm.
    destruct (Z.eqb y a) eqn:Hy.
    + destruct (Z.eqb o who) eqn:Ho.
      * inversion Hrm; subst. simpl in Hin. simpl. right; assumption.
      * destruct (remove_owned a who tl) eqn:Hr; try discriminate.
        inversion Hrm; subst. simpl in Hin. simpl.
        destruct Hin as [Hin | Hin].
        { left; assumption. }
        { right; eapply IH; eauto. }
    + destruct (remove_owned a who tl) eqn:Hr; try discriminate.
      inversion Hrm; subst. simpl in Hin. simpl.
      destruct Hin as [Hin | Hin].
      { left; assumption. }
      { right; eapply IH; eauto. }
Qed.

Lemma remove_owned_none_no_addr :
  forall a who ol,
    ~ In a (owned_addrs ol) ->
    remove_owned a who ol = None.
Proof.
  intros a who ol; induction ol as [|[y o] tl IH]; intros Hnotin.
  - reflexivity.
  - simpl in Hnotin. simpl.
    destruct (Z.eqb y a) eqn:Hy.
    + apply Z.eqb_eq in Hy; subst. exfalso. apply Hnotin. left. reflexivity.
    + assert (Hnotin_tl : ~ In a (owned_addrs tl)) by
        (intro Hin; apply Hnotin; right; exact Hin).
      specialize (IH Hnotin_tl).
      rewrite IH. reflexivity.
Qed.

Lemma remove_owned_addr_in :
  forall a who ol ol',
    remove_owned a who ol = Some ol' ->
    In a (owned_addrs ol).
Proof.
  intros a who ol; induction ol as [|[y o] tl IH]; intros ol' Hrm.
  - simpl in Hrm. discriminate.
  - simpl in Hrm.
    destruct (Z.eqb y a) eqn:Hy.
    + apply Z.eqb_eq in Hy; subst.
      destruct (Z.eqb o who) eqn:Ho.
      * inversion Hrm; subst. simpl. left. reflexivity.
      * destruct (remove_owned a who tl) eqn:Hr; try discriminate.
        inversion Hrm; subst. simpl. right. eapply IH; eauto.
    + destruct (remove_owned a who tl) eqn:Hr; try discriminate.
      inversion Hrm; subst. simpl. right. eapply IH; eauto.
Qed.

Lemma remove_owned_removes_addr :
  forall a who ol ol',
    valid_owned ol ->
    remove_owned a who ol = Some ol' ->
    ~ In a (owned_addrs ol').
Proof.
  intros a who ol; induction ol as [|[y o] tl IH]; intros ol' Hnd Hrm.
  - simpl in Hrm. discriminate.
  - simpl in Hrm.
    destruct (Z.eqb y a) eqn:Hy.
    + apply Z.eqb_eq in Hy; subst.
      destruct (Z.eqb o who) eqn:Ho.
      * inversion Hrm; subst. simpl.
        inversion Hnd as [|? ? Hnotin Hndtl]; subst.
        exact Hnotin.
      * destruct (remove_owned a who tl) eqn:Hr; try discriminate.
        inversion Hrm; subst.
        inversion Hnd as [|? ? Hnotin Hndtl]; subst.
        exfalso.
        pose proof (remove_owned_none_no_addr a who tl Hnotin) as Hnone.
        rewrite Hnone in Hr. discriminate.
    + destruct (remove_owned a who tl) eqn:Hr; try discriminate.
      inversion Hrm; subst. simpl.
      intro Hin.
      destruct Hin as [Hin | Hin].
      * apply Z.eqb_neq in Hy. subst. contradiction.
      * inversion Hnd as [|? ? Hnotin Hndtl]; subst.
        specialize (IH o0 Hndtl eq_refl).
        exact (IH Hin).
Qed.

Lemma remove_owned_preserves_nodup :
  forall a who ol ol',
    valid_owned ol ->
    remove_owned a who ol = Some ol' ->
    valid_owned ol'.
Proof.
  intros a who ol; induction ol as [|[y o] tl IH]; intros ol' Hnd Hrm.
  - simpl in Hrm. discriminate.
  - simpl in Hrm.
    destruct (Z.eqb y a) eqn:Hy.
    + destruct (Z.eqb o who) eqn:Ho.
      * inversion Hrm; subst. inversion Hnd; subst; assumption.
      * destruct (remove_owned a who tl) eqn:Hr; try discriminate.
        inversion Hrm; subst. simpl in *.
        inversion Hnd as [|? ? Hnotin Hndtl]; subst.
        constructor.
        { intro Hin.
          eapply remove_owned_subset in Hin; eauto. }
        { eapply IH; eauto. }
    + destruct (remove_owned a who tl) eqn:Hr; try discriminate.
      inversion Hrm; subst. simpl in *.
      inversion Hnd as [|? ? Hnotin Hndtl]; subst.
      constructor.
      { intro Hin.
        eapply remove_owned_subset in Hin; eauto. }
      { eapply IH; eauto. }
Qed.

Lemma remove_owned_disjoint :
  forall a who fl ol ol',
    disjoint_free_owned fl ol ->
    remove_owned a who ol = Some ol' ->
    disjoint_free_owned fl ol'.
Proof.
  intros a who fl ol ol' Hdis Hrm x Hinx Hinowned.
  apply (Hdis x Hinx).
  eapply remove_owned_subset; eauto.
Qed.

Lemma transfer_owned_map_fst :
  forall a from to ol ol',
    transfer_owned a from to ol = Some ol' ->
    owned_addrs ol' = owned_addrs ol.
Proof.
  intros a from to ol; induction ol as [|[y o] tl IH]; intros ol' Htr.
  - simpl in Htr. discriminate.
  - simpl in Htr.
    destruct (Z.eqb y a) eqn:Hy.
    + destruct (Z.eqb o from) eqn:Ho.
      * inversion Htr; subst; simpl; reflexivity.
      * destruct (transfer_owned a from to tl) eqn:Ht; try discriminate.
        inversion Htr; subst. simpl. f_equal. eapply IH; eauto.
    + destruct (transfer_owned a from to tl) eqn:Ht; try discriminate.
      inversion Htr; subst. simpl. f_equal. eapply IH; eauto.
Qed.

Lemma alloc_preserves_state :
  forall who fl ol a fl' ol',
    valid_state fl ol ->
    alloc_state who fl ol = Some (a, fl', ol') ->
    valid_state fl' ol'.
Proof.
  intros who fl ol a fl' ol' [Hfree [Howned Hdis]] Halloc.
  destruct fl as [|x tl]; simpl in Halloc; try discriminate.
  inversion Halloc; subst; clear Halloc.
  split.
  - apply valid_free_tail with (a:=a); assumption.
  - split.
    + assert (Hnotin : ~ In a (owned_addrs ol)) by
        (apply (Hdis a); left; reflexivity).
      constructor; [exact Hnotin | exact Howned].
    + intros addr HIn.
      simpl.
      intro HinOwned.
      destruct HinOwned as [Heq | HinOwned].
      * subst.
        inversion Hfree as [|? ? Hnotin_tl Hfree_tl]; subst.
        apply Hnotin_tl; exact HIn.
      * apply (Hdis addr). right; exact HIn. exact HinOwned.
Qed.

Lemma alloc_exclusive :
  forall who fl ol a fl' ol',
    valid_state fl ol ->
    alloc_state who fl ol = Some (a, fl', ol') ->
    ~ In a (owned_addrs ol).
Proof.
  intros who fl ol a fl' ol' [_ [_ Hdis]] Halloc.
  destruct fl as [|x tl]; simpl in Halloc; try discriminate.
  inversion Halloc; subst; clear Halloc.
  apply (Hdis a). left; reflexivity.
Qed.

Lemma free_preserves_state :
  forall who a fl ol ol' fl',
    valid_state fl ol ->
    remove_owned a who ol = Some ol' ->
    fl' = a :: fl ->
    valid_state fl' ol'.
Proof.
  intros who a fl ol ol' fl' [Hfree [Howned Hdis]] Hrm Hfl'.
  subst fl'.
  split.
  - constructor.
    + intro Hin.
      pose proof (remove_owned_addr_in _ _ _ _ Hrm) as HinOwned.
      specialize (Hdis a Hin).
      exact (Hdis HinOwned).
    + apply Hfree.
  - split.
    + eapply remove_owned_preserves_nodup; eauto.
    + intros addr HIn.
      simpl in HIn.
      destruct HIn as [HIn | HIn].
      * subst.
        eapply remove_owned_removes_addr; eauto.
      * pose proof (remove_owned_disjoint a who fl ol ol' Hdis Hrm) as Hdis'.
        apply (Hdis' addr HIn).
Qed.

Lemma transfer_preserves_state :
  forall a from to fl ol ol',
    valid_state fl ol ->
    transfer_owned a from to ol = Some ol' ->
    valid_state fl ol'.
Proof.
  intros a from to fl ol ol' [Hfree [Howned Hdis]] Htr.
  split; [assumption|].
  split.
  - unfold valid_owned. rewrite (transfer_owned_map_fst _ _ _ _ _ Htr). exact Howned.
  - unfold disjoint_free_owned in *.
    intros addr Hin.
    rewrite (transfer_owned_map_fst _ _ _ _ _ Htr).
    apply (Hdis addr Hin).
Qed.

End PageAllocatorSecurity.
