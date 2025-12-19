(** * 9P Router Model
    * Formal specification of kernel/9p_router.c
    * Focus: State machines, FID management, Pebble validation
    *)

Require Import Coq.Lists.List.
Require Import Coq.ZArith.ZArith.
Require Import Coq.Bool.Bool.
Require Import Lia.
Import ListNotations.
Open Scope Z_scope.

(* ========================================================================= *)
(* 1. BASIC TYPES                                                            *)
(* ========================================================================= *)

Definition ProcessId := Z.
Definition FidId := Z.
Definition Timestamp := Z.

(* ========================================================================= *)
(* 2. P9CONTROL STATUS STATE MACHINE                                         *)
(* ========================================================================= *)

(* Maps to P9_STATUS_* constants from 9p_router.h *)
Inductive P9Status :=
  | P9_Idle      (* P9_STATUS_IDLE = 0 *)
  | P9_Pending   (* P9_STATUS_PENDING = 1 *)
  | P9_Complete  (* P9_STATUS_COMPLETE = 2 *)
  | P9_Error.    (* P9_STATUS_ERROR = 3 *)

(* P9Control structure - simplified model *)
Record P9Control := mkP9Control {
  ctl_status : P9Status;
  ctl_req_seq : Z;
  ctl_rep_seq : Z;
  ctl_doorbell : bool
}.

(* Initial P9Control state *)
Definition p9_control_init : P9Control := {|
  ctl_status := P9_Idle;
  ctl_req_seq := 0;
  ctl_rep_seq := 0;
  ctl_doorbell := false
|}.

(* Status transition events *)
Inductive P9Event :=
  | EvDoorbellRing    (* User rings doorbell *)
  | EvDispatchSuccess (* Dispatch completes successfully *)
  | EvDispatchError   (* Dispatch fails *)
  | EvReset.          (* Reset to idle *)

(* Status transition function *)
Definition p9_transition (ctl : P9Control) (ev : P9Event) : P9Control :=
  match ev with
  | EvDoorbellRing =>
      match ctl.(ctl_status) with
      | P9_Idle => {| ctl_status := P9_Pending;
                      ctl_req_seq := ctl.(ctl_req_seq) + 1;
                      ctl_rep_seq := ctl.(ctl_rep_seq);
                      ctl_doorbell := false |}
      | _ => ctl (* Ignore if not idle *)
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

(* FID handler types from 9p_router.c *)
Inductive FidType :=
  | TYPE_PROC    (* /proc/ *)
  | TYPE_DEV     (* /dev/ *)
  | TYPE_ENV     (* /env/ *)
  | TYPE_SRV     (* /srv/ *)
  | TYPE_MNT     (* /mnt/ *)
  | TYPE_FD.     (* /fd/ *)

(* FID entry in the table *)
Record FidEntry := mkFidEntry {
  fid_id : FidId;
  fid_type : FidType;
  fid_subtype : Z;       (* Device subtype, e.g., DEV_CONS=1, DEV_NULL=2 *)
  fid_owner : ProcessId  (* Process that owns this FID *)
}.

Definition FidTable := list FidEntry.

(* Install a FID in the table *)
Definition fid_install (table : FidTable) (entry : FidEntry) : FidTable :=
  entry :: table.

(* Lookup FID by ID *)
Fixpoint fid_lookup (table : FidTable) (fid : FidId) : option FidEntry :=
  match table with
  | [] => None
  | e :: rest =>
      if Z.eqb e.(fid_id) fid then Some e
      else fid_lookup rest fid
  end.

(* Lookup FID type (returns None if not found) *)
Definition fid_get_type (table : FidTable) (fid : FidId) : option FidType :=
  match fid_lookup table fid with
  | Some e => Some e.(fid_type)
  | None => None
  end.

(* Remove FID from table *)
Fixpoint fid_remove (table : FidTable) (fid : FidId) : FidTable :=
  match table with
  | [] => []
  | e :: rest =>
      if Z.eqb e.(fid_id) fid then rest
      else e :: fid_remove rest fid
  end.

