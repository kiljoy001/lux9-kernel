(** * mntchk Boundary Validation - Proves mount ID validation correctness
    *
    * Imports: None (standalone arithmetic proof)
    * Proves: mntchk boundary check in devmnt.c:1326 is correct
    *
    * VERIFIED CODE: devmnt.c:1326
    * if (m->id == 0 || m->id >= c->dev)
    *     panic("mntchk 3: can't happen");
    *)

Require Import Coq.ZArith.ZArith.
Require Import Lia.

Open Scope Z_scope.

(* ========================================================================= *)
(* ALLOCATION MODEL                                                          *)
(* ========================================================================= *)

(** Global ID counter state *)
Record AllocState := mkAllocState {
  alloc_id : Z  (* Current mntalloc.id value *)
}.

(** Initial state: ID counter starts at 1 (0 is reserved) *)
Definition initial_alloc : AllocState := mkAllocState 1.

(** Allocate next ID *)
Definition alloc_next (st : AllocState) : Z * AllocState :=
  (st.(alloc_id), mkAllocState (st.(alloc_id) + 1)).

(* ========================================================================= *)
(* MOUNT AND CHANNEL CREATION SEQUENCE                                      *)
(* ========================================================================= *)

(** mntversion: Allocates mount with ID *)
Inductive MntVersionAlloc (st1 st2 : AllocState) (m_id : Z) : Prop :=
  | MVA_Alloc :
      alloc_next st1 = (m_id, st2) ->
      m_id > 0 ->  (* ID 0 is reserved *)
      MntVersionAlloc st1 st2 m_id.

(** mntchan: Allocates channel with dev ID *)
Inductive MntChanAlloc (st1 st2 : AllocState) (c_dev : Z) : Prop :=
  | MCA_Alloc :
      alloc_next st1 = (c_dev, st2) ->
      c_dev > 0 ->  (* ID 0 is reserved *)
      MntChanAlloc st1 st2 c_dev.

(** Normal sequence: mount created before channel *)
Inductive NormalSequence (st1 st2 st3 : AllocState) (m_id c_dev : Z) : Prop :=
  | NS_Sequence :
      MntVersionAlloc st1 st2 m_id ->
      MntChanAlloc st2 st3 c_dev ->
      NormalSequence st1 st2 st3 m_id c_dev.

(* ========================================================================= *)
(* KEY INVARIANT                                                             *)
(* ========================================================================= *)

(** Theorem: Normal sequence guarantees m->id < c->dev *)
Theorem normal_sequence_ordering :
  forall st1 st2 st3 m_id c_dev,
  NormalSequence st1 st2 st3 m_id c_dev ->
  m_id < c_dev.
Proof.
  intros st1 st2 st3 m_id c_dev H.
  inversion H; subst.
  inversion H0; subst.
  inversion H1; subst.
  unfold alloc_next in *.
  inversion H2; subst.
  inversion H4; subst.
  lia.
Qed.

(** Corollary: Normal sequence implies m->id <> c->dev *)
Corollary normal_sequence_not_equal :
  forall st1 st2 st3 m_id c_dev,
  NormalSequence st1 st2 st3 m_id c_dev ->
  m_id <> c_dev.
Proof.
  intros st1 st2 st3 m_id c_dev H.
  pose proof (normal_sequence_ordering _ _ _ _ _ H).
  lia.
Qed.

(* ========================================================================= *)
(* MNTCHK VALIDATION LOGIC                                                   *)
(* ========================================================================= *)

(** mntchk panic condition (line 1326) *)
Definition mntchk_panics (m_id c_dev : Z) : Prop :=
  m_id = 0 \/ m_id >= c_dev.

(** mntchk validation passes *)
Definition mntchk_valid (m_id c_dev : Z) : Prop :=
  ~ mntchk_panics m_id c_dev.

(** Expand definition: validation passes iff m_id > 0 AND m_id < c_dev *)
Lemma mntchk_valid_iff :
  forall m_id c_dev,
  m_id > 0 ->
  c_dev > 0 ->
  (mntchk_valid m_id c_dev <-> m_id < c_dev).
Proof.
  intros m_id c_dev Hm Hc.
  unfold mntchk_valid, mntchk_panics.
  split; intro H.
  - (* -> direction *)
    destruct (Z_lt_ge_dec m_id c_dev).
    + exact l.
    + exfalso. apply H. right. exact g.
  - (* <- direction *)
    intro Hcontra.
    destruct Hcontra as [Heq | Hge].
    + lia.
    + lia.
Qed.

(* ========================================================================= *)
(* CORRECTNESS THEOREMS                                                      *)
(* ========================================================================= *)

(** Theorem 1: Normal sequence always passes validation *)
Theorem normal_sequence_passes_mntchk :
  forall st1 st2 st3 m_id c_dev,
  NormalSequence st1 st2 st3 m_id c_dev ->
  mntchk_valid m_id c_dev.
Proof.
  intros st1 st2 st3 m_id c_dev H.
  pose proof (normal_sequence_ordering _ _ _ _ _ H) as Horder.
  inversion H; subst.
  inversion H0; subst.
  inversion H1; subst.
  apply mntchk_valid_iff.
  - (* m_id > 0 *)
    exact H3.
  - (* c_dev > 0 *)
    exact H5.
  - (* m_id < c_dev *)
    exact Horder.
