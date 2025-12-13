(** * Broker Transfer Protocol Verification
    * Models the 'borrow_broker_transfer' function from kernel/borrowchecker.c
    * 
    * Key Properties Verified:
    * 1. Ownership Safety: Resource transfers only happen between valid owners.
    * 2. Capability Security: Transfers require knowledge of the secret nonce.
    * 3. Replay Protection: Successful transfers rotate the nonce/generation.
    * 4. Exclusivity: Transfers are blocked if the resource is borrowed.
    *)

Require Import Coq.ZArith.ZArith.
Require Import Coq.Lists.List.
Require Import Coq.Bool.Bool.
Import ListNotations.

Open Scope Z_scope.

(* ========================================================================= *)
(* TYPES & STATE *)
(* ========================================================================= *)

Definition Pid := Z.
Definition ResourceId := Z.
Definition Gen := Z.
Definition Nonce := Z.

Record Capability := mkCap {
  gen : Gen;
  nonce : Nonce;
}.

Inductive BorrowState :=
  | Free
  | Exclusive
  | SharedOwned
  | MutLent.

Record Resource := mkResource {
  owner : option Pid;
  state : BorrowState;
  cap : Capability;
  shared_count : Z;
  mut_borrower : option Pid;
}.

Definition SystemState := ResourceId -> Resource.

(* Helper for checking equality of capabilities *)
Definition cap_eqb (c1 c2 : Capability) : bool :=
  (Z.eqb c1.(gen) c2.(gen)) && (Z.eqb c1.(nonce) c2.(nonce)).

(* Update function for state *)
Definition update_res (s : SystemState) (r_id : ResourceId) (new_r : Resource) : SystemState :=
  fun id => if Z.eqb id r_id then new_r else s id.

(* ========================================================================= *)
(* THE BROKER TRANSFER MODEL *)
(* ========================================================================= *)

(* 
 * Models: borrow_broker_transfer(Proc *sender, Proc *receiver, uintptr key, struct IdentKey cap)
 *)
Inductive BrokerTransfer (sender : Pid) (receiver : Pid) (r_id : ResourceId) (input_cap : Capability) (new_nonce : Nonce) 
  (s1 s2 : SystemState) : Prop :=
  | Transfer_Success :
      let r := s1 r_id in
      (* Check 1: Ledger Existence (Implicit in our total map, but check state != Free) *)
      r.(state) <> Free ->
      
      (* Check 2: Ownership *)
      r.(owner) = Some sender ->
      
      (* Check 3: Capability Match (Two-Factor) *)
      cap_eqb r.(cap) input_cap = true ->
      
      (* Check 4: Exclusivity (No borrows active) *)
      r.(shared_count) = 0 ->
      r.(mut_borrower) = None ->
      
      (* Effect: Update Owner, Increment Gen, Rotate Nonce *)
      let new_cap := mkCap (r.(cap).(gen) + 1) new_nonce in
      let new_r := mkResource (Some receiver) Exclusive new_cap 0 None in
      s2 = update_res s1 r_id new_r ->
      
      BrokerTransfer sender receiver r_id input_cap new_nonce s1 s2.

(* ========================================================================= *)
(* HELPER LEMMAS *)
(* ========================================================================= *)

Lemma update_res_eq : forall s r_id new_r,
  (update_res s r_id new_r) r_id = new_r.
Proof.
  intros. unfold update_res.
  rewrite Z.eqb_refl. reflexivity.
Qed.

(* ========================================================================= *)
(* MAIN THEOREMS *)
(* ========================================================================= *)

(* Theorem 1: Authorization
   A transfer implies that the caller provided the correct current capability. 
*)
Theorem Transfer_Requires_Capability :
  forall sender receiver r_id input_cap nonce s1 s2,
  BrokerTransfer sender receiver r_id input_cap nonce s1 s2 ->
  cap_eqb (s1 r_id).(cap) input_cap = true.
Proof.
  intros sender receiver r_id input_cap nonce s1 s2 H.
  inversion H. subst.
  assumption.
Qed.

(* Theorem 2: Nonce Rotation (Replay Protection)
   After a transfer, the OLD capability is no longer valid for the resource.
*)
Theorem Transfer_Rotates_Nonce :
  forall sender receiver r_id old_cap nonce s1 s2,
  BrokerTransfer sender receiver r_id old_cap nonce s1 s2 ->
  cap_eqb (s2 r_id).(cap) old_cap = false.
Proof.
  intros sender receiver r_id old_cap nonce s1 s2 H.
  inversion H as [r Hstate Howner Hcap Hshared Hmut Hnonce Hnewr Hupdate]. 
  subst.
  
  rewrite update_res_eq.
  simpl.
  
  (* Proving that (gen + 1 =? gen) is false *)
  unfold cap_eqb.
  apply andb_false_intro1.
  apply Z.eqb_neq.
  
  (* From Hcap we know gen (cap r) = gen old_cap *)
  unfold cap_eqb in Hcap.
  apply andb_true_iff in Hcap.
  destruct Hcap as [Hgen _].
  apply Z.eqb_eq in Hgen.
  
  rewrite <- Hgen.
  apply Z.neq_succ_diag_l.
Qed.

(* Theorem 3: Ownership Preservation
   If a transfer occurs, the new owner is indeed the receiver.
*)
Theorem Transfer_Changes_Owner :
  forall sender receiver r_id input_cap nonce s1 s2,
  BrokerTransfer sender receiver r_id input_cap nonce s1 s2 ->
  (s2 r_id).(owner) = Some receiver.
Proof.
  intros sender receiver r_id input_cap nonce s1 s2 H.
  inversion H. subst.
  rewrite update_res_eq.
  reflexivity.
Qed.

(* Theorem 4: Exclusivity
   A transfer cannot occur if the resource is currently borrowed (shared or mutable).
*)
Theorem Transfer_Respects_Borrows :
  forall sender receiver r_id input_cap nonce s1 s2,
  (s1 r_id).(shared_count) > 0 \/ (s1 r_id).(mut_borrower) <> None ->
  ~ BrokerTransfer sender receiver r_id input_cap nonce s1 s2.
Proof.
  intros sender receiver r_id input_cap nonce s1 s2 Borrows Transfer.
  inversion Transfer as [r Hstate Howner Hcap Hshared Hmut Hnonce Hnewr Hupdate]. 
  subst.
  
  (* Align terms for rewriting *)
  replace (s1 r_id) with r in Borrows by reflexivity.
  
  destruct Borrows as [Shared | Mut].
  - (* Shared count > 0 case *)
    rewrite Hshared in Shared.
    (* Shared : 0 > 0 which implies False *)
    discriminate Shared.
  - (* Mut borrower case *)
    rewrite Hmut in Mut. 
    apply Mut. 
    reflexivity.
Qed.
