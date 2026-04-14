(* Argon2 model based on Monocypher's block operations. *)

Require Import Coq.Lists.List.
Require Import Coq.NArith.NArith.
Require Import Coq.Arith.PeanoNat.
Require Import Coq.Bool.Bool.
Require Import Coq.micromega.Lia.

Import ListNotations.

Definition word64 := N.
Definition mask64 : N := 2 ^ 64.
Definition mask32 : N := 2 ^ 32.
Definition u32_max : N := (2 ^ 32 - 1)%N.

Definition mod64 (x : N) : N := N.modulo x mask64.
Definition lsb32 (x : N) : N := N.modulo x mask32.

Definition add64 (x y : N) : N := mod64 (x + y).
Definition xor64 (x y : N) : N := N.lxor x y.

Definition shl64 (x n : N) : N := mod64 (N.shiftl x n).
Definition shr64 (x n : N) : N := N.shiftr x n.

Definition rotr64 (x n : N) : N :=
  let n' := N.modulo n 64 in
  let left := shl64 x (64 - n') in
  let right := shr64 x n' in
  mod64 (N.lor left right).

Definition Block := list word64.

Definition block_len : nat := 128.

Definition block_get (b : Block) (i : nat) : word64 :=
  nth i b 0%N.

Definition block_set (b : Block) (i : nat) (v : word64) : Block :=
  firstn i b ++ [v] ++ skipn (S i) b.

Definition block_xor (b1 b2 : Block) : Block :=
  map (fun xy => xor64 (fst xy) (snd xy)) (combine b1 b2).

Lemma block_set_length :
  forall b i v,
    i < length b ->
    length (block_set b i v) = length b.
Proof.
  intros b i v Hlt.
  unfold block_set.
  rewrite length_app.
  rewrite length_app.
  rewrite length_skipn.
  simpl.
  rewrite firstn_length_le by lia.
  lia.
Qed.

Lemma block_xor_length :
  forall b1 b2,
    length (block_xor b1 b2) = Nat.min (length b1) (length b2).
Proof.
  intros b1 b2.
  unfold block_xor.
  rewrite length_map.
  rewrite length_combine.
  reflexivity.
Qed.

Definition g_step (a b c d : word64) : (word64 * word64 * word64 * word64) :=
  let a1 := add64 a (add64 b (shl64 (lsb32 a * lsb32 b) 1)) in
  let d1 := rotr64 (xor64 d a1) 32 in
  let c1 := add64 c (add64 d1 (shl64 (lsb32 c * lsb32 d1) 1)) in
  let b1 := rotr64 (xor64 b c1) 24 in
  let a2 := add64 a1 (add64 b1 (shl64 (lsb32 a1 * lsb32 b1) 1)) in
  let d2 := rotr64 (xor64 d1 a2) 16 in
  let c2 := add64 c1 (add64 d2 (shl64 (lsb32 c1 * lsb32 d2) 1)) in
  let b2 := rotr64 (xor64 b1 c2) 63 in
  (a2, b2, c2, d2).

Definition apply_g (b : Block) (i j k l : nat) : Block :=
  let a := block_get b i in
  let b0 := block_get b j in
  let c := block_get b k in
  let d := block_get b l in
  let '(a1, b1, c1, d1) := g_step a b0 c d in
  let b' := block_set b i a1 in
  let b'' := block_set b' j b1 in
  let b''' := block_set b'' k c1 in
  block_set b''' l d1.

Definition mono_round_indices (b : Block) (idxs : list nat) : Block :=
  match idxs with
  | i0 :: i1 :: i2 :: i3 :: i4 :: i5 :: i6 :: i7 ::
    i8 :: i9 :: i10 :: i11 :: i12 :: i13 :: i14 :: i15 :: nil =>
      let b0 := apply_g b i0 i4 i8 i12 in
      let b1 := apply_g b0 i1 i5 i9 i13 in
      let b2 := apply_g b1 i2 i6 i10 i14 in
      let b3 := apply_g b2 i3 i7 i11 i15 in
      let b4 := apply_g b3 i0 i5 i10 i15 in
      let b5 := apply_g b4 i1 i6 i11 i12 in
      let b6 := apply_g b5 i2 i7 i8 i13 in
      apply_g b6 i3 i4 i9 i14
  | _ => b
  end.

Definition mono_round (b : Block) (base : nat) : Block :=
  mono_round_indices b
    [base + 0; base + 1; base + 2; base + 3;
     base + 4; base + 5; base + 6; base + 7;
     base + 8; base + 9; base + 10; base + 11;
     base + 12; base + 13; base + 14; base + 15].

Fixpoint column_rounds (b : Block) (i : nat) : Block :=
  match i with
  | 0 => mono_round b 0
  | S i' => column_rounds (mono_round b (16 * i)) i'
  end.

Fixpoint row_rounds (b : Block) (i : nat) : Block :=
  match i with
  | 0 =>
      mono_round_indices b
        [0; 1; 16; 17; 32; 33; 48; 49; 64; 65; 80; 81; 96; 97; 112; 113]
  | S i' =>
      let base := 2 * i in
      let idxs :=
        [base + 0; base + 1;
         base + 16; base + 17;
         base + 32; base + 33;
         base + 48; base + 49;
         base + 64; base + 65;
         base + 80; base + 81;
         base + 96; base + 97;
         base + 112; base + 113] in
      row_rounds (mono_round_indices b idxs) i'
  end.

Definition g_rounds (b : Block) : Block :=
  let b1 := column_rounds b 7 in
  row_rounds b1 7.

Definition P (b : Block) : Block := g_rounds b.

Definition G (X Y : Block) : Block :=
  let Z := block_xor X Y in
  block_xor (P Z) Z.

Record Argon2Config : Type := mkArgon2Config {
  lanes : nat;
  slices : nat;
  passes : nat;
  blocks_per_lane : nat;
}.

Record BlockAddr : Type := mkBlockAddr {
  pass_num : nat;
  slice_num : nat;
  lane_num : nat;
  block_index : nat;
}.

Definition phi (addr : BlockAddr) : nat :=
  match block_index addr with
  | 0 => 0
  | S n => n
  end.

Definition nat_to_N (n : nat) : N := N.of_nat n.

Definition lane_select (pass slice segment nb_lanes : nat) (index_seed : N) : nat :=
  if Nat.eqb pass 0 && Nat.eqb slice 0 then
    segment
  else
    N.to_nat (N.modulo (N.shiftr index_seed 32) (nat_to_N nb_lanes)).

Definition window_start (pass slice segment_size : nat) : nat :=
  if Nat.eqb pass 0 then 0
  else ((S slice) mod 4) * segment_size.

Definition window_size
  (pass slice segment block lane segment_size : nat) : N :=
  let nb_segments :=
    if Nat.eqb pass 0 then nat_to_N slice else 3%N in
  let base := N.mul nb_segments (nat_to_N segment_size) in
  if Nat.eqb lane segment then
    N.add base (nat_to_N (block - 1))
  else if Nat.eqb block 0 then
    N.add base u32_max
  else
    base.

Definition reference_index
  (pass slice segment block nb_lanes segment_size lane_size : nat)
  (index_seed : N) : nat :=
  let lane := lane_select pass slice segment nb_lanes index_seed in
  let ws := window_size pass slice segment block lane segment_size in
  let j1 := N.modulo index_seed mask32 in
  let x := N.shiftr (N.mul j1 j1) 32 in
  let y := N.shiftr (N.mul ws x) 32 in
  let z := N.sub (N.sub ws 1%N) y in
  let ref := N.modulo (N.add (nat_to_N (window_start pass slice segment_size)) z)
                       (nat_to_N lane_size) in
  N.to_nat ref.
