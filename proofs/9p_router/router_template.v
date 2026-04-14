(** * 9P Router Template
    * Generic specification of the 9P Router logic parameterized by a Resource Algebra.
    * Allows the router to be instantiated with different security models (e.g., Lux9 Pebble, 9front Generic).
    *)

Require Import Coq.Lists.List.
Require Import Coq.ZArith.ZArith.
Require Import Coq.Bool.Bool.
Require Import Lia.
Require Import Lux9.Router.resource_algebra.

Import ListNotations.
Open Scope Z_scope.

Module MakeRouter (RA : ResourceAlgebra).

  (* ========================================================================= *)
  (* 1. BASIC TYPES                                                            *)
  (* ========================================================================= *)

  Definition ProcessId := Z.
  Definition FidId := Z.
  Definition Timestamp := Z.

  (* ========================================================================= *)
  (* 2. P9CONTROL STATUS STATE MACHINE                                         *)
  (* ========================================================================= *)

  Inductive P9Status :=
    | P9_Idle
    | P9_Pending
    | P9_Complete
    | P9_Error.

  Record P9Control := mkP9Control {
    ctl_status : P9Status;
    ctl_req_seq : Z;
    ctl_rep_seq : Z;
    ctl_doorbell : bool
  }.

  Definition p9_control_init : P9Control := {|
    ctl_status := P9_Idle;
    ctl_req_seq := 0;
    ctl_rep_seq := 0;
    ctl_doorbell := false
  |}.

  Inductive P9Event :=
    | EvDoorbellRing
    | EvDispatchSuccess
    | EvDispatchError
    | EvReset.

  Definition p9_transition (ctl : P9Control) (ev : P9Event) : P9Control :=
    match ev with
    | EvDoorbellRing =>
        match ctl.(ctl_status) with
        | P9_Idle => {| ctl_status := P9_Pending;
                        ctl_req_seq := ctl.(ctl_req_seq) + 1;
                        ctl_rep_seq := ctl.(ctl_rep_seq);
                        ctl_doorbell := false |}
        | _ => ctl
        end
    | EvDispatchSuccess =>
        match ctl.(ctl_status) with
        | P9_Pending => {| ctl_status := P9_Complete;
                           ctl_req_seq := ctl.(ctl_req_seq);
                           ctl_rep_seq := ctl.(ctl_rep_seq) + 1;
                           ctl_doorbell := ctl.(ctl_doorbell) |}
        | _ => ctl
        end
    | EvDispatchError =>
        match ctl.(ctl_status) with
        | P9_Pending => {| ctl_status := P9_Error;
                           ctl_req_seq := ctl.(ctl_req_seq);
                           ctl_rep_seq := ctl.(ctl_rep_seq);
                           ctl_doorbell := ctl.(ctl_doorbell) |}
        | _ => ctl
        end
    | EvReset =>
        {| ctl_status := P9_Idle;
           ctl_req_seq := ctl.(ctl_req_seq);
           ctl_rep_seq := ctl.(ctl_rep_seq);
           ctl_doorbell := false |}
    end.

  (* ========================================================================= *)
  (* 3. FID TABLE MODEL                                                        *)
  (* ========================================================================= *)

  Inductive FidType :=
    | TYPE_PROC
    | TYPE_DEV
    | TYPE_ENV
    | TYPE_SRV
    | TYPE_MNT
    | TYPE_FD.

  Record FidEntry := mkFidEntry {
    fid_id : FidId;
    fid_type : FidType;
    fid_subtype : Z;
    fid_owner : ProcessId
  }.

  Definition FidTable := list FidEntry.

  Definition fid_install (table : FidTable) (entry : FidEntry) : FidTable :=
    entry :: table.

  Fixpoint fid_lookup (table : FidTable) (fid : FidId) : option FidEntry :=
    match table with
    | [] => None
    | e :: rest =>
        if Z.eqb e.(fid_id) fid then Some e
        else fid_lookup rest fid
    end.

  Definition fid_get_type (table : FidTable) (fid : FidId) : option FidType :=
    match fid_lookup table fid with
    | Some e => Some e.(fid_type)
    | None => None
    end.

  Fixpoint fid_remove (table : FidTable) (fid : FidId) : FidTable :=
    match table with
    | [] => []
    | e :: rest =>
        if Z.eqb e.(fid_id) fid then rest
        else e :: fid_remove rest fid
    end.

  Definition fid_owned_by (table : FidTable) (fid : FidId) (pid : ProcessId) : bool :=
    match fid_lookup table fid with
    | Some e => Z.eqb e.(fid_owner) pid
    | None => false
    end.

  (* ========================================================================= *)
  (* 4. PATH ROUTING MODEL                                                     *)
  (* ========================================================================= *)

  Inductive PathPrefix :=
    | PathProc
    | PathDev
    | PathEnv
    | PathSrv
    | PathMnt
    | PathFd
    | PathUnknown.

  Definition path_to_type (prefix : PathPrefix) : option FidType :=
    match prefix with
    | PathProc => Some TYPE_PROC
    | PathDev => Some TYPE_DEV
    | PathEnv => Some TYPE_ENV
    | PathSrv => Some TYPE_SRV
    | PathMnt => Some TYPE_MNT
    | PathFd => Some TYPE_FD
    | PathUnknown => None
    end.

  (* ========================================================================= *)
  (* 5. 9P MESSAGE TYPES                                                       *)
  (* ========================================================================= *)

  Inductive MsgType :=
    | Tversion | Rversion
    | Tauth | Rauth
    | Tattach | Rattach
    | Twalk | Rwalk
    | Topen | Ropen
    | Tcreate | Rcreate
    | Tread | Rread
    | Twrite | Rwrite
    | Tclunk | Rclunk
    | Rerror
    | Texec | Rexec.

  Record Fcall := mkFcall {
    fcall_type : MsgType;
    fcall_fid : FidId;
    fcall_path : PathPrefix;
    fcall_token : RA.Token (* Abstract Token *)
  }.

  Definition reply_type (t : MsgType) : MsgType :=
    match t with
    | Tversion => Rversion
    | Tauth => Rauth
    | Tattach => Rattach
    | Twalk => Rwalk
    | Topen => Ropen
    | Tcreate => Rcreate
    | Tread => Rread
    | Twrite => Rwrite
    | Tclunk => Rclunk
    | Texec => Rexec
    | _ => Rerror
    end.

  (* ========================================================================= *)
  (* 6. DISPATCH MODEL                                                         *)
  (* ========================================================================= *)

  Inductive DispatchResult :=
    | DispatchOk (table : FidTable) (reply : Fcall)
    | DispatchErr (msg : nat).

  Definition dispatch (table : FidTable) (pid : ProcessId) (t : Fcall) (time : Timestamp) (perms : RA.Permissions) : DispatchResult :=
    (* Validate Token First *)
    if RA.validate t.(fcall_token) time perms then
      match t.(fcall_type) with
      | Tattach =>
          match path_to_type t.(fcall_path) with
          | Some ftype =>
              let entry := mkFidEntry t.(fcall_fid) ftype 0 pid in
              let new_table := fid_install table entry in
              let reply := mkFcall Rattach t.(fcall_fid) t.(fcall_path) t.(fcall_token) in
              DispatchOk new_table reply
          | None =>
              DispatchErr 1 (* Unknown path *)
          end
      | Tclunk =>
          if fid_owned_by table t.(fcall_fid) pid then
            let new_table := fid_remove table t.(fcall_fid) in
            let reply := mkFcall Rclunk t.(fcall_fid) PathUnknown t.(fcall_token) in
            DispatchOk new_table reply
          else
            DispatchErr 2 (* Permission denied *)
      | _ =>
          if fid_owned_by table t.(fcall_fid) pid then
            let reply := mkFcall (reply_type t.(fcall_type)) t.(fcall_fid) PathUnknown t.(fcall_token) in
            DispatchOk table reply
          else
            DispatchErr 2 (* Permission denied / FID not found *)
      end
    else
      DispatchErr 3. (* Token Validation Failed *)

End MakeRouter.
