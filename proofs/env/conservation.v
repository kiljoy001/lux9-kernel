(** * Environment Conservation - Reference counting and allocation bounds
    * 
    * Imports: env/types
    * Proves: Reference counting correctness, allocation limit preservation
    *)

Require Import env.types.

(* ========================================================================= *)
(* REFERENCE COUNTING TRANSITIONS                                            *)
(* ========================================================================= *)

(** newegrp: Creates new environment group with ref=1 *)
Inductive NewEgrp (eg : EgrpState) : Prop :=
  | NE_Success :
      eg = initial_egrp ->
      NewEgrp eg.

(** incref: Increments reference count *)
Inductive IncRef (eg1 eg2 : EgrpState) : Prop :=
  | IR_Success :
      eg1.(eg_ref) >= 1 ->
      eg2 = incr_ref eg1 ->
      IncRef eg1 eg2.

(** decref: Decrements reference count *)
Inductive DecRef (eg1 eg2 : EgrpState) : Prop :=
  | DR_Success :
      eg1.(eg_ref) > 1 ->
      eg2 = decr_ref eg1 ->
      DecRef eg1 eg2.

(** closeegrp: Final decrement leads to ref=0 or confegrp exception *)
Inductive CloseEgrp (eg1 eg2 : EgrpState) : Prop :=
  | CE_LastRef :
      eg1.(eg_ref) = 1 ->
      eg2.(eg_ref) = 0 ->
      CloseEgrp eg1 eg2
  | CE_MoreRefs :
      eg1.(eg_ref) > 1 ->
      eg2 = decr_ref eg1 ->
      CloseEgrp eg1 eg2.

(* ========================================================================= *)
(* ALLOCATION TRANSITIONS                                                    *)
(* ========================================================================= *)

(** envcreate: Adds new entry, increases allocation *)
Inductive EnvCreate (size : Z) (eg1 eg2 : EgrpState) : Prop :=
  | EC_Success :
      size > 0 ->
      eg1.(eg_alloc) + size <= Maxenvsize ->
      eg2 = add_entry_alloc eg1 size ->
      EnvCreate size eg1 eg2.

(** envremove: Removes entry, decreases allocation *)
Inductive EnvRemove (size : Z) (eg1 eg2 : EgrpState) : Prop :=
  | ER_Success :
      size > 0 ->
      eg1.(eg_alloc) >= size ->
      eg2 = remove_entry_alloc eg1 size ->
      EnvRemove size eg1 eg2.

(** envwrite: May increase allocation if value grows *)
Inductive EnvWrite (diff : Z) (eg1 eg2 : EgrpState) : Prop :=
  | EW_Grow :
      diff > 0 ->
      eg1.(eg_alloc) + diff <= Maxenvsize ->
      eg2 = mkEgrp eg1.(eg_ref) eg1.(eg_nent) (eg1.(eg_alloc) + diff)
                   (eg1.(eg_vers) + 1) eg1.(eg_path) eg1.(eg_low) ->
      EnvWrite diff eg1 eg2
  | EW_NoGrow :
      diff <= 0 ->
      eg2 = mkEgrp eg1.(eg_ref) eg1.(eg_nent) eg1.(eg_alloc)
                   (eg1.(eg_vers) + 1) eg1.(eg_path) eg1.(eg_low) ->
      EnvWrite diff eg1 eg2.

(* ========================================================================= *)
(* REFERENCE COUNTING PROOFS                                                 *)
(* ========================================================================= *)

Theorem newegrp_creates_ref :
  forall eg, NewEgrp eg -> eg.(eg_ref) = 1.
Proof.
  intros eg H. inversion H. subst.
  unfold initial_egrp. simpl. reflexivity.
Qed.

Theorem newegrp_wellformed :
  forall eg, NewEgrp eg -> Inv_WellFormed eg.
Proof.
  intros eg H. inversion H. subst.
  apply initial_egrp_wellformed.
Qed.

Theorem incref_increments :
  forall eg1 eg2, IncRef eg1 eg2 -> eg2.(eg_ref) = eg1.(eg_ref) + 1.
Proof.
  intros eg1 eg2 H. inversion H. subst.
  unfold incr_ref. simpl. reflexivity.
Qed.

