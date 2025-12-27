(** * Mount Safety - Namespace containment and privilege preservation
    *
    * Imports: namespace_model.v, channel_model.v, message_routing.v
    * Proves: Walk containment, mount boundaries, version protocol
    *
    * Used by: Future devmnt_verified.v integration
    *
    * IMPLEMENTATION: kernel/9front-port/devmnt.c:378-456 (mntwalk)
    *                 kernel/9front-port/devmnt.c:309-361 (mntattach)
    *                 kernel/9front-port/devmnt.c:100-259 (mntversion)
    *)

Require Import Coq.ZArith.ZArith.
Require Import Coq.Lists.List.
Require Import Coq.Bool.Bool.
Require Import Coq.Arith.Arith.
Require Import Lia.
Import ListNotations.

Require Import mnt.namespace_model.
Require Import mnt.channel_model.
Require Import mnt.message_routing.

Open Scope Z_scope.

(* ========================================================================= *)
(* WALK OPERATIONS                                                           *)
(* ========================================================================= *)

(** Path component (simplified - actual implementation uses strings) *)
Definition PathComponent : Type := nat.

(** Special path components *)
Definition DOT : PathComponent := 0%nat.
Definition DOTDOT : PathComponent := 1%nat.

(** Walk result *)
Inductive WalkResult :=
  | WalkSuccess (c : Chan)
  | WalkError (msg : nat).  (* Error code *)

(** Walk operation model - parameterized by process group for namespace tracking *)
Inductive Walk : Pgrp -> Chan -> list PathComponent -> WalkResult -> Prop :=
  | Walk_Empty : forall pg c,
      PgrpWellFormed pg ->
      InNamespace pg c ->
      ChanWellFormed c ->
      Walk pg c [] (WalkSuccess c)
  | Walk_Dot : forall pg c path result,
      Walk pg c path result ->
      Walk pg c (DOT :: path) result
  | Walk_DotDot : forall pg c path result c_parent,
      chan_ismtpt c = true ->
      IsParentOf pg c_parent c ->  (* EXPLICIT PARENT RELATIONSHIP *)
      Walk pg c_parent path result ->
      Walk pg c (DOTDOT :: path) result
  | Walk_Component : forall pg c comp path result c_child,
      comp <> DOT ->
      comp <> DOTDOT ->
      InNamespace pg c_child ->  (* Child must be in namespace *)
      Walk pg c_child path result ->
      Walk pg c (comp :: path) result
  | Walk_Error : forall pg c path err,
      (* Error conditions: permission denied, not found, etc. *)
      Walk pg c path (WalkError err).

(* ========================================================================= *)
(* NAMESPACE CONTAINMENT                                                     *)
(* ========================================================================= *)

(** Theorem: Walk_DotDot implies parent relationship
    * When Walk_DotDot gives us c_parent, it is the actual parent of c.
    * Now proven directly from Walk_DotDot constructor.
    * For WalkError, the existential is vacuously true.
    *)
Theorem walk_dotdot_implies_parent : forall pg c path result,
  Walk pg c (DOTDOT :: path) result ->
  match result with
  | WalkSuccess _ => exists c_parent,
      IsParentOf pg c_parent c /\
      Walk pg c_parent path result
  | WalkError _ => True
  end.
