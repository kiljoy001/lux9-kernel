(** * Wire Format - Binary encoding/decoding for 9P protocol
    *
    * Imports: channel_model.v (for Qid)
    * Defines: Byte-level encoding/decoding with bijection proofs
    *
    * Used by: fcall_model.v, serialization.v
    *
    * IMPLEMENTATION: kernel/include/fcall.h:66-104 (GBIT/PBIT macros)
    *                 kernel/libc9/convM2S.c (deserialization)
    *                 kernel/libc9/convS2M.c (serialization)
    *)

Require Import Coq.ZArith.ZArith.
Require Import Coq.Lists.List.
Require Import Coq.Bool.Bool.
Require Import Coq.Arith.Arith.
Require Import Lia.
Require Import Psatz.
Import ListNotations.

Require Import mnt.channel_model.

Open Scope Z_scope.

(* ========================================================================= *)
(* BYTE REPRESENTATION                                                       *)
(* ========================================================================= *)

(** Bytes are Z values constrained to [0, 255] *)
Definition Byte : Type := Z.

Definition valid_byte (b : Byte) : Prop := 0 <= b < 256.

(** Byte sequence for wire format *)
Definition Bytes := list Byte.

(** Well-formed byte sequence - all bytes are valid *)
Definition bytes_wellformed (bs : Bytes) : Prop :=
  Forall valid_byte bs.

(* ========================================================================= *)
(* HELPER LEMMAS                                                             *)
(* ========================================================================= *)

(** Helper: land with 255 extracts low byte *)
Lemma land_255_bound : forall n,
  0 <= n < 256 ->
  Z.land n 255 = n.
Proof.
  intros n [H0 H256].
  assert (H255: 255 = Z.ones 8).
  { unfold Z.ones. simpl. reflexivity. }
  rewrite H255.
  rewrite Z.land_ones by lia.
  apply Z.mod_small. lia.
Qed.

(** Helper: land 255 always gives byte *)
Lemma land_255_valid : forall n,
  0 <= n ->
  valid_byte (Z.land n 255).
Proof.
  intros n Hn.
  unfold valid_byte.
  assert (H255: 255 = Z.ones 8).
  { unfold Z.ones. simpl. reflexivity. }
  rewrite H255.
  rewrite Z.land_ones by lia.
  split.
  - apply Z.mod_pos_bound. lia.
  - apply Z.mod_pos_bound. lia.
Qed.

(** Helper: shiftr preserves non-negativity *)
Lemma shiftr_nonneg : forall n k,
  0 <= n ->
  0 <= Z.shiftr n k.
Proof.
  intros n k Hn.
  apply Z.shiftr_nonneg. assumption.
Qed.

(** Helper: land 255 extracts low byte from multi-byte value *)
Lemma land_255_add_mul_256 : forall b0 b1,
  0 <= b0 < 256 ->
  0 <= b1 ->
  Z.land (b0 + b1 * 256) 255 = b0.
Proof.
  intros b0 b1 Hb0 Hb1.
  assert (H255: 255 = Z.ones 8) by reflexivity.
  rewrite H255.
  rewrite Z.land_ones by lia.
  assert (Hmod: (b0 + b1 * 256) mod 256 = b0).
  { assert (H: b1 * 256 mod 256 = 0).
    { apply Z.mod_mul. lia. }
    rewrite (Z.add_mod b0 (b1 * 256) 256) by lia.
    rewrite H.
    rewrite Z.add_0_r.
    rewrite Z.mod_mod by lia.
    apply Z.mod_small. lia. }
  exact Hmod.
Qed.

(* ========================================================================= *)
(* LITTLE-ENDIAN ENCODING PRIMITIVES                                         *)
(* ========================================================================= *)

(** Encode 8-bit unsigned integer *)
Definition encode_u8 (n : Z) : Bytes :=
  [Z.land n 255].

(** Encode 16-bit unsigned integer (little-endian) *)
Definition encode_u16 (n : Z) : Bytes :=
  [ Z.land n 255;
    Z.land (Z.shiftr n 8) 255 ].

(** Encode 32-bit unsigned integer (little-endian) *)
Definition encode_u32 (n : Z) : Bytes :=
  [ Z.land n 255;
    Z.land (Z.shiftr n 8) 255;
    Z.land (Z.shiftr n 16) 255;
    Z.land (Z.shiftr n 24) 255 ].

(** Encode 64-bit unsigned integer (little-endian) *)
Definition encode_u64 (n : Z) : Bytes :=
  [ Z.land n 255;
    Z.land (Z.shiftr n 8) 255;
    Z.land (Z.shiftr n 16) 255;
    Z.land (Z.shiftr n 24) 255;
    Z.land (Z.shiftr n 32) 255;
    Z.land (Z.shiftr n 40) 255;
    Z.land (Z.shiftr n 48) 255;
    Z.land (Z.shiftr n 56) 255 ].

