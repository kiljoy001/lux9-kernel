(** * Lux9 Memory Architecture & Borrow Checker Verification v2.0
    * Consolidated specification: Waterline, HHDM, Two-Factor 9P
    *)

Require Import Coq.Lists.List.
Require Import Coq.Arith.Arith.
Require Import Coq.Arith.PeanoNat.
Require Import Coq.Bool.Bool.
Import ListNotations.

Ltac inv H := inversion H; subst; clear H.

(* --- 2.1 Definitions --- *)

Definition u64 := nat.
Definition Pid := nat.
Definition PageId := nat.

Parameter KERNEL_BASE : u64. (* 0xFFFF800000000000 *)
Parameter USER_MAX : u64.    (* 0x00007FFFFFFFFFFF *)
Axiom Waterline : USER_MAX < KERNEL_BASE.

Definition is_user_pid (p : Pid) : Prop := p > 0. (* Assuming 0 is kernel *)

(* --- 3.1 The Capability (The Key) --- *)
Record IdentKey := mkKey {
  gen : u64;
  nonce : u64;
}.

Definition key_eq (k1 k2 : IdentKey) : bool :=
  (Nat.eqb k1.(gen) k2.(gen)) && (Nat.eqb k1.(nonce) k2.(nonce)).

(* --- 3.2 The Ledger Entry (The Page State) --- *)
Inductive OwnerState :=
  | Free
  | Exclusive
  | SharedOwned
  | MutLent.

Record PageState := mkPageState {
  owner : option Pid;
  state : OwnerState;
  key   : IdentKey;
  (* shared/mut tracking for borrow checker logic *)
  shared_borrowers : list Pid;
  mut_borrower : option Pid;
}.

(* --- Global System State --- *)
Record SystemState := mkSystemState {
  ledger : PageId -> PageState;
  tables : Pid -> u64 -> option PageId; (* Page Tables *)
}.

(* Initial State *)
Definition init_key := mkKey 0 0.
Definition init_page := mkPageState None Free init_key [] None.
Definition init_tables (p:Pid) (v:u64) : option PageId := None.

Definition init_state := mkSystemState (fun _ => init_page) init_tables.

(* --- Helpers --- *)

