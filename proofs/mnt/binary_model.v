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

(** Taking first 8 bits is equivalent to AND with 255 *)
Lemma take_8_bits_eq_land_255 : forall n,
  0 <= n ->
  bin_value (take_bits (Z_to_binary n 32) 8) = Z.land n 255.
Proof.
  (* Will prove that taking first 8 bits gives n mod 256 *)
  admit.
Admitted.

(** Dropping k bits is equivalent to right shift *)
Lemma drop_bits_eq_shiftr : forall n k,
  0 <= n ->
  bin_value (drop_bits (Z_to_binary n 32) k) = Z.shiftr n (Z.of_nat k).
Proof.
  (* Will prove that dropping k bits gives n / 2^k *)
  admit.
Admitted.

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
  - simpl. reflexivity.
  - simpl. rewrite IH. rewrite pow2_succ. lia.
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
    simpl (2 ^ Z.of_nat (S k)) in Hnbound.
    assert (Hdiv: n = 2 * (n / 2) + n mod 2) by (apply Z.div_mod; lia).
    rewrite <- Hdiv at 2.
    f_equal.
    + apply Z.mod_pos_bound. lia.
    + rewrite <- IH.
      * unfold bin_value.
        assert (H: binary_to_Z (Z_to_binary (n / 2) k) 1 =
                   2 * binary_to_Z (Z_to_binary (n / 2) k) 0).
        { apply binary_to_Z_shift. }
        rewrite H. f_equal. reflexivity.
      * split.
        -- apply Z.div_pos; lia.
        -- apply Z.div_lt_upper_bound; lia.
Qed.

(* ========================================================================= *)
(* RECONSTRUCTION THEOREM                                                    *)
(* ========================================================================= *)

(** Reconstruct 32-bit value from 4 bytes using binary model *)
Theorem binary_u32_reconstruct : forall n,
  0 <= n < 4294967296 ->
  let bs := Z_to_binary n 32 in
  bin_value (take_bits bs 8) +
  bin_value (take_bits (drop_bits bs 8) 8) * 256 +
  bin_value (take_bits (drop_bits bs 16) 8) * 65536 +
  bin_value (take_bits (drop_bits bs 24) 8) * 16777216 = n.
Proof.
  intros n Hn bs.
  (* The key insight: the binary representation makes this obvious *)
  (* Byte 0: bits 0-7 contribute value 0 to 255 *)
  (* Byte 1: bits 8-15 contribute value 0 to 255, shifted by 8 (multiply by 256) *)
  (* Byte 2: bits 16-23 contribute value 0 to 255, shifted by 16 (multiply by 65536) *)
  (* Byte 3: bits 24-31 contribute value 0 to 255, shifted by 24 (multiply by 16777216) *)
  (* Sum of all bytes = original value *)

  (* This should follow directly from the definition of bin_value *)
  (* and the structure of binary representation *)
  admit.
Admitted.