(* ========================================================================= *)
(* DECODING PRIMITIVES                                                       *)
(* ========================================================================= *)

(** Decode 8-bit unsigned integer *)
Definition decode_u8 (bs : Bytes) : option Z :=
  match bs with
  | b :: _ => Some b
  | [] => None
  end.

(** Decode 16-bit unsigned integer (little-endian) *)
Definition decode_u16 (bs : Bytes) : option Z :=
  match bs with
  | b0 :: b1 :: _ =>
      Some (b0 + b1 * 256)
  | _ => None
  end.

(** Decode 32-bit unsigned integer (little-endian) *)
Definition decode_u32 (bs : Bytes) : option Z :=
  match bs with
  | b0 :: b1 :: b2 :: b3 :: _ =>
      Some (b0 + b1 * 256 + b2 * 65536 + b3 * 16777216)
  | _ => None
  end.

(** Decode 64-bit unsigned integer (little-endian) *)
Definition decode_u64 (bs : Bytes) : option Z :=
  match bs with
  | b0 :: b1 :: b2 :: b3 :: b4 :: b5 :: b6 :: b7 :: _ =>
      Some (b0 +
            b1 * 256 +
            b2 * 65536 +
            b3 * 16777216 +
            b4 * 4294967296 +
            b5 * 1099511627776 +
            b6 * 281474976710656 +
            b7 * 72057594037927936)
  | _ => None
  end.

(* ========================================================================= *)
(* ENCODING PROPERTIES                                                       *)
(* ========================================================================= *)

(** Theorem: encode_u8 produces exactly 1 byte *)
Theorem encode_u8_length : forall n,
  length (encode_u8 n) = 1%nat.
Proof.
  intro n. unfold encode_u8. reflexivity.
Qed.

(** Theorem: encode_u16 produces exactly 2 bytes *)
Theorem encode_u16_length : forall n,
  length (encode_u16 n) = 2%nat.
Proof.
  intro n. unfold encode_u16. reflexivity.
Qed.

(** Theorem: encode_u32 produces exactly 4 bytes *)
Theorem encode_u32_length : forall n,
  length (encode_u32 n) = 4%nat.
Proof.
  intro n. unfold encode_u32. reflexivity.
Qed.

(** Theorem: encode_u64 produces exactly 8 bytes *)
Theorem encode_u64_length : forall n,
  length (encode_u64 n) = 8%nat.
Proof.
  intro n. unfold encode_u64. reflexivity.
Qed.

(** Theorem: encode_u8 produces valid bytes *)
Theorem encode_u8_valid : forall n,
  0 <= n ->
  bytes_wellformed (encode_u8 n).
Proof.
  intros n Hn.
  unfold bytes_wellformed, encode_u8.
  constructor.
  - apply land_255_valid. assumption.
  - constructor.
Qed.

(** Theorem: encode_u16 produces valid bytes *)
Theorem encode_u16_valid : forall n,
  0 <= n ->
  bytes_wellformed (encode_u16 n).
Proof.
  intros n Hn.
  unfold bytes_wellformed, encode_u16.
  repeat constructor; apply land_255_valid;
  try assumption; apply shiftr_nonneg; assumption.
Qed.

(** Theorem: encode_u32 produces valid bytes *)
Theorem encode_u32_valid : forall n,
  0 <= n ->
  bytes_wellformed (encode_u32 n).
Proof.
  intros n Hn.
  unfold bytes_wellformed, encode_u32.
  repeat constructor; apply land_255_valid;
  try assumption; apply shiftr_nonneg; assumption.
Qed.

(** Theorem: encode_u64 produces valid bytes *)
Theorem encode_u64_valid : forall n,
  0 <= n ->
  bytes_wellformed (encode_u64 n).
Proof.
  intros n Hn.
  unfold bytes_wellformed, encode_u64.
  repeat constructor; apply land_255_valid;
  try assumption; apply shiftr_nonneg; assumption.
Qed.

(* ========================================================================= *)
(* ENCODE-DECODE BIJECTIONS                                                  *)
(* ========================================================================= *)

(** Theorem: decode_u8 is left inverse of encode_u8 for valid range *)
Theorem decode_encode_u8 : forall n,
  0 <= n < 256 ->
  decode_u8 (encode_u8 n) = Some n.
Proof.
  intros n Hrange.
  unfold decode_u8, encode_u8.
  simpl. f_equal.
  apply land_255_bound. assumption.
Qed.

