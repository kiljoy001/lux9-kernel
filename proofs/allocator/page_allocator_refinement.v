(** Base page allocator mechanics modeled after page.c.
    Proves allocation/free preserve disjointness and uniqueness. *)

From Coq Require Import List ZArith Lia.
Import ListNotations.
Local Open Scope Z_scope.

Section PageAllocatorRefinement.

Record Page := mkPage { paddr : Z; refc : nat }.

Definition addrs (l : list Page) : list Z := map paddr l.

Definition valid_free (fl : list Page) : Prop :=
  Forall (fun p => refc p = 0%nat) fl /\ NoDup (addrs fl).

Definition valid_active (al : list Page) : Prop :=
  Forall (fun p => (0%nat < refc p)%nat) al /\ NoDup (addrs al).

Definition disjoint_lists (fl al : list Page) : Prop :=
  forall a, In a (addrs fl) -> ~ In a (addrs al).

Definition valid_state (fl al : list Page) : Prop :=
  valid_free fl /\ valid_active al /\ disjoint_lists fl al.

Definition alloc_page (fl al : list Page)
  : option (Page * list Page * list Page) :=
  match fl with
  | [] => None
  | p :: tl =>
      let p' := mkPage (paddr p) 1%nat in
      Some (p', tl, p' :: al)
  end.

Fixpoint remove_page (a : Z) (al : list Page) : option (list Page) :=
  match al with
  | [] => None
  | p :: tl =>
      if Z.eqb (paddr p) a then Some tl
      else
        match remove_page a tl with
        | None => None
        | Some tl' => Some (p :: tl')
        end
  end.

Definition free_page (a : Z) (fl al : list Page)
  : option (list Page * list Page) :=
  match remove_page a al with
  | None => None
  | Some al' => Some (mkPage a 0%nat :: fl, al')
  end.

Lemma addrs_cons : forall p l, addrs (p :: l) = paddr p :: addrs l.
Proof. reflexivity. Qed.

Lemma valid_free_tail :
  forall p fl, valid_free (p :: fl) -> valid_free fl.
