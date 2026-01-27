(** * Pebble Global Budget - PoW-gated token transfers
    *
    * Models global pool transfers and PoW gating as in pebble_increase_budget().
    *)

Require Import pebble.types.
Require Import pebble.conservation.
Require Import Lia.

Open Scope Z_scope.

(* ========================================================================= *)
(* SYSTEM STATE                                                              *)
(* ========================================================================= *)

Record SystemState := mkSystem {
  global_bank : Z;
  total_tokens : Z;
  pid : Z;
  proc : PebbleState;
}.

Definition SysConservation (s : SystemState) : Prop :=
  global_bank s + BudgetPotential (proc s) = total_tokens s.

Definition SysNonNegative (s : SystemState) : Prop :=
  global_bank s >= 0 /\ Inv_NonNegative (proc s).

(* Helper to update the per-process colorless bank. *)
Definition proc_add_colorless (ps : PebbleState) (delta : Z) : PebbleState :=
  mkPebble
    (colorless ps + delta)
    (black ps)
    (blue ps)
    (red ps)
    (white_pending ps)
    (white_verified ps)
    (live_caps ps)
    (freed_caps ps)
    (next_cap_id ps)
    (pending_authorizations ps).

(* ========================================================================= *)
(* PoW-GATED BUDGET INCREASE                                                 *)
(* ========================================================================= *)

Inductive IncreaseBudget
          (PowValid : Z -> Z -> Z -> Prop)
          (size nonce : Z)
          (s1 s2 : SystemState) : Prop :=
  | IB_Success :
      size > 0 ->
      global_bank s1 >= size ->
      global_bank s1 - size >= 0 ->
      PowValid size nonce (pid s1) ->
      s2 = mkSystem
             (global_bank s1 - size)
             (total_tokens s1)
             (pid s1)
             (proc_add_colorless (proc s1) size) ->
      IncreaseBudget PowValid size nonce s1 s2.

(* Return tokens to the global pool (cleanup path). *)
Inductive ReturnBudget (size : Z) (s1 s2 : SystemState) : Prop :=
  | RB_Success :
      size > 0 ->
      colorless (proc s1) >= size ->
      s2 = mkSystem
             (global_bank s1 + size)
             (total_tokens s1)
             (pid s1)
             (proc_add_colorless (proc s1) (-size)) ->
      ReturnBudget size s1 s2.

(* ========================================================================= *)
(* PROPERTIES                                                                *)
(* ========================================================================= *)

Theorem increase_budget_requires_pow :
  forall PowValid s1 s2 size nonce,
  IncreaseBudget PowValid size nonce s1 s2 ->
  PowValid size nonce (pid s1).
Proof.
  intros PowValid s1 s2 size nonce Hinc.
  inversion Hinc; subst. assumption.
Qed.

Theorem increase_budget_preserves_conservation :
  forall PowValid s1 s2 size nonce,
  SysConservation s1 ->
  IncreaseBudget PowValid size nonce s1 s2 ->
  SysConservation s2.
Proof.
  intros PowValid s1 s2 size nonce Hcons Hinc.
  inversion Hinc; subst.
  unfold SysConservation, BudgetPotential in *. simpl in *.
  lia.
Qed.

Theorem return_budget_preserves_conservation :
  forall s1 s2 size,
  SysConservation s1 ->
  ReturnBudget size s1 s2 ->
  SysConservation s2.
Proof.
  intros s1 s2 size Hcons Hret.
  inversion Hret; subst.
  unfold SysConservation, BudgetPotential in *. simpl in *.
  lia.
Qed.

Theorem increase_budget_preserves_nonneg :
  forall PowValid s1 s2 size nonce,
  SysNonNegative s1 ->
  IncreaseBudget PowValid size nonce s1 s2 ->
  SysNonNegative s2.
Proof.
  intros PowValid s1 s2 size nonce Hnn Hinc.
  inversion Hinc as [Hsz Hgbsize Hgbnonneg Hpow Heq]; subst.
  unfold SysNonNegative in *.
  destruct Hnn as [Hgb Hpn].
  split.
  - exact Hgbnonneg.
  - unfold proc_add_colorless. cbn.
    unfold Inv_NonNegative in *.
    destruct Hpn as [Hc [Hb [Hbl [Hr [Hwp Hwv]]]]].
    destruct (proc s1) as [c b bl r wp wv live freed next pend] eqn:Hps.
    simpl in *.
    repeat split.
    + apply Z.le_ge.
      apply Z.add_nonneg_nonneg; [apply Z.ge_le; exact Hc | lia].
    + exact Hb.
    + exact Hbl.
    + exact Hr.
    + exact Hwp.
    + exact Hwv.
Qed.

Theorem return_budget_preserves_nonneg :
  forall s1 s2 size,
  SysNonNegative s1 ->
  ReturnBudget size s1 s2 ->
  SysNonNegative s2.
Proof.
  intros s1 s2 size Hnn Hret.
  inversion Hret as [Hsz Hcsize Heq]; subst.
  unfold SysNonNegative in *.
  destruct Hnn as [Hgb Hpn].
  split.
  - apply Z.le_ge.
    apply Z.add_nonneg_nonneg; [apply Z.ge_le; exact Hgb | lia].
  - unfold proc_add_colorless. cbn.
    unfold Inv_NonNegative in *.
    destruct Hpn as [Hc [Hb [Hbl [Hr [Hwp Hwv]]]]].
    destruct (proc s1) as [c b bl r wp wv live freed next pend] eqn:Hps.
    simpl in *.
    repeat split.
    + apply Z.le_ge. lia.
    + exact Hb.
    + exact Hbl.
    + exact Hr.
    + exact Hwp.
    + exact Hwv.
Qed.
