(** * 9P Router Model
    * Formal specification of kernel/9p_router.c
    * Focus: State machines, FID management, Pebble validation
    * Instantiates the generic router template with Lux9's Pebble Algebra.
    *)

Require Import Coq.Lists.List.
Require Import Coq.ZArith.ZArith.
Require Import Coq.Bool.Bool.
Require Import Lia.
Require Import Lux9.Router.resource_algebra.
Require Import Lux9.Router.router_template.

Import ListNotations.
Open Scope Z_scope.

(* ========================================================================= *)
(* PEBBLE ALGEBRA IMPLEMENTATION                                             *)
(* ========================================================================= *)

Module PebbleAlgebra <: ResourceAlgebra.

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

  Definition Token := PebbleToken.
  Definition Permissions := Z.

  (* Check if token has required permission *)
  Definition pebble_has_permission (tok : PebbleToken) (perm : Z) : bool :=
    Z.eqb (Z.land tok.(pebble_permissions) perm) perm.

  (* Validate pebble token *)
  Definition validate (tok : Token) (current_time : Z) (required : Permissions) : bool :=
    (* Check expiration *)
    let not_expired := orb (Z.eqb tok.(pebble_expires) 0)
                           (Z.ltb current_time tok.(pebble_expires)) in
    (* Check permissions *)
    let has_perms := pebble_has_permission tok required in
    andb not_expired has_perms.

End PebbleAlgebra.

(* ========================================================================= *)
(* ROUTER MODEL INSTANTIATION                                                *)
(* ========================================================================= *)

Module Export Model := MakeRouter(PebbleAlgebra).
