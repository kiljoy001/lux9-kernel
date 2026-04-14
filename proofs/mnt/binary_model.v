(** * Binary Representation Model
    *
    * Explicit binary representation to prove bitwise reconstruction
    * Models how bits actually work: each position doubles going right to left
    *)

Require Import Coq.ZArith.ZArith.
Require Import Coq.Lists.List.
Require Import Coq.Bool.Bool.
Require Import Lia.
Import ListNotations.

Require Import mnt.wire_format.

Open Scope Z_scope.

(* ========================================================================= *)
(* BINARY REPRESENTATION                                                     *)
(* ========================================================================= *)

(** A bit is 0 or 1 *)
Definition Bit : Type := Z.

Definition valid_bit (b : Bit) : Prop := b = 0 \/ b = 1.

(** Binary representation: list of bits, least significant first *)
Definition Binary : Type := list Bit.

(** Convert binary to Z value: sum of (bit * 2^position) *)
Fixpoint binary_to_Z (bs : Binary) (pos : nat) : Z :=
  match bs with
  | [] => 0
  | b :: rest => b * (2 ^ Z.of_nat pos) + binary_to_Z rest (S pos)
  end.

Definition bin_value (bs : Binary) : Z := binary_to_Z bs 0.

(** Convert Z to binary (up to n bits) *)
Fixpoint Z_to_binary (n : Z) (bits : nat) : Binary :=
  match bits with
  | O => []
  | S rest => (n mod 2) :: Z_to_binary (n / 2) rest
  end.

(* ========================================================================= *)
(* BITWISE OPERATIONS ON BINARY                                             *)
(* ========================================================================= *)

(** Take first n bits *)
Fixpoint take_bits (bs : Binary) (n : nat) : Binary :=
  match n, bs with
  | O, _ => []
  | S n', [] => []
  | S n', b :: rest => b :: take_bits rest n'
  end.

(** Drop first n bits (right shift) *)
Fixpoint drop_bits (bs : Binary) (n : nat) : Binary :=
  match n, bs with
  | O, _ => bs
  | S n', [] => []
  | S n', _ :: rest => drop_bits rest n'
  end.

(** Bitwise AND: bit-by-bit AND operation *)
Fixpoint binary_and (bs1 bs2 : Binary) : Binary :=
  match bs1, bs2 with
  | [], _ => []
  | _, [] => []
  | b1 :: rest1, b2 :: rest2 =>
      (if (b1 =? 1) && (b2 =? 1) then 1 else 0) :: binary_and rest1 rest2
  end.

(* ========================================================================= *)
(* KEY LEMMAS                                                                *)
(* ========================================================================= *)

(** Helper: power of 2 for successor *)
Lemma pow2_succ : forall n,
  2 ^ Z.of_nat (S n) = 2 * 2 ^ Z.of_nat n.
Proof.
  intro n.
  rewrite Nat2Z.inj_succ.
  rewrite Z.pow_succ_r by lia.
  reflexivity.
Qed.

(** Helper: binary_to_Z is additive in position *)
Lemma binary_to_Z_shift : forall bs pos,
  binary_to_Z bs (S pos) = 2 * binary_to_Z bs pos.
Proof.
  induction bs as [| b rest IH]; intros pos.
  - cbn [binary_to_Z]. ring.
  - cbn [binary_to_Z].
    rewrite (IH (S pos)).
    change (Z.pow_pos 2 (Pos.of_succ_nat pos)) with (2 ^ Z.of_nat (S pos)).
    rewrite pow2_succ.
    ring.
Qed.

(** Take bits of a full representation is just a shorter representation *)
Lemma take_bits_Z_to_binary : forall n bits k,
  (k <= bits)%nat ->
  take_bits (Z_to_binary n bits) k = Z_to_binary n k.
