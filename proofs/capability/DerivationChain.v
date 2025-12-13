(* Transitive derivation chains and monotonicity of permissions. *)
Require Import proofs.capability.PermsBitmask.
Require Import proofs.capability.CapabilityModel.

Section DerivationChain.

Inductive derived_chain (ct : cap_table) : Capability -> Capability -> Prop :=
| DC_Refl : forall c, derived_chain ct c c
| DC_Step : forall c p g,
    derived_from ct c p ->
    derived_chain ct p g ->
    derived_chain ct c g.

Lemma derived_chain_perms_monotonic :
  forall ct c anc,
    derived_chain ct c anc ->
    perms_subset (cap_perms c) (cap_perms anc).
Proof.
  intros ct c anc Hchain.
  induction Hchain.
  - apply perms_subset_refl.
  - eapply perms_subset_trans.
    + apply derived_perm_monotonic with (ct:=ct) (p:=p) in H.
      exact H.
    + exact IHHchain.
Qed.

End DerivationChain.
