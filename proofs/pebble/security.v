(** * Pebble Security - Memory safety proofs
    * 
    * Imports: pebble/types, pebble/conservation
    * Proves: Double-free, UAF, authorization chain, budget bounds
    *)

Require Import pebble.types.
Require Import pebble.conservation.
Require Import Lia.

Open Scope Z_scope.

(* ========================================================================= *)
(* SECURITY INVARIANTS                                                       *)
(* ========================================================================= *)

Definition Inv_NoDoubleFree (s : PebbleState) : Prop :=
  forall c, ~ (In c s.(live_caps) /\ In c s.(freed_caps)).

Definition Inv_NoUseAfterFree (s : PebbleState) : Prop :=
  forall c, In c s.(freed_caps) -> ~ In c s.(live_caps).

Definition Inv_ValidCaps (s : PebbleState) : Prop :=
  s.(next_cap_id) > 0 /\
  forall c, In c s.(live_caps) -> c > 0 /\ c < s.(next_cap_id).

Definition Valid (s : PebbleState) (total : Z) : Prop :=
  Inv_Conservation s total /\
  Inv_NoDoubleFree s /\
  Inv_NoUseAfterFree s /\
  Inv_ValidCaps s /\
  Inv_NonNegative s.

(* ========================================================================= *)
(* DOUBLE-FREE PREVENTION                                                    *)
(* ========================================================================= *)

Theorem double_free_impossible :
  forall s1 s2 size cap,
  Inv_NoDoubleFree s1 -> BlackFree size cap s1 s2 -> ~ In cap s1.(freed_caps).
Proof.
  intros. inversion H0. assumption.
Qed.

Theorem blackfree_moves_to_freed :
  forall s1 s2 size cap,
  BlackFree size cap s1 s2 -> In cap s2.(freed_caps).
Proof.
  intros. inversion H. subst. simpl. left. reflexivity.
Qed.

Theorem blackfree_removes_from_live :
  forall s1 s2 size cap,
  BlackFree size cap s1 s2 -> ~ In cap s2.(live_caps).
Proof.
  intros. inversion H. subst. simpl. apply remove_cap_not_in.
Qed.

Theorem blackfree_preserves_no_double_free :
  forall s1 s2 size cap,
  Inv_NoDoubleFree s1 -> BlackFree size cap s1 s2 -> Inv_NoDoubleFree s2.
Proof.
  intros s1 s2 size cap Hinv Hfree.
  unfold Inv_NoDoubleFree. intros c Hcontra.
  destruct Hcontra as [Hin_live Hin_freed].
  inversion Hfree. subst. simpl in *.
  destruct Hin_freed as [Heq | Hin_old_freed].
  - subst c. apply remove_cap_not_in in Hin_live. contradiction.
  - unfold remove_cap in Hin_live. apply filter_In in Hin_live.
    destruct Hin_live as [Hin_old_live _]. apply (Hinv c). split; assumption.
Qed.

Theorem blackalloc_preserves_no_double_free :
  forall s1 s2 size cap,
  Inv_NoDoubleFree s1 -> BlackAlloc size cap s1 s2 -> Inv_NoDoubleFree s2.
Proof.
  intros s1 s2 size cap Hinv Halloc.
  unfold Inv_NoDoubleFree. intros c Hcontra.
  destruct Hcontra as [Hin_live Hin_freed].
  inversion Halloc. subst. simpl in *.
  destruct Hin_live as [Heq | Hold].
  - subst c. (* Case c = new_cap *)
    (* new_cap cannot be in freed_caps by precondition *)
    contradiction.
  - (* Case c was old live cap *)
    apply (Hinv c). split; assumption.
Qed.

(* ========================================================================= *)
(* USE-AFTER-FREE PREVENTION                                                 *)
(* ========================================================================= *)

Theorem blackfree_invalidates :
  forall s1 s2 size cap,
  BlackFree size cap s1 s2 -> In cap s2.(freed_caps) /\ ~ In cap s2.(live_caps).
Proof.
  intros. split.
  - apply blackfree_moves_to_freed with s1 size. assumption.
  - apply blackfree_removes_from_live with s1 size. assumption.
Qed.

Theorem blackfree_preserves_no_uaf :
  forall s1 s2 size cap,
  Inv_NoUseAfterFree s1 -> BlackFree size cap s1 s2 -> Inv_NoUseAfterFree s2.
Proof.
  intros s1 s2 size cap Hinv Hfree.
  unfold Inv_NoUseAfterFree. intros c Hin_freed.
  inversion Hfree. subst. simpl in *.
  destruct Hin_freed as [Heq | Hin_old_freed].
  - subst c. apply remove_cap_not_in.
  - intro Hin_live. unfold remove_cap in Hin_live.
    apply filter_In in Hin_live. destruct Hin_live as [Hin_old_live _].
    apply (Hinv c Hin_old_freed). assumption.
Qed.

Theorem blackalloc_preserves_no_uaf :
  forall s1 s2 size cap,
  Inv_NoUseAfterFree s1 -> BlackAlloc size cap s1 s2 -> Inv_NoUseAfterFree s2.
Proof.
  intros s1 s2 size cap Hinv Halloc.
  unfold Inv_NoUseAfterFree. intros c Hin_freed.
  inversion Halloc. subst. simpl in *.
  intro Hin_live.
  destruct Hin_live as [Heq | Hold].
  - subst c. (* new_cap not in freed *)
    contradiction.
  - apply (Hinv c Hin_freed Hold).
Qed.

(* ========================================================================= *)
(* AUTHORIZATION CHAIN                                                       *)
(* ========================================================================= *)