Proof.
  intros pg c path result Hwalk.
  destruct result as [c'|err].
  - (* WalkSuccess c' *)
    inversion Hwalk; subst; try discriminate.
    + (* Walk_DotDot - c_parent is the parent *)
      exists c_parent. split; assumption.
    + (* Walk_Component - impossible because comp <> DOTDOT *)
      exfalso. contradiction.
  - (* WalkError err *)
    exact I.
Qed.

(** Theorem: Parent of mount point is in namespace *)
Theorem mount_parent_in_namespace : forall pg c c_parent,
  PgrpWellFormed pg ->
  InNamespace pg c ->
  chan_ismtpt c = true ->
  IsParentOf pg c_parent c ->
  InNamespace pg c_parent.
Proof.
  intros pg c c_parent Hwf_pg Hin_c Hmtpt Hparent.
  apply (parent_in_namespace pg c_parent c); assumption.
Qed.

(** Axiom: Child channel from walk stays in namespace
    * Requires full 9P walk RPC model.
    *)
Axiom walk_child_in_namespace : forall pg c c_child,
  PgrpWellFormed pg ->
  InNamespace pg c ->
  ChanWellFormed c_child ->
  (* Simplified: c_child obtained from 9P walk on c *)
  InNamespace pg c_child.

(** Theorem: Walk preserves namespace membership *)
Theorem walk_namespace_containment : forall pg c path result,
  PgrpWellFormed pg ->
  InNamespace pg c ->
  Walk pg c path result ->
  match result with
  | WalkSuccess c' => InNamespace pg c'
  | WalkError _ => True
  end.
Proof.
  intros pg c path result Hwf_pg Hin Hwalk.
  induction Hwalk.
  - (* Walk_Empty: c stays in namespace *)
    exact H0.
  - (* Walk_Dot: . doesn't change channel *)
    apply IHHwalk; assumption.
  - (* Walk_DotDot: parent is in namespace *)
    apply IHHwalk; try assumption.
    (* c_parent is in namespace because it's the parent of c *)
    apply (parent_in_namespace pg c_parent c); try assumption.
  - (* Walk_Component: child is in namespace *)
    apply IHHwalk; try assumption.
  - (* Walk_Error: trivially true *)
    exact I.
Qed.

(** Lemma: Walking on single-element DOTDOT path *)
Lemma walk_single_dotdot : forall pg c result,
  Walk pg c [DOTDOT] result ->
  (exists c_parent, Walk pg c_parent [] result /\ chan_ismtpt c = true /\ IsParentOf pg c_parent c) \/
  (exists err, result = WalkError err).
Proof.
  intros pg c result H.
  inversion H; subst; try discriminate.
  - (* Walk_DotDot *)
    left. exists c_parent. split; [| split]; assumption.
  - (* Walk_Component: DOTDOT contradicts comp <> DOTDOT *)
    exfalso. contradiction.
  - (* Walk_Error *)
    right. exists err. reflexivity.
Qed.

(** Lemma: Walking on empty path gives same channel *)
Lemma walk_empty_same : forall pg c result,
  Walk pg c [] result ->
  result = WalkSuccess c \/ exists err, result = WalkError err.
Proof.
  intros pg c result H.
  inversion H; subst; try discriminate.
  - (* Walk_Empty *)
    left. reflexivity.
  - (* Walk_Error *)
    right. exists err. reflexivity.
Qed.

(** Theorem: Dotdot bounded by mount depth *)
Theorem dotdot_bounded : forall pg (c c' : Chan),
  chan_ismtpt c = true ->
  Walk pg c [DOTDOT] (WalkSuccess c') ->
  (mount_depth c' <= mount_depth c)%nat.
Proof.
  intros pg c c' Hmtpt Hwalk.
  apply walk_single_dotdot in Hwalk.
  destruct Hwalk as [[c_parent [Hwalk_parent [Hmtpt' Hparent]]] | [err Herr]].
  - (* Walk pg c_parent [] (WalkSuccess c') *)
    apply walk_empty_same in Hwalk_parent.
    destruct Hwalk_parent as [Heq | [err' Herr']].
    + (* WalkSuccess c_parent *)
      inversion Heq; subst.
      simpl. unfold mount_depth. rewrite Hmtpt. simpl.
      destruct (chan_ismtpt c_parent); auto with arith.
    + (* WalkError: contradicts WalkSuccess c' *)
      congruence.
  - (* WalkError: contradicts WalkSuccess c' *)
    congruence.
Qed.

(* ========================================================================= *)
(* VERSION PROTOCOL                                                          *)
(* ========================================================================= *)

(** Mnt state with version *)
Record MntConn := mkMntConn {
  mnt_version_str : list nat;  (* Version string (simplified) *)
  mnt_msize : Z;                (* Message size *)
  mnt_attached : bool           (* Has attach completed? *)
}.

(** Valid version string *)
Definition ValidVersion (v : list nat) : Prop :=
  v <> [].

(** Mnt connection well-formed *)
Definition MntConnWellFormed (m : MntConn) : Prop :=
  ValidVersion (mnt_version_str m) /\
  mnt_msize m > 0 /\
  mnt_msize m <= 8192.

(** Version negotiation *)
Inductive VersionNegotiation : list nat -> Z -> MntConn -> Prop :=
  | Version_Success : forall version msize,
      ValidVersion version ->
      msize > 0 ->
      msize <= 8192 ->
      VersionNegotiation version msize (mkMntConn version msize false).

(** Attach operation *)
Inductive AttachOp : MntConn -> Chan -> MntConn -> Prop :=
  | Attach_Success : forall m c,
      MntConnWellFormed m ->
      mnt_attached m = false ->
      ChanWellFormed c ->
      AttachOp m c (mkMntConn (mnt_version_str m) (mnt_msize m) true).

(** Theorem: Version must be negotiated before attach *)
Theorem version_before_attach : forall m_before c m_after,
  AttachOp m_before c m_after ->
  ValidVersion (mnt_version_str m_before).
Proof.
  intros m_before c m_after Hattach.
  inversion Hattach; subst.
  destruct H as [Hversion _].
  exact Hversion.
Qed.

(** Theorem: Attach preserves well-formedness *)
Theorem attach_preserves_wellformed : forall m c m',
  MntConnWellFormed m ->
  AttachOp m c m' ->
  MntConnWellFormed m'.
Proof.
  intros m c m' Hwf Hattach.
  inversion Hattach; subst.
  unfold MntConnWellFormed in *. simpl.
  exact Hwf.
Qed.

(** Theorem: Cannot attach twice *)
Theorem no_double_attach : forall m c1 c2 m' m'',
  MntConnWellFormed m ->
  AttachOp m c1 m' ->
  ~ AttachOp m' c2 m''.
Proof.
  intros m c1 c2 m' m'' Hwf Hattach1 Hattach2.
  inversion Hattach1; subst.
  inversion Hattach2; subst.
  simpl in H1.
  discriminate.
Qed.

(* ========================================================================= *)
(* MOUNT OPERATIONS SAFETY                                                   *)
(* ========================================================================= *)

(** Mount operation *)
Inductive MountOp : Pgrp -> Chan -> Chan -> Mount -> Pgrp -> Prop :=
  | Mount_New : forall pg c_from c_to m,
      PgrpWellFormed pg ->
      InNamespace pg c_from ->
      ChanWellFormed c_to ->
      MountWellFormed m ->
      mount_to m = c_to ->
      (* Simplified: actual code adds to hash table *)
      MountOp pg c_from c_to m pg.  (* TODO: Update pg with new mount *)

(** Unmount operation *)
Inductive UnmountOp : Pgrp -> Chan -> Z -> Pgrp -> Prop :=
  | Unmount_Success : forall pg c mid,
      PgrpWellFormed pg ->
      MountPointExists pg c ->
      (* Simplified: actual code removes from hash table *)
      UnmountOp pg c mid pg.  (* TODO: Update pg with removed mount *)

(** Theorem: Mount preserves Pgrp well-formedness *)
Theorem mount_preserves_pgrp : forall pg c_from c_to m pg',
  MountOp pg c_from c_to m pg' ->
  PgrpWellFormed pg'.
Proof.
  intros pg c_from c_to m pg' Hmount.
  inversion Hmount; subst.
  exact H.
Qed.

(** Theorem: Unmount preserves Pgrp well-formedness *)
Theorem unmount_preserves_pgrp : forall pg c mid pg',
  UnmountOp pg c mid pg' ->
  PgrpWellFormed pg'.
Proof.
  intros pg c mid pg' Hunmount.
  inversion Hunmount; subst.
  exact H.
Qed.

(* ========================================================================= *)
(* UNION MOUNT TRAVERSAL                                                     *)
(* ========================================================================= *)

(** Try walk on each mount in union *)
Fixpoint try_union_walk (mounts : list Mount) (path : list PathComponent)
  : option Chan :=
  match mounts with
  | [] => None
  | m :: rest =>
      (* Simplified: actual code does 9P walk on mount_to m *)
      match path with
      | [] => Some (mount_to m)
      | _ => try_union_walk rest path
      end
  end.

(** Theorem: Union walk result is from one of the mounts *)
Theorem union_walk_from_mount : forall mounts path c,
  try_union_walk mounts path = Some c ->
  exists m, In m mounts /\ mount_to m = c.
Proof.
  intros mounts path c.
  induction mounts as [| m rest IH].
  - simpl. intro H. discriminate.
  - simpl. destruct path as [| comp path'].
    + intro H. injection H as H. subst.
      exists m. split.
      * left. reflexivity.
      * reflexivity.
    + intro H. apply IH in H.
      destruct H as [m' [Hin Heq]].
      exists m'. split.
      * right. exact Hin.
      * exact Heq.
Qed.

(** Theorem: If mounts are well-formed, union walk result is well-formed *)
Theorem union_walk_wellformed : forall mounts path c,
  Forall MountWellFormed mounts ->
  try_union_walk mounts path = Some c ->
  ChanWellFormed c.
Proof.
  intros mounts path c Hwf Htry.
  apply union_walk_from_mount in Htry.
  destruct Htry as [m [Hin Heq]].
  subst.
  apply Forall_forall with (x := m) in Hwf.
  - destruct Hwf as [_ [_ Hchan]]. exact Hchan.
  - exact Hin.
Qed.

(* ========================================================================= *)
(* INTEGRATION THEOREMS                                                      *)
(* ========================================================================= *)

(** Theorem: Mount points in well-formed Pgrp have well-formed channels *)
Theorem mountpoint_channels_wellformed : forall pg mh,
  PgrpWellFormed pg ->
  In (Some mh) (pgrp_mnthash pg) ->
  ChanWellFormed (mhead_from mh) /\
  Forall (fun m => ChanWellFormed (mount_to m)) (mhead_mounts mh).
Proof.
  intros pg mh Hwf_pg Hin.
  destruct Hwf_pg as [_ [_ Hforall]].
  apply Forall_forall with (x := Some mh) in Hforall.
  - destruct Hforall as [_ [Hfrom [Hmounts _]]].
    split.
    + exact Hfrom.
    + apply Forall_impl with (P := MountWellFormed).
      * intros m Hwf_m. destruct Hwf_m as [_ [_ Hchan]]. exact Hchan.
      * exact Hmounts.
  - exact Hin.
Qed.

(** Axiom: Walk on well-formed channel produces well-formed result
    * Full proof requires modeling the 9P walk RPC and reply handling.
    *)
Axiom walk_preserves_wellformed : forall pg c path result,
  ChanWellFormed c ->
  Walk pg c path result ->
  match result with
  | WalkSuccess c' => ChanWellFormed c'
  | WalkError _ => True
  end.

Print Assumptions walk_namespace_containment.
Print Assumptions dotdot_bounded.
Print Assumptions version_before_attach.
Print Assumptions attach_preserves_wellformed.
Print Assumptions no_double_attach.
Print Assumptions mount_preserves_pgrp.
Print Assumptions unmount_preserves_pgrp.
Print Assumptions union_walk_from_mount.
Print Assumptions union_walk_wellformed.
Print Assumptions mountpoint_channels_wellformed.
