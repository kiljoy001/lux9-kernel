(** * Pebble Conservation - Core resource accounting
    * 
    * Imports: pebble/types
    * Proves: Memory conservation across all color transitions
    *)

Require Import pebble.types.

(* ========================================================================= *)
(* CONSERVATION INVARIANT                                                    *)
(* ========================================================================= *)

Definition Inv_Conservation (s : PebbleState) (total : Z) : Prop :=
  s.(colorless) + s.(black) + s.(blue) + s.(red) = total.

Definition Inv_NonNegative (s : PebbleState) : Prop :=
  s.(colorless) >= 0 /\
  s.(black) >= 0 /\
  s.(blue) >= 0 /\
  s.(red) >= 0 /\
  s.(white_pending) >= 0 /\
  s.(white_verified) >= 0.

(* ========================================================================= *)
(* TRANSITIONS                                                               *)
(* ========================================================================= *)

Inductive WhiteVerify (size : Z) (s1 s2 : PebbleState) : Prop :=
  | WV_Success :
      size > 0 ->
      s2 = mkPebble
             s1.(colorless) s1.(black) s1.(blue) s1.(red)
             (s1.(white_pending) + size)
             (s1.(white_verified) + 1)
             s1.(live_caps) s1.(freed_caps) s1.(next_cap_id)
             (s1.(pending_authorizations) + 1) ->
      WhiteVerify size s1 s2.

Inductive BlackAlloc (size : Z) (new_cap : CapId) (s1 s2 : PebbleState) : Prop :=
  | BA_Success :
      size > 0 ->
      s1.(white_verified) > 0 ->
      s1.(white_pending) >= size ->
      s1.(colorless) >= size ->
      new_cap = s1.(next_cap_id) ->
      ~ In new_cap s1.(live_caps) ->
      ~ In new_cap s1.(freed_caps) ->
      s2 = mkPebble
             (s1.(colorless) - size)
             (s1.(black) + size)
             s1.(blue) s1.(red)
             (s1.(white_pending) - size)
             (s1.(white_verified) - 1)
             (new_cap :: s1.(live_caps))
             s1.(freed_caps)
             (s1.(next_cap_id) + 1)
             s1.(pending_authorizations) ->
      BlackAlloc size new_cap s1 s2.

Inductive BlackFree (size : Z) (cap : CapId) (s1 s2 : PebbleState) : Prop :=
  | BF_Success :
      size > 0 ->
      s1.(black) >= size ->
      In cap s1.(live_caps) ->
      ~ In cap s1.(freed_caps) ->
      s2 = mkPebble
             (s1.(colorless) + size)
             (s1.(black) - size)
             s1.(blue) s1.(red)
             s1.(white_pending)
             s1.(white_verified)
             (remove_cap cap s1.(live_caps))
             (cap :: s1.(freed_caps))
             s1.(next_cap_id)
             s1.(pending_authorizations) ->
      BlackFree size cap s1 s2.

Inductive BlueAlloc (size : Z) (s1 s2 : PebbleState) : Prop :=
  | BlA_Success :
      size > 0 ->
      s1.(colorless) >= size ->
      s2 = mkPebble
             (s1.(colorless) - size) s1.(black) (s1.(blue) + size) s1.(red)
             s1.(white_pending) s1.(white_verified)
             s1.(live_caps) s1.(freed_caps) s1.(next_cap_id)
             s1.(pending_authorizations) ->
      BlueAlloc size s1 s2.

Inductive BlueFree (size : Z) (s1 s2 : PebbleState) : Prop :=
  | BlF_Success :
      size > 0 ->
      s1.(blue) >= size ->
      s2 = mkPebble
             (s1.(colorless) + size) s1.(black) (s1.(blue) - size) s1.(red)
             s1.(white_pending) s1.(white_verified)
             s1.(live_caps) s1.(freed_caps) s1.(next_cap_id)
             s1.(pending_authorizations) ->
      BlueFree size s1 s2.

Inductive RedAlloc (size : Z) (s1 s2 : PebbleState) : Prop :=
  | RA_Success :
      size > 0 ->
      s1.(colorless) >= size ->
      s2 = mkPebble
             (s1.(colorless) - size) s1.(black) s1.(blue) (s1.(red) + size)
             s1.(white_pending) s1.(white_verified)
             s1.(live_caps) s1.(freed_caps) s1.(next_cap_id)
             s1.(pending_authorizations) ->
      RedAlloc size s1 s2.

Inductive RedFree (size : Z) (s1 s2 : PebbleState) : Prop :=
  | RF_Success :
      size > 0 ->
      s1.(red) >= size ->
      s2 = mkPebble
             (s1.(colorless) + size) s1.(black) s1.(blue) (s1.(red) - size)
             s1.(white_pending) s1.(white_verified)
             s1.(live_caps) s1.(freed_caps) s1.(next_cap_id)
             s1.(pending_authorizations) ->
      RedFree size s1 s2.

(* ========================================================================= *)
(* CONSERVATION PROOFS                                                       *)
(* ========================================================================= *)

Theorem white_verify_conserves :
  forall s1 s2 size total,
  Inv_Conservation s1 total -> WhiteVerify size s1 s2 -> Inv_Conservation s2 total.
Proof.
  intros. inversion H0. subst. unfold Inv_Conservation in *. simpl. exact H.
Qed.

Theorem black_alloc_conserves :
  forall s1 s2 size cap total,
  Inv_Conservation s1 total -> BlackAlloc size cap s1 s2 -> Inv_Conservation s2 total.
Proof.
  intros. inversion H0. subst. unfold Inv_Conservation in *. simpl. lia.
Qed.

Theorem black_free_conserves :
  forall s1 s2 size cap total,
  Inv_Conservation s1 total -> BlackFree size cap s1 s2 -> Inv_Conservation s2 total.
Proof.
  intros. inversion H0. subst. unfold Inv_Conservation in *. simpl. lia.
Qed.

Theorem blue_alloc_conserves :
  forall s1 s2 size total,
  Inv_Conservation s1 total -> BlueAlloc size s1 s2 -> Inv_Conservation s2 total.
Proof.
  intros. inversion H0. subst. unfold Inv_Conservation in *. simpl. lia.
Qed.

Theorem blue_free_conserves :
  forall s1 s2 size total,
  Inv_Conservation s1 total -> BlueFree size s1 s2 -> Inv_Conservation s2 total.
Proof.
  intros. inversion H0. subst. unfold Inv_Conservation in *. simpl. lia.
Qed.

Theorem red_alloc_conserves :
  forall s1 s2 size total,
  Inv_Conservation s1 total -> RedAlloc size s1 s2 -> Inv_Conservation s2 total.
Proof.
  intros. inversion H0. subst. unfold Inv_Conservation in *. simpl. lia.
Qed.

Theorem red_free_conserves :
  forall s1 s2 size total,
  Inv_Conservation s1 total -> RedFree size s1 s2 -> Inv_Conservation s2 total.
Proof.
  intros. inversion H0. subst. unfold Inv_Conservation in *. simpl. lia.
Qed.
