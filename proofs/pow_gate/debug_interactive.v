Require Import Coq.ZArith.ZArith.
Require Import Coq.ZArith.Zdiv.
Require Import Coq.Lists.List.
Require Import Coq.Bool.Bool.
Require Import Lia.

Open Scope Z_scope.

Inductive OpClass : Set :=
  | POW_OP_ALLOC
  | POW_OP_STACK_ALLOC
  | POW_OP_SPAWN
  | POW_OP_NET_BIND
  | POW_OP_REALTIME
  | POW_OP_DEFAULT.

Definition base_difficulty (op : OpClass) : Z :=
  match op with
  | POW_OP_ALLOC      => 1
  | POW_OP_STACK_ALLOC => 1
  | POW_OP_SPAWN      => 12
  | POW_OP_NET_BIND   => 8
  | POW_OP_REALTIME   => 16
  | POW_OP_DEFAULT    => 4
  end.

Definition alloc_magnitude_scaling (magnitude : Z) : Z :=
  let mag := Z.max magnitude 4096 in
  let mb64 := 64 * 1024 * 1024 in
  let base := 1 + (mag / mb64) in
  if mag <? mb64 then
    let penalty := Z.min 6 (Z.log2 (mb64 / mag)) in
    base + penalty
  else
    base.

Definition stack_alloc_scaling (magnitude : Z) : Z :=
  if magnitude <? 1024 then 1
  else 2 + (magnitude / (16 * 1024 * 1024)).

Definition calculate_difficulty (op : OpClass) (magnitude : Z) (congestion : Z) : Z :=
  let raw_diff := 
    match op with
    | POW_OP_ALLOC => alloc_magnitude_scaling magnitude
    | POW_OP_STACK_ALLOC => stack_alloc_scaling magnitude
    | _ => base_difficulty op
    end
  in
  Z.min 32 (raw_diff + congestion).

Definition raw_diff (op : OpClass) (magnitude : Z) : Z :=
  match op with
  | POW_OP_ALLOC => alloc_magnitude_scaling magnitude
  | POW_OP_STACK_ALLOC => stack_alloc_scaling magnitude
  | _ => base_difficulty op
  end.

Lemma raw_diff_ge_1 :
  forall op magnitude,
  1 <= raw_diff op magnitude.
Proof.
  intros op magnitude.
  unfold raw_diff.
  destruct op; simpl.
  - (* POW_OP_ALLOC *)
    unfold alloc_magnitude_scaling.
    set (mb64 := 64 * 1024 * 1024).
    assert (Hmb64_pos: mb64 > 0) by (unfold mb64; lia).
    
    destruct (magnitude <? 4096) eqn:Hmag_lt_4096.
    + (* Case: magnitude < 4096 *)
      rewrite Z.max_r with (n:=magnitude) (m:=4096); [|lia].
      set (mag := 4096).
      assert (Hmag_div_mb64_is_0: mag / mb64 = 0) by (unfold mag, mb64; apply Z.div_small; lia).
      destruct (mag <? mb64) eqn:Hmag_lt_mb64.
      * (* Small allocation *)
        assert (Hmag_pos: mag > 0) by lia.
        assert (mb64_div_mag_val: mb64 / mag = 16384) by (unfold mag, mb64; vm_compute; reflexivity).
        assert (Hmb64_div_mag_pos: mb64 / mag > 0) by lia.
        set (penalty := Z.min 6 (Z.log2 (mb64 / mag))).
        assert (Hpenalty_val: penalty = 6) by (unfold penalty, mag, mb64; vm_compute; reflexivity).
        rewrite Hpenalty_val.
        rewrite Hmag_div_mb64_is_0.
        lia.
      * (* Large allocation *)
        lia.
    + (* Case: magnitude >= 4096 *)
      rewrite Z.max_l with (n:=magnitude) (m:=4096); [|lia].
      set (mag := magnitude).
      assert (Hm: mag >= 4096) by lia.
      assert (Hmag_ge0: mag >= 0) by lia.
      
      (* DEBUG POINT: Inspect state before apply Z.div_pos *)
      assert (Hmag_div_mb64_ge0: 0 <= mag / (64 * 1024 * 1024)).
      apply Z.div_pos.