Proof.
  intros p fl [Hf Hnd].
  inversion Hf as [|? ? Hp Hf']; subst.
  inversion Hnd as [|? ? Hnotin Hnd']; subst.
  split; [exact Hf' | exact Hnd'].
Qed.

Lemma valid_active_tail :
  forall p al, valid_active (p :: al) -> valid_active al.
Proof.
  intros p al [Hf Hnd].
  inversion Hf as [|? ? Hp Hf']; subst.
  inversion Hnd as [|? ? Hnotin Hnd']; subst.
  split; [exact Hf' | exact Hnd'].
Qed.

Lemma remove_page_subset :
  forall a al al' x,
    remove_page a al = Some al' ->
    In x (addrs al') ->
    In x (addrs al).
Proof.
  intros a al; induction al as [|p tl IH]; intros al' x Hrm Hin.
  - simpl in Hrm. discriminate.
  - simpl in Hrm.
    destruct (Z.eqb (paddr p) a) eqn:Heq.
    + inversion Hrm; subst. simpl in Hin. simpl. right; exact Hin.
    + destruct (remove_page a tl) eqn:Hr; try discriminate.
      inversion Hrm; subst. simpl in Hin. simpl.
      destruct Hin as [Hin | Hin].
      * left; exact Hin.
      * right; eapply IH; eauto.
Qed.

Lemma remove_page_addr_in :
  forall a al al',
    remove_page a al = Some al' ->
    In a (addrs al).
Proof.
  intros a al; induction al as [|p tl IH]; intros al' Hrm.
  - simpl in Hrm. discriminate.
  - simpl in Hrm.
    destruct (Z.eqb (paddr p) a) eqn:Heq.
    + apply Z.eqb_eq in Heq; subst. simpl. left. reflexivity.
    + destruct (remove_page a tl) eqn:Hr; try discriminate.
      inversion Hrm; subst. simpl. right. eapply IH; eauto.
Qed.

Lemma remove_page_preserves_nodup :
  forall a al al',
    NoDup (addrs al) ->
    remove_page a al = Some al' ->
    NoDup (addrs al').
Proof.
  intros a al; induction al as [|p tl IH]; intros al' Hnd Hrm.
  - simpl in Hrm. discriminate.
  - simpl in Hrm.
    destruct (Z.eqb (paddr p) a) eqn:Heq.
    + inversion Hrm; subst. inversion Hnd; subst; assumption.
    + destruct (remove_page a tl) eqn:Hr; try discriminate.
      inversion Hrm; subst. inversion Hnd as [|? ? Hnotin Hndtl]; subst.
      constructor.
      * intro Hin. eapply remove_page_subset in Hin; eauto.
      * eapply IH; eauto.
Qed.

Lemma remove_page_removes_addr :
  forall a al al',
    NoDup (addrs al) ->
    remove_page a al = Some al' ->
    ~ In a (addrs al').
Proof.
  intros a al; induction al as [|p tl IH]; intros al' Hnd Hrm.
  - simpl in Hrm. discriminate.
  - simpl in Hrm.
    destruct (Z.eqb (paddr p) a) eqn:Heq.
    + apply Z.eqb_eq in Heq; subst.
      inversion Hrm; subst. simpl.
      inversion Hnd as [|? ? Hnotin Hndtl]; subst.
      exact Hnotin.
    + destruct (remove_page a tl) eqn:Hr; try discriminate.
      inversion Hrm; subst. simpl.
      intro Hin. destruct Hin as [Hin | Hin].
      * apply Z.eqb_neq in Heq; subst. contradiction.
      * inversion Hnd as [|? ? Hnotin Hndtl]; subst.
        specialize (IH l Hndtl eq_refl).
        exact (IH Hin).
Qed.

Lemma remove_page_preserves_forall :
  forall a al al',
    Forall (fun p => (0%nat < refc p)%nat) al ->
    remove_page a al = Some al' ->
    Forall (fun p => (0%nat < refc p)%nat) al'.
Proof.
  intros a al; induction al as [|p tl IH]; intros al' Hf Hrm.
  - simpl in Hrm. discriminate.
  - inversion Hf as [|? ? Hp Hf']; subst.
    simpl in Hrm.
    destruct (Z.eqb (paddr p) a) eqn:Heq.
    + inversion Hrm; subst; exact Hf'.
    + destruct (remove_page a tl) eqn:Hr; try discriminate.
      inversion Hrm; subst. constructor; [exact Hp|].
      eapply IH; eauto.
Qed.

Lemma alloc_preserves_state :
  forall fl al p fl' al',
    valid_state fl al ->
    alloc_page fl al = Some (p, fl', al') ->
    valid_state fl' al'.
Proof.
  intros fl al p fl' al' [Hfree [Hact Hdis]] Halloc.
  destruct fl as [|x tl]; simpl in Halloc; try discriminate.
  inversion Halloc; subst; clear Halloc.
  split.
  - apply valid_free_tail with (p:=x); exact Hfree.
  - split.
    + destruct Hact as [Hf Hnd].
      split.
      * constructor; [simpl; lia|exact Hf].
      * constructor.
        { intro Hin.
          apply (Hdis (paddr x)). left. reflexivity.
          simpl in Hin. exact Hin. }
        { exact Hnd. }
    + intros a Hin.
      intro HinOwned.
      simpl in HinOwned.
      destruct HinOwned as [Heq | HinOwned].
      * subst.
        destruct Hfree as [_ Hnd].
        inversion Hnd as [|? ? Hnotin Hndtl]; subst.
        apply Hnotin. exact Hin.
      * apply (Hdis a). right. exact Hin. exact HinOwned.
Qed.

Lemma free_preserves_state :
  forall fl al a fl' al',
    valid_state fl al ->
    free_page a fl al = Some (fl', al') ->
    valid_state fl' al'.
Proof.
  intros fl al a fl' al' [Hfree [Hact Hdis]] Hfreep.
  unfold free_page in Hfreep.
  destruct (remove_page a al) eqn:Hrm; try discriminate.
  inversion Hfreep; subst; clear Hfreep.
  destruct Hfree as [Hf Hnd].
  destruct Hact as [Hact_forall Hact_nodup].
  assert (Hfree' : valid_free (mkPage a 0%nat :: fl)).
  { split;
      [constructor; [reflexivity|exact Hf]
      |apply NoDup_cons;
         [intro Hin;
          pose proof (remove_page_addr_in a al al' Hrm) as Hrm_in;
          apply (Hdis a) in Hin;
          exact (Hin Hrm_in)
         |exact Hnd] ]. }
  pose proof (remove_page_preserves_forall a al al' Hact_forall Hrm) as Hact_forall'.
  pose proof (remove_page_preserves_nodup a al al' Hact_nodup Hrm) as Hact_nodup'.
  assert (Hact' : valid_active al') by exact (conj Hact_forall' Hact_nodup').
  assert (Hdis' : disjoint_lists (mkPage a 0%nat :: fl) al').
  { intros x Hin.
    simpl in Hin.
    destruct Hin as [Hin | Hin];
      [ subst x; intro Hin2;
        pose proof (remove_page_removes_addr a al al' Hact_nodup Hrm) as Hnotin;
        exact (Hnotin Hin2)
      | intro Hin2;
        apply (remove_page_subset a al al' x) in Hin2; [|exact Hrm];
        apply (Hdis x); [exact Hin|exact Hin2] ]. }
  exact (conj Hfree' (conj Hact' Hdis')).
Qed.

End PageAllocatorRefinement.