Theorem blackalloc_requires_fresh_cap :
  forall s1 s2 size cap,
  BlackAlloc size cap s1 s2 ->
  ~ In cap s1.(live_caps) /\ ~ In cap s1.(freed_caps).
Proof.
  intros s1 s2 size cap Halloc.
  inversion Halloc; subst.
  split; assumption.
Qed.

Theorem whiteverify_authorizes :
  forall s1 s2 size,
  WhiteVerify size s1 s2 -> s2.(pending_authorizations) = s1.(pending_authorizations) + 1.
Proof.
  intros. inversion H. subst. simpl. reflexivity.
Qed.

(* ========================================================================= *)
(* BUDGET NON-BYPASS                                                         *)
(* ========================================================================= *)

Theorem blackalloc_preserves_nonneg :
  forall s1 s2 size cap,
  Inv_NonNegative s1 -> BlackAlloc size cap s1 s2 -> Inv_NonNegative s2.
Proof.
  intros. inversion H0. subst. unfold Inv_NonNegative in *. simpl.
  destruct H as [Hc [Hb [Hbl [Hr [Hwp Hwv]]]]].
  repeat split; try assumption; try lia.
Qed.

Theorem blackfree_preserves_nonneg :
  forall s1 s2 size cap,
  Inv_NonNegative s1 -> BlackFree size cap s1 s2 -> Inv_NonNegative s2.
Proof.
  intros. inversion H0. subst. unfold Inv_NonNegative in *. simpl.
  destruct H as [Hc [Hb [Hbl [Hr [Hwp Hwv]]]]].
  repeat split; try assumption; try lia.
Qed.

(* ========================================================================= *)
(* CAPABILITY UNFORGABILITY & VALIDITY                                       *)
(* ========================================================================= *)

Theorem blackalloc_mints_valid_cap :
  forall s1 s2 size cap,
  Inv_ValidCaps s1 -> BlackAlloc size cap s1 s2 ->
  In cap s2.(live_caps) /\ cap > 0 /\ cap < s2.(next_cap_id).
Proof.
  intros s1 s2 size cap Hvalid Halloc.
  destruct Hvalid as [Hpos Hcaps].
  inversion Halloc. subst. simpl.
  split. left. reflexivity.
  split. assumption. lia.
Qed.

Theorem blackalloc_preserves_valid_caps :
  forall s1 s2 size cap,
  Inv_ValidCaps s1 -> BlackAlloc size cap s1 s2 -> Inv_ValidCaps s2.
Proof.
  intros s1 s2 size cap Hvalid Halloc.
  destruct s1. destruct Hvalid as [Hpos Hcaps].
  unfold Inv_ValidCaps. split.
  - destruct Halloc; subst; simpl in *. lia.
  - intros c Hin.
    destruct Halloc; subst; simpl in *.
    destruct Hin as [Heq | Hold].
    + subst c. split. assumption. apply Z.lt_succ_diag_r.
    + apply Hcaps in Hold. destruct Hold as [Hc_pos Hc_lt].
      split. assumption.
      apply Z.lt_trans with (m := next_cap_id). assumption. apply Z.lt_succ_diag_r.
Qed.

Theorem blackfree_preserves_valid_caps :
  forall s1 s2 size cap,
  Inv_ValidCaps s1 -> BlackFree size cap s1 s2 -> Inv_ValidCaps s2.
Proof.
  intros s1 s2 size cap Hvalid Hfree.
  destruct Hvalid as [Hpos Hcaps].
  unfold Inv_ValidCaps. split.
  - destruct Hfree; subst; simpl in *. assumption.
  - intros c Hin.
    destruct Hfree; subst; simpl in *.
    unfold remove_cap in Hin. apply filter_In in Hin.
    destruct Hin as [Hold Hneq].
    apply Hcaps in Hold. assumption.
Qed.

(* ========================================================================= *)
(* MASTER THEOREMS: Alloc and Free are secure                                *)
(* ========================================================================= *)

Theorem blackfree_secure :
  forall s1 s2 size cap total,
  Valid s1 total -> BlackFree size cap s1 s2 ->
  Inv_Conservation s2 total /\ Inv_NoDoubleFree s2 /\
  Inv_NoUseAfterFree s2 /\ Inv_ValidCaps s2 /\ Inv_NonNegative s2.
Proof.
  intros s1 s2 size cap total Hvalid Hfree.
  destruct Hvalid as [Hcons [Hnodf [Hnouaf [Hvcaps Hnonneg]]]].
  split. { eapply black_free_conserves; eauto. }
  split. { eapply blackfree_preserves_no_double_free; eauto. }
  split. { eapply blackfree_preserves_no_uaf; eauto. }
  split. { eapply blackfree_preserves_valid_caps; eauto. }
  eapply blackfree_preserves_nonneg; eauto.
Qed.

Theorem blackalloc_secure :
  forall s1 s2 size cap total,
  Valid s1 total -> BlackAlloc size cap s1 s2 ->
  Inv_Conservation s2 total /\ Inv_NoDoubleFree s2 /\
  Inv_NoUseAfterFree s2 /\ Inv_ValidCaps s2 /\ Inv_NonNegative s2.
Proof.
  intros s1 s2 size cap total Hvalid Halloc.
  destruct Hvalid as [Hcons [Hnodf [Hnouaf [Hvcaps Hnonneg]]]].
  split. { eapply black_alloc_conserves; eauto. }
  split. { eapply blackalloc_preserves_no_double_free; eauto. }
  split. { eapply blackalloc_preserves_no_uaf; eauto. }
  split. { eapply blackalloc_preserves_valid_caps; eauto. }
  eapply blackalloc_preserves_nonneg; eauto.
Qed.