Proof.
  intros n bits k Hk.
  revert n bits Hk.
  induction k as [| k IH]; intros n bits Hk.
  - destruct (Z_to_binary n bits); reflexivity.
  - destruct bits as [| bits']; [lia|].
    simpl. f_equal. apply IH. lia.
Qed.

(** Drop bits corresponds to shifting the input by k *)
Lemma drop_bits_Z_to_binary : forall n bits k,
  (k <= bits)%nat ->
  drop_bits (Z_to_binary n bits) k =
    Z_to_binary (n / 2 ^ Z.of_nat k) (bits - k).
Proof.
  intros n bits k Hk.
  revert n bits Hk.
  induction k as [| k IH]; intros n bits Hk.
  - simpl. rewrite Z.div_1_r. rewrite Nat.sub_0_r.
    destruct (Z_to_binary n bits); reflexivity.
  - destruct bits as [| bits']; [lia|].
    simpl.
    assert (Hk' : (k <= bits')%nat) by lia.
    rewrite IH by exact Hk'.
    f_equal.
    rewrite Z.div_div by (lia).
    rewrite <- pow2_succ.
    reflexivity.
Qed.

(** Mod decomposition for powers of two *)
Lemma mod_pow2_succ : forall n k,
  0 <= n ->
  n mod (2 ^ Z.of_nat (S k)) =
    (n mod 2) + 2 * ((n / 2) mod (2 ^ Z.of_nat k)).
Proof.
  intros n k Hn.
  set (m := 2 ^ Z.of_nat k).
  assert (Hm_pos : 0 < m) by (subst m; apply Z.pow_pos_nonneg; lia).
  assert (Hmod2 : 0 <= n mod 2 < 2) by (apply Z.mod_pos_bound; lia).
  assert (Hmodm : 0 <= (n / 2) mod m < m) by (apply Z.mod_pos_bound; lia).
  assert (Hsum_bound :
            0 <= (n mod 2) + 2 * ((n / 2) mod m) < 2 * m) by lia.
  assert (Hdiv2 : n = 2 * (n / 2) + n mod 2) by (apply Z.div_mod; lia).
  assert (Hmul :
            (2 * (n / 2)) mod (2 * m) = 2 * ((n / 2) mod m)).
  { set (q := (n / 2) / m).
    set (r := (n / 2) mod m).
    assert (Hq : n / 2 = m * q + r) by (subst q r; apply Z.div_mod; lia).
    rewrite Hq.
    replace (2 * (m * q + r)) with ((2 * m) * q + 2 * r) by ring.
    rewrite Z.add_mod by lia.
    rewrite (Z.mul_comm (2 * m) q).
    rewrite Z.mod_mul by lia.
    rewrite Z.add_0_l.
    rewrite Z.mod_mod by lia.
    apply Z.mod_small. lia. }
  rewrite pow2_succ.
  change (2 * 2 ^ Z.of_nat k) with (2 * m).
  rewrite Hdiv2 at 1.
  rewrite Z.add_mod by lia.
  rewrite Hmul.
  rewrite (Z.mod_small (n mod 2)) by lia.
  replace (n mod 2 + 2 * ((n / 2) mod m)) with
      (2 * ((n / 2) mod m) + n mod 2) by ring.
  apply Z.mod_small. lia.
Qed.

(** Value of a binary representation is just n modulo 2^bits *)
Lemma Z_to_binary_value_mod : forall n bits,
  0 <= n ->
  bin_value (Z_to_binary n bits) = n mod 2 ^ Z.of_nat bits.
Proof.
  intros n bits Hn.
  unfold bin_value.
  revert n Hn.
  induction bits as [| k IH]; intros n Hn.
  - simpl. rewrite Z.mod_1_r. reflexivity.
  - simpl Z_to_binary. simpl binary_to_Z.
    replace (n mod 2 * 1) with (n mod 2) by lia.
    assert (Hshift : binary_to_Z (Z_to_binary (n / 2) k) 1 =
                     2 * binary_to_Z (Z_to_binary (n / 2) k) 0).
    { apply binary_to_Z_shift. }
    rewrite Hshift.
    rewrite IH by (apply Z.div_pos; lia).
    rewrite mod_pow2_succ by exact Hn.
    reflexivity.
Qed.

(** Value of the first k bits is n modulo 2^k *)
Lemma take_bits_value_mod : forall n bits k,
  0 <= n ->
  (k <= bits)%nat ->
  bin_value (take_bits (Z_to_binary n bits) k) = n mod 2 ^ Z.of_nat k.
Proof.
  intros n bits k Hn Hk.
  rewrite (take_bits_Z_to_binary n bits k) by exact Hk.
  apply Z_to_binary_value_mod. exact Hn.
Qed.

(** Taking first 8 bits is equivalent to AND with 255 *)
Lemma take_8_bits_eq_land_255 : forall n,
  0 <= n ->
  bin_value (take_bits (Z_to_binary n 32) 8) = Z.land n 255.
Proof.
  intros n Hn.
  assert (Hk : (8 <= 32)%nat) by lia.
  rewrite (take_bits_Z_to_binary n 32 8) by exact Hk.
  rewrite (Z_to_binary_value_mod n 8) by exact Hn.
  assert (H255 : 255 = Z.ones 8) by reflexivity.
  rewrite H255.
  rewrite Z.land_ones by lia.
  reflexivity.
Qed.

(** Dropping k bits is equivalent to right shift *)
Lemma drop_bits_eq_shiftr : forall n k,
  0 <= n < 2 ^ Z.of_nat 32 ->
  (k <= 32)%nat ->
  bin_value (drop_bits (Z_to_binary n 32) k) = Z.shiftr n (Z.of_nat k).
Proof.
  intros n k [Hn0 Hn32] Hk.
  rewrite (drop_bits_Z_to_binary n 32 k) by exact Hk.
  rewrite (Z_to_binary_value_mod (n / 2 ^ Z.of_nat k) (32 - k)).
  - rewrite Z.shiftr_div_pow2 by lia.
    apply Z.mod_small.
    assert (Hpowk_pos : 0 < 2 ^ Z.of_nat k) by (apply Z.pow_pos_nonneg; lia).
    assert (Hpow32k_pos : 0 < 2 ^ Z.of_nat (32 - k)) by (apply Z.pow_pos_nonneg; lia).
    assert (Hpow : 2 ^ Z.of_nat 32 = 2 ^ Z.of_nat k * 2 ^ Z.of_nat (32 - k)).
    { assert (Hsum : Z.of_nat 32 = Z.of_nat k + Z.of_nat (32 - k)) by (f_equal; lia).
      rewrite Hsum.
      rewrite Z.pow_add_r by lia.
      reflexivity. }
    split.
    + apply Z.div_pos; lia.
    + apply Z.div_lt_upper_bound; lia.
  - apply Z.div_pos; lia.
Qed.
(** Binary representation is correct *)
Lemma Z_to_binary_correct : forall n bits,
  0 <= n < 2 ^ Z.of_nat bits ->
  bin_value (Z_to_binary n bits) = n.
Proof.
  intros n bits Hn.
  unfold bin_value.
  revert n Hn.
  induction bits as [| k IH]; intros n [Hn0 Hnbound].
  - simpl. simpl in Hnbound. assert (n = 0) by lia. subst. reflexivity.
  - simpl Z_to_binary. simpl binary_to_Z.
    replace (n mod 2 * 1) with (n mod 2) by lia.
    assert (Hshift : binary_to_Z (Z_to_binary (n / 2) k) 1 =
                     2 * binary_to_Z (Z_to_binary (n / 2) k) 0).
    { apply binary_to_Z_shift. }
    rewrite Hshift.
    rewrite IH.
    + assert (Hdiv: n = 2 * (n / 2) + n mod 2) by (apply Z.div_mod; lia).
      lia.
    + split.
      * apply Z.div_pos; lia.
      * assert (Hbound' : n < 2 * 2 ^ Z.of_nat k). {
          rewrite pow2_succ in Hnbound.
          exact Hnbound.
        }
        apply Z.div_lt_upper_bound; lia.
Qed.

(* ========================================================================= *)
(* RECONSTRUCTION THEOREM                                                    *)
(* ========================================================================= *)

(** Reconstruct 32-bit value from 4 bytes using binary model *)
Theorem binary_u32_reconstruct : forall n,
  0 <= n < 4294967296 ->
  bin_value (take_bits (Z_to_binary n 32) 8) +
  bin_value (take_bits (drop_bits (Z_to_binary n 32) 8) 8) * 256 +
  bin_value (take_bits (drop_bits (Z_to_binary n 32) 16) 8) * 65536 +
  bin_value (take_bits (drop_bits (Z_to_binary n 32) 24) 8) * 16777216 = n.
Proof.
  intros n Hn.
  (* The key insight: the binary representation makes this obvious *)
  (* Byte 0: bits 0-7 contribute value 0 to 255 *)
  (* Byte 1: bits 8-15 contribute value 0 to 255, shifted by 8 (multiply by 256) *)
  (* Byte 2: bits 16-23 contribute value 0 to 255, shifted by 16 (multiply by 65536) *)
  (* Byte 3: bits 24-31 contribute value 0 to 255, shifted by 24 (multiply by 16777216) *)
  (* Sum of all bytes = original value *)

  assert (Hb0 : bin_value (take_bits (Z_to_binary n 32) 8) = Z.land n 255).
  { apply take_8_bits_eq_land_255. lia. }
  assert (Hb1 : bin_value (take_bits (drop_bits (Z_to_binary n 32) 8) 8) =
                Z.land (Z.shiftr n 8) 255).
  { rewrite (drop_bits_Z_to_binary n 32 8) by lia.
    rewrite take_bits_Z_to_binary by lia.
    rewrite (Z_to_binary_value_mod (n / 2 ^ Z.of_nat 8) 8) by (apply Z.div_pos; lia).
    assert (H255 : 255 = Z.ones 8) by reflexivity.
    rewrite H255.
    rewrite Z.land_ones by lia.
    rewrite Z.shiftr_div_pow2 by lia.
    reflexivity. }
  assert (Hb2 : bin_value (take_bits (drop_bits (Z_to_binary n 32) 16) 8) =
                Z.land (Z.shiftr n 16) 255).
  { rewrite (drop_bits_Z_to_binary n 32 16) by lia.
    rewrite take_bits_Z_to_binary by lia.
    rewrite (Z_to_binary_value_mod (n / 2 ^ Z.of_nat 16) 8) by (apply Z.div_pos; lia).
    assert (H255 : 255 = Z.ones 8) by reflexivity.
    rewrite H255.
    rewrite Z.land_ones by lia.
    rewrite Z.shiftr_div_pow2 by lia.
    reflexivity. }
  assert (Hb3 : bin_value (take_bits (drop_bits (Z_to_binary n 32) 24) 8) =
                Z.land (Z.shiftr n 24) 255).
  { rewrite (drop_bits_Z_to_binary n 32 24) by lia.
    rewrite take_bits_Z_to_binary by lia.
    rewrite (Z_to_binary_value_mod (n / 2 ^ Z.of_nat 24) 8) by (apply Z.div_pos; lia).
    assert (H255 : 255 = Z.ones 8) by reflexivity.
    rewrite H255.
    rewrite Z.land_ones by lia.
    rewrite Z.shiftr_div_pow2 by lia.
    reflexivity. }
  rewrite Hb0, Hb1, Hb2, Hb3.
  apply decode_u32_reconstruct. assumption.
Qed.