Theorem decref_decrements :
  forall eg1 eg2, DecRef eg1 eg2 -> eg2.(eg_ref) = eg1.(eg_ref) - 1.
Proof.
  intros eg1 eg2 H. inversion H. subst.
  unfold decr_ref. simpl. reflexivity.
Qed.

Theorem incref_preserves_wf :
  forall eg1 eg2,
  Inv_WellFormed eg1 -> IncRef eg1 eg2 -> Inv_WellFormed eg2.
Proof.
  intros eg1 eg2 [Href [Halloc Hent]] H.
  inversion H. subst.
  unfold Inv_WellFormed, Inv_RefPositive, Inv_AllocBounded, Inv_EntriesNonNeg.
  unfold incr_ref. simpl.
  unfold Inv_RefPositive in Href.
  unfold Inv_AllocBounded in Halloc.
  unfold Inv_EntriesNonNeg in Hent.
  lia.
Qed.

Theorem decref_preserves_wf :
  forall eg1 eg2,
  Inv_WellFormed eg1 -> DecRef eg1 eg2 -> Inv_WellFormed eg2.
Proof.
  intros eg1 eg2 [Href [Halloc Hent]] H.
  inversion H. subst.
  unfold Inv_WellFormed, Inv_RefPositive, Inv_AllocBounded, Inv_EntriesNonNeg.
  unfold decr_ref. simpl.
  unfold Inv_RefPositive in Href.
  unfold Inv_AllocBounded in Halloc.
  unfold Inv_EntriesNonNeg in Hent.
  lia.
Qed.

(* ========================================================================= *)
(* ALLOCATION BOUND PROOFS                                                   *)
(* ========================================================================= *)

Theorem envcreate_bounded :
  forall size eg1 eg2,
  Inv_AllocBounded eg1 -> EnvCreate size eg1 eg2 -> Inv_AllocBounded eg2.
Proof.
  intros size eg1 eg2 Halloc H.
  inversion H. subst.
  unfold Inv_AllocBounded, add_entry_alloc in *. simpl.
  lia.
Qed.

Theorem envremove_bounded :
  forall size eg1 eg2,
  Inv_AllocBounded eg1 -> EnvRemove size eg1 eg2 -> Inv_AllocBounded eg2.
Proof.
  intros size eg1 eg2 Halloc H.
  inversion H. subst.
  unfold Inv_AllocBounded, remove_entry_alloc in *. simpl.
  lia.
Qed.

Theorem envwrite_bounded :
  forall diff eg1 eg2,
  Inv_AllocBounded eg1 -> EnvWrite diff eg1 eg2 -> Inv_AllocBounded eg2.
Proof.
  intros diff eg1 eg2 Halloc H.
  inversion H; subst; unfold Inv_AllocBounded in *; simpl; lia.
Qed.

(* ========================================================================= *)
(* INVERSE THEOREMS                                                          *)
(* ========================================================================= *)

Theorem incref_decref_inverse :
  forall eg1 eg2 eg3,
  IncRef eg1 eg2 -> DecRef eg2 eg3 ->
  eg3.(eg_ref) = eg1.(eg_ref).
Proof.
  intros eg1 eg2 eg3 Hi Hd.
  inversion Hi. inversion Hd. subst.
  unfold incr_ref, decr_ref. simpl. lia.
Qed.

Theorem create_remove_allocation_inverse :
  forall size eg1 eg2 eg3,
  size > 0 ->
  EnvCreate size eg1 eg2 -> EnvRemove size eg2 eg3 ->
  eg3.(eg_alloc) = eg1.(eg_alloc).
Proof.
  intros size eg1 eg2 eg3 Hsize Hc Hr.
  inversion Hc. inversion Hr. subst.
  unfold add_entry_alloc, remove_entry_alloc. simpl. lia.
Qed.

Theorem alloc_never_exceeds_max :
  forall eg,
  Inv_AllocBounded eg -> eg.(eg_alloc) <= Maxenvsize.
Proof.
  intros eg [_ H]. assumption.
Qed.

Theorem closeegrp_frees_on_last_ref :
  forall eg1 eg2,
  CloseEgrp eg1 eg2 -> eg1.(eg_ref) = 1 -> eg2.(eg_ref) = 0.
Proof.
  intros eg1 eg2 H Href.
  inversion H; subst.
  - assumption.
  - lia.
Qed.
