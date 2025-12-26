(** * Namespace Model - Process groups, mount points, and namespace containment
    *
    * Imports: channel_model.v (for Chan)
    * Models: Pgrp (process group), Mhead (mount point), Mount (union mount entry)
    *
    * Used by: mount_safety.v for namespace containment proofs
    *
    * IMPLEMENTATION: kernel/include/portdat.h:262-280 (Pgrp, Mhead, Mount)
    *                 kernel/9front-port/chan.c (namespace operations)
    *)

Require Import Coq.ZArith.ZArith.
Require Import Coq.Lists.List.
Require Import Coq.Bool.Bool.
Require Import Lia.
Import ListNotations.

Require Import mnt.channel_model.

Open Scope Z_scope.

(* ========================================================================= *)
(* MOUNT STRUCTURES                                                          *)
(* ========================================================================= *)

(** Mount flags (from portdat.h) *)
Definition MREPL : Z := 0x0000.   (* Mount replaces object *)
Definition MBEFORE : Z := 0x0001. (* Mount goes before others in union *)
Definition MAFTER : Z := 0x0002.  (* Mount goes after others *)
Definition MCREATE : Z := 0x0004. (* Permit creation in mounted dir *)
Definition MCACHE : Z := 0x0010.  (* Cache some data *)
Definition MMASK : Z := 0x0017.   (* All mount flags *)

(** Individual mount in a union mount *)
Record Mount := mkMount {
  mount_id : Z;          (* uvlong: unique mount ID *)
  mount_flag : Z;        (* int: mount flags (MREPL/MBEFORE/MAFTER) *)
  mount_to : Chan;       (* Chan*: channel replacing mounted-on channel *)
  mount_spec : list nat  (* char[]: mount specification string (abstracted as list) *)
}.

(** Mount head - represents a mount point *)
Record Mhead := mkMhead {
  mhead_ref : Z;              (* long: reference count *)
  mhead_from : Chan;          (* Chan*: channel being mounted upon *)
  mhead_mounts : list Mount   (* Mount*: union mount list *)
}.

(** Process group namespace hash size *)
Definition MNTHASH : Z := 32.

(** Process group with namespace *)
Record Pgrp := mkPgrp {
  pgrp_ref : Z;                      (* long: reference count *)
  pgrp_mnthash : list (option Mhead) (* Mhead*[MNTHASH]: mount point hash table *)
}.

(* ========================================================================= *)
(* NAMESPACE PREDICATES                                                      *)
(* ========================================================================= *)

