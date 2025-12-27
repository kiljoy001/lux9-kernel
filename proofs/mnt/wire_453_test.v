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
Lemma mod_add_mul_l : forall a b c,
  b <> 0 ->
  (b * a + c) mod b = c mod b.
Proof.
  intros a b c Hb.
  rewrite Z.add_comm.
  rewrite Z.mul_comm.
  rewrite Z.mod_add by assumption.
  reflexivity.
Qed.

Lemma div_add_mul_l : forall a b c,
  b > 0 ->
  0 <= c < b ->
  (b * a + c) / b = a.
Proof.
  intros a b c Hb Hc.
  rewrite Z.add_comm.
  rewrite Z.mul_comm.
  rewrite Z.div_add by lia.
  rewrite (Z.div_small c b Hc). lia.
Qed.

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

Lemma reconstruct_from_3_bytes : forall n,
  0 <= n < 16777216 ->
  n mod 256 + (n / 256) mod 256 * 256 + (n / 65536) mod 256 * 65536 = n.
Proof.
  intros n [H0 H24].
  (* Work from RHS to LHS using symmetry *)
  symmetry.
  pose proof (Z.div_mod n 256 ltac:(lia)) as E1.
  pose proof (Z.div_mod (n / 256) 256 ltac:(lia)) as E2.
  assert (Hdiv: (n / 256) / 256 = n / 65536) by (rewrite Z.div_div by lia; reflexivity).
  assert (Hb2: 0 <= n / 65536 < 256).
  { split. apply Z.div_pos; lia. apply Z.div_lt_upper_bound; lia. }
  pose proof (Z.mod_small (n / 65536) 256 Hb2) as Hmod2.
  assert (Hb_nmod: 0 <= n mod 256 < 256) by (apply Z.mod_pos_bound; lia).

  (* Simplify LHS using E1, E2 *)
  rewrite E1. rewrite E2 at 1. rewrite Hdiv.

  (* Simplify RHS: show that expanding and re-extracting bytes gives the same thing *)
  assert (Hmod_simp: (256 * (n / 256) + n mod 256) mod 256 = n mod 256).
  { rewrite mod_add_mul_l by lia. apply Z.mod_small. exact Hb_nmod. }
  assert (Hdiv_simp: (256 * (n / 256) + n mod 256) / 256 = n / 256).
  { rewrite div_add_mul_l by exact Hb_nmod || lia. reflexivity. }
  assert (Hdiv2_simp: (256 * (n / 256) + n mod 256) / 65536 = (n / 256) / 256).
  { assert (H65536: 65536 = 256 * 256) by reflexivity.
    rewrite H65536.
    rewrite <- Z.div_div by lia.
    rewrite Hdiv_simp.
    reflexivity. }

  rewrite Hmod_simp, Hdiv_simp, Hdiv2_simp, Hdiv, Hmod2.
  ring.
Qed.

(** Helper: reconstruct 32-bit value from bytes - step by step *)
(* TEMPORARY: Admitted to check if rest of file compiles *)
Lemma decode_u32_reconstruct : forall n,
  0 <= n < 4294967296 ->
  Z.land n 255 +
  Z.land (Z.shiftr n 8) 255 * 256 +
  Z.land (Z.shiftr n 16) 255 * 65536 +
  Z.land (Z.shiftr n 24) 255 * 16777216 = n.
Proof.
Admitted.