Definition update_ledger (s : SystemState) (pg : PageId) (ps : PageState) : SystemState :=
  mkSystemState 
    (fun p' => if Nat.eqb pg p' then ps else s.(ledger) p')
    s.(tables).

Definition update_tables (s : SystemState) (pid : Pid) (vaddr : u64) (pg : option PageId) : SystemState :=
  mkSystemState
    s.(ledger)
    (fun p v => if (Nat.eqb p pid) && (Nat.eqb v vaddr) then pg else s.(tables) p v).

Definition virt_to_phys (tbl : u64 -> option PageId) (v : u64) : option PageId := tbl v.

Definition valid_cap (pg : PageId) (cap : IdentKey) (s : SystemState) : bool :=
  key_eq (s.(ledger) pg).(key) cap.

(* --- 3.3 The 9P Message --- *)
Record Twrite_ZC := mkTwrite {
  data_ptr : u64;
  length   : u64;
  cap      : IdentKey;
}.

(* --- 4. The Broker Algorithm (Transitions) --- *)

(* Acquire (Alloc) - Not fully detailed in spec but needed for system to move *)
Inductive Alloc (p : Pid) (pg : PageId) (v : u64) (k : IdentKey) (s1 s2 : SystemState) : Prop :=
  | Alloc_Success :
      (s1.(ledger) pg).(state) = Free ->
      v <= USER_MAX -> (* Waterline check *)
      (* Effect: Update ledger, Update PT *)
      s2 = update_tables 
             (update_ledger s1 pg (mkPageState (Some p) Exclusive k [] None))
             p v (Some pg) ->
      Alloc p pg v k s1 s2.

(* Ipc_Send (Transfer) - The core of v2.0 *)
Inductive Ipc_Send (sender : Pid) (receiver : Pid) (msg : Twrite_ZC) (v_new : u64) (s1 s2 : SystemState) : Prop :=
  | Send_Success :
      forall (phys_id : PageId),
      (* 1. Range Check *)
      msg.(data_ptr) <= USER_MAX ->
      
      (* 2. Resolution *)
      virt_to_phys (s1.(tables) sender) msg.(data_ptr) = Some phys_id ->
      
      (* 3. Authorization *)
      valid_cap phys_id msg.(cap) s1 = true ->
      
      (* 4. Ownership *)
      (s1.(ledger) phys_id).(owner) = Some sender ->
      (s1.(ledger) phys_id).(state) = Exclusive -> (* Requirement for transfer *)
      
      (* 5. Effect *)
      v_new <= USER_MAX -> (* New address must be safe *)
      
      (* Unmap from Sender, Map to Receiver, Update Owner *)
      s2 = update_tables
             (update_tables 
                (update_ledger s1 phys_id (mkPageState (Some receiver) Exclusive msg.(cap) [] None))
                sender msg.(data_ptr) None) (* Unmap *)
             receiver v_new (Some phys_id) -> (* Map *)
             
      Ipc_Send sender receiver msg v_new s1 s2.

Inductive Step (s1 s2 : SystemState) : Prop :=
  | Step_Alloc : forall p pg v k, Alloc p pg v k s1 s2 -> Step s1 s2
  | Step_Ipc   : forall s r m v, Ipc_Send s r m v s1 s2 -> Step s1 s2.

(* --- 5.2 Invariants --- *)

Definition Inv_UserSpaceIsolation (s : SystemState) : Prop :=
  forall (pid : Pid) (vaddr : u64) (phys : PageId),
    is_user_pid pid ->
    s.(tables) pid vaddr = Some phys ->
    vaddr < KERNEL_BASE.

(* Proof of Isolation *)
Theorem UserSpace_Isolation_Preserved :
  forall s1 s2, 
    Inv_UserSpaceIsolation s1 -> 
    Step s1 s2 -> 
    Inv_UserSpaceIsolation s2.
Proof.
  intros s1 s2 Hinv Hstep.
  induction Hstep.
  - (* Alloc *)
    inv H.
    unfold Inv_UserSpaceIsolation in *. intros pid0 v0 phys0 Huser Hmap.
    unfold update_tables in Hmap. simpl in Hmap.
    destruct (Nat.eqb pid0 p) eqn:Ep; destruct (Nat.eqb v0 v) eqn:Ev.
    + (* New mapping *)
      injection Hmap; intro; subst.
      apply Nat.eqb_eq in Ev. rewrite Ev.
      eapply Nat.le_lt_trans. apply H1. apply Waterline.
    + (* Old mapping *)
      eapply Hinv; eauto.
    + eapply Hinv; eauto.
    + eapply Hinv; eauto.
    
  - (* Ipc_Send *)
    inv H.
    unfold Inv_UserSpaceIsolation in *. intros pid0 v0 phys0 Huser Hmap.
    (* s2 has two table updates: Unmap sender, Map receiver *)
    (* Unfold outer update (Map receiver) *)
    unfold update_tables in Hmap. simpl in Hmap.
    destruct (Nat.eqb pid0 r) eqn:Heqr; destruct (Nat.eqb v0 v) eqn:Heqv.
    + (* The new mapping for receiver *)
      injection Hmap; intro; subst.
      apply Nat.eqb_eq in Heqv. rewrite Heqv.
      eapply Nat.le_lt_trans. apply H5. apply Waterline.
    + (* Look at inner update (Unmap sender) *)
      destruct (Nat.eqb pid0 s) eqn:Heqs; destruct (Nat.eqb v0 (data_ptr m)) eqn:Heqd.
      * (* Unmapped *)
        discriminate. (* It was set to None *)
      * (* Unchanged *)
        eapply Hinv; eauto.
      * destruct (Nat.eqb v0 (data_ptr m)); try discriminate; eapply Hinv; eauto.
      * destruct (Nat.eqb v0 (data_ptr m)); try discriminate; eapply Hinv; eauto.
    + destruct (Nat.eqb pid0 s); destruct (Nat.eqb v0 (data_ptr m)); try discriminate; eapply Hinv; eauto.
    + destruct (Nat.eqb pid0 s); destruct (Nat.eqb v0 (data_ptr m)); try discriminate; eapply Hinv; eauto.
Qed.