(* Check if process owns FID *)
Definition fid_owned_by (table : FidTable) (fid : FidId) (pid : ProcessId) : bool :=
  match fid_lookup table fid with
  | Some e => Z.eqb e.(fid_owner) pid
  | None => false
  end.

(* ========================================================================= *)
(* 4. PEBBLE TOKEN MODEL                                                     *)
(* ========================================================================= *)

(* Permission flags from 9p_router.h *)
Definition PEBBLE_PERM_READ  : Z := 1.  (* 0x01 *)
Definition PEBBLE_PERM_WRITE : Z := 2.  (* 0x02 *)
Definition PEBBLE_PERM_EXEC  : Z := 4.  (* 0x04 *)
Definition PEBBLE_PERM_DELETE : Z := 8. (* 0x08 *)
Definition PEBBLE_PERM_ADMIN : Z := 128. (* 0x80 *)

(* PebbleToken structure *)
Record PebbleToken := mkPebbleToken {
  pebble_ledger_id : Z;
  pebble_expires : Z;      (* 0 = never expires *)
  pebble_permissions : Z   (* Bitmask of PEBBLE_PERM_* *)
}.

(* Check if token has required permission *)
Definition pebble_has_permission (tok : PebbleToken) (perm : Z) : bool :=
  Z.eqb (Z.land tok.(pebble_permissions) perm) perm.

(* Validate pebble token (simplified - no crypto) *)
Definition pebble_validate (tok : PebbleToken) (current_time : Timestamp) (required : Z) : bool :=
  (* Check expiration *)
  let not_expired := orb (Z.eqb tok.(pebble_expires) 0)
                         (Z.ltb current_time tok.(pebble_expires)) in
  (* Check permissions *)
  let has_perms := pebble_has_permission tok required in
  andb not_expired has_perms.

(* ========================================================================= *)
(* 5. PATH ROUTING MODEL                                                     *)
(* ========================================================================= *)

(* Simplified path representation *)
Inductive PathPrefix :=
  | PathProc   (* /proc/* *)
  | PathDev    (* /dev/* *)
  | PathEnv    (* /env/* *)
  | PathSrv    (* /srv/* *)
  | PathMnt    (* /mnt/* *)
  | PathFd     (* /fd/* *)
  | PathUnknown.

(* Map path prefix to FID type *)
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
(* 6. 9P MESSAGE TYPES                                                       *)
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
  | Texec | Rexec.  (* Lux9 extension *)

(* Simplified Fcall *)
Record Fcall := mkFcall {
  fcall_type : MsgType;
  fcall_fid : FidId;
  fcall_path : PathPrefix  (* For Tattach *)
}.

(* Reply type mapping *)
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
(* 7. DISPATCH MODEL                                                         *)
(* ========================================================================= *)

(* Dispatch result *)
Inductive DispatchResult :=
  | DispatchOk (table : FidTable) (reply : Fcall)
  | DispatchErr (msg : nat).  (* Error code *)

(* Simplified dispatch - handles Tattach and Tclunk *)
Definition dispatch (table : FidTable) (pid : ProcessId) (t : Fcall) : DispatchResult :=
  match t.(fcall_type) with
  | Tattach =>
      match path_to_type t.(fcall_path) with
      | Some ftype =>
          let entry := mkFidEntry t.(fcall_fid) ftype 0 pid in
          let new_table := fid_install table entry in
          let reply := mkFcall Rattach t.(fcall_fid) t.(fcall_path) in
          DispatchOk new_table reply
      | None =>
          DispatchErr 1 (* Unknown path *)
      end
  | Tclunk =>
      (* Verify ownership before removing *)
      if fid_owned_by table t.(fcall_fid) pid then
        let new_table := fid_remove table t.(fcall_fid) in
        let reply := mkFcall Rclunk t.(fcall_fid) PathUnknown in
        DispatchOk new_table reply
      else
        DispatchErr 2 (* Permission denied *)
  | _ =>
      (* Other messages: verify FID exists and is owned *)
      if fid_owned_by table t.(fcall_fid) pid then
        let reply := mkFcall (reply_type t.(fcall_type)) t.(fcall_fid) PathUnknown in
        DispatchOk table reply
      else
        DispatchErr 2 (* Permission denied / FID not found *)
  end.

