(* Decidable checks that correspond to the capability predicates. *)
Require Import Coq.Bool.Bool.
Require Import proofs.capability.PermsBitmask.
Require Import proofs.capability.CapabilityModel.

Section Decidability.

(* Boolean version of permission subset: for each bit, either it is absent
   from the child or present in the parent. *)
Definition perms_subsetb (a b : nat) : bool :=
  (negb (has_perm a perm_read)     || has_perm b perm_read) &&
  (negb (has_perm a perm_write)    || has_perm b perm_write) &&
  (negb (has_perm a perm_exec)     || has_perm b perm_exec) &&
  (negb (has_perm a perm_transfer) || has_perm b perm_transfer) &&
  (negb (has_perm a perm_grant)    || has_perm b perm_grant).

Lemma implies_from_bool :
  forall x y : bool,
    (negb x || y) = true ->
    x = true -> y = true.
Proof.
  intros [] [] H; simpl in H; intros Hx; try discriminate; auto.
Qed.

Lemma perms_subsetb_correct :
  forall a b, perms_subsetb a b = true <-> perms_subset a b.
Proof.
  unfold perms_subsetb, perms_subset.
  split.
  - repeat rewrite andb_true_iff; intros [[[[Hread Hwrite] Hexec] Htransfer] Hgrant] bit Hbit Hha;
      destruct Hbit as [-> | [-> | [-> | [-> | ->]]]]; eapply implies_from_bool; eauto.
  - intros Hsubset.
    repeat (apply andb_true_intro; split);
      (destruct (has_perm a _) eqn:Ha; [| reflexivity]);
      try (specialize (Hsubset _ (or_introl eq_refl) Ha);
           rewrite Hsubset; reflexivity);
      try (specialize (Hsubset _ (or_intror (or_introl eq_refl)) Ha);
           rewrite Hsubset; reflexivity);
      try (specialize (Hsubset _ (or_intror (or_intror (or_introl eq_refl))) Ha);
           rewrite Hsubset; reflexivity);
      try (specialize (Hsubset _ (or_intror (or_intror (or_intror (or_introl eq_refl)))) Ha);
           rewrite Hsubset; reflexivity);
      try (specialize (Hsubset _ (or_intror (or_intror (or_intror (or_intror eq_refl)))) Ha);
           rewrite Hsubset; reflexivity).
Qed.

(* Boolean validation mirroring derivation_valid. *)
Definition derivation_validb (ct : cap_table) (child : Capability) : bool :=
  match cap_parent child with
  | None => false
  | Some pid =>
      match find_cap ct pid with
      | None => false
      | Some parent =>
          perms_subsetb (cap_perms child) (cap_perms parent)
      end
  end.

Lemma derivation_validb_correct :
  forall ct child,
    derivation_validb ct child = true <-> derivation_valid ct child.
Proof.
  intros ct child; unfold derivation_validb, derivation_valid.
  destruct (cap_parent child) as [pid|] eqn:Hpar.
  - destruct (find_cap ct pid) as [parent|] eqn:Hfind.
    + apply perms_subsetb_correct.
    + split; intro H; simpl in H; try discriminate; try contradiction.
  - split; intro H; simpl in H; try discriminate; try contradiction.
Qed.

Lemma derivation_valid_dec :
  forall ct child, { derivation_valid ct child } + { ~ derivation_valid ct child }.
Proof.
  intros ct child.
  destruct (derivation_validb ct child) eqn:Hb.
  - left; apply derivation_validb_correct; assumption.
  - right; intro Hc; apply derivation_validb_correct in Hc; congruence.
Qed.

End Decidability.