(** Channel is in process group's namespace *)
Definition InNamespace (pg : Pgrp) (c : Chan) : Prop :=
  exists mh : Mhead,
    In (Some mh) (pgrp_mnthash pg) /\
    (mhead_from mh = c \/ exists m, In m (mhead_mounts mh) /\ mount_to m = c).

(** Mount point exists in namespace *)
Definition MountPointExists (pg : Pgrp) (c : Chan) : Prop :=
  exists mh : Mhead,
    In (Some mh) (pgrp_mnthash pg) /\
    mhead_from mh = c.

(** Path depth (number of components from root) *)
Fixpoint path_depth (path : list nat) : nat :=
  match path with
  | [] => 0
  | _ :: rest => S (path_depth rest)
  end.

(** Mount depth (distance from root in mount tree) *)
Definition mount_depth (c : Chan) : nat :=
  (* Simplified: would track actual mount nesting in full model *)
  if chan_ismtpt c then 1 else 0.

(** Channel is a mount point *)
Definition is_mountpoint (c : Chan) : Prop :=
  chan_ismtpt c = true.

(* ========================================================================= *)
(* NAMESPACE WELL-FORMEDNESS                                                 *)
(* ========================================================================= *)

(** Well-formed mount *)
Definition MountWellFormed (m : Mount) : Prop :=
  mount_id m > 0 /\
  Z.land (mount_flag m) MMASK = mount_flag m /\  (* Only valid flags *)
  ChanWellFormed (mount_to m).

(** Well-formed mount head *)
Definition MheadWellFormed (mh : Mhead) : Prop :=
  mhead_ref mh > 0 /\
  ChanWellFormed (mhead_from mh) /\
  Forall MountWellFormed (mhead_mounts mh) /\
  (* Union mount ordering is consistent *)
  (forall m1 m2,
    In m1 (mhead_mounts mh) ->
    In m2 (mhead_mounts mh) ->
    mount_id m1 = mount_id m2 ->
    m1 = m2).

(** Well-formed process group namespace *)
Definition PgrpWellFormed (pg : Pgrp) : Prop :=
  pgrp_ref pg > 0 /\
  length (pgrp_mnthash pg) = Z.to_nat MNTHASH /\
  Forall (fun mh_opt =>
    match mh_opt with
    | None => True
    | Some mh => MheadWellFormed mh
    end) (pgrp_mnthash pg).

(* ========================================================================= *)
(* MOUNT OPERATIONS                                                          *)
(* ========================================================================= *)

(** Add mount to mount head *)
Definition add_mount (mh : Mhead) (m : Mount) : Mhead :=
  mkMhead
    (mhead_ref mh)
    (mhead_from mh)
    (m :: mhead_mounts mh).  (* Simplified: actual code handles ordering *)

(** Remove mount from mount head *)
Fixpoint remove_mount (mid : Z) (mounts : list Mount) : list Mount :=
  match mounts with
  | [] => []
  | m :: rest =>
      if Z.eqb (mount_id m) mid
      then rest
      else m :: remove_mount mid rest
  end.

(** Find mount head for channel in namespace *)
Fixpoint find_mhead (c : Chan) (hash : list (option Mhead)) : option Mhead :=
  match hash with
  | [] => None
  | None :: rest => find_mhead c rest
  | Some mh :: rest =>
      if chan_dev (mhead_from mh) =? chan_dev c
      then Some mh
      else find_mhead c rest
  end.

(* ========================================================================= *)
(* NAMESPACE CONTAINMENT PROPERTIES                                          *)
(* ========================================================================= *)

(** Theorem: Adding mount preserves well-formedness *)
Theorem add_mount_preserves_wellformed : forall mh m,
  MheadWellFormed mh ->
  MountWellFormed m ->
  (forall m', In m' (mhead_mounts mh) -> mount_id m' <> mount_id m) ->
  MheadWellFormed (add_mount mh m).
Proof.
  intros mh m Hwf_mh Hwf_m Huniq.
  destruct Hwf_mh as [Href [Hfrom [Hmounts Huniq_mh]]].
  unfold MheadWellFormed, add_mount. simpl.
  split; [| split; [| split]].
  - exact Href.
  - exact Hfrom.
  - constructor.
    + exact Hwf_m.
    + exact Hmounts.
  - intros m1 m2 Hin1 Hin2 Heq.
    simpl in Hin1, Hin2.
    destruct Hin1 as [Heq1 | Hin1], Hin2 as [Heq2 | Hin2].
    + subst. reflexivity.
    + subst. exfalso. apply (Huniq m2 Hin2). symmetry. exact Heq.
    + subst. exfalso. apply (Huniq m1 Hin1). exact Heq.
    + apply Huniq_mh; assumption.
Qed.

(** Helper: Elements in removed list are subset of original *)
Lemma remove_mount_subset : forall mid m mounts,
  In m (remove_mount mid mounts) -> In m mounts.
Proof.
  intros mid m mounts.
  induction mounts as [| x rest IH].
  - simpl. intro H. contradiction.
  - simpl. destruct (Z.eqb (mount_id x) mid) eqn:E.
    + intro H. right. exact H.
    + simpl. intro H. destruct H as [H | H].
      * left. exact H.
      * right. apply IH. exact H.
Qed.

(** Theorem: Removing mount preserves well-formedness *)
Theorem remove_mount_preserves_wellformed : forall mid mh,
  MheadWellFormed mh ->
  MheadWellFormed (mkMhead
    (mhead_ref mh)
    (mhead_from mh)
    (remove_mount mid (mhead_mounts mh))).
Proof.
  intros mid mh Hwf.
  destruct Hwf as [Href [Hfrom [Hmounts Huniq]]].
  unfold MheadWellFormed. simpl.
  split; [| split; [| split]].
  - exact Href.
  - exact Hfrom.
  - induction (mhead_mounts mh) as [| m rest IH].
    + simpl. constructor.
    + simpl. destruct (Z.eqb (mount_id m) mid) eqn:E.
      * inversion Hmounts. assumption.
      * inversion Hmounts as [| ? ? H_m H_rest]; subst.
        constructor.
        -- exact H_m.
        -- apply IH.
           ++ exact H_rest.
           ++ intros m1 m2 Hin1 Hin2 Heq.
              apply Huniq; try (right; assumption); exact Heq.
  - intros m1 m2 Hin1 Hin2 Heq.
    apply Huniq.
    + apply (remove_mount_subset mid). exact Hin1.
    + apply (remove_mount_subset mid). exact Hin2.
    + exact Heq.
Qed.

(** Theorem: Mount IDs are unique in well-formed mount head *)
Theorem mount_ids_unique : forall mh m1 m2,
  MheadWellFormed mh ->
  In m1 (mhead_mounts mh) ->
  In m2 (mhead_mounts mh) ->
  mount_id m1 = mount_id m2 ->
  m1 = m2.
Proof.
  intros mh m1 m2 Hwf Hin1 Hin2 Heq.
  destruct Hwf as [_ [_ [_ Huniq]]].
  apply Huniq; assumption.
Qed.

Print Assumptions add_mount_preserves_wellformed.
Print Assumptions remove_mount_preserves_wellformed.
Print Assumptions mount_ids_unique.