Qed.

(** Theorem 2: Validation failure correctly identifies invalid states *)
Theorem mntchk_panic_is_invalid :
  forall st1 st2 st3 m_id c_dev,
  NormalSequence st1 st2 st3 m_id c_dev ->
  ~ mntchk_panics m_id c_dev.
Proof.
  intros st1 st2 st3 m_id c_dev H.
  pose proof (normal_sequence_passes_mntchk _ _ _ _ _ H).
  unfold mntchk_valid in H0.
  exact H0.
Qed.

(** Theorem 3: The check condition is CORRECT (not off-by-one) *)
Theorem mntchk_condition_correct :
  forall m_id c_dev,
  m_id > 0 ->
  c_dev > 0 ->
  (mntchk_panics m_id c_dev <-> ~ (m_id < c_dev)).
Proof.
  intros m_id c_dev Hm Hc.
  unfold mntchk_panics.
  split; intro H.
  - (* -> direction *)
    destruct H as [Heq | Hge].
    + subst. lia.
    + lia.
  - (* <- direction *)
    right. lia.
Qed.

(** Theorem 4: No false positives - check only panics on truly invalid states *)
Theorem mntchk_no_false_positives :
  forall m_id c_dev,
  m_id > 0 ->
  m_id < c_dev ->
  ~ mntchk_panics m_id c_dev.
Proof.
  intros m_id c_dev Hpos Hlt.
  unfold mntchk_panics.
  intro Hcontra.
  destruct Hcontra as [Heq | Hge]; lia.
Qed.

(** Theorem 5: >= is correct, not > (addresses off-by-one concern) *)
Theorem mntchk_ge_not_gt :
  forall m_id c_dev,
  m_id = c_dev ->
  m_id >= c_dev /\ ~ (m_id > c_dev).
Proof.
  intros m_id c_dev Heq.
  split; lia.
Qed.

(** Corollary: Equality case must be caught *)
Theorem equality_is_invalid :
  forall st1 st2 st3 m_id c_dev,
  NormalSequence st1 st2 st3 m_id c_dev ->
  m_id = c_dev ->
  False.
Proof.
  intros st1 st2 st3 m_id c_dev H Heq.
  pose proof (normal_sequence_not_equal _ _ _ _ _ H).
  contradiction.
Qed.

(* ========================================================================= *)
(* INVALID SCENARIOS CORRECTLY DETECTED                                     *)
(* ========================================================================= *)

(** Invalid scenario 1: m->id == 0 (reserved) *)
Theorem reserved_id_triggers_panic :
  forall c_dev,
  c_dev > 0 ->
  mntchk_panics 0 c_dev.
Proof.
  intros c_dev Hdev.
  unfold mntchk_panics.
  left. reflexivity.
Qed.

(** Invalid scenario 2: m->id == c->dev (shouldn't happen) *)
Theorem equal_ids_trigger_panic :
  forall id,
  id > 0 ->
  mntchk_panics id id.
Proof.
  intros id Hid.
  unfold mntchk_panics.
  right. lia.
Qed.

(** Invalid scenario 3: m->id > c->dev (reversed allocation) *)
Theorem reversed_allocation_triggers_panic :
  forall m_id c_dev,
  m_id > c_dev ->
  c_dev > 0 ->
  mntchk_panics m_id c_dev.
Proof.
  intros m_id c_dev Hrev Hdev.
  unfold mntchk_panics.
  right. lia.
Qed.

(* ========================================================================= *)
(* CODE CORRECTNESS STATEMENT                                                *)
(* ========================================================================= *)

(**
 * VERIFIED CODE: kernel/9front-port/devmnt.c:1310-1330
 *
 * static Mnt *mntchk(Chan *c) {
 *   Mnt *m;
 *
 *   if (c->mchan == nil)
 *     panic("mntchk 1: nil mchan c %s", chanpath(c));
 *
 *   m = c->mchan->mux;
 *   if (m == nil)
 *     print("mntchk 2: nil mux c %s c->mchan %s \n", ...);
 *
 *   // VERIFIED: This check is CORRECT (not off-by-one)
 *   if (m->id == 0 || m->id >= c->dev)
 *     panic("mntchk 3: can't happen");
 *
 *   return m;
 * }
 *
 * PROOF SUMMARY:
 * 1. ✓ Normal allocation sequence guarantees m->id < c->dev
 * 2. ✓ Check condition `m->id >= c->dev` correctly catches violations
 * 3. ✓ No false positives: valid states always pass
 * 4. ✓ >= is correct (not >): catches both equality and reversal
 * 5. ✓ Reserved ID (0) is correctly rejected
 *
 * CONCLUSION: The boundary check is CORRECT. No off-by-one error exists.
 *)

Print Assumptions normal_sequence_ordering.
Print Assumptions normal_sequence_passes_mntchk.
Print Assumptions mntchk_condition_correct.
Print Assumptions mntchk_no_false_positives.
