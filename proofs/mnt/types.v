(** * Mount Device Types - Base definitions for devmnt verification
    * 
    * Models the Mnt, Mntrpc, and tag allocation structures from devmnt.c
    *)

Require Export Coq.ZArith.ZArith.
Require Export Coq.Bool.Bool.
Require Export Coq.Lists.List.
Require Export Lia.
Export ListNotations.

Open Scope Z_scope.

(* ========================================================================= *)
(* CONSTANTS FROM devmnt.c                                                   *)
(* ========================================================================= *)

Definition TAGSHIFT : Z := 5.
Definition TAGMASK : Z := 31.  (* (1 << TAGSHIFT) - 1 *)
Definition NMASK : Z := 2048.  (* 64K >> TAGSHIFT *)
Definition NOTAG : Z := 65535. (* Maximum tag value, reserved *)

(* ========================================================================= *)
(* TAG ALLOCATION STATE                                                      *)
(* ========================================================================= *)

(* Tag bitmap abstraction: set of allocated tags *)
Definition TagSet := Z -> bool.

Definition empty_tagset : TagSet := fun _ => false.

Definition tag_allocated (ts : TagSet) (tag : Z) : Prop :=
  ts tag = true.

Definition tag_free (ts : TagSet) (tag : Z) : Prop :=
  ts tag = false.

Definition alloc_tag (ts : TagSet) (tag : Z) : TagSet :=
  fun t => if Z.eqb t tag then true else ts t.

Definition free_tag (ts : TagSet) (tag : Z) : TagSet :=
  fun t => if Z.eqb t tag then false else ts t.

(* ========================================================================= *)
(* RPC STATE                                                                 *)
(* ========================================================================= *)

Record RpcState := mkRpc {
  rpc_tag : Z;
  rpc_done : bool;
  rpc_flushed : bool;
}.

(* ========================================================================= *)
(* MNTALLOC STATE                                                            *)
(* ========================================================================= *)

Record MntAllocState := mkMntAlloc {
  ma_id : Z;              (* Next mount ID *)
  ma_nrpcfree : Z;        (* Number of free RPCs *)
  ma_nrpcused : Z;        (* Number of used RPCs *)
  ma_tags : TagSet;       (* Tag allocation bitmap *)
}.

Definition initial_mntalloc : MntAllocState :=
  mkMntAlloc 1 0 0 (fun tag => 
    if Z.eqb tag 0 then true           (* Tag 0 reserved *)
    else if Z.eqb tag NOTAG then true  (* NOTAG reserved *)
    else false).

(* ========================================================================= *)
(* MNT STATE                                                                 *)
(* ========================================================================= *)

Record MntState := mkMnt {
  mnt_id : Z;
  mnt_msize : Z;
  mnt_queue_len : Z;   (* Number of pending RPCs *)
}.

(* ========================================================================= *)
(* INVARIANTS                                                                *)
(* ========================================================================= *)

Definition Inv_TagValid (tag : Z) : Prop :=
  tag >= 0 /\ tag < 65535 /\ tag <> 0 /\ tag <> NOTAG.

Definition Inv_RpcCountsNonNeg (ma : MntAllocState) : Prop :=
  ma.(ma_nrpcfree) >= 0 /\ ma.(ma_nrpcused) >= 0.

Definition Inv_IdPositive (ma : MntAllocState) : Prop :=
  ma.(ma_id) >= 1.

Definition Inv_WellFormed (ma : MntAllocState) : Prop :=
  Inv_RpcCountsNonNeg ma /\ Inv_IdPositive ma.

(* ========================================================================= *)
(* BASIC LEMMAS                                                              *)
(* ========================================================================= *)

Lemma initial_mntalloc_wellformed : Inv_WellFormed initial_mntalloc.
Proof.
  unfold Inv_WellFormed, Inv_RpcCountsNonNeg, Inv_IdPositive.
  unfold initial_mntalloc. simpl.
  lia.
Qed.

Lemma alloc_tag_sets : forall ts tag,
  tag_allocated (alloc_tag ts tag) tag.
Proof.
  intros ts tag. unfold tag_allocated, alloc_tag.
  rewrite Z.eqb_refl. reflexivity.
Qed.

Lemma free_tag_clears : forall ts tag,
  tag_free (free_tag ts tag) tag.
Proof.
  intros ts tag. unfold tag_free, free_tag.
  rewrite Z.eqb_refl. reflexivity.
Qed.