(** Theorem: encode_u8 is left inverse of decode_u8 *)
Theorem encode_decode_u8 : forall bs n,
  decode_u8 bs = Some n ->
  valid_byte n ->
  exists bs', bs = encode_u8 n ++ bs'.
Proof.
  intros bs n Hdec Hvalid.
  destruct bs as [| b bs'].
  - simpl in Hdec. discriminate.
  - unfold decode_u8 in Hdec. simpl in Hdec.
    injection Hdec as Hdec. subst.
    exists bs'.
    unfold encode_u8. simpl.
    f_equal. symmetry. apply land_255_bound. assumption.
Qed.

(** Helper: reconstruct 16-bit value from bytes *)
Lemma decode_u16_reconstruct : forall n,
  0 <= n < 65536 ->
  Z.land n 255 + Z.land (Z.shiftr n 8) 255 * 256 = n.
Proof.
  intros n [H0 H65536].
  assert (Hlo: Z.land n 255 = n mod 256).
  { assert (H: 255 = Z.ones 8) by reflexivity.
    rewrite H. rewrite Z.land_ones by lia. reflexivity. }
  assert (Hhi: Z.land (Z.shiftr n 8) 255 = (n / 256) mod 256).
  { assert (H: 255 = Z.ones 8) by reflexivity.
    rewrite H. rewrite Z.land_ones by lia.
    rewrite Z.shiftr_div_pow2 by lia.
    reflexivity. }
  rewrite Hlo, Hhi.
  (* n < 65536 = 256 * 256, so n / 256 < 256 *)
  assert (Hdiv: n / 256 < 256).
  { assert (H: n < 256 * 256) by lia.
    assert (H2: 256 > 0) by lia.
    apply (Z.div_lt_upper_bound n 256 256); lia. }
  assert (Hdiv_pos: 0 <= n / 256).
  { apply Z.div_pos; lia. }
  (* Simplify (n / 256) mod 256 = n / 256 since 0 <= n / 256 < 256 *)
  rewrite Z.mod_small with (a := n / 256) (b := 256) by lia.
  (* Now use n = (n / 256) * 256 + n mod 256 *)
  assert (Hdiv_mod: n = 256 * (n / 256) + n mod 256).
  { apply Z.div_mod. lia. }
  rewrite Z.mul_comm in Hdiv_mod.
  lia.
Qed.

(** Theorem: decode_u16 is left inverse of encode_u16 *)
Theorem decode_encode_u16 : forall n,
  0 <= n < 65536 ->
  decode_u16 (encode_u16 n) = Some n.
Proof.
  intros n Hrange.
  unfold decode_u16, encode_u16.
  simpl. f_equal.
  apply decode_u16_reconstruct. assumption.
Qed.

(** Theorem: encode_u16 is left inverse of decode_u16 *)
Theorem encode_decode_u16 : forall bs n,
  decode_u16 bs = Some n ->
  0 <= n < 65536 ->
  bytes_wellformed bs ->
  exists bs', bs = encode_u16 n ++ bs'.
Proof.
  intros bs n Hdec Hrange Hwf.
  destruct bs as [| b0 [| b1 bs']].
  - simpl in Hdec. discriminate.
  - simpl in Hdec. discriminate.
  - unfold decode_u16 in Hdec. simpl in Hdec.
    injection Hdec as Hdec. subst.
    exists bs'.
    unfold encode_u16. simpl.
    unfold bytes_wellformed in Hwf.
    inversion Hwf as [|? ? Hb0 Hwf1]. subst.
    inversion Hwf1 as [|? ? Hb1 Hwf2]. subst.
    unfold valid_byte in Hb0, Hb1.
    f_equal. f_equal.
    + (* Show b0 = Z.land (b0 + b1 * 256) 255 *)
      symmetry. apply land_255_add_mul_256; lia.
    + (* Show Z.land (Z.shiftr (b0 + b1 * 256) 8) 255 = b1 *)
      assert (Hshift: Z.shiftr (b0 + b1 * 256) 8 = b1).
      { rewrite Z.shiftr_div_pow2 by lia.
        simpl.
        replace (b0 + b1 * 256) with (b1 * 256 + b0) by lia.
        rewrite Z.div_add_l by lia.
        rewrite Z.div_small by lia.
        lia. }
      rewrite Hshift.
      f_equal.
      symmetry. apply land_255_bound. assumption.
Qed.

(** Helper: generalized byte extraction lemma for multi-byte values *)
Lemma land_255_add_mul_256_n : forall b_low b_high mult,
  0 <= b_low < 256 ->
  0 <= b_high ->
  mult = 256 ->
  Z.land (b_low + b_high * mult) 255 = b_low.
Proof.
  intros b_low b_high mult Hlow Hhigh Hmult.
  subst mult.
  apply land_255_add_mul_256; assumption.
Qed.

(** Helper lemmas for byte reconstruction *)
Lemma reconstruct_from_2_bytes : forall n,
  0 <= n < 65536 ->
  n mod 256 + (n / 256) mod 256 * 256 = n.
Proof.
  intros n [H0 H16].
  assert (Hdiv: n = 256 * (n / 256) + n mod 256) by (apply Z.div_mod; lia).
  assert (Hbound: 0 <= n / 256 < 256) by (split; [apply Z.div_pos; lia | apply Z.div_lt_upper_bound; lia]).
  assert (Hmod: (n / 256) mod 256 = n / 256) by (apply Z.mod_small; exact Hbound).
  rewrite Hmod. lia.
Qed.

(** Helper: reconstruct 32-bit value from bytes - step by step *)
Lemma decode_u32_reconstruct : forall n,
  0 <= n < 4294967296 ->
  Z.land n 255 +
  Z.land (Z.shiftr n 8) 255 * 256 +
  Z.land (Z.shiftr n 16) 255 * 65536 +
  Z.land (Z.shiftr n 24) 255 * 16777216 = n.
Proof.
  intros n [H0 H32].
  (* Convert bitwise ops to mod/div *)
  assert (Hlo: Z.land n 255 = n mod 256).
  { assert (H: 255 = Z.ones 8) by reflexivity.
    rewrite H. rewrite Z.land_ones by lia. reflexivity. }
  assert (Hb1: Z.land (Z.shiftr n 8) 255 = (n / 256) mod 256).
  { assert (H: 255 = Z.ones 8) by reflexivity.
    rewrite H. rewrite Z.land_ones by lia.
    rewrite Z.shiftr_div_pow2 by lia. reflexivity. }
  assert (Hb2: Z.land (Z.shiftr n 16) 255 = (n / 65536) mod 256).
  { assert (H: 255 = Z.ones 8) by reflexivity.
    rewrite H. rewrite Z.land_ones by lia.
    rewrite Z.shiftr_div_pow2 by lia. reflexivity. }
  assert (Hb3: Z.land (Z.shiftr n 24) 255 = (n / 16777216) mod 256).
  { assert (H: 255 = Z.ones 8) by reflexivity.
    rewrite H. rewrite Z.land_ones by lia.
    rewrite Z.shiftr_div_pow2 by lia. reflexivity. }
  rewrite Hlo, Hb1, Hb2, Hb3.

  (* Manually expand using div_mod - similar to 3-byte case *)
  pose proof (Z.div_mod n 256 ltac:(lia)) as E1.
  pose proof (Z.div_mod (n / 256) 256 ltac:(lia)) as E2.
  pose proof (Z.div_mod (n / 65536) 256 ltac:(lia)) as E3.
  assert (Hdiv2: (n / 256) / 256 = n / 65536) by (rewrite Z.div_div by lia; reflexivity).
  assert (Hdiv3: (n / 65536) / 256 = n / 16777216) by (rewrite Z.div_div by lia; reflexivity).
  assert (Hbound3: 0 <= n / 16777216 < 256).
  { split. apply Z.div_pos; lia. apply Z.div_lt_upper_bound; lia. }
  pose proof (Z.mod_small (n / 16777216) 256 Hbound3) as Hmod3.
  (* Just rewrite everything and let lia handle it *)
  rewrite E1, E2, E3, Hdiv2, Hdiv3, Hmod3.
  lia.
Qed.

(** Theorem: decode_u32 is left inverse of encode_u32 *)
Theorem decode_encode_u32 : forall n,
  0 <= n < 4294967296 ->
  decode_u32 (encode_u32 n) = Some n.
Proof.
  intros n Hrange.
  unfold decode_u32, encode_u32.
  simpl. f_equal.
  apply decode_u32_reconstruct. assumption.
Qed.

(** Theorem: encode_u32 is left inverse of decode_u32 *)
Theorem encode_decode_u32 : forall bs n,
  decode_u32 bs = Some n ->
  0 <= n < 4294967296 ->
  bytes_wellformed bs ->
  exists bs', bs = encode_u32 n ++ bs'.
Proof.
  intros bs n Hdec Hrange Hwf.
  destruct bs as [| b0 [| b1 [| b2 [| b3 bs']]]].
  - simpl in Hdec. discriminate.
  - simpl in Hdec. discriminate.
  - simpl in Hdec. discriminate.
  - simpl in Hdec. discriminate.
  - unfold decode_u32 in Hdec. simpl in Hdec.
    injection Hdec as Hdec. subst.
    exists bs'.
    unfold encode_u32. simpl.
    unfold bytes_wellformed in Hwf.
    inversion Hwf as [|? ? Hb0 Hwf1]. subst.
    inversion Hwf1 as [|? ? Hb1 Hwf2]. subst.
    inversion Hwf2 as [|? ? Hb2 Hwf3]. subst.
    inversion Hwf3 as [|? ? Hb3 Hwf4]. subst.
    unfold valid_byte in Hb0, Hb1, Hb2, Hb3.
    f_equal. f_equal.
    + symmetry. apply land_255_bound. assumption.
    + f_equal.
      * assert (Hshift: Z.shiftr (b0 + b1 * 256 + b2 * 65536 + b3 * 16777216) 8 =
                        b1 + b2 * 256 + b3 * 16777216).
        { rewrite Z.shiftr_div_pow2 by lia.
          rewrite Z.add_assoc. rewrite Z.add_assoc.
          rewrite (Z.add_comm b0).
          rewrite <- Z.add_assoc. rewrite <- Z.add_assoc.
          rewrite Z.div_add by lia.
          rewrite Z.div_small by assumption.
          rewrite Z.add_0_r.
          rewrite Z.mul_comm with (n := 256).
          rewrite Z.mul_assoc.
          rewrite <- Z.mul_add_distr_l.
          rewrite Z.div_mul by lia.
          reflexivity. }
        rewrite Hshift.
        symmetry. apply land_255_bound. assumption.
      * f_equal.
        -- assert (Hshift: Z.shiftr (b0 + b1 * 256 + b2 * 65536 + b3 * 16777216) 16 =
                          b2 + b3 * 256).
           { rewrite Z.shiftr_div_pow2 by lia.
             assert (Heq: b0 + b1 * 256 + b2 * 65536 + b3 * 16777216 =
                         (b0 + b1 * 256) + (b2 + b3 * 256) * 65536) by lia.
             rewrite Heq.
             rewrite Z.div_add by lia.
             assert (Hsmall: b0 + b1 * 256 < 65536) by lia.
             rewrite Z.div_small by assumption.
             rewrite Z.add_0_l. reflexivity. }
           rewrite Hshift.
           symmetry. apply land_255_bound. assumption.
        -- assert (Hshift: Z.shiftr (b0 + b1 * 256 + b2 * 65536 + b3 * 16777216) 24 = b3).
           { rewrite Z.shiftr_div_pow2 by lia.
             assert (Heq: b0 + b1 * 256 + b2 * 65536 + b3 * 16777216 =
                         (b0 + b1 * 256 + b2 * 65536) + b3 * 16777216) by lia.
             rewrite Heq.
             rewrite Z.div_add by lia.
             assert (Hsmall: b0 + b1 * 256 + b2 * 65536 < 16777216) by lia.
             rewrite Z.div_small by assumption.
             rewrite Z.add_0_l. reflexivity. }
           rewrite Hshift.
           symmetry. apply land_255_bound. assumption.
Qed.

(** Helper: reconstruct 64-bit value from bytes *)
Lemma decode_u64_reconstruct : forall n,
  0 <= n < 18446744073709551616 ->
  Z.land n 255 +
  Z.land (Z.shiftr n 8) 255 * 256 +
  Z.land (Z.shiftr n 16) 255 * 65536 +
  Z.land (Z.shiftr n 24) 255 * 16777216 +
  Z.land (Z.shiftr n 32) 255 * 4294967296 +
  Z.land (Z.shiftr n 40) 255 * 1099511627776 +
  Z.land (Z.shiftr n 48) 255 * 281474976710656 +
  Z.land (Z.shiftr n 56) 255 * 72057594037927936 = n.
Proof.
  intros n [H0 H64].
  assert (H255: 255 = Z.ones 8) by reflexivity.
  repeat rewrite H255.
  repeat rewrite Z.land_ones by lia.
  repeat rewrite Z.shiftr_div_pow2 by lia.
  (* Extract each byte *)
  replace n with (n mod 256 + (n / 256) * 256) at 8 by (apply Z.div_mod; lia).
  replace (n / 256) with ((n / 256) mod 256 + (n / 65536) * 256) by
    (rewrite Z.div_div by lia; rewrite Z.mul_comm at 2; apply Z.div_mod; lia).
  replace (n / 65536) with ((n / 65536) mod 256 + (n / 16777216) * 256) by
    (rewrite Z.div_div by lia; rewrite Z.mul_comm at 2; apply Z.div_mod; lia).
  replace (n / 16777216) with ((n / 16777216) mod 256 + (n / 4294967296) * 256) by
    (rewrite Z.div_div by lia; rewrite Z.mul_comm at 2; apply Z.div_mod; lia).
  replace (n / 4294967296) with ((n / 4294967296) mod 256 + (n / 1099511627776) * 256) by
    (rewrite Z.div_div by lia; rewrite Z.mul_comm at 2; apply Z.div_mod; lia).
  replace (n / 1099511627776) with ((n / 1099511627776) mod 256 + (n / 281474976710656) * 256) by
    (rewrite Z.div_div by lia; rewrite Z.mul_comm at 2; apply Z.div_mod; lia).
  replace (n / 281474976710656) with ((n / 281474976710656) mod 256 + (n / 72057594037927936) * 256) by
    (rewrite Z.div_div by lia; rewrite Z.mul_comm at 2; apply Z.div_mod; lia).
  assert (Hdiv7: n / 72057594037927936 < 256) by (apply Z.div_lt_upper_bound; lia).
  rewrite Z.mod_small with (a := n / 72057594037927936) by lia.
  lia.
Qed.

(** Theorem: decode_u64 is left inverse of encode_u64 *)
Theorem decode_encode_u64 : forall n,
  0 <= n < 18446744073709551616 ->
  decode_u64 (encode_u64 n) = Some n.
Proof.
  intros n Hrange.
  unfold decode_u64, encode_u64.
  simpl. f_equal.
  apply decode_u64_reconstruct. assumption.
Qed.

(** Theorem: encode_u64 is left inverse of decode_u64 *)
Theorem encode_decode_u64 : forall bs n,
  decode_u64 bs = Some n ->
  0 <= n < 18446744073709551616 ->
  bytes_wellformed bs ->
  exists bs', bs = encode_u64 n ++ bs'.
Proof.
  intros bs n Hdec Hrange Hwf.
  destruct bs as [| b0 [| b1 [| b2 [| b3 [| b4 [| b5 [| b6 [| b7 bs']]]]]]]].
  - simpl in Hdec. discriminate.
  - simpl in Hdec. discriminate.
  - simpl in Hdec. discriminate.
  - simpl in Hdec. discriminate.
  - simpl in Hdec. discriminate.
  - simpl in Hdec. discriminate.
  - simpl in Hdec. discriminate.
  - simpl in Hdec. discriminate.
  - unfold decode_u64 in Hdec. simpl in Hdec.
    injection Hdec as Hdec. subst.
    exists bs'.
    unfold encode_u64. simpl.
    unfold bytes_wellformed in Hwf.
    do 8 (try (inversion Hwf as [|? ? ? Hwf_next]; subst; rename Hwf_next into Hwf)).
    (* Extract all 8 byte constraints - Coq's destructuring gives us the constraints *)
    rewrite decode_u64_reconstruct by assumption.
    reflexivity.
Qed.

(* ========================================================================= *)
(* STRING ENCODING                                                           *)
(* ========================================================================= *)

(** 9P strings: 16-bit length prefix + data (no null terminator) *)
Definition encode_string (s : Bytes) : Bytes :=
  encode_u16 (Z.of_nat (length s)) ++ s.

(** Decode string from byte sequence *)
Definition decode_string (bs : Bytes) : option (Bytes * Bytes) :=
  match decode_u16 bs with
  | None => None
  | Some len =>
      if (length bs <? 2 + Z.to_nat len)%nat then None
      else
        let str := firstn (Z.to_nat len) (skipn 2 bs) in
        let rest := skipn (2 + Z.to_nat len) bs in
        Some (str, rest)
  end.

(** Theorem: encode_string length is 2 + string length *)
Theorem encode_string_length : forall s,
  length (encode_string s) = (2 + length s)%nat.
Proof.
  intro s.
  unfold encode_string.
  rewrite length_app.
  rewrite encode_u16_length.
  reflexivity.
Qed.

(** Theorem: decode_string is left inverse of encode_string *)
Theorem decode_encode_string : forall s,
  (length s < 65536)%nat ->
  decode_string (encode_string s) = Some (s, []).
Proof.
  intros s Hlen.
  unfold decode_string, encode_string.
  rewrite decode_encode_u16.
  2: { split; [lia | apply Nat2Z.inj_lt; assumption]. }
  rewrite length_app.
  rewrite encode_u16_length.
  assert (Hcmp: (2 + length s <? 2 + Z.to_nat (Z.of_nat (length s)))%nat = false).
  { rewrite Nat2Z.id. apply Nat.ltb_irrefl. }
  rewrite Hcmp.
  rewrite Nat2Z.id.
  rewrite skipn_app.
  rewrite encode_u16_length.
  replace (2 - 2)%nat with O by lia.
  simpl.
  rewrite firstn_all.
  rewrite skipn_all.
  reflexivity.
Qed.

(** Theorem: encode_string is left inverse of decode_string *)
Theorem encode_decode_string : forall bs s rest,
  decode_string bs = Some (s, rest) ->
  bytes_wellformed bs ->
  bs = encode_string s ++ rest.
Proof.
  intros bs s rest Hdec Hwf.
  unfold decode_string in Hdec.
  destruct (decode_u16 bs) as [len|] eqn:E; try discriminate.
  destruct (length bs <? 2 + Z.to_nat len)%nat eqn:Ecmp; try discriminate.
  injection Hdec as Hs Hrest. subst.
  apply Nat.ltb_ge in Ecmp.
  unfold encode_string.
  assert (Hbs: exists bs', bs = encode_u16 len ++ bs').
  { apply encode_decode_u16. assumption.
    assert (Hlen_pos: 0 <= len).
    { destruct bs as [|b0 [|b1 bs']]; simpl in E; try discriminate.
      injection E as E. lia. }
    assert (Hlen_bound: len < 65536).
    { destruct bs as [|b0 [|b1 bs']]; simpl in E; try discriminate.
      injection E as E. subst. lia. }
    split; assumption.
    assumption. }
  destruct Hbs as [bs' Hbs]. subst.
  rewrite Hbs in E.
  rewrite decode_encode_u16 in E by lia.
  injection E as E. subst.
  rewrite firstn_skipn with (l := bs') (n := Z.to_nat len).
  f_equal.
  - f_equal. symmetry. apply decode_encode_u16. assumption. lia.
  - rewrite Hbs in Ecmp.
    rewrite length_app in Ecmp.
    rewrite encode_u16_length in Ecmp.
    simpl in Hrest. rewrite <- Hrest.
    rewrite skipn_app.
    rewrite encode_u16_length.
    replace (2 + Z.to_nat len - 2)%nat with (Z.to_nat len) by lia.
    reflexivity.
Qed.

(* ========================================================================= *)
(* QID ENCODING                                                              *)
(* ========================================================================= *)

(** Qid encoding: type(1) + vers(4) + path(8) = 13 bytes *)
Definition encode_qid (q : Qid) : Bytes :=
  encode_u8 (qid_type q) ++
  encode_u32 (qid_vers q) ++
  encode_u64 (qid_path q).

(** Decode Qid from byte sequence *)
Definition decode_qid (bs : Bytes) : option (Qid * Bytes) :=
  match decode_u8 bs with
  | None => None
  | Some qtype =>
      match decode_u32 (skipn 1 bs) with
      | None => None
      | Some qvers =>
          match decode_u64 (skipn 5 bs) with
          | None => None
          | Some qpath =>
              Some (mkQid qpath qvers qtype, skipn 13 bs)
          end
      end
  end.

(** Theorem: encode_qid produces exactly 13 bytes *)
Theorem encode_qid_size : forall q,
  length (encode_qid q) = 13%nat.
Proof.
  intro q.
  unfold encode_qid.
  repeat rewrite length_app.
  rewrite encode_u8_length.
  rewrite encode_u32_length.
  rewrite encode_u64_length.
  reflexivity.
Qed.

(** Theorem: decode_qid is left inverse of encode_qid *)
Theorem decode_encode_qid : forall q,
  0 <= qid_type q < 256 ->
  0 <= qid_vers q < 4294967296 ->
  0 <= qid_path q < 18446744073709551616 ->
  decode_qid (encode_qid q) = Some (q, []).
Proof.
  intros q Htype Hvers Hpath.
  unfold decode_qid, encode_qid.
  rewrite decode_encode_u8 by assumption.
  rewrite skipn_app.
  rewrite encode_u8_length.
  replace (1 - 1)%nat with O by lia.
  simpl. rewrite decode_encode_u32 by assumption.
  rewrite skipn_app.
  rewrite length_app.
  rewrite encode_u8_length.
  rewrite encode_u32_length.
  replace (5 - (1 + 4))%nat with O by lia.
  simpl. rewrite decode_encode_u64 by assumption.
  rewrite skipn_app.
  repeat rewrite length_app.
  rewrite encode_u8_length.
  rewrite encode_u32_length.
  rewrite encode_u64_length.
  replace (13 - (1 + 4 + 8))%nat with O by lia.
  simpl. reflexivity.
Qed.

(** Theorem: encode_qid is left inverse of decode_qid *)
Theorem encode_decode_qid : forall bs q rest,
  decode_qid bs = Some (q, rest) ->
  bytes_wellformed bs ->
  bs = encode_qid q ++ rest.
Proof.
  intros bs q rest Hdec Hwf.
  unfold decode_qid in Hdec.
  destruct (decode_u8 bs) as [qtype|] eqn:Et; try discriminate.
  destruct (decode_u32 (skipn 1 bs)) as [qvers|] eqn:Ev; try discriminate.
  destruct (decode_u64 (skipn 5 bs)) as [qpath|] eqn:Ep; try discriminate.
  injection Hdec as Hq. rewrite <- Hq in *.
  simpl in *.
  unfold encode_qid.
  assert (Htype: exists bs1, bs = encode_u8 qtype ++ bs1).
  { apply encode_decode_u8. assumption.
    destruct bs as [|b bs']; simpl in Et; try discriminate.
    injection Et as Et. subst. unfold valid_byte. lia.
    assumption. }
  destruct Htype as [bs1 Hbs1]. subst.
  (* Extract wellformedness of bs1 *)
  assert (Hwf1: bytes_wellformed bs1).
  { rewrite Hbs1 in Hwf. unfold bytes_wellformed in Hwf.
    rewrite Forall_app in Hwf. destruct Hwf as [_ Hwf1]. assumption. }
  rewrite skipn_app in Ev.
  rewrite encode_u8_length in Ev.
  replace (1 - 1)%nat with O in Ev by lia.
  simpl in Ev.
  assert (Hvers: exists bs2, bs1 = encode_u32 qvers ++ bs2).
  { apply encode_decode_u32. assumption.
    destruct bs1 as [|b0 [|b1 [|b2 [|b3 bs']]]]; simpl in Ev; try discriminate.
    injection Ev as Ev. subst. lia.
    assumption. }
  destruct Hvers as [bs2 Hbs2]. subst.
  (* Extract wellformedness of bs2 *)
  assert (Hwf2: bytes_wellformed bs2).
  { rewrite Hbs2 in Hwf1. unfold bytes_wellformed in Hwf1.
    rewrite Forall_app in Hwf1. destruct Hwf1 as [_ Hwf2]. assumption. }
  rewrite app_assoc.
  rewrite skipn_app in Ep.
  repeat rewrite length_app in Ep.
  rewrite encode_u8_length in Ep.
  rewrite encode_u32_length in Ep.
  replace (5 - (1 + 4))%nat with O in Ep by lia.
  simpl in Ep.
  assert (Hpath: exists bs3, bs2 = encode_u64 qpath ++ bs3).
  { apply encode_decode_u64. assumption.
    destruct bs2 as [|b0 [|b1 [|b2 [|b3 [|b4 [|b5 [|b6 [|b7 bs']]]]]]]];
      simpl in Ep; try discriminate.
    injection Ep as Ep. subst. lia.
    assumption. }
  destruct Hpath as [bs3 Hbs3]. subst.
  repeat rewrite app_assoc.
  f_equal.
  rewrite skipn_app.
  repeat rewrite length_app.
  rewrite encode_u8_length.
  rewrite encode_u32_length.
  rewrite encode_u64_length.
  replace (13 - (1 + 4 + 8))%nat with O by lia.
  simpl. reflexivity.
Qed.

(* ========================================================================= *)
(* DECODING FAILURE CONDITIONS                                               *)
(* ========================================================================= *)

(** Theorem: decode_u8 fails on empty input *)
Theorem decode_u8_empty_fails :
  decode_u8 [] = None.
Proof.
  unfold decode_u8. reflexivity.
Qed.

(** Theorem: decode_u16 fails on insufficient bytes *)
Theorem decode_u16_short_fails : forall b,
  decode_u16 [b] = None.
Proof.
  intro b. unfold decode_u16. reflexivity.
Qed.

(** Theorem: decode_u32 fails on insufficient bytes *)
Theorem decode_u32_short_fails : forall bs,
  (length bs < 4)%nat ->
  decode_u32 bs = None.
Proof.
  intros bs Hlen.
  destruct bs as [| b0 [| b1 [| b2 [| b3 bs']]]];
    try (simpl in Hlen; lia);
    unfold decode_u32; reflexivity.
Qed.

(** Theorem: decode_u64 fails on insufficient bytes *)
Theorem decode_u64_short_fails : forall bs,
  (length bs < 8)%nat ->
  decode_u64 bs = None.
Proof.
  intros bs Hlen.
  destruct bs as [| b0 [| b1 [| b2 [| b3 [| b4 [| b5 [| b6 [| b7 bs']]]]]]]];
    try (simpl in Hlen; lia);
    unfold decode_u64; reflexivity.
Qed.

Print Assumptions decode_encode_u8.
Print Assumptions encode_decode_u8.
Print Assumptions decode_encode_u16.
Print Assumptions encode_decode_u16.
Print Assumptions decode_encode_u32.
Print Assumptions encode_decode_u32.
Print Assumptions decode_encode_u64.
Print Assumptions encode_decode_u64.
Print Assumptions decode_encode_string.
Print Assumptions encode_decode_string.
Print Assumptions decode_encode_qid.
Print Assumptions encode_decode_qid.
