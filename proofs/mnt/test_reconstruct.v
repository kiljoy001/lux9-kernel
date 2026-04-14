Require Import Coq.ZArith.ZArith.
Require Import Lia.

Open Scope Z_scope.

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

Print Assumptions reconstruct_from_2_bytes.

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
  (* Use E1: n = 256 * (n / 256) + n mod 256 *)
  rewrite E1.
  (* Use E2 to expand n/256: n/256 = 256 * (n/65536) + (n/256) mod 256 *)
  rewrite E2 at 1.
  rewrite Hdiv, Hmod2.
  (* Now both sides should be equal *)
  ring.
Qed.

Print Assumptions reconstruct_from_3_bytes.
